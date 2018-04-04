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
		 -p/--parallel if you wish to run the tests in parallel (-v will be ignored if present)
		 -f <testprefix>/--filter=<testprefix> only run tests that start with the prefix
		 --xml produces no output while running, but a JUnit compatible XML when completed
		 --json produces no output file running, but a Google test compatible JSON when completed

		 Returns one of the following values:
		  0 - all tests passed
		  >0 - the number of tests that failed

		 @throws std::invalid_argument if any of the arguments are missing or wrong.
		 @throws std::runtime_error if something goes wrong with the run itself.
		 */
		int run(const std::string& testRunName, int argc, const char* const* argv);

		// MARK: Assertions

		/*!
		 Macro to perform a single test. This works like assert but instead of halting
		 execution it updates the internal status of the test suit.
		 */
#		define KSS_ASSERT(expr) ((void) ((expr) ? _success() : _failure(#expr, __FILE__, __LINE__)))

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
			using test_case_fn = std::function<void(const std::string& testCaseName)>;

			/*!
			 Construct a test suite. All the test cases are lambdas that conform to test_case_fn.
			 They are all included in the constructor and a static version of each TestSuite
			 must be declared. (It is the static declaration that will register the test
			 suite with the internal infrastructure.

			 Note that the lambdas are not run in the construction. They will be run at
			 the appropriate time when kss::testing::run is called.
			 */
			explicit TestSuite(const std::string& testSuiteName,
							   const std::initializer_list<test_case_fn> fns);
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
			struct Impl;
			std::unique_ptr<Impl> _impl;
		};


		// MARK: TestSuite Modifiers

		// Most of the time you will likely use TestSuite "as is" just providing the
		// tests in the constructor. However, you can modify it's default behavious by
		// creating a subclass that inherits from one or more of the following interfaces.

		/*!
		 Extend your TestSuite with this interface if you wish it to call code before
		 and after all all the tests in your TestSuite. (Note that you must provide both
		 or neither of the methods, although you can make them empty methods if desired.)
		 */
		class HasBeforeAll {
		public:
			virtual void beforeAll() = 0;
			virtual void afterAll() = 0;
		};

		/*!
		 Extend your TestSuite with this interface if you wish it to call code before and
		 after each of the tests in your TestSuite. (Note that you must provide both or
		 neither of the methods, although you can make them empty methods if desired.)
		 */
		class HasBeforeEach {
		public:
			virtual void beforeEach() = 0;
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
