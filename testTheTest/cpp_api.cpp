//
//  cpp_api.cpp
//  testTheTest
//
//  Created by Steven W. Klassen on 2018-02-17.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.  This code may
//    be used/modified/redistributed without restriction or attribution. It comes with no
//    warranty or promise of suitability.
//

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


class MyExtendedTestSet : public TestSet {
public:
	explicit MyExtendedTestSet(const string& testName, initializer_list<test_fn> fns)
	: TestSet(testName, fns)
	{}

	virtual void beforeAll() override {
		++beforeAllCallCount;
	}

	virtual void afterAll() override {
		++afterAllCallCount;
	}

	int beforeAllCallCount = 0;
	int afterAllCallCount = 0;
};

static MyExtendedTestSet mets("extended C++ tests", {
	[]{
		KSS_TEST_GROUP("group1");
	}
});

static TestSet afterMets("zzz extended C++ test results", {
	[]{
		KSS_TEST_GROUP("examining extended results");
		KSS_ASSERT(mets.beforeAllCallCount == 1);
		KSS_ASSERT(mets.afterAllCallCount == 1);
	}
});
