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

// https://tools.ietf.org/html/rfc7049
#include "main.h"
#include <iostream>
#include <fstream>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestAssert.h>

using namespace std;

CPPUNIT_TEST_SUITE_REGISTRATION( CborTest );

void CborTest::testGet()
{
    CPPUNIT_ASSERT_EQUAL(get<int>(this->i), 1);
    CPPUNIT_ASSERT_EQUAL(get<float>(this->f), static_cast<float>(1.1));
    CPPUNIT_ASSERT_EQUAL(get<string>(this->s), string("Hello World!"));
    CPPUNIT_ASSERT_EQUAL(get<vector<cbor_byte>>(this->b)[0], static_cast<cbor_byte>('b'));
    CPPUNIT_ASSERT_EQUAL(get<cbor_array>(this->a)[0], cbor_variant { 1 });
    CPPUNIT_ASSERT_EQUAL(get<cbor_map>(this->m)["Aye"], cbor_variant { 1 });
}

void CborTest::testNested()
{
    CPPUNIT_ASSERT_EQUAL(get<float>(get<cbor_array>(get<cbor_map>(this->m)["Eh"])[1]), static_cast<float>(1.1));
}

void CborTest::testType()
{
    CPPUNIT_ASSERT_EQUAL(this->i.index(), static_cast<unsigned long>(cbor_variant::integer));
    CPPUNIT_ASSERT_EQUAL(this->f.index(), static_cast<unsigned long>(cbor_variant::floating_point));
    CPPUNIT_ASSERT_EQUAL(this->s.index(), static_cast<unsigned long>(cbor_variant::unicode_string));
    CPPUNIT_ASSERT_EQUAL(this->n.index(), static_cast<unsigned long>(cbor_variant::none));
    CPPUNIT_ASSERT_EQUAL(this->b.index(), static_cast<unsigned long>(cbor_variant::bytes));
    CPPUNIT_ASSERT_EQUAL(this->a.index(), static_cast<unsigned long>(cbor_variant::array));
    CPPUNIT_ASSERT_EQUAL(this->m.index(), static_cast<unsigned long>(cbor_variant::map));
}

void CborTest::catchException() {
    CPPUNIT_ASSERT_THROW(get<int>(this->f), std::bad_variant_access);
}

void CborTest::roundTrip()
{
    // Integers
    for (int j : {0, 1, 23, 24, 25, 254, 255, 256, 257, 65534, 65535, 65536, 65537, 12345678,
         -1, -23, -24, -25, -254, -255, -256, -257, -65534, -65535, -65536, -65537, -12345678}) {
        cbor_variant i { j };
        this->scratchpad.clear();
        i.encode_onto(&this->scratchpad);
        auto d_i=cbor_variant::construct_from(this->scratchpad);
        CPPUNIT_ASSERT_EQUAL(get<int>(d_i), j);
    }

    // Floats
    this->scratchpad.clear();
    this->f.encode_onto(&this->scratchpad);
    auto d_f=cbor_variant::construct_from(this->scratchpad);
    CPPUNIT_ASSERT_EQUAL(get<float>(this->f), get<float>(d_f));

    // Bytes
    this->scratchpad.clear();
    this->b.encode_onto(&this->scratchpad);
    auto d_b=cbor_variant::construct_from(this->scratchpad);
    for (unsigned int i=0; i < get<vector<cbor_byte>>(this->b).size(); i++) {
        CPPUNIT_ASSERT_EQUAL(this->b[i], d_b[i]);
    }

    // Strings
    this->scratchpad.clear();
    this->s.encode_onto(&this->scratchpad);
    auto d_s=cbor_variant::construct_from(this->scratchpad);
    CPPUNIT_ASSERT_EQUAL(get<string>(this->s), get<string>(d_s));

    // Arrays
    this->scratchpad.clear();
    this->a.encode_onto(&this->scratchpad);
    auto d_a=cbor_variant::construct_from(this->scratchpad);
    CPPUNIT_ASSERT_EQUAL(get<int>(get<cbor_array>(this->a)[0]), get<int>(get<cbor_array>(d_a)[0]));
    CPPUNIT_ASSERT_EQUAL(get<float>(get<cbor_array>(this->a)[1]), get<float>(get<cbor_array>(d_a)[1]));
    CPPUNIT_ASSERT_EQUAL(get<string>(get<cbor_array>(this->a)[2]), get<string>(get<cbor_array>(d_a)[2]));

    // Maps
    this->scratchpad.clear();
    this->m.encode_onto(&this->scratchpad);
    auto d_m=cbor_variant::construct_from(this->scratchpad);
    auto the_map=get<cbor_map>(d_m);
    CPPUNIT_ASSERT_EQUAL(get<float>(the_map["Eff"]), get<float>(get<cbor_map>(this->m)["Eff"]));
    CPPUNIT_ASSERT_EQUAL(get<int>(the_map["Aye"]), get<int>(get<cbor_map>(this->m)["Aye"]));
    auto the_array=get<cbor_array>(the_map["Eh"]);
    CPPUNIT_ASSERT_EQUAL(get<string>(the_array[2]), get<string>(get<cbor_array>(get<cbor_map>(this->m)["Eh"])[2]));
}

void CborTest::pythonCompat()
{
    const int buffer_length=32;
    vector<cbor_byte> working(buffer_length);

    // Integers
    ifstream int_enc("int", ios::in|ios::binary);
    int_enc.read(reinterpret_cast<char*>(working.data()), buffer_length);
    auto int_decoded=cbor_variant::construct_from(working);
    CPPUNIT_ASSERT_EQUAL(get<int>(int_decoded), -12345678);

    // Floats
    ifstream float_enc("float", ios::in|ios::binary);
    float_enc.read(reinterpret_cast<char*>(working.data()), buffer_length);
    auto float_decoded=cbor_variant::construct_from(working);
    CPPUNIT_ASSERT_EQUAL(get<float>(float_decoded), static_cast<float>(1.1));

    // Strings
    ifstream string_enc("string", ios::in|ios::binary);
    string_enc.read(reinterpret_cast<char*>(working.data()), buffer_length);
    auto string_decoded=cbor_variant::construct_from(working);
    CPPUNIT_ASSERT_EQUAL(get<string>(string_decoded), string("smeg"));

    // Bytes
    ifstream bytes_enc("bytes", ios::in|ios::binary);
    bytes_enc.read(reinterpret_cast<char*>(working.data()), buffer_length);
    auto bytes_decoded=cbor_variant::construct_from(working);
    vector<cbor_byte> ideal { 'b', 'y', 't', 'e', 's' };
    for (unsigned int i=0; i < get<vector<cbor_byte>>(this->b).size(); i++) {
        CPPUNIT_ASSERT_EQUAL(get<vector<cbor_byte>>(bytes_decoded)[i], ideal[i]);
    }

    // None
    ifstream none_enc("none", ios::in|ios::binary);
    none_enc.read(reinterpret_cast<char*>(working.data()), buffer_length);
    auto none_decoded=cbor_variant::construct_from(working);
    get<monostate>(none_decoded);

    ifstream array_enc("array", ios::in|ios::binary);
    array_enc.read(reinterpret_cast<char*>(working.data()), buffer_length);
    auto array_decoded=cbor_variant::construct_from(working);
    CPPUNIT_ASSERT_EQUAL(get<int>(get<cbor_array>(array_decoded)[0]), 8);
    CPPUNIT_ASSERT_EQUAL(get<float>(get<cbor_array>(array_decoded)[1]), static_cast<float>(1.1));
    CPPUNIT_ASSERT_EQUAL(get<string>(get<cbor_array>(array_decoded)[2]), string("pie"));
}

int main(int argc, char* argv[])
{
    CppUnit::Test* suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();
    CppUnit::TextUi::TestRunner runner;
    runner.addTest( suite );
    runner.setOutputter( new CppUnit::CompilerOutputter( &runner.result(), std::cerr ) );
    return runner.run() ? 0 : 1;
}
