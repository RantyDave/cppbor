#include "cppbor.hpp"
#include <exception>
#include <string>
#include <arpa/inet.h>

using namespace std;
cbor_variant cbor_variant::construct_from(const std::vector<cbor_byte>& in)
{
    int dummy_offset=0;
    return construct_from(in, &dummy_offset);
}

cbor_variant cbor_variant::construct_from(const std::vector<cbor_byte>& in, int* offset)
{
    // nothing to read?
    if (in.size()<=*offset) throw length_error("No header byte while decoding cbor");

    // header object
    const header* h=(const header*)(const void*)in.data()+*offset;

    // integers
    switch (h->major) {
        case 0: return cbor_variant { read_integer_header(in, h, offset) };  // +ve integer
        case 1: return cbor_variant { -1-read_integer_header(in, h, offset) };  // -ve integer

        case 2: // bytes and strings
        case 3: {
            int length=read_integer_header(in, h, offset);
            int offset_at_begin=*offset;
            *offset+=length;
            if (in.size()<*offset) throw length_error("Insufficient data bytes while decoding cbor");
            if (h->major==2) return cbor_variant { vector<cbor_byte>(&in[offset_at_begin], &in[offset_at_begin+length]) };
            else return cbor_variant { string(&in[offset_at_begin], &in[offset_at_begin+length]) };
        }

        case 4: {  // arrays
            vector<cbor_variant> rtn;
            for (int pending_items=read_integer_header(in, h, offset); pending_items>0; pending_items--)
                rtn.push_back(construct_from(in, offset));
            return cbor_variant { rtn };
        }

        case 7: {  // floats and none
            const cbor_byte* first_data_byte=in.data()+*offset+1;
            if (h->additional==26) {  // single precision
                *offset+=5;
                float rtn;
                float_to_big_endian(first_data_byte, (cbor_byte*)(void*)&rtn, 4);
                return cbor_variant { rtn };
            }
            if (h->additional==27) {  // double precision, gets cast down to single
                *offset+=9;
                double rtn;
                float_to_big_endian(first_data_byte, (cbor_byte*)(void*)&rtn, 8);
                return cbor_variant { (float)rtn };
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
        case 0: { // integers
            int val=get<0>(*this);
            if (val>=0) append_integer_header(0, val, in);
            else append_integer_header(1, (-val)-1, in);
            return;
        }

        // https://tools.ietf.org/html/rfc7049#section-2.3
        case 1: { // floats
            header h(7, 26);
            h.append_onto(in);
            float val=get<1>(*this);
            float big_endian;
            cbor_byte* p_big_endian=(cbor_byte*)(void*)&big_endian;
            float_to_big_endian((cbor_byte*)(void*)&val, p_big_endian, sizeof(float));
            in->insert(in->end(), p_big_endian, p_big_endian+sizeof(float));
            return;
        }

        case 4: { // bytes
            const vector<cbor_byte> val=get<4>(*this);
            append_integer_header(2, (int)val.size(), in);
            in->insert(in->end(), val.begin(), val.end());
            return;
        }

        case 2: {  // string
            const string val=get<2>(*this);
            append_integer_header(3, (int)val.size(), in);
            in->insert(in->end(), val.begin(), val.end());
            return;
        }

        case 5: {  // array
            vector<cbor_variant> val { get<5>(*this) };
            append_integer_header(4, (int)val.size(), in);
            for (const cbor_variant& v : val) v.encode_onto(in);
        }

        default: { // none (case 3)
            in->push_back(0xf6);
        }
    }
}

int cbor_variant::integer_length(int additional)
{
    if (additional<24) return 1;   // just the header
    if (additional==24) return 2;  // header plus one byte
    if (additional==25) return 3;  // header plus a short
    return 5;  // header plus an int
}

void cbor_variant::append_integer_header(int major, int val, std::vector<cbor_byte>* in)
{
    if (val<24) { header(major, val).append_onto(in); return; }
    if (val<256) { header_byte(major, 24, val).append_onto(in); return; }
    if (val<65536) { header_short(major, 25, htons(val)).append_onto(in); return; }
    header_int(major, 26, htonl(val)).append_onto(in);
}

int cbor_variant::read_integer_header(const std::vector<cbor_byte>& in, const header* h, int* offset)
{
    int first_offset=*offset;
    *offset+=integer_length(h->additional);
    if (h->additional<24) return h->additional;
    if (in.size()<*offset) throw length_error("Insufficient additional size byte(s) while decoding cbor");
    const cbor_byte* p_data=&in[first_offset+1];
    switch (h->additional) {
        case 24: return (int)*p_data;
        case 25: return (int)ntohs(*(unsigned short*)(void*)p_data);
        case 26: return (int)ntohl(*(unsigned int*)(void*)p_data);
        default: throw range_error("Tried to construct an integer greater than 32 bits");
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
