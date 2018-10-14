//
//  version.hpp
//  ksstest
//
//  Created by Steven W. Klassen on 2018-06-07.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//

#ifndef ksstest_version_hpp
#define ksstest_version_hpp

#include <string>

namespace kss {
    namespace test {

        /*!
         Returns a string of the form x.y.z<optional tags> that describes the version
         of this library.
         */
        std::string version() noexcept;

    }
}

#endif /* version_hpp */