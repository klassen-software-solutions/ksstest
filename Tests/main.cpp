//
//  main.cpp
//  unittest
//
//  Created by Steven W. Klassen on 2018-04-04.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//
// However a license is hereby granted for this code to be used, modified and redistributed
// without restriction or requirement, other than you cannot hinder anyone else from doing
// the same.

#include <ksstest.h>
#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {
	try {
		return kss::testing::run("KSSTest", argc, argv);
	}
	catch (const exception& e) {
		cerr << e.what() << endl;
		return -1;
	}
}
