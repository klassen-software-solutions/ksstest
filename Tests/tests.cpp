//
//  tests.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2018-04-04.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//

#include <kss/test/all.h>
#include <cassert>
#include <stdexcept>

using namespace std;
using namespace kss::test;


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexceptions"

static void should_call_terminate() noexcept {
	throw runtime_error("hi");
}

#pragma clang diagnostic pop


static void my_test(TestSuite& self) {
}

static TestSuite basicTests("Basic Tests", {
	make_pair("test1", [](TestSuite& self){
		KSS_ASSERT(true);
		KSS_ASSERT(doesNotThrowException([]{
			assert(0 == 0);
		}));
	}),
	make_pair("test2", [](TestSuite& self){
		skip();
		KSS_ASSERT(false);
	}),
	make_pair("test3", my_test),
	make_pair("testTerminate", [](TestSuite&){
		KSS_ASSERT(terminates([]{ should_call_terminate(); }));
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


static TestSuite ts2("TestSuite with Failures", {
	make_pair("mytest", [](TestSuite& self) {
		skip();
		throw runtime_error("uncaught");
	}),
	make_pair("myTestWithError", [](TestSuite&) {
		skip();
		throw runtime_error("uncaught");
	}),
	make_pair("myTestWithFailure", [](TestSuite&) {
		skip();
		KSS_ASSERT(false);
	})
});
