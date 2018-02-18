//
//  ksstest.h
//
//  Created by Steven W. Klassen on 2011-10-22.
//
//  COPYING
//
//  Copyright (c) 2011 Klassen Software Solutions. All rights reserved. This code may
//    be used/modified/redistributed without restriction or attribution. It comes with no
//    warranty or promise of suitability.
//
//  USAGE INSTRUCTIONS
//
//  This library was created to provide a simple and standalone unit testing ability to either
//  C or C++ projects. Note that there are more powerful unit testing solutions out there,
//  but if you want to add unit tests to a library that you want truly standalone, i.e. you
//  don't want to require that a specific unit testing library be downloaded and installed,
//  then this is a good option.
//
//  To add these unit tests to your system, all you need to do is add the files ksstest.h
//  and ksstest.cpp to your testing project. Then include "ksstest.h" where you need it. Once
//  done you call kss_testing_add to add tests to the framework and kss_testing_run to actually
//  run them. In C++ you can use kss::testing::TestSet to easily add tests without having to
//  explicitly reference them in your main function. In C (at least in gcc and clang) you
//  can declare a function with a constructor attribute to do much the same thing. For example,
//
//	   static void test1Fn() {
//         KSS_TEST_GROUP("group1");
//         ... assertions go here ...
//     }
//
//     static void test2Fn() {
//         KSS_TEST_GROUP("group2");
//         ... assertions go here ...
//     }
//
//     void __attribute__ ((constructor)) add_local_tests() {
//         kss_testing_add("test 1", test1Fn);
//         kss_testing_add("test 2", test2Fn);
//     }
//
//  To use this in a strictly C library you should rename ksstest.cpp to ksstest.c (or
//  add whatever flags you need to your build infrastructure to compile it using C instead
//  of C++). This will cause the C++ portions of the test structure to be left out.
//
//  LIMITATIONS
//
//  Multithreading
//
//  This library does not deal well with threads. Unit tests really should be small and
//  separate enough that this isn't a problem, but if you really need to run multiple
//  threads, say to test client/server systems, it can be done but you may be better off
//  with a more significant test structure.
//
//  TestSet and beforeAll/afterAll implementation
//
//  A TestSet is implemented in the underlying C API by giving each test it contains the
//  same name. The C API sorts all the tests allowing it to consider all the tests with
//	the same name as related. The beforeAll and afterAll interfaces are injected before
//	and after, respectively, the group of identically named tests. This is why they
//  are implemented as separate interfaces, rather than as virtual methods of TestSet.
//  It also implies that if you have multiple TestSet instances with the same test name,
//  the beforeAll and afterAll will run before and after the full, multiple sets, and if
//  they all implement a beforeAll and afterAll it is undefined which instances will
//  actually be run. In other words, don't give multiple TestSet instances the same name.
//
//  API NAMING CONVENTIONS
//
//  All macros defined here are of the form KSS_... and are all capitals.
//  All public functions are of the form kss_testing_... and are all lower case.
//  All private non-static functions (i.e. need to be public but should not be called
//  directly), are of the form _kss_testing_...  Similarly in the C++ code, any symbol
//  within kss::testing that starts with an underscore should be considered private and
//  not called directly.
//
//  EXAMPLES
//
// 	The directory "testTheTest" provides example C and C++ code that I use to test the
//	testing infrastructure.
//
//  HISTORY
//
//  V4.1 - refactored to separate from the kssutil library
//	V4.1.1 - added testTheTest to allow the testing infrastructure to itself be tested.
//
//  V4.2 - significant improvements to the C++ api
//


#ifndef ksstest_h
#define ksstest_h


#ifdef __cplusplus
extern "C" {
#endif
    
    /**
     * Macro used to perform a single test in a test function. This works like assert
     * but instead of halting execution it updates the internal status of our test
     * suite singleton.
     */
#define KSS_ASSERT(expr) ((void) ((expr) ? _kss_testing_success() : _kss_testing_failure(#expr, __FILE__, __LINE__)))
    
    /**
     * Macro used to produce a warning message in a manner that is readable with the
     * testing output.
     */
#define KSS_WARNING(msg) ((void) _kss_testing_warning(msg))

    /**
     * Macro used to divide (that is, to label) a test into groups. This only takes affect
	 * (i.e. only affects the display) when run in verbose mode.
     */
#define KSS_TEST_GROUP(name) ((void) _kss_testing_group(name))

    
    /**
     * Add a test function to the test suite. Note that every test function should be
     * independant as the test suite will be sorted before it is run. If testName is the
     * same as an existing test case, the new function will be added to that test case.
     */
    void kss_testing_add(const char* testName, void (*testFn)(void));

    /**
     * Clear all the tests from the test suite.
     */
    void kss_testing_clear(void);
    
    /**
     * Run the tests and report their results. Typically you would call kss_testing_add
     * a number of times, then kss_testing_run once. After you have run the tests the
     * test suite (which is maintained as a singleton) will be cleared. You could then
     * add tests and run them (which would be a separate test suite) but typically you
     * would have one test program that runs all your tests as a single suite.
     *
     * NOTE: This is NOT thread-safe. The tests should be added and run from the
     *  same thread.
     *
     * @param testSuiteName - only used for display purposes
     * @param argc - number of parameters in argv.
     * @param argv - optional command line parameters. May be 0, NULL if you
     *  don't want this support. The following parameters are supported. All others
     *  are quietly ignored.
     *  -h/--help displays a usage message
     *  -q/--quiet suppresses the test infrastructure output. Useful if you only want your
     *      program output or if you want it to be quietly used in a larger test structure.
     *      (You would have to rely on the return value to determine if all the tests
     *      pass.)
     *  -v/--verbose displays more information. -q/--quiet will override this setting
     *  -f <testprefix>/--filter=<testprefix> only run tests that start with testprefix
     *
     * @return one of the following values:
     *   0 - implies the tests ran and all tests passed.
     *   -1 - no tests were defined.
     *   -2 - there was a problem running the test platform
     *   >0 - implies the tests ran and the returned number of tests failed. 
     */
    int kss_testing_run(const char* testSuiteName, int argc, const char* const* argv);


    void _kss_testing_success(void);
    void _kss_testing_failure(const char* expr, const char* filename, unsigned int line);
    void _kss_testing_warning(const char* msg);
    void _kss_testing_group(const char* groupname);

#ifdef __cplusplus
}

#include <exception>
#include <functional>
#include <string>

    /*!
     Macro used to test that an expression causes terminate() to be called. Although not
     obvious, this will only work in C++ code as the set_terminate function is a C++
     function.
     */
#   define KSS_ASSERT_TERMINATE(expr) ((void)(kss::testing::_test_terminate([=]{expr}) == 1 ? _kss_testing_success() : _kss_testing_failure(#expr " did not terminate", __FILE__, __LINE__)))

namespace kss {
    namespace testing {

        int _test_terminate(std::function<void(void)> lambda);
        std::string _test_build_exception_desc(const std::exception& e);


        /*!
         Macro used to test that an exception is thrown by an expression.
         */
#       define KSS_ASSERT_EXCEPTION(expr, ex) { bool _caught=false; try { expr; } catch(ex& e) { _caught=true; } catch(...) { } ((void) (_caught ? _kss_testing_success() : _kss_testing_failure(#ex " not thrown by " #expr, __FILE__, __LINE__))); } ((void) true)

        /*!
         Macro used to test that no exception is thrown by an expression.
         */
#       define KSS_ASSERT_NOEXCEPTION(expr) { bool _caught=false; std::string _exdesc("unknown exception thrown by "); try { expr; } catch(std::exception& e) { _caught=true; _exdesc=kss::testing::_test_build_exception_desc(e); } catch(...) { _caught=true; } ((void) (!_caught ? _kss_testing_success() : _kss_testing_failure((_exdesc + #expr).c_str(), __FILE__, __LINE__))); } ((void) true)


		/*!
		 If a TestSet requires a method that will run once before all of its tests, it must
		 implement this interface.
		 */
		class HasBeforeAll {
		public:
			virtual void beforeAll() = 0;
		};

		/*!
		 If a TestSet requires a method that will run once after all of its tests has
		 completed, it must implement this interface.
		 */
		class HasAfterAll {
		public:
			virtual void afterAll() = 0;
		};


        /*!
         The TestSet class is used to provide a nicer way for C++ to add tests to this
         framework. In particular it makes use of the fact that static constructors in C++
         will execute before main() is called. This makes it possible for the constructors
         to call kss_testing_add automatically and to avoid having to create external
         references to the tests for the sake of the main() method.
         
         To use this you create a static instance of this class and pass it an initialization
         list of lambdas or function pointers conforming to the test_fn signature. These
         will be automatically added to the testing framework, all with the test name given
         in the test set constructor.
         
         It is recommended (but not required) that the first line of each of these functions
         be a KSS_TEST_GROUP call in order to annotate the groups within the test set.
         
         An example...
         
         static void myfunctionexample() {
            KSS_TEST_GROUP("function test 1");
            // a bunch of tests
         }

         static kss::testing::TestSet myTests("my_tests", {
            []{
                KSS_TEST_GROUP("lambda test 1");
                // a bunch of tests
            },
            []{
                KSS_TEST_GROUP("lambda test 2");
                // a bunch of tests
            },
            myfunctionexample
         });
         
         This will result in kss_testing_add("my_tests", ...) being called three times with
         the appropriate function pointer. (The underlying C can handle the lambdas so long
         as they require no arguments and contain no references to their context.)

		 Starting with V4.2 the TestSet may be subclassed in order to add code that is
		 run before and after the test set and before and after each test in the test set.
		 Note that all of these are optional.
         */
        class TestSet {
        public:
            using test_fn = void (*)();     // Type of functions (or lambdas) accepted by test sets.

            /*!
             Construct a test set. Note that for this to work in this infrastructure the
             test set instances must be declared statically.
             */
            explicit TestSet(const std::string& testName, std::initializer_list<test_fn> fns) : _testName(testName) {
				register_instance();
                for (const auto& fn : fns) {
                    add_test(fn);
                }
            }

			virtual ~TestSet() noexcept {
			}

			// TestSets are intended ot be static and should not be moved or copied.
			TestSet(const TestSet&) = delete;
			TestSet(TestSet&&) = delete;
			TestSet& operator=(const TestSet&) = delete;
			TestSet& operator=(TestSet&&) = delete;

        private:
            std::string _testName;

			void add_test(test_fn fn) const noexcept;
			void register_instance();
        };
    }
}
#endif

#endif


