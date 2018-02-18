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
