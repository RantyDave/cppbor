#ifndef cppbor_hpp
#define cppbor_hpp
#include <variant>
#include <string>
#include <vector>
#include <unordered_map>

struct cbor_variant;
typedef unsigned char cbor_byte;
typedef std::vector<cbor_variant> cbor_array;
typedef std::unordered_map<std::string, cbor_variant> cbor_map;

// Not supported: 64 bit ints, indefinite lengths
// Map keys are assumed to be std::string
struct cbor_variant : std::variant<int, float, std::string, std::monostate, std::vector<cbor_byte>, cbor_array, cbor_map>
{
    // construct a variant from a vector of bytes
    static cbor_variant construct_from(const std::vector<cbor_byte>& in);
    static cbor_variant construct_from(const std::vector<cbor_byte>& in, unsigned int* offset);

    // encode this variant onto the end of the passed vector
    void encode_onto(std::vector<cbor_byte>* in) const;

    // call index() to return type
    enum types { integer, floating_point, unicode_string, none, bytes, array, map };

    // stops cppunit from objecting
    operator const char*() const { return "";}

private:
    // https://tools.ietf.org/html/rfc7049#section-2
    // (m)ajor, (a)dditional, (d)ata
    struct header {
        header(unsigned int m, unsigned int a) : major(static_cast<cbor_byte>(m)), additional(static_cast<cbor_byte>(a)) {}
        operator cbor_byte*() { return reinterpret_cast<cbor_byte*>(this); }
        void append_onto(std::vector<cbor_byte>* in) { in->insert(in->end(), reinterpret_cast<cbor_byte*>(this), reinterpret_cast<cbor_byte*>(this)+1); }
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        cbor_byte additional : 5;
        cbor_byte major : 3;
#else
        cbor_byte major : 3;
        cbor_byte additional : 5;
#endif
    };
    struct header_byte : header {
        header_byte(unsigned int m, unsigned int a, cbor_byte d) : header(m, a), data(d) {}
        void append_onto(std::vector<cbor_byte>* in) { in->insert(in->end(), reinterpret_cast<cbor_byte*>(this), reinterpret_cast<cbor_byte*>(this)+2); }
        cbor_byte data;
    };
    struct header_short : header {
        header_short(unsigned int m, unsigned int a, unsigned short d) : header(m, a) {*reinterpret_cast<unsigned short*>(&data)=d;}
        void append_onto(std::vector<cbor_byte>* in) { in->insert(in->end(), reinterpret_cast<cbor_byte*>(this), reinterpret_cast<cbor_byte*>(this)+3); }
        cbor_byte data[2];
    };
    struct header_int : header {
        header_int(unsigned int m, unsigned int a, unsigned int d) : header(m, a) {*reinterpret_cast<unsigned int*>(&data)=d;}
        void append_onto(std::vector<cbor_byte>* in) { in->insert(in->end(), reinterpret_cast<cbor_byte*>(this), reinterpret_cast<cbor_byte*>(this)+5); }
        cbor_byte data[4];
    };
    static unsigned int integer_length(int additional);
    static void append_integer_header(unsigned int major, unsigned int val, std::vector<cbor_byte>* in);
    static int read_integer_header(const std::vector<cbor_byte>& in, const header* h, unsigned int* offset);
    static void float_to_big_endian(const cbor_byte* p_src, cbor_byte* p_dest, unsigned int bytes);
};

#endif /* cppbor_hpp */
