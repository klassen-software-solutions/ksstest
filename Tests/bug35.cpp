//
//  bug35.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2019-12-21.
//  Copyright © 2019 Klassen Software Solutions. All rights reserved.
//

#include <kss/test/all.h>

using namespace std;
using namespace kss::test;


static TestSuite ts("bug35", {
make_pair("isNotEqualTo description", [] {
    // Can't actually run these tests in the CI since I need to see their failure output.
    // Comment out the skip to actually see the results.
    skip();
    KSS_ASSERT(isNotEqualTo<int>(1, [] { return 1; }));
    KSS_ASSERT(isEqualTo<int>(1, [] { return 2; }));
})
});
