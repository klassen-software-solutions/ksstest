//
//  bug30.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2019-11-26.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//

#include <thread>
#include <kss/test/all.h>

using namespace std;
using namespace kss::test;


static TestSuite ts("bug30", {
make_pair("short delay returns false", [] {
    KSS_ASSERT(completesWithin(1s, [] { this_thread::sleep_for(1ns); }));
    KSS_ASSERT(!completesWithin(10ms, [] { this_thread::sleep_for(15ms); }));
}),
make_pair("long delay terminates", [] {
    KSS_ASSERT(terminates([] {
        (void) completesWithin(10ms, [] { this_thread::sleep_for(100ms); });
    }));
})
});
