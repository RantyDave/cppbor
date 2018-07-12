# What's this about then?
[cbor](http://cbor.io/) is a marshalling (pickling) protocol designed to be compact both in representation and implementation. I first encountered it when looking for a fast json implementation for Python, and discovered it can do useful things that json can't such as:
* Treat binary data and unicode strings as separate types
* Encode 'None'

# Why another implementation?
There are other C++ implementations but are generally streamed with callbacks (like a SAX parser) and that all looked a bit messy. This implementation focusses on:
* A clean API
* A simple implementation and
* Being unit tested
* (including interoperability with [cbor on Python](https://github.com/brianolson/cbor_py))

It makes heavy use of [C++17 variants](https://en.cppreference.com/w/cpp/utility/variant) to act as 'universal object' for both POD and object data types. Just like dynamic languages do (under the hood). In this case, the variant can hold int, float, monostate (like None in Python), unicode strings, binary data, arrays of other variants, or maps from strings to variants.

# Clean eh?
There are (effectively) only two calls in the API and they both act on [a specialisation of the base class variant](https://github.com/RantyDave/cppbor/blob/662ea6321661e99fa5edf1820fdea912a63a76e3/cppbor/cppbor.hpp#L15). They either construct a variant from a vector of bytes, or encode a variant onto one:
```
    // construct a variant from a vector of bytes
    static cbor_variant construct_from(const std::vector<cbor_byte>& in);
    
    // encode this variant onto the end of the passed vector
    void encode_onto(std::vector<cbor_byte>* in) const;
```
(cbor_byte is just typdef'd to unsigned char for clarity)
Variants can be created 'from scratch' by just [constructing them with the appropriate class in their initialiser list](https://github.com/RantyDave/cppbor/blob/662ea6321661e99fa5edf1820fdea912a63a76e3/cppbor/main.h#L28). You can find type by calling `.index()` where the result will be [one of these enumerations](https://github.com/RantyDave/cppbor/blob/662ea6321661e99fa5edf1820fdea912a63a76e3/cppbor/cppbor.hpp#L25).
Your best guide, other than that, is to look at [the unit tests](https://github.com/RantyDave/cppbor/blob/662ea6321661e99fa5edf1820fdea912a63a76e3/cppbor/main.cpp#L14).

# Many variants
There are two techniques for putting many variants into a single buffer - either create a single variant that is an array of other variants, or just place them sequentially in a buffer. To support this case there is a second factory method:
```
    static cbor_variant construct_from(const std::vector<cbor_byte>& in, unsigned int* offset);
```
To use, initialise an unsigned int to zero and pass a pointer to it. Once the call has returned the offset will now be the index of the first byte not used. If that is not off the end of the buffer, the next variant can be constructed by just making the call again. You can see this being used as [part of the implementation of arrays](https://github.com/RantyDave/cppbor/blob/662ea6321661e99fa5edf1820fdea912a63a76e3/cppbor/cppbor.cpp#L40).

# Performance
Has not been a concern although efforts have been made to ensure move semantics (for example) are correctly used. I imagine it's plenty fast, but probably not a candidate for tight space embedded projects.

# Caveats
The cbor spec is quite wide so there are some omissions and shortcuts:
* Maps can only use strings as keys.
* 64 bit integers are not supported for either read or write.
* Incoming floats can be of any width but will be coerced to a 32 bit float. Only 32 bit floats will be written.
* Tagging is not supported (and ignored on ingestion).
