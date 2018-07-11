# What's this about then?
[cbor](http://cbor.io/) is a marshalling (pickling) protocol designed to be compact both in representation and implementation. I first encountered it when looking for a fast json implementation for Python, and discovered it can do useful things that json can't such as:
* Treat binary data and unicode strings as separate types
* Encode 'None'

# Why another implementation?
There are other C++ implementations but are generally streamed with callbacks (like a SAX parser) and that all looked a bit messy. **This implementation aims for the cleanest possible API**. To do this it makes heavy use of [C++17 variants](https://en.cppreference.com/w/cpp/utility/variant) to act as 'universal object' for both POD and object data types. Just like dynamic languages do (under the hood). In this case, the variant can hold int, float, monostate (like None in Python), unicode strings, binary data, arrays of other variants, or maps from strings to variants.


# Clean eh?
There are (effectively) only two calls in the API. They either construct a variant from a vector of bytes, or encode a variant onto one:
```
    // construct a variant from a vector of bytes
    static cbor_variant construct_from(const std::vector<cbor_byte>& in);
    
    // encode this variant onto the end of the passed vector
    void encode_onto(std::vector<cbor_byte>* in) const;
```
(cbor_byte is just typdef'd to unsigned char for clarity)
