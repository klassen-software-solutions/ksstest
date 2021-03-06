//
//  before_after_each.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2018-04-07.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <kss/test/all.h>
#include <iostream>

using namespace std;
using namespace kss::test;

namespace {
	class BeforeEachSuite : public TestSuite, public HasBeforeEach, public HasAfterAll {
	public:
		BeforeEachSuite(const string& name, test_case_list_t fns): TestSuite(name, fns) {}

		virtual void beforeEach() override {
			++counter;
		}

		virtual void afterAll() override {
			KSS_ASSERT(counter == 4);	// 2 from the tests and 2 from the beforeEach
		}

		int counter = 0;
	};

	class AfterEachSuite : public TestSuite, public HasAfterEach, public HasAfterAll {
	public:
		AfterEachSuite(const string& name, test_case_list_t fns): TestSuite(name, fns) {}

		virtual void afterEach() override {
			++counter;
		}

		virtual void afterAll() override {
			KSS_ASSERT(counter == 4);	// 2 from the tests and 2 from the afterEach
		}

		int counter = 0;
	};

	class BeforeAndAfterEachSuite : public BeforeEachSuite, public HasAfterEach {
	public:
		BeforeAndAfterEachSuite(const string& name, test_case_list_t fns): BeforeEachSuite(name, fns) {}

		virtual void afterEach() override {
			++counter;
		}

		virtual void afterAll() override {
			KSS_ASSERT(counter == 6);	// 2 from the tests, 2 from the beforeEach and 2 from the afterEach
		}
	};
}

static BeforeEachSuite suite("BeforeEachSuite", {
	make_pair("test1", [] {
		BeforeEachSuite& bas = dynamic_cast<BeforeEachSuite&>(TestSuite::get());
		++bas.counter;
	}),
	make_pair("test2", [] {
		BeforeEachSuite& bas = dynamic_cast<BeforeEachSuite&>(TestSuite::get());
		++bas.counter;
	})
});

static AfterEachSuite suite2("AfterEachSuite", {
	make_pair("test1", [] {
		AfterEachSuite& aas = dynamic_cast<AfterEachSuite&>(TestSuite::get());
		++aas.counter;
	}),
	make_pair("test2", [] {
		AfterEachSuite& aas = dynamic_cast<AfterEachSuite&>(TestSuite::get());
		++aas.counter;
	})
});

static BeforeAndAfterEachSuite suite3("BeforeAndAfterEachSuite", {
	make_pair("test1", [] {
		BeforeAndAfterEachSuite& baaas = dynamic_cast<BeforeAndAfterEachSuite&>(TestSuite::get());
		++baaas.counter;
	}),
	make_pair("test2", [] {
		BeforeAndAfterEachSuite& baaas = dynamic_cast<BeforeAndAfterEachSuite&>(TestSuite::get());
		++baaas.counter;
	})
});

