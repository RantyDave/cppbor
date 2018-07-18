import cbor

with open("int", 'wb') as f:
    f.write(cbor.dumps(-12345678))

with open("string", 'wb') as f:
    f.write(cbor.dumps("smeg"))

with open("bytes", 'wb') as f:
    f.write(cbor.dumps(b'bytes'))

with open("none", 'wb') as f:
    f.write(cbor.dumps(None))

with open("float", 'wb') as f:
    f.write(cbor.dumps(1.1))

with open("array", 'wb') as f:
    f.write(cbor.dumps([8, 1.1, "pie"]))

with open("map", 'wb') as m:
    m.write(cbor.dumps({"one": 1, "two": "deux"}))

with open("array-map", 'wb') as m:
    m.write(cbor.dumps([{"one": 1, "two": "deux", "nowt": None, "data": b'01234'},
                        {"three": 3, "four-something": 4.3, "lots": [1, 2, 3]}]))
