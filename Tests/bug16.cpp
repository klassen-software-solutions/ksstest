//
//  bug16.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2019-01-16.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//

#include <future>
#include <thread>
#include <kss/test/all.h>

using namespace std;
using namespace kss::test;


static TestSuite ts("bug16", {
    make_pair("thread", [] {
        auto& ts = TestSuite::get();
        thread th { [&ts, ctx=ts.testCaseContext()] {
            ts.setTestCaseContext(ctx);
            KSS_ASSERT(true);
//            KSS_ASSERT(false);
        }};
        th.join();
    }),
    make_pair("async", [] {
        auto& ts = TestSuite::get();
        auto fut = async([&ts, ctx=ts.testCaseContext()]{
            ts.setTestCaseContext(ctx);
            KSS_ASSERT(true);
//            KSS_ASSERT(false);
        });
        fut.wait();
    })
});
