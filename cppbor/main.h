//Copyright (c) 2018 David Preece, All rights reserved.

//Permission to use, copy, modify, and/or distribute this software for any
//purpose with or without fee is hereby granted.

//THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#ifndef MAIN_H
#define MAIN_H
#include "cppbor.hpp"
#include <string>
#include <vector>
#include <cppunit/extensions/HelperMacros.h>

class CborTest : public CppUnit::TestFixture
{
public:
    CPPUNIT_TEST_SUITE( CborTest );
    CPPUNIT_TEST( testGet );
    CPPUNIT_TEST( testNested );
    CPPUNIT_TEST( testType );
    CPPUNIT_TEST( catchException );
    CPPUNIT_TEST( roundTrip );
    CPPUNIT_TEST( pythonCompat );
    CPPUNIT_TEST_SUITE_END();

    void testGet();
    void testNested();
    void testType();
    void catchException();
    void roundTrip();
    void pythonCompat();

private:
    cbor_variant i { 1 };
    cbor_variant f { static_cast<float>(1.1) };
    cbor_variant s { std::string("Hello World!") };
    cbor_variant n { std::monostate() };  // 'None' in Python
    cbor_variant b { std::vector<cbor_byte> { 'b', 'y', 't', 'e', 's' } };
    cbor_variant a { cbor_array {
                        i,
                        f,
                        s} };
    cbor_variant m { cbor_map {
                        {std::string("Aye"), i},
                        {std::string("Eff"), f},
                        {std::string("Eh"), a}}};

    std::vector<cbor_byte> scratchpad;
};

#endif // MAIN_H
