//
//  version.cpp
//  ksstest
//
//  Created by Steven W. Klassen on 2018-06-07.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//

#include "version.hpp"
#include "_version_internal.h"

using namespace std;

string kss::test::version() noexcept {
    return KSSTEST_VERSION;
}
