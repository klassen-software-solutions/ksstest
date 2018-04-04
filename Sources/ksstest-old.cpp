//
//  ksstest.cpp
//  cckss
//
//  Created by Steven W. Klassen on 2011-10-22.
//  Copyright (c) 2011 Klassen Software Solutions. All rights reserved.  This code may
//    be used/modified/redistributed without restriction or attribution. It comes with no
//    warranty or promise of suitability.
//


#if defined(OLD)

#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ksstest.h"


typedef void (*testFnPtr)();

typedef struct _test_case { 
    const char* testName;
    size_t      numberOfChecks;
    size_t      numberOfFailures;
    short       skipThisTest;
    testFnPtr   testFn;
} test_case;


static const struct option testing_options[] = {
    { "help", no_argument, NULL, 'h' },
    { "quiet", no_argument, NULL, 'q' },
    { "verbose", no_argument, NULL, 'v' },
    { "filter", required_argument, NULL, 'f' },
    { NULL, 0, NULL, 0 }
};



static test_case*   TEST_SUITE_SINGLETON = NULL;
static size_t       testSuiteN = 0;
static size_t       testSuitePhysN = 0;
static test_case*   CURRENT_TC = NULL;
static size_t       PRINTED_DOTS = 0;
static size_t       MAX_DOTS = 80;


// Used to handle the optional "beforeAll" and "afterAll" C++ items.
#if !defined(__cplusplus)
static inline void perform_before_all(const char* testName) {}
static inline void perform_after_all(const char* testName) {}
static inline void perform_test_call(const char* testName, testFnPtr fn) { fn(); }
static inline void perform_cpp_cleanup() {}
#else
namespace {
	void perform_before_all(const char* testName);
	void perform_after_all(const char* testName);
	void perform_test_call(const char* testName, testFnPtr fn);
	void perform_cpp_cleanup() noexcept;
}
#endif

// Used to pad lines for a consistent appearance.
static void create_dot_padding(char* buf) {
    size_t ii = 0;
    if (PRINTED_DOTS < MAX_DOTS) {
        while (PRINTED_DOTS < MAX_DOTS) {
            buf[ii] = ' ';
            ++PRINTED_DOTS;
            ++ii;
        }
    }
    buf[ii] = '\0';
}

// Compare two test cases by name.
static int compare_test_case_by_name(const void* a, const void* b) {
    const test_case* tca = (const test_case*)a;
    const test_case* tcb = (const test_case*)b;
    return strcmp(tca->testName, tcb->testName);
}

// Add a test case.
void kss_testing_add(const char* testName, void (*testFn)()) {
    // Ensure the test suite has space for at least one more entry.
    if (!TEST_SUITE_SINGLETON) {
        TEST_SUITE_SINGLETON = (test_case*)malloc(10 * sizeof(test_case));
        if (!TEST_SUITE_SINGLETON) {
            fprintf(stderr, "FATAL: Could not allocate memory for the test suite.\n");
            exit(-2);
        }
        testSuiteN = 0;
        testSuitePhysN = 10;
    }

    if (testSuiteN >= testSuitePhysN) {
        size_t newN = testSuitePhysN + (testSuitePhysN / 2) + 1;
        TEST_SUITE_SINGLETON = (test_case*)realloc(TEST_SUITE_SINGLETON, newN * sizeof(test_case));
        if (!TEST_SUITE_SINGLETON) {
            fprintf(stderr, "FATAL: Could not allocate memory for the test suite.\n");
            exit(-2);
        }
        testSuitePhysN = newN;
    }

    // Add the test case to the test suite.
    test_case* tc = TEST_SUITE_SINGLETON+testSuiteN;
    tc->testName = testName;
    tc->testFn = testFn;
    tc->numberOfChecks = 0;
    tc->numberOfFailures = 0;
    tc->skipThisTest = 0;
    ++testSuiteN;
}

// Clear all the test cases.
void kss_testing_clear(void) {
    testSuiteN = 0;
	perform_cpp_cleanup();
}

// Flush the current output.
static int quiet = 0;
static int verbose = 0;
static const char* groupPadding = "      ";
static void flush_output(size_t* failedTests) {
    if (CURRENT_TC->skipThisTest)
        return;

	perform_after_all(CURRENT_TC->testName);
    if (!CURRENT_TC->numberOfFailures) {
        char dot_padding[MAX_DOTS+1];
        create_dot_padding(dot_padding);
        if (!quiet) {
            if (verbose)    printf("\n%sPASSED %lu checks.\n", groupPadding, CURRENT_TC->numberOfChecks);
            else            printf("%s PASSED %lu checks.\n", dot_padding, CURRENT_TC->numberOfChecks);
        }
    }
    else {
        ++(*failedTests);
        if (!quiet) {
            printf("\n%sFAILED %lu of %lu checks.\n", groupPadding, CURRENT_TC->numberOfFailures, CURRENT_TC->numberOfChecks);
        }
    }
    if (!quiet) fflush(stdout);
}

// returns 1 if str starts with prefix and 0 otherwise.
static int starts_with(const char* str, const char* prefix) {
    /* All strings - even empty ones - start with an empty prefix. */
    if (!prefix)
        return 1;
    size_t plen = strlen(prefix);
    if (!plen)
        return 1;

    /* Empty strings, or strings shorter than the prefix, cannot start with the prefix. */
    if (!str)
        return 0;
    size_t slen = strlen(str);
    if (slen < plen)
        return 0;

    /* Compare the start of the string to see if it matches the prefix. */
    return (strncmp(str, prefix, plen) == 0);
}

// Run the test suite.
static size_t nameFieldSize = 0;
static int testing_run_fn(const char* testSuiteName, int isQuiet, int isVerbose, const char* filter) {
    // Set the global parameters.
    quiet = isQuiet;
    verbose = isVerbose;

    // Check that there are tests to run.
    if (!quiet) printf("Running test suite %s...\n", testSuiteName);
    if (!TEST_SUITE_SINGLETON) {
        if (!quiet) printf("  No tests currently defined.\n");
        return -1;
    }

    // Perform some computations for our output.
    nameFieldSize = 0;
    size_t i, len = testSuiteN;
    for (i = 0 ; i < len ; ++i) {
        test_case* tc = TEST_SUITE_SINGLETON+i;
        size_t slen = strlen(tc->testName);
        if (slen > nameFieldSize)
            nameFieldSize = slen;

    }

    char testNameFormat[100];
    sprintf(testNameFormat, "  %%-%lus ", (unsigned long)nameFieldSize);
    MAX_DOTS = 80 - 25 - nameFieldSize;

    // Run the tests.
    size_t failedTests = 0;
    size_t skippedTests = 0;
    if (testSuiteN > 1) {
        qsort(TEST_SUITE_SINGLETON, testSuiteN, sizeof(test_case), compare_test_case_by_name);
    }

    size_t numberOfTestCases = 0;
    for (i = 0 ; i < len ; ++i) {
        test_case* tc = TEST_SUITE_SINGLETON+i;
        if (filter && !starts_with(tc->testName, filter))
            tc->skipThisTest = 1;

        if (!CURRENT_TC || strcmp(CURRENT_TC->testName, tc->testName)) {
			if (CURRENT_TC) {
                flush_output(&failedTests);
			}
            if (!quiet && !tc->skipThisTest) {
                if (verbose) {
                    MAX_DOTS = 80 - strlen(tc->testName) - 1;
                    printf("  %s ", tc->testName);
                }
                else {
                    printf(testNameFormat, tc->testName);
                }
				perform_before_all(tc->testName);
            }
            CURRENT_TC = tc;
            PRINTED_DOTS = 0;
            ++numberOfTestCases;
            if (tc->skipThisTest)
                ++skippedTests;
        }

		if (!tc->skipThisTest) {
			perform_test_call(tc->testName, tc->testFn);
		}
    }
    flush_output(&failedTests);

    // Print out a summary and clean up.
    if (!quiet) {
        if (!failedTests)
            printf("  PASSED all %lu test cases.", (unsigned long)(numberOfTestCases-skippedTests));
        else
            printf("  FAILED %lu of %lu test cases.",
                   (unsigned long)failedTests, (unsigned long)(numberOfTestCases-skippedTests));

        if (filter) {
            printf(" (Filtering on '%s' skipped %lu test cases.)", filter, (unsigned long)skippedTests);
        }
        printf("\n");
    }

    kss_testing_clear();
    return (int)failedTests;
}

// Create a duplicate of argv that is not const.
static char** duplicate_argv(int argc, const char* const* argv) {
    int i;
    assert(argc > 0);
    char** newargv = (char**)malloc(sizeof(char*)*(size_t)argc);
    if (!newargv)
        return NULL;

    for (i = 0; i <argc; ++i)
        newargv[i] = (char*)argv[i];
    return newargv;
}

int kss_testing_run(const char* testSuiteName, int argc, const char* const* argv) {
    int ret = 0;
    
    // Handle the command line arguments.
    int localQuiet = 0;
    int localVerbose = 0;
    char* filter = NULL;
    char** newargv = NULL;

    if (argc > 0 && argv != NULL) {
        int ch = 0;

        newargv = duplicate_argv(argc, argv);
        if (!newargv) {
            goto setup_error;
        }

        while ((ch = getopt_long(argc, newargv, "hqvf:", testing_options, NULL)) != -1) {
            switch (ch) {
                case 'h':
                    fprintf(stdout, "usage: %s [opts]\n", (argc > 0 ? argv[0] : "unknown"));
                    fprintf(stdout, "  where opts may be any of the following\n");
                    fprintf(stdout, "   -h/--help - display this help message\n");
                    fprintf(stdout, "   -q/--quiet - quite mode, suppress the test result output\n");
                    fprintf(stdout, "   -v/--verbose - verbose mode, display more details test result output\n");
                    fprintf(stdout, "   -f <prefix>/--filter=<prefix> - only run tests that have the given prefix\n");
                    fprintf(stdout, "  return value will be one of the following\n");
                    fprintf(stdout, "   -1 - indicates there were no tests defined\n");
                    fprintf(stdout, "   -2 - indicates there was a problem configuring the tests\n");
                    fprintf(stdout, "   0 - all tests ran with no errors\n");
                    fprintf(stdout, "   <positive integer> - the number of failed tests\n");
                    ret = 0;
                    goto done;
                case 'q':
                    localQuiet = 1;
                    break;
                case 'v':
                    localVerbose = 1;
                    break;
                case 'f':
                    filter = strdup(optarg);
                    break;
            }
        }
    }

    // Run the tests.
    ret = testing_run_fn(testSuiteName, localQuiet, localVerbose, filter);
    goto done;

setup_error:
    ret = -2;
    goto done;

done:
    if (filter) free(filter);
    if (newargv) free(newargv);
    return ret;
}

// Record a test success.
void _kss_testing_success(void) {
    if (!quiet) {
        if (verbose) {
            if (PRINTED_DOTS < MAX_DOTS) {
                ++PRINTED_DOTS;
                printf(".");
                fflush(stdout);
            }
        }
        else {
            if (!(CURRENT_TC->numberOfChecks % 10) && (PRINTED_DOTS < MAX_DOTS)) {
                ++PRINTED_DOTS;
                printf("+");
                fflush(stdout);
            }
        }
    }

    ++CURRENT_TC->numberOfChecks;
}

// Record a test failure.
void _kss_testing_failure(const char* exp, const char* filename, unsigned int line) {
    if (CURRENT_TC) {
        ++CURRENT_TC->numberOfChecks;
        ++CURRENT_TC->numberOfFailures;
    }
    if (!quiet) {
        printf("\n%sFAILED '%s' at %s:%u", groupPadding, exp, filename, line);
        fflush(stdout);
    }
}

// Print a warning in a manner that fits our formatting.
void _kss_testing_warning(const char* msg) {
    if (!quiet) {
        if (verbose) {
            printf("\n%sWARNING: %s ", groupPadding, msg);
        }
        else {
            char format[200];
            sprintf(format, "...\nWARNING: %%s\n  %%-%lus...", (unsigned long)(nameFieldSize+PRINTED_DOTS-2));
            fprintf(stdout, format, msg, "");
        }
    }
}

// Print out the group.
void _kss_testing_group(const char* name) {
    if (verbose && !quiet) {
        printf("\n%s%s ", groupPadding, name);
        MAX_DOTS = 80 - strlen(name) - strlen(groupPadding) + 1;
        PRINTED_DOTS = 0;
    }
}


// MARK: C++ extensions
#if defined(__cplusplus)

#include <exception>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include <cxxabi.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;
using namespace kss::testing;

static void my_signal_handler(int sig) {
}

static void my_terminate_handler() {
    _exit(0);           // Correct response.
}

int kss::testing::_test_terminate(std::function<void ()> lambda) {

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
            lambda();
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
            return 1;   // success
        return 0;       // failure
    }
}

// Build a description of an exception.
string kss::testing::_test_build_exception_desc(const std::exception &e) {
    int ignoreMe = 0;
    const string exceptionName = abi::__cxa_demangle(typeid(e).name(), 0, 0, &ignoreMe);

    ostringstream os;
    os << exceptionName << " (" << e.what() << ") thrown by ";
    return os.str();
}

// Handle the optional beforeAll and afterAll code for a test set. Note that we don't
// have the ability (yet) to determine if beforeAll and afterAll have actually been
// provided. But by checking if they have been subclassed we can at least rule out
// the cases where just a "normal" TestSet is used.
namespace {
	static unordered_map<string, TestSet*> testSetMap;

	void perform_before_all(const char* testName) {
		const auto it = testSetMap.find(testName);
		if (it != testSetMap.end()) {
			if (auto hba = dynamic_cast<HasBeforeAll*>(it->second)) {
				if (verbose) printf("\n%sbeforeAll", groupPadding);
				hba->beforeAll();
			}
		}
	}

	void perform_after_all(const char* testName) {
		const auto it = testSetMap.find(testName);
		if (it != testSetMap.end()) {
			if (auto haa = dynamic_cast<HasAfterAll*>(it->second)) {
				if (verbose) printf("\n%safterAll", groupPadding);
				haa->afterAll();
			}
		}
	}

	void perform_test_call(const char* testName, testFnPtr fn) {
		const auto it = testSetMap.find(testName);
		if (it == testSetMap.end()) {
			fn();
		}
		else {
			it->second->beforeEach();
			fn();
			it->second->afterEach();
		}
	}

	void perform_cpp_cleanup() noexcept {
		testSetMap.clear();
	}
}

void TestSet::register_instance() {
	testSetMap[_testName] = this;
}


// Handle the addition of a test.
void TestSet::add_test(test_fn fn) const noexcept {
	kss_testing_add(_testName.c_str(), fn);
}



#endif
#endif

