//
//  c_api.c
//  testTheTest
//
//  Created by Steven W. Klassen on 2018-02-17.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.  This code may
//    be used/modified/redistributed without restriction or attribution. It comes with no
//    warranty or promise of suitability.
//

#include "ksstest.h"


static void basicCTests() {
	KSS_TEST_GROUP("C Tests");
	int i = 10;
	KSS_ASSERT(i == 10);
	KSS_WARNING("This is a warning");
	KSS_ASSERT(i < 1000);
}

void __attribute__ ((constructor)) add_c_tests() {
	kss_testing_add("basic C tests", basicCTests);
}
