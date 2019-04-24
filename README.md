# ksstest

## What is it?

This is a simple C++ unit testing tool. It's main advantage over other unit testing tools is that it is designed
to be easily embedded into your test applications. Specifically, you don't have to build this as a library and
add it a a prerequesite to your projects - you can simply copy the two files "ksstest.hpp" and "ksstest.cpp" into
your project.

Features:

* Suitable for embedding in projects (i.e. you don't have to add it as a dependancy to your project)
* Lambda-based tests
* Minimal dependance on macros (in fact there is only one)
* Very little "boilerplate" to write - your code concentrates on the tests themselves
* Expressive assertions
* Runtime test filtering
* Verbose mode useful for running tests in IDEs
* Quite mode useful for running tests in automated scripts
* Parallel or non-parallel execution
* JSON output compatible with that of JUnit
* XML output compatible with that of gUnit

[API Documentation](http://www.kss.cc/apis/ksstest/docs/index.html) 


## What has changed in V6?

Version 6 has simplified the API by no longer requiring that the test suite be passed into each test case.
Instead if you need to access the suite from within your case, you call `TestSuite::get()` in order to
obtain a reference to it. We decided to go this route since we were finding that it was very rare that
we needed the TestSuite hence it did not make sense to require it in each and every test case.


## What has changed in V5?

Version 5 is a complete, ground-up rewrite of this library. The cost of this is that
it can no longer be used in a C-only project. This decision was made as the underlying C implementation of the 
previous versions were really holding back the ability to implement the more advanced features that I wanted to 
add to the library. In addition I find myself writing less and less C code, so my need for a C testing infrastructure 
has really gone away.

Of course this does not mean that this library cannot be used to test C-only libraries. It still can. But the tests
themselves must be written in C++.

If you really do need a C-only library, you can still use V4 found on the branch `release_4.2`.


## Installing the Library

You can use this code either by building and installing it as a library (useful if you want to use it for a number
of your own private modules) or by embedding it in your project (useful if you want to provide a library 
that has minimal dependancies on other code).

In order to install it as a library, run the following commands

```
./configure
make
make check
sudo make install
```
This will create the library and install it under /usr/local. Presently this code has been tested on macOS
and Ubuntu Linux.

In order to embed it in your own code, just copy the files ksstest.hpp and ksstest.cpp into your system. They
are designed to have minimal external requirements so they should work as is with any reasonably modern
POSIX system.

## Using the Library

The header file ksstest.hpp is heavily commented with descriptions and examples. We provide a brief usage
guide here, but you should read the comments in the header file for the full documentation. In addition,
there are a number of examples found in the Tests directory ("make check" will run these tests)

### kss::test::run

This is the function you call to run all the currently registered tests - typically in your main. A typical main
file might look like the following:

```
#include <kss/test/all.h>

int main(int argc, char* argv[]) {
    return kss::test::run("My Test Program", argc, argv);
}
```

Run your program with the "--help" (or -h) command line options to receive a detailed description of how
to use the test program.

### kss::test::TestSuite

This class is how you register a test suite with the system. To use it you create a static instance of the class,
providing the test cases in the constructor. These test cases are not run in the constructor, but will be run
later when kss::test::run is called.

Each test case is represented by a pair<string, test_case_fn> object. The string is a name used to identify
the test case in the various reports.  The lambda is where you actually run the tests for the test case. 

This can look something like the following. Note that each test case is either a lambda, function, or
functional, so long as it takes no arguments and has a void return.

```
static void myVoidFunction() {
    KSS_ASSERT(...something...);
}

namespace {
    struct myVoidFunctional {
        void operator()() {
            KSS_ASSERT(...something...);
        }
    };
}

static TestSuite basicTests("Basic Tests", {
    make_pair("test1", [] {
        KSS_ASSERT(true);
    }),
    make_pair("testAssertionTypes", []{
        KSS_ASSERT(isTrue([]{ return true; }));
        KSS_ASSERT(isFalse([]{ return false; }));
        KSS_ASSERT(isEqualTo<int>(10, []{ return 10; }));
        KSS_ASSERT(isNotEqualTo<int>(10, []{ return 11; }));
        KSS_ASSERT(throwsException<runtime_error>([]{ throw runtime_error("hi"); }));
        KSS_ASSERT(doesNotThrowException([]{}));
    }),
    make_pair("function", myVoidFunction),
    make_pair("functional", myVoidFunctional())
});
```

### TestSuite Modifiers

Most often you will find that you can use TestSuite without any modifications. However, sometimes you will
need to subclass it in order to handle your specific needs. In addition to subclassing from TestSuite,
there are a number of interfaces (pure virtual classes) that you can inherit from in order to change the way
the tests are handled in a TestSuite. Here is a brief list of the modifiers and what they do. See the header
file for more complete documentation and the Tests directory for examples.

* HasBeforeAll: allows for code that will run before any of the tests in the suite are run
* HasAfterAll: allows for code that will run after any of the tests in the suite are run
* HasBeforeEach: allows for code that will run before each test in the suite
* HasAfterEach: allows for code that will run after each test in the suite
* MustNotBeParallel: ensures the test suite will always be run in series regardless of the command line options

If you need to access your subclass you combine the `TestSuite::get()` with a `dynamic_cast` in 
order to obtain access. For example,

```
class MyTestSuite : public TestSuite { 
public:
    void myCustomFunction();
    ...remainder of class definition...
};

static MyTestSuite mts("Test suite name", {
    make_pair("test1", [] {
        auto& ts = dynamic_cast<MyTestSuite&>(TestSuite::get());
        ts.myCustomFunction();
    })
});
```

### kss::test::skip

Calling this from within a test case will cause any tests from that point on (within the test case only) to be 
skipped. Typically this would be called as the first line of a test case when you want to leave out the test
case. There are two common reasons for wanting to do this:

* You are following a form of test-first programming and don't want your automated tests always failing. In this
case you can use the skip method to have your tests written but skipped, and the developers can remove
the skip statements as they complete the implementations of the code.
* A particular test case is failing. The ideal solution is to fix the code so the test case doesn't fail (otherwise
why spend the time writing the test case), but sometimes that isn't an immediate option.

### KSS_ASSERT

This macro is used to perform the individual test assertions. It acts somewhat like the standard assert
macro, except that instead of causing execution to halt, it records the result and continues. Using it is as
simple as giving it the test expression. In the case of a failure it will automatically record the test name, the
filename, and the line number of the failure (which is why it must be a macro).

### Other Assertions

The KSS_ASSERT macro is suitable for use on its own for simple, one-line expressions. However as your
expressions get complex, and especially if they require logic and multiple lines, the macro can become
cumbersome. For that reason we provide a number of lambda-based function calls that can be used with
the KSS_ASSERT macro to make your tests more readable. In addition, many of these functions will
display more information than the simple assertion. For example, an `isEqualTo` will not only show
the failed expression, it will also show the value of the actual result.

We list the functions here, but you should
read the header file for the full documentation of each, and see the Tests directory for examples of their
use. Note that each of the functions returns a boolean value and would typically be embedded in a 
single KSS_ASSERT call. For example,

```
KSS_ASSERT(isTrue([]{
    ...bunch of code...
    ...eventually returning a value that should be true...
    ...the assertion will fail if the lambda returns false...
});
```

* isTrue: determines if a block of code returns a true value
* isFalse: determines if a block of code returns a false value
* isEqualTo<T>: determines if a block of code returns a specific value
* isNotEqualTo<T>: determines if a block of code does not return a specific value
* isCloseTo<T>: determines if a block of code returns a value within a given tolerance
* isNotCloseTo<T>: determines if a block of code returns a value not within a given tolerance
* throwsException<E>: determines if a block of code throws a specific exception
* throwsSystemErrorWithCategory: determines if a block throws an std::system_error with a given category
* throwsSystemErrorWithCode: determines if a block throws an std::system_error with a given code
* doesNotThrowException: determines if a block of code throws no exceptions
* completesWithin<Duration>: determines if a block of code completes within a given time
* terminates: determines if a block of code causes terminate() to be called

### Calling KSS_ASSERT Within a Thread

In order to have the ability to run the test suites in parallel, we make use of some thread local
storage to keep KSS_ASSERT "happy." However, this has the result that if you create your
own threads within a test case, that thread local storage will not be initialized (or more
accuratly, it will be initialized to null values). As a result the KSS_ASSERT calls will fail
on an assertion (in debug mode) or an illegal access or SEGV (in non-debug mode).

One way to work around this would be to not call KSS_ASSERT within your threads, but
rather to store the necessary results and call KSS_ASSERT after your threads have 
completed. Needless to say, that can become cumbersome.

To make this easier the TestSuite class has the ability to obtain the current test case
context and to set it. You can use this to ensure that the thread local storage in your
thread is initialized properly before you make any KSS_ASSERT calls. This would look
like the following (the example assumes that we are adding a test case to a test suite):

```
make_pair("manual thread", [] {
    auto& ts = TestSuite::get();
    thread th { [&ts, ctx=ts.testCaseContext()] {
        ts.setTestCaseContext(ctx);
        KSS_ASSERT(true);
    }};
    th.join();
}),
```

The important thing to remember is that `testCaseContext()` must be called in the original
thread that is running the test case while `setTestCaseContext()` must be called in your
target thread. In addition `ctx` should be passed into that thread by value, not by
reference. (It is just a pointer so the copy is cheap.)

See the file `Tests/bug16.cpp` for a couple of examples.

## Limitations

This code is designed to be simple to use. In accomplishing this we have used certain
programming standards that we do not typically use. In particular the use of static objects
when defining the tests makes the registration of them simpler, however it means that
tests cannot be added or removed dynamically. They must all be defined at compile
time.

Second, the tests can only be run once. At the end of the call to run(), the vector that
contains all the tests is deleted. Calling run() again is likely to cause a seg fault.

## Contributing

If you wish to make changes to this library that you believe will be useful to others, you can
contribute to the project. If you do, there are a number of policies you should follow:

* Check the issues to see if there are already similar bug reports or enhancements.
* Feel free to add bug reports and enhancements at any time.
* Don't work on things that are not part of an approved project.
* Don't create projects - they are only created the by owner.
* Projects are created based on conversations on the wiki.
* Feel free to initiate or join conversations on the wiki.
* Follow our [C++ Coding Standards](https://www.kss.cc/standards/c-.html).
