//
//  version.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2018-06-07.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//

#include <kss/test/all.h>

using namespace std;
using namespace kss::test;


static TestSuite ts("version", {
    make_pair("version", [](TestSuite&) {
        KSS_ASSERT(!kss::test::version().empty());
        KSS_ASSERT(!kss::test::license().empty());
    })
});
