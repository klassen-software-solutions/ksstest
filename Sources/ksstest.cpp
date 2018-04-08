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
#include <cassert>
#include <cstdlib>
#include <exception>
#include <future>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include <cxxabi.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;
using namespace kss::testing;

namespace {

	// Name demangling.
	template <typename T>
	string demangle(const T& t = T()) {
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

	// Utility class to enforce RAII cleanup on C style calls.
	class finally {
	public:
		typedef std::function<void(void)> lambda;

		finally(lambda cleanupCode) : _cleanupCode(cleanupCode) {}
		~finally() { _cleanupCode(); }
		finally& operator=(const finally&) = delete;

	private:
		lambda _cleanupCode;
	};



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
	static bool								isQuiet = false;
	static bool								isVerbose = false;
	static bool								isParallel = true;

	// Parse the command line. Returns true if we should continue or false if we
	// should exit.
	static const struct option commandLineOptions[] = {
		{ "help", no_argument, nullptr, 'h' },
		{ "quiet", no_argument, nullptr, 'q' },
		{ "verbose", no_argument, nullptr, 'v' },
		{ "filter", required_argument, nullptr, 'f' },
		{ "xml", no_argument, nullptr, 'X' },
		{ "json", no_argument, nullptr, 'J' },
		{ "no-parallel", no_argument, nullptr, 'N' },
		{ nullptr, 0, nullptr, 0 }
	};

	static char** duplicateArgv(int argc, const char* const* argv) {
		// Create a duplicate of argv that is not const. This may seem dangerous but is needed
		// for the underlying C calls, even though they will not modify argv.
		int i;
		assert(argc > 0);
		char** newargv = (char**)malloc(sizeof(char*)*(size_t)argc);
		if (!newargv) throw bad_alloc();

		for (i = 0; i < argc; ++i) {
			newargv[i] = (char*)argv[i];
		}
		return newargv;
	}

	void printUsageMessage(const string& progName) {
		cout << "usage: " << basename(progName) << " [options]" << R"(

The following are the accepted command line options:
    -h/--help displays this usage message
    -q/--quiet suppress test result output (useful if all you want is the return value)
    -v/--verbose displays more information (-q will override this if present. Specifying
        this option will also cause --no-parallel to be assumed.)
    -f <testprefix>/--filter=<testprefix> only run tests that start with the prefix
    --xml produces no output while running, but a JUnit compatible XML when completed
    --json produces no output while running, but a Google test compatible JSON when completed
    --no-parallel will force all tests to be run in the same thread (This is assumed if
        the --verbose option is specified.)

The display options essentially run in three modes.

In the "quiet" mode (--quiet is specified) no output at all is written and the only
indication of the test results is the return code. This is useful for inclusion in scripts
where you only want a pass/fail result and do not care about the details. It is also
assumed if you specify either --xml or --json so that everything written to the standard
output device will be the XML or JSON reports. Unless --no-parallel is specified, the
tests will be run in multiple threads.

In the "normal" mode (neither --quiet nor --verbose is specified) the program will print a
header line when the tests begin, then will print one of the following characters for each
test suite, followed by a summary stating how many tests passed, failed, and skipped,
finishing with details of all the failed tests:
    . - all tests in the suite ran without error or failure
	S - one or more tests in the suite were skipped (due to use of the skip() method)
	E - one or more of the tests in the suite caused an error condition
	F - one or more of the tests in the suite failed an assertion
Unless --no-parallel is specified, the tests will be run in multiple threads.

In the "verbose" mode (--verbose is specified) more details are written while the tests
are run. In particular you will see a header line for each test suite and an individual
line for each test case within the test suite. For each test case you will see one of
the following characters for each test (i.e. for each call to KSS_ASSERT):
	. - the assertion passed
	+ - 10 consecutive assertions passed
	* - 100 consecutive assertions passed
	S - skip() was called (it will be the last report on the line)
	E - an error occurred while running the test (it will be the last report on the line)
	F - the test failed
If a tests has errors or failures, they will be written out on the following lines. When
the output for all the test cases in a suite is completed, a summary line for the test
suite will be output. Note that in order for this output to make sense, specifying --verbose
will also imply --no-parallel.

In addition specifying --xml or --json will imply --quiet so that the only output is of
the specified format.

Filtering can be used to limit the tests that are run without having to add skip()
statements in your code. This is most useful when you are developing/debugging a particular
section and don't want to repeat all the other test until you have completed. It is also
generally useful to specify --verbose when you are filtering, but that is not assumed.

The return value, when all the tests are done, will be one of the following:
    -1 if there was one or more error conditions raised,
    0 if all tests completed with no errors or failures (although some may have skipped), or
	>0 if some tests failed. The value will be the number of failures (i.e. the number of
        times that KSS_ASSERT failed) in all the test cases in all the test suites.

		)";
	}

	bool parseCommandLine(int argc, const char* const* argv) {
		if (argc > 0 && argv != nullptr) {
			char** newargv = duplicateArgv(argc, argv);
			finally cleanup([&]{ free(newargv); });

			int ch = 0;
			while ((ch = getopt_long(argc, newargv, "hqvf:", commandLineOptions, nullptr)) != -1) {
				switch (ch) {
					case 'h':
						printUsageMessage(argv[0]);
						return false;
					case 'q':
						isQuiet = true;
						break;
					case 'v':
						isVerbose = true;
						break;
					case 'N':
						isParallel = false;
				}
			}

			// Fix any command line dependances.
			if (isQuiet) {
				isVerbose = false;
			}
			if (isVerbose) {
				isParallel = false;
			}
		}
		return true;
	}
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

	// Run a test.
	void runTestCase(TestCaseWrapper& t) {
		currentTest = &t;
		try {
			if (auto* hbe = as<HasBeforeEach>(parent)) {
				if (t.name != "BeforeAll" && t.name != "AfterAll") hbe->beforeEach();
			}
			t.fn(*parent);
			if (auto* haa = as<HasAfterEach>(parent)) {
				if (t.name != "BeforeAll" && t.name != "AfterAll") haa->afterEach();
			}
		}
		catch (const SkipTestCase&) {
			t.skipped = true;
			if (isVerbose) {
				cout << "SKIPPED";
			}
		}
		catch (const exception& e) {
			if (isVerbose) {
				cout << "E";
			}
			t.errors.push_back(demangle(e) + ": " + e.what());
		}
		catch (...) {
			if (isVerbose) {
				cout << "E";
			}
			t.errors.push_back("Unknown exception");
		}
		currentTest = nullptr;
	}

	// Returns the test suite result as one of the following:
	// '.' - all tests ran without problem
	// 'S' - one or more tests were skipped
	// 'E' - one or more tests caused an error condition
	// 'F' - one or more tests failed.
	char result() const noexcept {
		char res = '.';
		for (const auto& t : tests) {
			if (t.skipped) {
				res = 'S';
				break;
			}
			else if (!t.errors.empty()) {
				assert(res != 'S');
				res = 'E';
			}
			else if (!t.failures.empty()) {
				assert(res != 'S');
				if (res != 'E') {
					res = 'F';
				}
			}
		}
		return res;
	}

	// Returns the number of failures (KSS_ASSERT failing) in all the test cases
	// for the suite.
	int numberOfFailures() const noexcept {
		int total = 0;
		for (const auto& t : tests) {
			total += t.failures.size();
		}
		return total;
	}
};


// MARK: Test reporting

namespace {
	void printTestRunHeader(const string& testRunName) {
		if (!isQuiet) {
			cout << "Running test suites for " << testRunName << "..." << endl;;
			if (!isVerbose) {
				cout << "  ";
				flush(cout);	// Need flush before we start any threads.
			}
		}
	}

	void printTestRunSummary() {
		if (!isQuiet) {
			if (isParallel) {
				flush(cout);	// Ensure all the thread output is flushed.
			}
			if (!isVerbose) {
				cout << endl;
			}

			const auto numberOfTestSuites = testSuites.size();
			unsigned numberOfErrors = 0, numberOfFailures = 0, numberOfSkips = 0, numberPassed = 0;
			for (const auto& ts : testSuites) {
				switch (ts.suite->_implementation()->result()) {
					case '.':	++numberPassed; break;
					case 'S':	++numberOfSkips; break;
					case 'E':	++numberOfErrors; break;
					case 'F':	++numberOfFailures; break;
				}
			}

			if (!numberOfFailures && !numberOfErrors && !numberOfSkips) {
				cout << "  PASSED all " << numberOfTestSuites << " test suites." << endl;
			}
			else {
				cout << "  Passed " << numberPassed << " of " << numberOfTestSuites << " test suites";
				if (numberOfSkips > 0) {
					cout << ", " << numberOfSkips << " skipped";
				}
				if (numberOfErrors > 0) {
					cout << ", " << numberOfErrors << (numberOfErrors == 1 ? " error" : " errors");
				}
				if (numberOfFailures > 0) {
					cout << ", " << numberOfFailures << " failed";
				}
				cout << "." << endl;
			}

			if (!isVerbose) {
				// If we are not verbose we need to identify the errors and failures. If
				// we are verbose, then that information was displayed earlier.
				if (numberOfErrors > 0) {
					cout << "  Errors:" << endl;
					for (const auto& ts : testSuites) {
						for (const auto& t : ts.suite->_implementation()->tests) {
							for (const auto& err : t.errors) {
								cout << "    " << err << endl;
							}
						}
					}
				}
				if (numberOfFailures > 0) {
					cout << "  Failures:" << endl;
					for (const auto& ts : testSuites) {
						for (const auto& t : ts.suite->_implementation()->tests) {
							for (const auto& f : t.failures) {
								cout << "    " << f << endl;
							}
						}
					}
				}
			}
		}
	}

	void printTestSuiteHeader(const TestSuite& ts) {
		if (isVerbose) cout << "  " << ts.name() << endl;
	}

	void printTestSuiteSummary(const TestSuite& ts) {
		if (!isQuiet) {
			const auto* impl = ts._implementation();
			if (isVerbose) {
				unsigned numberOfAssertions = 0;
				unsigned numberSkipped = 0;
				unsigned numberOfErrors = 0;
				unsigned numberOfFailures = 0;
				for (const auto& t : impl->tests) {
					numberOfAssertions += t.assertions;
					if (t.skipped) ++numberSkipped;
					numberOfErrors += t.errors.size();
					numberOfFailures += t.failures.size();
				}

				if (!numberOfErrors && !numberOfFailures) {
					cout << "    PASSED all " << numberOfAssertions << " checks";
				}
				else {
					cout << "    Passed " << numberOfAssertions << " checks";
				}

				if (numberSkipped > 0) {
					cout << ", " << numberSkipped << " test " << (numberSkipped == 1 ? "case" : "cases") << " SKIPPED";
				}
				if (numberOfErrors > 0) {
					cout << ", " << numberOfErrors << (numberOfErrors == 1 ? "error" : "errors");
				}
				if (numberOfFailures > 0) {
					cout << ", " << numberOfFailures << " FAILED";
				}
				cout << "." << endl;

				if (numberOfErrors > 0) {
					cout << "    Errors:" << endl;
					for (const auto& t : impl->tests) {
						for (const auto& err : t.errors) {
							cout << "      " << err << endl;
						}
					}
				}
				if (numberOfFailures > 0) {
					cout << "    Failures:" << endl;
					for (const auto& t : impl->tests) {
						for (const auto& f : t.failures) {
							cout << "      " << f << endl;
						}
					}
				}
			}
			else {
				cout << impl->result();
			}
		}
	}

	void printTestCaseHeader(const TestCaseWrapper& t) {
		if (isVerbose) {
			cout << "    " << t.name << " ";
		}
	}

	void printTestCaseSummary(const TestCaseWrapper& t) {
		if (isVerbose) {
			cout << endl;
		}
	}

	int testResultCode() {
		int numberOfFailures = 0;
		for (const auto& ts : testSuites) {
			const auto& impl = ts.suite->_implementation();
			const auto res = impl->result();
			if (res == 'E') {
				return -1;
			}
			else if (res == 'F') {
				numberOfFailures += impl->numberOfFailures();
			}
		}
		return numberOfFailures;
	}

	void runTestSuite(TestSuiteWrapper* wrapper) {
		printTestSuiteHeader(*wrapper->suite);
		auto* impl = wrapper->suite->_implementation();
		impl->addBeforeAndAfterAll();
		currentSuite = wrapper;
		for (auto& t : impl->tests) {
			printTestCaseHeader(t);
			impl->runTestCase(t);
			printTestCaseSummary(t);
		}
		currentSuite = nullptr;
		printTestSuiteSummary(*wrapper->suite);
	}
}


namespace kss { namespace testing {

	int run(const string& testRunName, int argc, const char *const *argv) {
		if (parseCommandLine(argc, argv)) {
			printTestRunHeader(testRunName);
			sort(testSuites.begin(), testSuites.end());
			vector<future<bool>> futures;
			for (auto& ts : testSuites) {
				if (!isParallel || as<MustNotBeParallel>(ts.suite)) {
					runTestSuite(&ts);
				}
				else {
					TestSuiteWrapper* tsw = &ts;
					futures.push_back(async([tsw]{
						runTestSuite(tsw);
						return true;
					}));
				}
			}
			
			if (!futures.empty()) {
				for (auto& fut : futures) {
					fut.get();
				}
			}
			printTestRunSummary();
		}
		return testResultCode();
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
		if (isVerbose) {
			cout << ".";
		}
	}

	void _failure(const char* expr, const char* filename, unsigned int line) noexcept {
		++currentTest->assertions;
		currentTest->failures.push_back(basename(filename) + ": " + to_string(line) + ", " + expr);
		if (isVerbose) {
			cout << "F";
		}
	}
}}}
