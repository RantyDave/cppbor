#include "cppbor.hpp"
#include <exception>
#include <string>
#include <arpa/inet.h>

using namespace std;
cbor_variant cbor_variant::construct_from(const std::vector<cbor_byte>& in)
{
    unsigned int dummy_offset=0;
    return construct_from(in, &dummy_offset);
}

cbor_variant cbor_variant::construct_from(const std::vector<cbor_byte>& in, unsigned int* offset)
{
    // nothing to read?
    if (in.size()<=*offset) throw length_error("No header byte while decoding cbor");

    // header object
    const header* h=reinterpret_cast<const header*>(&in[*offset]);

    // integers
    switch (h->major) {
        case 0: return cbor_variant { read_integer_header(in, h, offset) };  // +ve integer
        case 1: return cbor_variant { -1-read_integer_header(in, h, offset) };  // -ve integer

        case 2: // bytes and strings
        case 3: {
            int length=read_integer_header(in, h, offset);
            if (length<0) throw runtime_error("A negative length was given for a byte array or string");
            unsigned int offset_at_begin=*offset;
            *offset+=static_cast<unsigned int>(length);
            if (in.size()<*offset) throw length_error("Insufficient data bytes while decoding cbor");
            if (h->major==2) return cbor_variant { vector<cbor_byte>(&in[offset_at_begin], &in[offset_at_begin+static_cast<unsigned int>(length)]) };
            else return cbor_variant { string(&in[offset_at_begin], &in[offset_at_begin+static_cast<unsigned int>(length)]) };
        }

        case 4: {  // arrays
            cbor_array rtn;
            for (int pending_items=read_integer_header(in, h, offset); pending_items>0; pending_items--)
                rtn.push_back(construct_from(in, offset));
            return cbor_variant { rtn };
        }

        case 5: {  // maps
            cbor_map rtn;
            for (int pending_items=read_integer_header(in, h, offset); pending_items>0; pending_items--) {
                // get the key
                h=reinterpret_cast<const header*>(&in[*offset]);
                if (h->major!=3) throw runtime_error("Asked to process a map entry whose key is not a string");
                int key_length=read_integer_header(in, h, offset);
                if (key_length<0) throw runtime_error("Length of a (map) key was expressed as a negative number");
                string key { string(&in[*offset], &in[*offset+static_cast<unsigned int>(key_length)]) };
                *offset+=static_cast<unsigned int>(key_length);
                // get the variant
                rtn[key]=construct_from(in, offset);
            }
            return cbor_variant { rtn };
        }

        case 6: {  // tags (are ignored)
            read_integer_header(in, h, offset);
            // but we return the actual object that the tag referred to
            return construct_from(in, offset);
        }

        case 7: {  // floats and none
            const cbor_byte* first_data_byte=in.data()+*offset+1;
            if (h->additional==26) {  // single precision
                *offset+=5;
                float rtn;
                float_to_big_endian(first_data_byte, reinterpret_cast<cbor_byte*>(&rtn), 4);
                return cbor_variant { rtn };
            }
            if (h->additional==27) {  // double precision, gets cast down to single
                *offset+=9;
                double rtn;
                float_to_big_endian(first_data_byte, reinterpret_cast<cbor_byte*>(&rtn), 8);
                return cbor_variant { static_cast<float>(rtn) };
            }
            if (h->additional==22) {
                *offset+=1;
                return cbor_variant { monostate() };
            }
            throw runtime_error("Asked to process a major type 7 that is neither a float nor a double");
        }
    }

    // none
    throw runtime_error("Asked to handle an unknown major type");
};

// encode just this one variant
void cbor_variant::encode_onto(std::vector<cbor_byte>* in) const
{
    // https://tools.ietf.org/html/rfc7049#section-2.1
    switch (index()) {
        case integer: { // integers
            int val=get<0>(*this);
            if (val>=0) append_integer_header(0, static_cast<unsigned int>(val), in);
            else append_integer_header(1, static_cast<unsigned int>((-val)-1), in);
            return;
        }

        // https://tools.ietf.org/html/rfc7049#section-2.3
        case floating_point: { // floats
            header h(7, 26);
            h.append_onto(in);
            float val=get<1>(*this);
            float big_endian;
            cbor_byte* p_big_endian=reinterpret_cast<cbor_byte*>(&big_endian);
            float_to_big_endian(reinterpret_cast<cbor_byte*>(&val), p_big_endian, sizeof(float));
            in->insert(in->end(), p_big_endian, p_big_endian+sizeof(float));
            return;
        }

        case bytes: { // bytes
            const vector<cbor_byte> val=get<4>(*this);
            append_integer_header(2, static_cast<unsigned int>(val.size()), in);
            in->insert(in->end(), val.begin(), val.end());
            return;
        }

        case unicode_string: {  // string
            const string val=get<2>(*this);
            append_integer_header(3, static_cast<unsigned int>(val.size()), in);
            in->insert(in->end(), val.begin(), val.end());
            return;
        }

        case array: {  // variant array
            const cbor_array val { get<5>(*this) };
            append_integer_header(4, static_cast<unsigned int>(val.size()), in);
            for (auto& v : val) v.encode_onto(in);
            return;
        }

        case map: {  // string -> variant map
            const cbor_map val { get<6>(*this) };
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

unsigned int cbor_variant::integer_length(int additional)
{
    if (additional<24) return 1;   // just the header
    if (additional==24) return 2;  // header plus one byte
    if (additional==25) return 3;  // header plus a short
    return 5;  // header plus an int
}

void cbor_variant::append_integer_header(unsigned int major, unsigned int val, std::vector<cbor_byte>* in)
{
    if (val<24) { header(major, val).append_onto(in); return; }
    if (val<256) { header_byte(major, 24, static_cast<cbor_byte>(val)).append_onto(in); return; }
    if (val<65536) { header_short(major, 25, htons(val)).append_onto(in); return; }
    header_int(major, 26, htonl(val)).append_onto(in);
}

int cbor_variant::read_integer_header(const std::vector<cbor_byte>& in, const header* h, unsigned int* offset)
{
    unsigned int first_offset=*offset;
    *offset+=integer_length(h->additional);
    if (h->additional<24) return h->additional;
    if (in.size()<*offset) throw length_error("Insufficient additional size byte(s) while decoding cbor");
    const cbor_byte* p_data=&in[first_offset+1];
    switch (h->additional) {
        case 24: return static_cast<int>(*p_data);
        case 25: return static_cast<int>(ntohs(*reinterpret_cast<const unsigned short*>(p_data)));
        case 26: return static_cast<int>(ntohl(*reinterpret_cast<const unsigned int*>(p_data)));
        case 27: throw range_error("This implementation does not support 64 bit integers");
        case 31: throw runtime_error("This implementation does not support indefinite length types");
        default: throw runtime_error("Don't know how to handle additional data in header");
    }
}

void cbor_variant::float_to_big_endian(const cbor_byte* p_src, cbor_byte* p_dest, unsigned int bytes)
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
