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

#ifndef cppbor_hpp
#define cppbor_hpp
#include <cstdint>
#include <variant>
#include <string>
#include <vector>
#include <unordered_map>

struct cbor_variant;
typedef std::vector<cbor_variant> cbor_array;
typedef std::unordered_map<std::string, cbor_variant> cbor_map;

// Not supported: 64 bit ints, indefinite lengths
// Map keys are assumed to be std::string
struct cbor_variant : std::variant<int, float, std::string, std::monostate, std::vector<uint8_t>, cbor_array, cbor_map>
{
    // construct a variant from a vector of bytes
    static void construct_from_into(const std::vector<uint8_t>& in, cbor_variant* dest);
    static void construct_from_into(const std::vector<uint8_t>& in, cbor_variant* dest, unsigned int* offset);

    // encode this variant onto the end of the passed vector
    void encode_onto(std::vector<uint8_t>* in) const;

    // describe this variant using a Python compatible format
    std::string as_python() const;

    // call index() to return type
    enum types { integer, floating_point, unicode_string, none, bytes, array, map };

    // just because this is such a PITA
    static void read_file_into(const char* name, std::vector<uint8_t>* dest);

    // stops cppunit from objecting
    operator const char*() const {return "";}

private:
    // https://tools.ietf.org/html/rfc7049#section-2
    // (m)ajor, (a)dditional, (d)ata
    struct header {
        header(unsigned int m, unsigned int a) : major(static_cast<uint8_t>(m)), additional(static_cast<uint8_t>(a)) {}
        operator uint8_t*() { return reinterpret_cast<uint8_t*>(this); }
        void append_onto(std::vector<uint8_t>* in) { in->insert(in->end(), reinterpret_cast<uint8_t*>(this), reinterpret_cast<uint8_t*>(this)+1); }
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        uint8_t additional : 5;
        uint8_t major : 3;
#else
        uint8_t major : 3;
        uint8_t additional : 5;
#endif
    };
    struct header_byte : header {
        header_byte(unsigned int m, unsigned int a, uint8_t d) : header(m, a), data(d) {}
        void append_onto(std::vector<uint8_t>* in) { in->insert(in->end(), reinterpret_cast<uint8_t*>(this), reinterpret_cast<uint8_t*>(this)+2); }
        uint8_t data;
    };
    struct header_short : header {
        header_short(unsigned int m, unsigned int a, unsigned short d) : header(m, a) {*reinterpret_cast<unsigned short*>(&data)=d;}
        void append_onto(std::vector<uint8_t>* in) { in->insert(in->end(), reinterpret_cast<uint8_t*>(this), reinterpret_cast<uint8_t*>(this)+3); }
        uint8_t data[2];
    };
    struct header_int : header {
        header_int(unsigned int m, unsigned int a, unsigned int d) : header(m, a) {*reinterpret_cast<unsigned int*>(&data)=d;}
        void append_onto(std::vector<uint8_t>* in) { in->insert(in->end(), reinterpret_cast<uint8_t*>(this), reinterpret_cast<uint8_t*>(this)+5); }
        uint8_t data[4];
    };
    static unsigned int integer_length(int additional);
    static void append_integer_header(unsigned int major, unsigned int val, std::vector<uint8_t>* in);
    static int read_integer_header(const std::vector<uint8_t>& in, const header* h, unsigned int* offset);
    static void float_to_big_endian(const uint8_t* p_src, uint8_t* p_dest, unsigned int bytes);
};

#endif /* cppbor_hpp */
