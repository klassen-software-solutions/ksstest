//
//  ksstest.hpp
//  ksstest
//
//  Created by Steven W. Klassen on 2018-04-03.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//
// However a license is hereby granted for this code to be used, modified and redistributed
// without restriction or requirement, other than you cannot hinder anyone else from doing
// the same.

#ifndef ksstest_h
#define ksstest_h

#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>

namespace kss {
	namespace testing {

		// MARK: Running

		/*!
		 Run the currently registered tests and report their results.

		 Command line arguments (unsupported arguments are quietly ignored allowing
		 you to add to this list):

		 -h/--help displays a usage message
		 -q/--quiet suppress test result output. (Useful if all you want is the return value.)
		 -v/--verbose displays more information (-q will override this if present)
		 -f <testprefix>/--filter=<testprefix> only run tests that start with the prefix
		 --xml=<filename> writes a JUnit test compatible XML to the given filename
		 --json=<filename> writes a Google test compatible JSON to the given filename
		 --no-parallel if you want to force everything to run serially (assumed if --verbose is specified)

		 Returns one of the following values:
		  0 - all tests passed
		  -1 - one or more error conditions were reported
		  >0 - the number of tests that failed

		 @throws std::invalid_argument if any of the arguments are missing or wrong.
		 @throws std::runtime_error if something goes wrong with the run itself.
		 */
		int run(const std::string& testRunName, int argc, const char* const* argv);

		/*!
		 Call this within a test if you want to skip it. Typically this would be the first
		 line of your test, but it does not have to be. The the will be aborted at the
		 point this is called, and the test will be marked as skipped regardless of whether
		 it had passed or failed up to that point.

		 If you place this in the beforeEach method, then all the test cases for that test
		 suite will be skipped. If you place it in the afterEach method, then the test cases
		 for that suite will run, but will still be marked as skipped.

		 Note that the skip is implemented by throwing an exception and must not be caught
		 or the skip will not be performed. The exception is NOT subclassed from std::exception,
		 so as long as your code does not catch (...) it will be fine.
		 */
		void skip();

		// MARK: Assertions

		/*!
		 Macro to perform a single test. This works like assert but instead of halting
		 execution it updates the internal status of the test suit.
		 */
#		define KSS_ASSERT(expr) ((void) ((expr) ? _private::_success() : _private::_failure(#expr, __FILE__, __LINE__)))

		// The following are intended to be used inside KSS_ASSERT in order to make the
		// test intent clearer. They are especially useful if your test contains multiple
		// lines of code. For single line tests you may want to jsut use the assertion
		// on its own. For example,
		//  KSS_ASSERT(isEqualTo(3, []{ return i; }));
		// isn't really any clearer than
		//  KSS_ASSERT(i == 3);
		// However,
		//  KSS_ASSERT(isEqualTo(3, []{
		//     auto val = obtainAValueFromSomewhere();
		//     doSomeWorkOnTheValue(&val);
		//     return val;
		//  }));
		// is much clearer than trying to do it all in the assertions expr argument.

		/*!
		 Returns true if the lambda returns true.
		 example:
		   KSS_ASSERT(isTrue([]{ return true; }));
		 */
		inline bool isTrue(std::function<bool(void)> fn) { return fn(); }

		/*!
		 Returns true if the lambda returns false.
		 example:
		   KSS_ASSERT(isFalse([]{ return false; }));
		 */
		inline bool isFalse(std::function<bool(void)> fn) { return !fn(); }

		/*!
		 Returns true if the lambda returns a value that is equal to a.
		 example:
		   KSS_ASSERT(isEqualTo(23, []{ return 21+2; }));
		 */
		template <class T>
		bool isEqualTo(const T& a, std::function<T(void)> fn) {
			return (fn() == a);
		}

		/*!
		 Returns true if the lambda returns a value that is not equal to a.
		 example:
		   KSS_ASSERT(isNotEqualTo(23, []{ return 21-2; }));
		 */
		template <class T>
		bool isNotEqualTo(const T& a, std::function<T(void)> fn) {
			return (!(fn() == a));
		}

		/*!
		 Returns true if the lambda throws an exception of the given type.
		 example:
		   KSS_ASSERT(throwsException<std::runtime_error>([]{
		       throw std::runtime_error("hello");
		   }));
		 */
		template <class Exception>
		bool throwsException(std::function<void(void)> fn) {
			bool caughtCorrectException = false;
			try {
				fn();
			}
			catch (const Exception&) {
				caughtCorrectException = true;
			}
			catch (...) {
			}
			return caughtCorrectException;
		}

		/*!
		 Returns true if the lambda does not throw any exception.
		 example:
		   KSS_ASSERT(doesNotThrowException([]{ doSomeWork(); }));
		 */
		bool doesNotThrowException(std::function<void(void)> fn);

		/*!
		 Returns true if the lambda causes terminate to be called.
		 example:
		 inline void cannot_throw() noexcept { throw std::runtime_error("hi"); }

		 KSS_ASSERT(terminates([]{ cannot_throw() }));
		 */
		bool terminates(std::function<void(void)> fn);


		// MARK: TestSuite

		/*!
		 Use this class to define the test suites to be run. Most of the time you will
		 likely just use this class "as is" providing the necessary tests in the constructor.
		 But you can also subclass it if you need to modify its default behaviour.
		 */
		class TestSuite {
		public:
			using test_case_fn = std::function<void(TestSuite&)>;
			using test_case_list = std::initializer_list<std::pair<std::string, test_case_fn>>;

			/*!
			 Construct a test suite. All the test cases are lambdas that conform to test_case_fn.
			 They are all included in the constructor and a static version of each TestSuite
			 must be declared. (It is the static declaration that will register the test
			 suite with the internal infrastructure.

			 Note that the lambdas are not run in the construction. They will be run at
			 the appropriate time when kss::testing::run is called.
			 */
			explicit TestSuite(const std::string& testSuiteName, test_case_list fns);
			virtual ~TestSuite() noexcept;

			TestSuite(TestSuite&&);
			TestSuite& operator=(TestSuite&&);

			TestSuite(const TestSuite&) = delete;
			TestSuite& operator=(const TestSuite&) = delete;

			/*!
			 Accessors
			 */
			const std::string& name() const noexcept;

		private:
			friend int run(const std::string& testRunName, int argc, const char* const* argv);

			struct Impl;
			std::unique_ptr<Impl> _impl;

		public:
			// Never call this! It must be public to allow access in some of the
			// implementation internals.
			const Impl* _implementation() const noexcept { return _impl.get(); }
			Impl* _implementation() noexcept { return _impl.get(); }
		};


		// MARK: TestSuite Modifiers

		// Most of the time you will likely use TestSuite "as is" just providing the
		// tests in the constructor. However, you can modify it's default behavious by
		// creating a subclass that inherits from one or more of the following interfaces.

		/*!
		 Extend your TestSuite with these interfaces if you wish it to call code before
		 (or after) all all the tests in your TestSuite. It is acceptable to perform
		 tests in these methods - they will be treated as a test case called "BeforeAll"
		 and "AfterAll", respectively.
		 */
		class HasBeforeAll {
		public:
			virtual void beforeAll() = 0;
		};

		class HasAfterAll {
		public:
			virtual void afterAll() = 0;
		};

		/*!
		 Extend your TestSuite with these interfaces if you wish it to call code before
		 (or after) each of the tests in your TestSuite. It is acceptable to perform
		 tests in these methods - they will be treated as part of each test case.
		 */
		class HasBeforeEach {
		public:
			virtual void beforeEach() = 0;
		};

		class HasAfterEach {
		public:
			virtual void afterEach() = 0;
		};

		/*!
		 Extend your TestSuite with this interface if you wish it to always be run on
		 the main thread. Note that it still may run in parallel with other TestSuites
		 that do not use this interface. But any that inherit from this interface will
		 not run in parallel with each other.

		 (There are no methods in this interface. Simply having your class inherit from
		 it will be enough.)
		 */
		class MustNotBeParallel {
		};

		namespace _private {
			// This namespace defines items that need to be publically available for syntactical
			// reasons but should be treated as private and never called manually.
			void _success(void) noexcept;
			void _failure(const char* expr, const char* filename, unsigned int line) noexcept;
		}
	}
}

#endif /* ksstest_hpp */