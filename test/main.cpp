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
    CPPUNIT_ASSERT_EQUAL(get<double>(this->f), 1.1);
    CPPUNIT_ASSERT_EQUAL(get<string>(this->s), string("Hello World!"));
    CPPUNIT_ASSERT_EQUAL(get<vector<uint8_t>>(this->b)[0], static_cast<uint8_t>('b'));
    CPPUNIT_ASSERT_EQUAL(get<cbor_array>(this->a)[0], cbor_variant { 1 });
    CPPUNIT_ASSERT_EQUAL(get<cbor_map>(this->m)["Aye"], cbor_variant { 1 });
}

void CborTest::testNested()
{
    CPPUNIT_ASSERT_EQUAL(get<double>(get<cbor_array>(get<cbor_map>(this->m)["Eh"])[1]), static_cast<double>(1.1));
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
        cbor_variant dest_int=cbor_variant::construct_from(this->scratchpad);
        CPPUNIT_ASSERT_EQUAL(get<int>(dest_int), j);
    }

    // Floats
    this->scratchpad.clear();
    this->f.encode_onto(&this->scratchpad);
    cbor_variant dest_float=cbor_variant::construct_from(this->scratchpad);
    CPPUNIT_ASSERT_EQUAL(get<double>(this->f), get<double>(dest_float));

    // Bytes
    this->scratchpad.clear();
    this->b.encode_onto(&this->scratchpad);
    cbor_variant dest_bytes=cbor_variant::construct_from(this->scratchpad);
    for (unsigned int i=0; i < get<vector<uint8_t>>(this->b).size(); i++) {
        CPPUNIT_ASSERT_EQUAL(this->b[i], dest_bytes[i]);
    }

    // Strings
    this->scratchpad.clear();
    this->s.encode_onto(&this->scratchpad);
    cbor_variant dest_string=cbor_variant::construct_from(this->scratchpad);
    CPPUNIT_ASSERT_EQUAL(get<string>(this->s), get<string>(dest_string));

    // Arrays
    this->scratchpad.clear();
    this->a.encode_onto(&this->scratchpad);
    cbor_variant dest_array=cbor_variant::construct_from(this->scratchpad);
    CPPUNIT_ASSERT_EQUAL(get<int>(get<cbor_array>(this->a)[0]), get<int>(get<cbor_array>(dest_array)[0]));
    CPPUNIT_ASSERT_EQUAL(get<double>(get<cbor_array>(this->a)[1]), get<double>(get<cbor_array>(dest_array)[1]));
    CPPUNIT_ASSERT_EQUAL(get<string>(get<cbor_array>(this->a)[2]), get<string>(get<cbor_array>(dest_array)[2]));

    // Maps
    this->scratchpad.clear();
    this->m.encode_onto(&this->scratchpad);
    cbor_variant dest_map=cbor_variant::construct_from(this->scratchpad);
    auto the_map=get<cbor_map>(dest_map);
    CPPUNIT_ASSERT_EQUAL(get<double>(the_map["Eff"]), get<double>(get<cbor_map>(this->m)["Eff"]));
    CPPUNIT_ASSERT_EQUAL(get<int>(the_map["Aye"]), get<int>(get<cbor_map>(this->m)["Aye"]));
    auto the_array=get<cbor_array>(the_map["Eh"]);
    CPPUNIT_ASSERT_EQUAL(get<string>(the_array[2]), get<string>(get<cbor_array>(get<cbor_map>(this->m)["Eh"])[2]));
}

void CborTest::pythonCompat()
{
    vector<uint8_t> working;

    // Integers
    cbor_variant::read_file_into("int", &working);
    cbor_variant int_decoded=cbor_variant::construct_from(working);
    CPPUNIT_ASSERT_EQUAL(get<int>(int_decoded), -12345678);

    // Floats
    cbor_variant::read_file_into("float", &working);
    cbor_variant float_decoded=cbor_variant::construct_from(working);
    CPPUNIT_ASSERT_EQUAL(get<double>(float_decoded), static_cast<double>(1.1));

    // Strings
    cbor_variant::read_file_into("string", &working);
    cbor_variant string_decoded=cbor_variant::construct_from(working);
    CPPUNIT_ASSERT_EQUAL(get<string>(string_decoded), string("smeg"));

    // Bytes
    cbor_variant::read_file_into("bytes", &working);
    cbor_variant bytes_decoded=cbor_variant::construct_from(working);
    vector<uint8_t> ideal { 'b', 'y', 't', 'e', 's' };
    for (unsigned int i=0; i < get<vector<uint8_t>>(this->b).size(); i++) {
        CPPUNIT_ASSERT_EQUAL(get<vector<uint8_t>>(bytes_decoded)[i], ideal[i]);
    }

    // None
    cbor_variant::read_file_into("none", &working);
    cbor_variant none_decoded=cbor_variant::construct_from(working);
    get<monostate>(none_decoded);

    // Arrays
    cbor_variant::read_file_into("array", &working);
    cbor_variant array_decoded=cbor_variant::construct_from(working);
    CPPUNIT_ASSERT_EQUAL(get<int>(get<cbor_array>(array_decoded)[0]), 8);
    CPPUNIT_ASSERT_EQUAL(get<double>(get<cbor_array>(array_decoded)[1]), static_cast<double>(1.1));
    CPPUNIT_ASSERT_EQUAL(get<string>(get<cbor_array>(array_decoded)[2]), string("pie"));

    // Maps
    cbor_variant::read_file_into("map", &working);
    cbor_variant map_decoded=cbor_variant::construct_from(working);
    CPPUNIT_ASSERT_EQUAL(get<int>(get<cbor_map>(map_decoded)["one"]), 1);
    CPPUNIT_ASSERT_EQUAL(get<string>(get<cbor_map>(map_decoded)["two"]), string("deux"));
}

void CborTest::describe()
{
    cbor_variant empty_array { cbor_array { } };
    cbor_variant empty_map { cbor_map { } };
    CPPUNIT_ASSERT_EQUAL(i.as_python(), string("1"));
    CPPUNIT_ASSERT_EQUAL(f.as_python(), string("1.100000"));
    CPPUNIT_ASSERT_EQUAL(s.as_python(), string("\"Hello World!\""));
    CPPUNIT_ASSERT_EQUAL(n.as_python(), string("None"));
    CPPUNIT_ASSERT_EQUAL(b.as_python(), string("bytes([0x62, 0x79, 0x74, 0x65, 0x73])"));
    CPPUNIT_ASSERT_EQUAL(a.as_python(), string("[1, 1.100000, \"Hello World!\"]"));
    CPPUNIT_ASSERT_EQUAL(m.as_python(), string("{\"Aye\": 1, \"Eff\": 1.100000, \"Eh\": [1, 1.100000, \"Hello World!\"]}"));
    CPPUNIT_ASSERT_EQUAL(empty_array.as_python(), string("[]"));
    CPPUNIT_ASSERT_EQUAL(empty_map.as_python(), string("{}"));
}

int main(int argc, char* argv[])
{
    CppUnit::Test* suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();
    CppUnit::TextUi::TestRunner runner;
    runner.addTest( suite );
    runner.setOutputter( new CppUnit::CompilerOutputter( &runner.result(), std::cerr ) );
    return runner.run() ? 0 : 1;
}
