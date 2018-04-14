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
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <fstream>
#include <future>
#include <iostream>
#include <map>
#include <mutex>
#include <ostream>
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

// MARK: State of the world

namespace {
	typedef chrono::duration<double, std::chrono::seconds::period> fractional_seconds_t;

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
		TestSuite* 				suite;
		bool					filteredOut = false;
		string					timestamp;
		fractional_seconds_t	durationOfTestSuite;
		unsigned				numberOfErrors = 0;
		unsigned				numberOfFailures= 0;
		unsigned				numberOfSkippedTests = 0;

		bool operator<(const TestSuiteWrapper& rhs) const noexcept {
			return suite->name() < rhs.suite->name();
		}
	};

	static vector<TestSuiteWrapper> 		testSuites;
	thread_local static TestSuiteWrapper*	currentSuite = nullptr;
	thread_local static TestCaseWrapper*	currentTest = nullptr;
	static bool								isQuiet = false;
	static bool								isVerbose = false;
	static bool								isParallel = true;
	static string							filter;
	static string							xmlReportFilename;

	// Summary of test results.
	struct TestResultSummary {
		mutex					lock;
		string 					nameOfTestRun;
		string					nameOfHost;
		fractional_seconds_t	durationOfTestRun;
		unsigned				numberOfErrors = 0;
		unsigned				numberOfFailures = 0;
		unsigned				numberOfAssertions = 0;
	};
	static TestResultSummary reportSummary;
}


// MARK: Internal Utilities

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
	void my_signal_handler(int sig) {
	}

	// Terminate handler allowing us to catch the terminate call.
	void my_terminate_handler() {
		_exit(0);           // Correct response.
	}

	// Parse the command line. Returns true if we should continue or false if we
	// should exit.
	static const struct option commandLineOptions[] = {
		{ "help", no_argument, nullptr, 'h' },
		{ "quiet", no_argument, nullptr, 'q' },
		{ "verbose", no_argument, nullptr, 'v' },
		{ "filter", required_argument, nullptr, 'f' },
		{ "xml", required_argument, nullptr, 'X' },
		{ "json", required_argument, nullptr, 'J' },
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
    ---xml=<filename> writes a JUnit test compatible XML to the given filename
    --json=<filename> writes a Google test compatible JSON to the given filename
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

For --xml or --json you can specify "-" as the filename. In that case instead of writing
to a file the report will be written to the standard output device. Unless you have also
specified --quiet, the report will be preceeded by a line of all "=" characters to make
it possible to find the end of the live output and the start of the report.

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
						break;
					case 'f':
						if (!optarg) {
							printUsageMessage(argv[0]);
							exit(-1);
						}
						filter = string(optarg);
						break;
					case 'X':
						if (!optarg) {
							printUsageMessage(argv[0]);
							exit(-1);
						}
						xmlReportFilename = string(optarg);
						break;
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

	// Returns true if the given string starts with the given prefix and false otherwise.
	inline bool starts_with(const string& s, const string& prefix) noexcept {
		return (prefix == s.substr(0, prefix.size()));
	}

	// Returns true if the test case should pass the filter and false otherwise.
	bool passesFilter(const TestSuite& s) noexcept {
		if (!filter.empty()) {
			return starts_with(s.name(), filter);
		}
		return true;
	}

	// Write a file, throwing an exception if there is a problem.
	inline void throwProcessingError(const string& filename, const string& what_arg) {
		throw system_error(errno, system_category(), what_arg + " " + filename);
	}

	void write_file(const string& filename, function<void (ofstream&)> fn) {
		errno = 0;
		ofstream strm(filename);
		if (!strm.is_open()) throwProcessingError(filename, "Failed to open");
		fn(strm);
		if (strm.bad()) throwProcessingError(filename, "Failed while writing");
	}

	// Return the time that fn took to run.
	fractional_seconds_t time_of_execution(function<void ()> fn) {
		const auto start = chrono::high_resolution_clock::now();
		fn();
		return chrono::duration_cast<fractional_seconds_t>(chrono::high_resolution_clock::now() - start);
	}

	// Return the current timestamp in ISO 8601 format.
	string now() {
		time_t now;
		::time(&now);
		char buf[sizeof "9999-99-99T99:99:99Z "];
		strftime(buf, sizeof(buf), "%FT%TZ", ::gmtime(&now));
		return string(buf);
	}

	// Returns the hostname of the machine.
	string hostname() noexcept {
		char name[100];
		if (gethostname(name, sizeof(name)-1) == -1) {
			return "localhost";
		}
		return name;
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

		// Update the counters.
		currentTest = nullptr;
		currentSuite->numberOfErrors += t.errors.size();
		currentSuite->numberOfFailures += t.failures.size();
		if (t.skipped) ++currentSuite->numberOfSkippedTests;

		{
			lock_guard<mutex> l(reportSummary.lock);
			reportSummary.numberOfErrors += t.errors.size();
			reportSummary.numberOfFailures += t.failures.size();
			reportSummary.numberOfAssertions += t.assertions;
		}
	}

	// Returns the test suite result as one of the following:
	// '.' - all tests ran without problem
	// 'S' - one or more tests were skipped
	// 'E' - one or more tests caused an error condition
	// 'F' - one or more tests failed.
	char result() const noexcept {
		char res = '.';
		for (const auto& t : tests) {
			if (!t.errors.empty()) {
				res = 'E';
			}
			else if (!t.failures.empty()) {
				if (res != 'E') {
					res = 'F';
				}
			}
			else if (t.skipped) {
				if (res == '.') {
					res = 'S';
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
	void printTestRunHeader() {
		if (!isQuiet) {
			cout << "Running test suites for " << reportSummary.nameOfTestRun << "..." << endl;;
			if (!isVerbose) {
				cout << "  ";
				flush(cout);	// Need flush before we start any threads.
			}
		}
	}

	void outputStandardSummary() {
		if (!isQuiet) {
			if (isParallel) {
				flush(cout);	// Ensure all the thread output is flushed.
			}
			if (!isVerbose) {
				cout << endl;
			}

			const auto numberOfTestSuites = testSuites.size();
			unsigned numberOfErrors = 0, numberOfFailures = 0, numberOfSkips = 0, numberPassed = 0;
			unsigned numberFilteredOut = 0;
			for (const auto& ts : testSuites) {
				if (ts.filteredOut) {
					++numberFilteredOut;
				}

				switch (ts.suite->_implementation()->result()) {
					case '.':	++numberPassed; break;
					case 'S':	++numberOfSkips; break;
					case 'E':	++numberOfErrors; break;
					case 'F':	++numberOfFailures; break;
				}
			}

			if (!numberOfFailures && !numberOfErrors && !numberOfSkips) {
				cout << "  PASSED all " << (numberOfTestSuites-numberFilteredOut) << " test suites.";
			}
			else {
				cout << "  Passed " << numberPassed << " of " << (numberOfTestSuites-numberFilteredOut) << " test suites";
				if (numberOfSkips > 0) {
					cout << ", " << numberOfSkips << " skipped";
				}
				if (numberOfErrors > 0) {
					cout << ", " << numberOfErrors << (numberOfErrors == 1 ? " error" : " errors");
				}
				if (numberOfFailures > 0) {
					cout << ", " << numberOfFailures << " failed";
				}
				cout << ".";
			}

			if (numberFilteredOut > 0) {
				cout << "  (" << numberFilteredOut << " filtered out)";
			}
			cout << endl;

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

	using attributes_t = map<string, string>;

	// Based on code found at
	// https://stackoverflow.com/questions/5665231/most-efficient-way-to-escape-xml-html-in-c-string
	void encodeXml(string& data) {
		string buffer;
		buffer.reserve(data.size());
		for(size_t pos = 0; pos != data.size(); ++pos) {
			switch(data[pos]) {
				case '&':  buffer.append("&amp;");       break;
				case '\"': buffer.append("&quot;");      break;
				case '\'': buffer.append("&apos;");      break;
				case '<':  buffer.append("&lt;");        break;
				case '>':  buffer.append("&gt;");        break;
				default:   buffer.append(&data[pos], 1); break;
			}
		}
		data.swap(buffer);
	}

	void indent(ostream& strm, int sizeOfIndent) {
		while (sizeOfIndent-- > 0) strm << ' ';
	}

	void startTag(ostream& strm, int indentLevel, const string& name, attributes_t& attrs) {
		indent(strm, indentLevel*2);
		strm << "<" << name;
		if (attrs.empty()) {
			strm << '>' << endl;
		}
		else {
			for (auto attr : attrs) {
				encodeXml(attr.second);
				strm << ' ' << attr.first << "=\"" << attr.second << '"';
			}
			strm << '>' << endl;
		}
	}

	void startTag(ostream& strm, int indentLevel, const string& name) {
		attributes_t attrs;
		startTag(strm, indentLevel, name, attrs);
	}

	void endTag(ostream& strm, int indentLevel, const string& name) {
		indent(strm, indentLevel*2);
		strm << "</" << name << ">" << endl;
	}

	void writeXmlReportToStream(ostream& strm) {
		strm << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;

		attributes_t attrs;
		attrs["errors"] = to_string(reportSummary.numberOfErrors);
		attrs["failures"] = to_string(reportSummary.numberOfFailures);
		attrs["name"] = reportSummary.nameOfTestRun;
		attrs["tests"] = to_string(reportSummary.numberOfAssertions - reportSummary.numberOfFailures);
		attrs["time"] = to_string(reportSummary.durationOfTestRun.count());
		startTag(strm, 0, "testsuites", attrs);

		int id = 0;
		for (const auto& ts : testSuites) {
			attrs.clear();
			attrs["name"] = ts.suite->name();
			attrs["tests"] = to_string(ts.suite->_implementation()->tests.size());
			attrs["errors"] = to_string(ts.numberOfErrors);
			attrs["failures"] = to_string(ts.numberOfFailures);
			attrs["hostname"] = reportSummary.nameOfHost;
			attrs["id"] = to_string(id++);
			attrs["skipped"] = to_string(ts.numberOfSkippedTests);
			attrs["time"] = to_string(ts.durationOfTestSuite.count());
			attrs["timestamp"] = ts.timestamp;
			startTag(strm, 1, "testsuite", attrs);

			endTag(strm, 1, "testsuite");
		}

		endTag(strm, 0, "testsuites");
	}

	void printXmlReport() {
		if (!isQuiet) cout << "======================================" << endl;
		if (xmlReportFilename == "-") {
			writeXmlReportToStream(cout);
		}
		else {
			write_file(xmlReportFilename, [&](ofstream& strm) {
				writeXmlReportToStream(strm);
			});
		}
	}

	void printTestRunSummary() {
		if (!isQuiet) {
			outputStandardSummary();
		}
		if (!xmlReportFilename.empty()) {
			printXmlReport();
		}
	}

	void printTestSuiteHeader(const TestSuite& ts) {
		if (isVerbose) cout << "  " << ts.name() << endl;
	}

	void printTestSuiteSummary(const TestSuiteWrapper& w) {
		if (!isQuiet) {
			const auto* impl = w.suite->_implementation();
			if (isVerbose) {
				unsigned numberOfAssertions = 0;
				for (const auto& t : impl->tests) {
					numberOfAssertions += t.assertions;
				}

				if (!w.numberOfErrors && !w.numberOfFailures) {
					cout << "    PASSED all " << numberOfAssertions << " checks";
				}
				else {
					cout << "    Passed " << numberOfAssertions << " checks";
				}

				if (w.numberOfSkippedTests > 0) {
					cout << ", " << w.numberOfSkippedTests << " test " << (w.numberOfSkippedTests == 1 ? "case" : "cases") << " SKIPPED";
				}
				if (w.numberOfErrors > 0) {
					cout << ", " << w.numberOfErrors << (w.numberOfErrors == 1 ? " error" : " errors");
				}
				if (w.numberOfFailures > 0) {
					cout << ", " << w.numberOfFailures << " FAILED";
				}
				cout << "." << endl;

				if (w.numberOfErrors > 0) {
					cout << "    Errors:" << endl;
					for (const auto& t : impl->tests) {
						for (const auto& err : t.errors) {
							cout << "      " << err << endl;
						}
					}
				}
				if (w.numberOfFailures > 0) {
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
		if (reportSummary.numberOfErrors > 0) {
			return -1;
		}
		return reportSummary.numberOfFailures;
	}

	void runTestSuite(TestSuiteWrapper* wrapper) {
		if (!passesFilter(*wrapper->suite)) {
			wrapper->filteredOut = true;
			return;
		}

		wrapper->timestamp = now();
		printTestSuiteHeader(*wrapper->suite);
		auto* impl = wrapper->suite->_implementation();
		impl->addBeforeAndAfterAll();
		currentSuite = wrapper;

		wrapper->durationOfTestSuite = time_of_execution([&]{
			for (auto& t : impl->tests) {
				printTestCaseHeader(t);
				impl->runTestCase(t);
				printTestCaseSummary(t);
			}
		});

		currentSuite = nullptr;
		printTestSuiteSummary(*wrapper);
	}
}


namespace kss { namespace testing {

	int run(const string& testRunName, int argc, const char *const *argv) {
		reportSummary.nameOfTestRun = testRunName;
		reportSummary.nameOfHost = hostname();
		if (parseCommandLine(argc, argv)) {
			printTestRunHeader();
			sort(testSuites.begin(), testSuites.end());

			reportSummary.durationOfTestRun = time_of_execution([&]{
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
			});

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
