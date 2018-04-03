# ksstest

## What is it?

This is a simple C/C++ unit testing tool. It's main advantage over other unit testing tools is that it is designed
to be easily embedded into your test applications. Specifically, you don't have to build this as a library and
add it a a prerequesite to your projects - you can simply copy the two files "ksstest.h" and "ksstest.cpp" into
your project.

Additionally, this can be used for tests that need to be written in C instead of C++. Simply rename
"ksstest.cpp" to "ksstest.c" and it will compile the C portion of the API while leaving out the C++ portion.

## License

There are no restrictions in the use of this software. You are free to use, modify and redistribute it without
restriction, other than you cannot stop others from doing the same. See the LICENSE file for details.

## Using the C API

### kss_testing_run

This is the function you call to run all the currently registered tests - typically in your main. So a typical main
file might look like the following:

```
#include "ksstest.h"

int main(int argc, const char* argv[]) {
    return kss_testing_run("My Test Program");
}
```

This will run your tests and automatically support the following command line options:

*  -h/--help displays a usage message
*  -q/--quiet suppresses the test infrastructure output. 
*  -v/--verbose displays more information. -q/--quiet will override this setting
*  -f <testprefix>/--filter=<testprefix> only run tests that start with testprefix

Depending on `-q` and `-v` there will be no, or a little, or a lot, of output written to the standard output
device. In addition if all tests pass, a return code of 0 will be returned from main. Otherwise the return
code will be the number of tests that failed.

### kss_testing_add

This is the function you use to register a test in the system. If necessary you can "extern" your actual
test functions and call `kss_testing_add` in your main function before you call `kss_testing_run`.
However if your C compiler supports the `constructor` attribute (both `gcc` and `clang` support this,
I don't know about other compilers.) In that case a file with a test could look like the following:

```
#include "ksstest.h"

static void basicCTests() {
    KSS_TEST_GROUP("C Tests");
    int i = 10;
    KSS_ASSERT(i == 10);
    KSS_WARNING("This is a warning");
    KSS_ASSERT(i < 1000);
}

void __attribute__ ((constructor)) add_c_tests() {
    kss_testing_add("basic C tests", basicCTests);
}
```

This will ensure that the test is registered before main is called.

### kss_testing_clear

This allows you to clear all the tests so that you can add new ones and run a different set. But I don't
recommend this. If you want to do this it would be better to write two separate test programs.

### KSS_ASSERT

This is the main test assertion macro. It needs to call an expression that will evaluate to 0 or non-0. 
A 0 result will indicate a failed test and be reported as such.

### KSS_TEST_GROUP

I like to put this at the start of my test functions, but it is not necessary. All it does is provide a suitable 
label in the output at runtime.

### KSS_WARNING

This is used to add a message (intended to be a warning message, but it can be anything you want it
to be) to the output in a means that is somewhat readable.

## Using the C++ API

To use the C++ API ensure that the `ksstest.cpp` file has not be renamed to have a `.c` extension.
All of the C API is still available to use, but in addition the C++ API adds the following:

### kss::testing::TestSet

This class allows you to create multiple tests as lambdas (see the restriction below) to the constructor
of a static version of this class. What this does for you is it eliminates the need for the `constructor`
attribute (since the constructor of a static class instance will accomplish what we need done) and
it calls `kss_testing_add` for you for each of the lambdas you provide. So a C++ test file may look
like the following:

```
#include "ksstest.h"
#include <stdexcept>

using namespace std;
using namespace kss::testing;

namespace {
	void function_that_does_throw() {
		throw runtime_error("hi");
	}

	void function_that_does_not_throw() noexcept {
	}
}

static TestSet ts("basic C++ tests", {
	[]{
		KSS_TEST_GROUP("group1");
		KSS_ASSERT(true);
	},
	[]{
		int i = 100;
		KSS_TEST_GROUP("group2");
		KSS_ASSERT(i > 10);
		KSS_ASSERT_EXCEPTION(function_that_does_throw(), runtime_error);
		KSS_ASSERT_NOEXCEPTION(function_that_does_not_throw());
	}
});
```

### HasBeforeAll and HasAfterAll

While you typically do not need to subclass `TestSet`, you can choose to do so in order to add code that
will be run before all the tests and after all the tests. (In earlier versions you had to rely on the fact the
the underlying C code would sort all the tests by name in order to accomplish this task.) If you do so you
must inherit from and then implement either or both of these interfaces.

Note that if you do this, you still need to make a `static` instance of the class that includes the actual
tests.

_So why do I need to subclass from the interfaces when the virtual methods are already part of the
`TestSet` API?_ The answer to this lies in the C++ RTTI limitations. There is no way (that I know of) do
determine at run time if a class has implemented a given method, but you can determine if it inherits
from a given class.

Code to use this might look like the following:

```
namespace {
	int beforeAllCount = 0;
	int afterAllCount = 0;
	int beforeEachCount = 0;
	int afterEachCount = 0;
}

class MyBeforeAllTestSet : public TestSet, public HasBeforeAll {
public:
	MyBeforeAllTestSet(const string& testName, initializer_list<test_fn> fns) : TestSet(testName, fns) {}

	virtual void beforeAll() override {
		++beforeAllCount;
	}

	bool ranBeforeAll = false;
};

class MyAfterAllTestSet : public TestSet, public HasAfterAll {
public:
	MyAfterAllTestSet(const string& testName, initializer_list<test_fn> fns) : TestSet(testName, fns) {}

	virtual void afterAll() override {
		++afterAllCount;
	}
};

class MyExtendedTestSet : public TestSet, public HasBeforeAll, public HasAfterAll {
public:
	explicit MyExtendedTestSet(const string& testName, initializer_list<test_fn> fns)
	: TestSet(testName, fns)
	{}

	virtual void beforeAll() override {
		++beforeAllCount;
	}

	virtual void afterAll() override {
		++afterAllCount;
	}

	virtual void beforeEach() override {
		++beforeEachCount;
	}

	virtual void afterEach() override {
		++afterEachCount;
	}
};

static MyAfterAllTestSet aats("extended C++ afterAll tests", {
	[]{
		KSS_TEST_GROUP("group1");
		KSS_ASSERT(beforeAllCount == 0);
		KSS_ASSERT(afterAllCount == 0);
	}
});

static MyBeforeAllTestSet bats("extended C++ beforeAll test", {
	[]{
		KSS_TEST_GROUP("group1");
		KSS_ASSERT(beforeAllCount == 1);
		KSS_ASSERT(afterAllCount == 1);
	}
});

static MyExtendedTestSet mets("extended C++ tests", {
	[]{
		KSS_TEST_GROUP("group1");
		KSS_ASSERT(beforeAllCount == 2);
		KSS_ASSERT(afterAllCount == 1);
	},
	[]{
		KSS_TEST_GROUP("group2");
	},
	[]{
		KSS_TEST_GROUP("group3");
	}
});

static TestSet afterMets("extended C++ zzz test results", {
	[]{
		KSS_TEST_GROUP("examining extended results");
		KSS_ASSERT(beforeAllCount == 2);
		KSS_ASSERT(afterAllCount == 2);
		KSS_ASSERT(beforeEachCount == 3);
		KSS_ASSERT(afterEachCount == 3);
	}
});
```

### KSS_ASSERT_EXCEPTION

This assertion is added to test if a specific exception is thrown by the given expression.

### KSS_ASSERT_NOEXCEPTION

This assertion tests that no exception is thrown by its given expression.

### KSS_ASSERT_TERMINATE

This assertion tests that `terminate()` is called by its given expression.

## Sample Code

A few examples can be found in the directory "testTheTest". 

## Limitations

###  Multithreading
This library does not deal well with threads. Unit tests really should be small and
  separate enough that this isn't a problem, but if you really need to run multiple
  threads, say to test client/server systems, you may be better off
  with a more significant test structure.

### TestSet and beforeAll/afterAll implementation

  A TestSet is implemented in the underlying C API by giving each test it contains the
  same name. The C API sorts all the tests allowing it to consider all the tests with
  the same name as related. The beforeAll and afterAll interfaces are injected before
  and after, respectively, the group of identically named tests. This is why they
  are implemented as separate interfaces, rather than as virtual methods of TestSet.
  It also implies that if you have multiple TestSet instances with the same test name,
  the beforeAll and afterAll will run before and after the full, multiple sets, and if
  they all implement a beforeAll and afterAll it is undefined which instances will
  actually be run. In other words, don't give multiple TestSet instances the same name.

### TestSet and state

  The underlying C implementation makes it difficult (impossible?) for a TestSet to
  have state. In general it is best if unit tests do not have state, but if it is
  necessary you will need to provide the state externally to the TestSet instance.

### TestSet and lambdas

Due to the underlying C implementation, the use of lambdas is somewhat restricted. In particular I have
been unable to get them to allow any sort of capture. If anyone has ideas on this I would love to hear
from them, but I may end up just redoing the whole thing in C++ from the ground up and dropping the
C interface altogether.
