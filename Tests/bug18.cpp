//
//  bug18.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2019-01-17.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//

#include <kss/test/all.h>

using namespace std;
using namespace kss::test;


static TestSuite ts("bug18", {
    // The bug is that this will not compile.
    make_pair("string", [] {
        KSS_ASSERT(isEqualTo<string>("hello world", [] {
            return string("hello world");
        }));
    })
});
