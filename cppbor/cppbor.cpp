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

#include "cppbor.hpp"
#include <cstdio>
#include <exception>
#include <string>
#include <sstream>
#include <arpa/inet.h>

using namespace std;
void cbor_variant::construct_from_into(const std::vector<uint8_t>& in, cbor_variant* dest)
{
    unsigned int dummy_offset=0;
    return construct_from_into(in, dest, &dummy_offset);
}

void cbor_variant::construct_from_into(const std::vector<uint8_t>& in, cbor_variant* dest, unsigned int* offset)
{
    // nothing to read?
    if (in.size()<=*offset) throw length_error("No header byte while decoding cbor");

    // header object
    const header* h=reinterpret_cast<const header*>(&in[*offset]);

    // integers
    switch (h->major) {
        case 0: {
            dest->emplace<integer>(read_integer_header(in, h, offset));  // +ve integer
            return;
        }

        case 1: {
            dest->emplace<integer>(-1-read_integer_header(in, h, offset));  // -ve integer
            return;
        }

        case 2: // bytes and strings
        case 3: {
            int length=read_integer_header(in, h, offset);
            if (length<0) throw runtime_error("A negative length was given for a byte array or string");
            unsigned int offset_at_begin=*offset;
            *offset+=static_cast<unsigned int>(length);
            if (in.size()<*offset) throw length_error("Insufficient data bytes while decoding cbor");
            if (h->major==2)
                dest->emplace<bytes>(vector<uint8_t>(&in[offset_at_begin], &in[offset_at_begin+static_cast<unsigned int>(length)]));
            else
                dest->emplace<unicode_string>(&in[offset_at_begin], &in[offset_at_begin+static_cast<unsigned int>(length)]);
            return;
        }

        case 4: {  // arrays
            int total_items=read_integer_header(in, h, offset);
            dest->emplace<array>(static_cast<unsigned>(total_items), cbor_variant());
            cbor_array* current_array=&get<array>(*dest);
            for (unsigned int this_item=0; this_item<static_cast<unsigned>(total_items); this_item++) {
                cbor_variant* value_dest=&current_array->at(this_item);
                construct_from_into(in, value_dest, offset);
            }
            return;
        }

        case 5: {  // maps
            dest->emplace<map>();
            cbor_map* current_map=&get<map>(*dest);
            for (int pending_items=read_integer_header(in, h, offset); pending_items>0; pending_items--) {
                // get the key
                h=reinterpret_cast<const header*>(&in[*offset]);
                if (h->major!=3) throw runtime_error("Asked to process a map entry whose key is not a string");
                int key_length=read_integer_header(in, h, offset);
                if (key_length<0) throw runtime_error("Length of a (map) key was expressed as a negative number");
                string key { string(&in[*offset], &in[*offset+static_cast<unsigned int>(key_length)]) };
                *offset+=static_cast<unsigned int>(key_length);

                // create the variant
                current_map->emplace(key, cbor_variant());
                cbor_variant* value_dest=&current_map->at(key);
                construct_from_into(in, value_dest, offset);
            }
            return;
        }

        case 6: {  // tags (are ignored)
            read_integer_header(in, h, offset);
            return;
        }

        case 7: {  // floats and none
            const uint8_t* first_data_byte=in.data()+*offset+1;
            if (h->additional==26) {  // single precision
                *offset+=5;
                float rtn;
                float_to_big_endian(first_data_byte, reinterpret_cast<uint8_t*>(&rtn), 4);
                dest->emplace<floating_point>(rtn);
                return;
            }
            if (h->additional==27) {  // double precision, gets cast down to single
                *offset+=9;
                double rtn;
                float_to_big_endian(first_data_byte, reinterpret_cast<uint8_t*>(&rtn), 8);
                dest->emplace<floating_point>(static_cast<float>(rtn));
                return;
            }
            if (h->additional==22) {
                *offset+=1;
                dest->emplace<none>();
                return;
            }
            throw runtime_error("Asked to process a major type 7 that is neither a float nor a double");
        }
    }

    // none
    throw runtime_error("Asked to handle an unknown major type");
};

// encode just this one variant
void cbor_variant::encode_onto(std::vector<uint8_t>* in) const
{
    // https://tools.ietf.org/html/rfc7049#section-2.1
    switch (index()) {
        case integer: { // integers
            int val=get<integer>(*this);
            if (val>=0) append_integer_header(0, static_cast<unsigned int>(val), in);
            else append_integer_header(1, static_cast<unsigned int>((-val)-1), in);
            return;
        }

        // https://tools.ietf.org/html/rfc7049#section-2.3
        case floating_point: { // floats
            header h(7, 26);
            h.append_onto(in);
            float val=get<floating_point>(*this);
            float big_endian;
            uint8_t* p_big_endian=reinterpret_cast<uint8_t*>(&big_endian);
            float_to_big_endian(reinterpret_cast<uint8_t*>(&val), p_big_endian, sizeof(float));
            in->insert(in->end(), p_big_endian, p_big_endian+sizeof(float));
            return;
        }

        case bytes: { // bytes
            const vector<uint8_t> val=get<bytes>(*this);
            append_integer_header(2, static_cast<unsigned int>(val.size()), in);
            in->insert(in->end(), val.begin(), val.end());
            return;
        }

        case unicode_string: {  // string
            const string val=get<unicode_string>(*this);
            append_integer_header(3, static_cast<unsigned int>(val.size()), in);
            in->insert(in->end(), val.begin(), val.end());
            return;
        }

        case array: {  // variant array
            const cbor_array val { get<array>(*this) };
            append_integer_header(4, static_cast<unsigned int>(val.size()), in);
            for (auto& v : val) v.encode_onto(in);
            return;
        }

        case map: {  // string -> variant map
            const cbor_map val { get<map>(*this) };
            append_integer_header(5, static_cast<unsigned int>(val.size()), in);
            for (auto& v : val) {
                // write the string key
                append_integer_header(3, static_cast<unsigned int>(v.first.size()), in);
                in->insert(in->end(), v.first.begin(), v.first.end());
                // and the value
                v.second.encode_onto(in);
            }
            return;
        }

        default: { // none (monostate)
            in->push_back(0xf6);
        }
    }
}

string cbor_variant::as_python() const
{
    switch (index()) {
        case integer: return to_string(get<integer>(*this));
        case floating_point: return to_string(get<floating_point>(*this));
        case unicode_string: return "\""+get<unicode_string>(*this)+"\"";
        case bytes: {
            stringstream stream;
            stream << "bytes([";
            for (auto& v : get<bytes>(*this)) stream << "0x" << hex << static_cast<int>(v) << ", ";
            string rtn(stream.str());
            if (rtn.size()!=7) rtn.erase(rtn.size()-2);
            return rtn+"])";
        }
        case array: {
            string rtn { "[" };
            for (auto& v : get<array>(*this)) rtn+=v.as_python()+", ";
            if (rtn.size()!=1) rtn.erase(rtn.size()-2);
            return rtn+"]";
        }
        case map: {
            string rtn { "{" };
            for (auto& v : get<map>(*this)) rtn+="\""+v.first+"\": "+v.second.as_python()+", ";
            if (rtn.size()!=1) rtn.erase(rtn.size()-2);
            return rtn+"}";
        }
    }
    return "None";
}

void cbor_variant::read_file_into(const char* name, vector<uint8_t>* dest)
{
    // open file, find it's size
    FILE* f=fopen(name, "r");
    if (f==nullptr)
        throw runtime_error(name);
    fseek(f , 0 , SEEK_END);
    size_t size=static_cast<size_t>(ftell(f));
    rewind(f);

    // create and load into buffer
    dest->resize(size);
    fread(dest->data(), size, 1, f);
    fclose(f);
}

unsigned int cbor_variant::integer_length(int additional)
{
    if (additional<24) return 1;   // just the header
    if (additional==24) return 2;  // header plus one byte
    if (additional==25) return 3;  // header plus a short
    return 5;  // header plus an int
}

void cbor_variant::append_integer_header(unsigned int major, unsigned int val, std::vector<uint8_t>* in)
{
    if (val<24) { header(major, val).append_onto(in); return; }
    if (val<256) { header_byte(major, 24, static_cast<uint8_t>(val)).append_onto(in); return; }
    if (val<65536) { header_short(major, 25, htons(val)).append_onto(in); return; }
    header_int(major, 26, htonl(val)).append_onto(in);
}

int cbor_variant::read_integer_header(const std::vector<uint8_t>& in, const header* h, unsigned int* offset)
{
    unsigned int first_offset=*offset;
    *offset+=integer_length(h->additional);
    if (h->additional<24) return h->additional;
    if (in.size()<*offset) throw length_error("Insufficient additional size byte(s) while decoding cbor");
    const uint8_t* p_data=&in[first_offset+1];
    switch (h->additional) {
        case 24: return static_cast<int>(*p_data);
        case 25: return static_cast<int>(ntohs(*reinterpret_cast<const unsigned short*>(p_data)));
        case 26: return static_cast<int>(ntohl(*reinterpret_cast<const unsigned int*>(p_data)));
        case 27: throw range_error("This implementation does not support 64 bit integers");
        case 31: throw runtime_error("This implementation does not support indefinite length types");
        default: throw runtime_error("Don't know how to handle additional data in header");
    }
}

void cbor_variant::float_to_big_endian(const uint8_t* p_src, uint8_t* p_dest, unsigned int bytes)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    p_dest+=bytes-1;
    while (bytes>0) {
        *p_dest=*p_src;
        ++p_src;
        --p_dest;
        --bytes;
    }
#else
    while (bytes>0) {
        *p_dest=*p_src;
        ++p_src;
        ++p_dest;
        --bytes;
    }
}
#endif
};
