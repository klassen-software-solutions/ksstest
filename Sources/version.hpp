//
//  version.hpp
//  ksstest
//
//  Created by Steven W. Klassen on 2018-06-07.
//  Copyright © 2018 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef ksstest_version_hpp
#define ksstest_version_hpp

#include <string>

namespace kss::test {

    /*!
     Returns a string of the form x.y.z<optional tags> that describes the version
     of this library.
     */
    std::string version() noexcept;

    /*!
     Returns the text of the software license.
     */
    std::string license() noexcept;
}

#endif
