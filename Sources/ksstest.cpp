//
//  ksstest.cpp
//  ksstest
//
//  Created by Steven W. Klassen on 2018-04-03.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//
// However a license is hereby granted for this code to be used, modified and redistributed
// without restriction or requirement, other than you cannot hinder anyone else from doing
// the same.

#include "ksstest.h"

#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include <cxxabi.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;
using namespace kss::testing;

namespace {

	// Name demangling.
	template <typename T>
	string name(const T& t = T()) {
		int status;
		return abi::__cxa_demangle(typeid(t).name(), 0, 0, &status);
	}

	// Basename of a path.
	string basename(const string &path) {
		const auto dirSepPos = path.find_last_of('/');
		if (dirSepPos == string::npos) {
			return path;
		}
		else {
			return path.substr(dirSepPos+1);
		}
	}

	// Test for subclasses.
	template <class T, class Base>
	T* as(Base* obj) noexcept {
		if (!obj) return nullptr;
		return dynamic_cast<T*>(obj);
	}

	// Exception that is thrown to skip a test case.
	class SkipTestCase {};

	// Signal handler to allow us to ignore SIGCHLD.
	static void my_signal_handler(int sig) {
	}

	// Terminate handler allowing us to catch the terminate call.
	static void my_terminate_handler() {
		_exit(0);           // Correct response.
	}

	struct TestCaseWrapper {
		string					name;
		TestSuite::test_case_fn	fn;
		unsigned int			assertions = 0;
		vector<string>			errors;
		vector<string>			failures;
		bool					skipped = false;

		bool operator<(const TestCaseWrapper& rhs) const noexcept {
			return name < rhs.name;
		}
	};

	struct TestSuiteWrapper {
		TestSuite* suite;

		bool operator<(const TestSuiteWrapper& rhs) const noexcept {
			return suite->name() < rhs.suite->name();
		}
	};

	// State of the world.
	static vector<TestSuiteWrapper> 		testSuites;
	thread_local static TestSuiteWrapper*	currentSuite = nullptr;
	thread_local static TestCaseWrapper*	currentTest = nullptr;
}


// MARK: TestSuite::Impl Implementation

struct TestSuite::Impl {
	TestSuite*				parent = nullptr;
	string 					name;
	vector<TestCaseWrapper>	tests;

	// Add the BeforeAll and AfterAll "tests" if appropriate.
	void addBeforeAndAfterAll() {
		if (auto* hba = as<HasBeforeAll>(parent)) {
			TestCaseWrapper wrapper;
			wrapper.name = "BeforeAll";
			wrapper.fn = [=](TestSuite& suite) { hba->beforeAll(); };
			tests.insert(tests.begin(), move(wrapper));
		}
		if (auto* haa = as<HasAfterAll>(parent)) {
			TestCaseWrapper wrapper;
			wrapper.name = "AfterAll";
			wrapper.fn = [=](TestSuite& suite) { haa->afterAll(); };
			tests.push_back(move(wrapper));
		}
	}
};


namespace {
}

namespace kss { namespace testing {

	int run(const string& testRunName, int argc, const char *const *argv) {
		sort(testSuites.begin(), testSuites.end());
		for (auto& ts : testSuites) {
			ts.suite->_impl->addBeforeAndAfterAll();
			currentSuite = &ts;
			cerr << ts.suite->name() << "..." << endl;
			for (auto& t : ts.suite->_impl->tests) {
				currentTest = &t;
				cerr << "  " << t.name << ": ";
				try {
					if (auto* hbe = as<HasBeforeEach>(ts.suite)) {
						if (t.name != "BeforeAll" && t.name != "AfterAll") hbe->beforeEach();
					}
					t.fn(*ts.suite);
					if (auto* haa = as<HasAfterEach>(ts.suite)) {
						if (t.name != "BeforeAll" && t.name != "AfterAll") haa->afterEach();
					}
				}
				catch (const SkipTestCase&) {
					t.skipped = true;
					cerr << "SKIPPED" << endl;
				}
				catch (const exception& e) {
					t.errors.push_back(name(e) + ": " + e.what());
				}
				catch (...) {
					t.errors.push_back("Unknown exception");
				}
				cerr << endl;
				currentTest = nullptr;

				if (!t.errors.empty()) {
					cerr << "    errors:" << endl;
					for (const auto& err : t.errors) {
						cerr << "      " << err << endl;
					}
				}
				if (!t.failures.empty()) {
					cerr << "    failures:" << endl;
					for (const auto& f : t.failures) {
						cerr << "      " << f << endl;
					}
				}
			}
			currentSuite = nullptr;
		}
		return 0;
	}

	void skip() {
		throw SkipTestCase();
	}
}}


// MARK: Assertions

namespace kss { namespace testing {

	bool doesNotThrowException(function<void(void)> fn) {
		bool caughtSomething = false;
		try {
			fn();
		}
		catch (...) {
			caughtSomething = true;
		}
		return !caughtSomething;
	}

	bool terminates(function<void(void)> fn) {
		// Need to ignore SIGCHLD for this test to work. We restore after the test. For some
		// reason we need to specify an empty signal handler and not just SIG_IGN.
		sig_t oldHandler = signal(SIGCHLD, my_signal_handler);

		// In the child process we set a terminate handler that will exit the process with a 0.
		pid_t pid = fork();
		if (pid == 0) {
			for (int i = 1; i <= 255; ++i)
				signal(i, my_signal_handler);
			set_terminate(my_terminate_handler);
			try {
				fn();
			}
			catch (...) {
				_exit(2);   // Error, an exception was thrown that did not cause a terminate().
			}
			_exit(1);       // Error, lambda exited without causing a terminate().
		}

		// In the parent process we wait for the child to exit, then see if it exited with
		// a 0 status.
		else {
			int status = -9;    // Any non-zero state.
			waitpid(pid, &status, 0);
			signal(SIGCHLD, oldHandler);
			if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
				return true;   // success
			return false;       // failure
		}
	}
}}


// MARK: TestSuite Implementation

TestSuite::TestSuite(const string& testSuiteName,
					 initializer_list<pair<string, test_case_fn>> fns)
: _impl(new Impl())
{
	_impl->parent = this;
	_impl->name = testSuiteName;

	for (auto& p : fns) {
		TestCaseWrapper wrapper;
		wrapper.name = p.first;
		wrapper.fn = p.second;
		_impl->tests.push_back(move(wrapper));
	}
	sort(_impl->tests.begin(), _impl->tests.end());

	TestSuiteWrapper wrapper;
	wrapper.suite = this;
	testSuites.push_back(wrapper);
}

TestSuite& TestSuite::operator=(TestSuite&& ts) {
	if (this != &ts) {
		_impl = move(ts._impl);
		ts._impl.reset();
	}
	return *this;
}

TestSuite::~TestSuite() noexcept {}
TestSuite::TestSuite(TestSuite&& ts) : _impl(move(ts._impl)) {}

const string& TestSuite::name() const noexcept {
	return _impl->name;
}


// MARK: _private Implementation

namespace kss { namespace testing { namespace _private {

	void _success(void) noexcept {
		++currentTest->assertions;
		cerr << "+";
	}

	void _failure(const char* expr, const char* filename, unsigned int line) noexcept {
		++currentTest->assertions;
		currentTest->failures.push_back(basename(filename) + ": " + to_string(line) + ", " + expr);
		cerr << "F";
	}
}}}
