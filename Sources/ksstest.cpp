//
//  ksstest.cpp
//  ksstest
//
//  Created by Steven W. Klassen on 2018-04-03.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//
// However a license is hereby granted for this code to be used, modified and redistributed
// without restriction or requirement, other than you cannot hinder anyone else from doing
// the same.

#include "ksstest.h"

using namespace std;
using namespace kss::testing;

// MARK: Running

namespace kss { namespace testing {

	int run(const string& testRunName, int argc, const char *const *argv) {
		// TODO: complete this
		return 0;
	}

}}


// MARK: Assertions

namespace kss { namespace testing {

	bool doesNotThrowException(function<void(void)> fn) {
		bool caughtSomething = false;
		try {
			fn();
		}
		catch (...) {
			caughtSomething = true;
		}
		return !caughtSomething;
	}

	bool terminates(function<void(void)> fn) {
		// TODO: complete this
		return false;
	}
}}


// MARK: TestSuite::Impl Implementation

struct TestSuite::Impl {
	string name;
};


// MARK: TestSuite Implementation

TestSuite::TestSuite(const string& testSuiteName, initializer_list<test_case_fn> fns) : _impl(new Impl()) {
	_impl->name = testSuiteName;

	// TODO: figure out how to register the fns
}

TestSuite& TestSuite::operator=(TestSuite&& ts) {
	if (this != &ts) {
		_impl = move(ts._impl);
		ts._impl.reset();
	}
	return *this;
}

TestSuite::~TestSuite() noexcept {}
TestSuite::TestSuite(TestSuite&& ts) : _impl(move(ts._impl)) {}

const string& TestSuite::name() const noexcept {
	return _impl->name;
}
