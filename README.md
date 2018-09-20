# ksstest

## What is it?

This is a simple C++ unit testing tool. It's main advantage over other unit testing tools is that it is designed
to be easily embedded into your test applications. Specifically, you don't have to build this as a library and
add it a a prerequesite to your projects - you can simply copy the two files "ksstest.h" and "ksstest.cpp" into
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


## What has changed from V4?

Version 5 is a complete, ground-up, and non-back-compatible rewrite of this library. The cost of this is that
it can no longer be used in a C-only project. This decision was made as the underlying C implementation of the 
previous versions were really holding back the ability to implement the more advanced features that I wanted to 
add to the library. In addition I find myself writing less and less C code, so my need for a C testing infrastructure 
has really gone away.

Of course this does not mean that this library cannot be used to test C-only libraries. It still can. But the tests
themselves must be written in C++.


## License

There are no restrictions in the use of this software. You are free to use, modify and redistribute it without
restriction, other than you cannot stop others from doing the same. See the LICENSE file for details.

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
This will create the library and install it under /usr/local. At present the configure file doesn't do anything,
but this will likely change in the near future to allow the compilers, environment variables and install location
to be modified. Presently this code has been tested on macOS and Ubuntu Linux.

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
the test case in the various reports. The function is a lambda taking a reference to a TestSuite and returning 
nothing. The lambda is where you actually run the tests for the test case. (The reference to the TestSuite is to 
allow you to reference state in the TestSuite instance. Doing so is generally a sign of a poorly designed unit test,
but there are times when it is necessary.)

This can look something like the following:

```
static TestSuite basicTests("Basic Tests", {
    make_pair("test1", [](TestSuite&){
        KSS_ASSERT(true);
    }),
    make_pair("testAssertionTypes", [](TestSuite&){
        KSS_ASSERT(isTrue([]{ return true; }));
        KSS_ASSERT(isFalse([]{ return false; }));
        KSS_ASSERT(isEqualTo<int>(10, []{ return 10; }));
        KSS_ASSERT(isNotEqualTo<int>(10, []{ return 11; }));
        KSS_ASSERT(throwsException<runtime_error>([]{ throw runtime_error("hi"); }));
        KSS_ASSERT(doesNotThrowException([]{}));
    })
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
the KSS_ASSERT macro to make your tests more readable. We list the functions here, but you should
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
* throwsException<E>: determines if a block of code throws a specific exception
* doesNotThrowException: determines if a block of code throws no exceptions
* terminates: determines if a block of code causes terminate() to be called
