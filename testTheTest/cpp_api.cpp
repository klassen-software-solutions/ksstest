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
using kss::testing::TestSet;

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
