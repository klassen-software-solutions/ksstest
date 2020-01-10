//
//  tests.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2018-04-04.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <kss/test/all.h>
#include <cassert>
#include <cerrno>
#include <stdexcept>
#include <kss/test/all.h>

using namespace std;
using namespace kss::test;


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexceptions"
#pragma clang diagnostic ignored "-Wunknown-warning-option"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wterminate"

static void should_call_terminate() noexcept {
	throw runtime_error("hi");
}

#pragma GCC diagnostic pop
#pragma clang diagnostic pop


static void myFunctionTest() {
    KSS_ASSERT(true);
}

namespace {
    struct myFunctionalTest {
        void operator()() {
            KSS_ASSERT(true);
        }
    };
}

static TestSuite basicTests("Basic Tests", {
make_pair("test1", []{
    KSS_ASSERT(true);
    KSS_ASSERT(doesNotThrowException([]{
        assert(0 == 0);
    }));
}),
make_pair("test2", []{
    skip();
    KSS_ASSERT(false);
}),
make_pair("function", myFunctionTest),
make_pair("functional", myFunctionalTest()),
make_pair("testTerminate", []{
    KSS_ASSERT(terminates([]{ should_call_terminate(); }));
}),
make_pair("testAssertionTypes", []{
    KSS_ASSERT(isTrue([]{ return true; }));
    KSS_ASSERT(isFalse([]{ return false; }));
    KSS_ASSERT(isEqualTo<int>(10, []{ return 10; }));
    KSS_ASSERT(isNotEqualTo<int>(10, []{ return 11; }));
    KSS_ASSERT(throwsException<runtime_error>([]{ throw runtime_error("hi"); }));
    KSS_ASSERT(doesNotThrowException([]{}));
    KSS_ASSERT(throwsSystemErrorWithCategory(system_category(), [] {
        throw system_error(EIO, system_category());
    }));
    KSS_ASSERT(throwsSystemErrorWithCode(error_code(EIO, system_category()), [] {
        throw system_error(EIO, system_category());
    }));
    KSS_ASSERT(completesWithin(1s, []{}));
}),
make_pair("isCloseTo", [] {
    KSS_ASSERT(isCloseTo<int>(10, 2, []{ return 11; }));
    KSS_ASSERT(isCloseTo<int>(10, []{ return 10; }));
    KSS_ASSERT(isCloseTo<double>(10.001, 0.01, []{ return 10.0; }));
    KSS_ASSERT(isCloseTo<double>(10.1, []{ return 10.1; }));
}),
make_pair("isNotCloseTo", [] {
    KSS_ASSERT(isNotCloseTo<int>(10, 2, []{ return 13; }));
    KSS_ASSERT(isNotCloseTo<int>(10, []{ return 11; }));
    KSS_ASSERT(isNotCloseTo<double>(10.1, 0.01, []{ return 10.0; }));
    KSS_ASSERT(isNotCloseTo<double>(10.1, []{ return 10.0001; }));
}),
make_pair("quiet and verbose", [] {
    // No meaningfull test since we don't know what mode we are running. But at least
    // we can check that the API doesn't change.
    if (isVerbose()) { KSS_ASSERT(!isQuiet()); }
    if (isQuiet()) { KSS_ASSERT(!isVerbose()); }
}),
make_pair("isLessThan", [] {
    KSS_ASSERT(isLessThan<int>(10, []{ return 9; }));
    KSS_ASSERT(isLessThan<double>(-10., []{ return -11.; }));
    KSS_ASSERT(isLessThan<string>(string("hello"), [] { return string("hallo"); }));
}),
make_pair("isLessThanOrEqualTo", [] {
    KSS_ASSERT(isLessThanOrEqualTo<int>(10, []{ return 9; }));
    KSS_ASSERT(isLessThanOrEqualTo<int>(10, []{ return 10; }));
    KSS_ASSERT(isLessThanOrEqualTo<double>(-10., []{ return -11.; }));
    KSS_ASSERT(isLessThanOrEqualTo<double>(-10., []{ return -10.; }));
    KSS_ASSERT(isLessThanOrEqualTo<string>(string("hello"), [] { return string("hallo"); }));
    KSS_ASSERT(isLessThanOrEqualTo<string>(string("hello"), [] { return string("hello"); }));
}),
make_pair("isGreaterThan", [] {
    KSS_ASSERT(isGreaterThan<int>(10, []{ return 11; }));
    KSS_ASSERT(isGreaterThan<double>(-10., []{ return -9.; }));
    KSS_ASSERT(isGreaterThan<string>(string("hello"), [] { return string("hzllo"); }));
}),
make_pair("isGreaterThanOrEqualTo", [] {
    KSS_ASSERT(isGreaterThanOrEqualTo<int>(10, []{ return 11; }));
    KSS_ASSERT(isGreaterThanOrEqualTo<int>(10, []{ return 10; }));
    KSS_ASSERT(isGreaterThanOrEqualTo<double>(-10., []{ return -9.; }));
    KSS_ASSERT(isGreaterThanOrEqualTo<double>(-10., []{ return -10.; }));
    KSS_ASSERT(isGreaterThanOrEqualTo<string>(string("hello"), [] { return string("hzllo"); }));
    KSS_ASSERT(isGreaterThanOrEqualTo<string>(string("hello"), [] { return string("hello"); }));
})
});


static TestSuite ts2("TestSuite with Failures", {
	make_pair("myTestWithError", [] {
		skip();
		throw runtime_error("uncaught");
	}),
	make_pair("myTestWithFailure", [] {
		skip();
		KSS_ASSERT(false);
	})
});
