#include <iostream>
#include <chrono>
#include "cppbor/cppbor.hpp"

using namespace std;
using namespace std::chrono;

int main()
{
    // open file, find it's size
    FILE* f=fopen("../cbor-python-wrote", "r");
    fseek(f , 0 , SEEK_END);
    size_t size=static_cast<size_t>(ftell(f));
    rewind(f);

    // create and load into buffer
    vector<uint8_t> cbor_data_in(size);
    fread(cbor_data_in.data(), size, 1, f);
    fclose(f);

    // decode cbor
    high_resolution_clock::time_point start=high_resolution_clock::now();
    cbor_variant cbor_original;
    cbor_variant::construct_from_into(cbor_data_in, &cbor_original);
    high_resolution_clock::time_point end=high_resolution_clock::now();

    // report
    duration<float> time_taken=duration_cast<duration<float>>(end-start);
    cout << "Places: " << get<cbor_map>(cbor_original).size() << endl;
    cout << "Parse time: " << time_taken.count() << endl;
    // cout << cbor_original.as_python() << endl;
}
