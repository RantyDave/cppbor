// https://tools.ietf.org/html/rfc7049
// https://stackoverflow.com/questions/23071024/how-do-i-make-xcode-use-an-alternative-version-of-clang
#include <iostream>
#include <fstream>
#include "cppbor.hpp"
using namespace std;

int main(int argc, const char * argv[]) {

    cbor_variant i { 1 };
    cbor_variant f { (float)1.1 };
    cbor_variant s { string("Hello World!") };
    cbor_variant n { std::monostate() };  // 'None' in Python
    cbor_variant b { vector<cbor_byte> { 'b', 'y', 't', 'e', 's' } };
    cbor_variant a { cbor_array {
                        i,
                        f,
                        s} };
    cbor_variant m { cbor_map {
                        {string("Aye"), i},
                        {string("Eff"), f},
                        {string("Eh"), a} }};

    cout << get<int>(i) << " " << get<float>(f) << " " << get<string>(s) << endl;
    cout << get<string>(get<cbor_array>(a)[2]) << endl;
    cout << get<int>(get<cbor_map>(m)[string("Aye")]) << endl;
    cout << get<float>(get<cbor_array>(get<cbor_map>(m)[string("Eh")])[1]) << endl;

    try {
        get<int>(f);
    } catch(const exception& e) {
        cout << e.what() << endl;
    }

    vector<cbor_byte> scratchpad;

    for (int j : {0, 1, 23, 24, 25, 254, 255, 256, 257, 65534, 65535, 65536, 65537, 12345678,
         -1, -23, -24, -25, -254, -255, -256, -257, -65534, -65535, -65536, -65537, -12345678}) {
        cbor_variant i { j };
        scratchpad.clear();
        i.encode_onto(&scratchpad);
        auto d_i=cbor_variant::construct_from(scratchpad);
        cout << get<int>(d_i) << endl;
    }

    scratchpad.clear();
    f.encode_onto(&scratchpad);
    auto d_f=cbor_variant::construct_from(scratchpad);
    cout << get<float>(d_f) << endl;

    scratchpad.clear();
    b.encode_onto(&scratchpad);
    auto d_b=cbor_variant::construct_from(scratchpad);
    for (cbor_byte b : get<vector<cbor_byte>>(d_b)) {
        cout << b;
    }
    cout << endl;

    scratchpad.clear();
    s.encode_onto(&scratchpad);
    auto d_s=cbor_variant::construct_from(scratchpad);
    cout << get<string>(d_s) << endl;

    scratchpad.clear();
    n.encode_onto(&scratchpad);
    auto d_n=cbor_variant::construct_from(scratchpad);
    get<monostate>(d_n);  // monostate doesn't convert to const void* so we can't cout it.

    scratchpad.clear();
    a.encode_onto(&scratchpad);
    auto d_a=cbor_variant::construct_from(scratchpad);
    cout << get<int>(get<cbor_array>(d_a)[0]) << ' '
         << get<float>(get<cbor_array>(d_a)[1]) << ' '
         << get<string>(get<cbor_array>(d_a)[2]) << endl;

    scratchpad.clear();
    m.encode_onto(&scratchpad);
    auto d_m=cbor_variant::construct_from(scratchpad);
    auto the_map=get<cbor_map>(d_m);
    auto the_array=get<cbor_array>(the_map["Eh"]);
    cout << get<float>(the_map["Eff"]) << ' '
         << get<int>(the_map["Aye"]) << ' '
         << get<string>(the_array[2]) << endl;

    // python compat
    // if you're getting unexpected crashes in here, make sure you've
    // * run sources.py
    // * in the same directory as you are running the exe in
    const int buffer_length=32;
    vector<cbor_byte> working(buffer_length);

    ifstream int_enc("int", ios::in|ios::binary);
    int_enc.read((char*)(void*)working.data(), buffer_length);
    auto int_decoded=cbor_variant::construct_from(working);
    cout << get<int>(int_decoded) << endl;

    double test_dble=1.1;

    ifstream float_enc("float", ios::in|ios::binary);
    float_enc.read((char*)(void*)working.data(), buffer_length);
    auto float_decoded=cbor_variant::construct_from(working);
    cout << get<float>(float_decoded) << endl;

    ifstream string_enc("string", ios::in|ios::binary);
    string_enc.read((char*)(void*)working.data(), buffer_length);
    auto string_decoded=cbor_variant::construct_from(working);
    cout << get<string>(string_decoded) << endl;

    ifstream bytes_enc("bytes", ios::in|ios::binary);
    bytes_enc.read((char*)(void*)working.data(), buffer_length);
    auto bytes_decoded=cbor_variant::construct_from(working);
    for (cbor_byte b : get<vector<cbor_byte>>(bytes_decoded)) {
        cout << b;
    }
    cout << endl;

    ifstream none_enc("none", ios::in|ios::binary);
    none_enc.read((char*)(void*)working.data(), buffer_length);
    auto none_decoded=cbor_variant::construct_from(working);
    get<monostate>(none_decoded);

    ifstream array_enc("array", ios::in|ios::binary);
    array_enc.read((char*)(void*)working.data(), buffer_length);
    auto array_decoded=cbor_variant::construct_from(working);
    cbor_variant zero=get<cbor_array>(array_decoded)[0];
    cbor_variant one=get<cbor_array>(array_decoded)[1];
    cbor_variant two=get<cbor_array>(array_decoded)[2];

    cout << get<int>(zero) << endl;
    cout << get<float>(one) << endl;
    cout << get<string>(two) << endl;

    return 0;
}
