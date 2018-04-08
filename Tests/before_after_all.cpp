//
//  before_after_all.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2018-04-07.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//

#include <ksstest.h>

using namespace std;
using namespace kss::testing;


namespace {
	class BeforeAllSuite : public TestSuite, public HasBeforeAll {
	public:
		BeforeAllSuite(const string& name, test_case_list fns) : TestSuite(name, fns) {}

		virtual void beforeAll() override {
			KSS_ASSERT(counter == 0);
		}

		int counter = 0;
	};

	class AfterAllSuite : public TestSuite, public HasAfterAll {
	public:
		AfterAllSuite(const string& name, test_case_list fns) : TestSuite(name, fns) {}

		virtual void afterAll() override {
			KSS_ASSERT(counter == 2);
		}

		int counter = 0;
	};

	class BeforeAndAfterAllSuite : public BeforeAllSuite, public HasAfterAll {
	public:
		BeforeAndAfterAllSuite(const string& name, test_case_list fns)
		: BeforeAllSuite(name, fns) {}

		virtual void afterAll() override {
			KSS_ASSERT(counter == 2);
		}
	};
}

static BeforeAllSuite suite("BeforeAllTest", {
	make_pair("test1", [](TestSuite& self) {
		BeforeAllSuite& bas = dynamic_cast<BeforeAllSuite&>(self);
		++bas.counter;
	}),
	make_pair("test2", [](TestSuite& self) {
		BeforeAllSuite& bas = dynamic_cast<BeforeAllSuite&>(self);
		++bas.counter;
	})
});

static AfterAllSuite suite2("AfterAllTest", {
	make_pair("test1", [](TestSuite& self) {
		AfterAllSuite& aas = dynamic_cast<AfterAllSuite&>(self);
		++aas.counter;
	}),
	make_pair("test2", [](TestSuite& self) {
		AfterAllSuite& aas = dynamic_cast<AfterAllSuite&>(self);
		++aas.counter;
	})
});

static BeforeAndAfterAllSuite suite3("BeforeAndAfterAllTest", {
	make_pair("test1", [](TestSuite& self) {
		BeforeAndAfterAllSuite& baaas = dynamic_cast<BeforeAndAfterAllSuite&>(self);
		++baaas.counter;
	}),
	make_pair("test2", [](TestSuite& self) {
		BeforeAndAfterAllSuite& baaas = dynamic_cast<BeforeAndAfterAllSuite&>(self);
		++baaas.counter;
	})
});

