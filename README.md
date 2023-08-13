A performance first library to help reading/writing JSON data in C++.

Apart from a few primitive data types (`int`, `double`, and `bool`), *jsrw* doesn't parse JSON bytes
into any data structure in memory. Instead, it exposes an API to let the user check what's next in
the input stream and parses data into user's data structures directly.

# Usage

## Reading
Parsing a vector:
```cpp
jsrw::Reader reader("[1,2,3]");
std::vector<int>  parsed;
for (reader.consume('['); !reader.next_is(']'); reader.consume(',')) {
  parsed.push_back(reader.read<int>());
}
assert(reader.consume(']'));
assert(parsed  ==  std::vector<int>({1, 2, 3}));
```

Parsing an object:
```cpp
struct Product {
  int id;
  std::string name;
  float price;
};

jsrw::Reader reader("{\"id\": 1, \"sku\": \"p1\", \"name\": \"product\", \"price\": 1}");

Product product;
std::string key;
for (reader.consume('{'); !reader.next_is('}'); reader.consume(',')) {
    reader.read_key(key);
    if (key == "id") {
        product.id = reader.read<int>();
    } else if (key == "name") {
        product.name = reader.read<std::string>();
    } else if (key == "price") {
        product.price = reader.read<double>();
    } else {
        std::cout << "Ignored key '" << key << "'\n";
        reader.consume();
    }
}
assert(product.id == 1);
assert(product.name == "product");
assert(dequal(product.price, 1));
```

## Writing
jsrw comes with an `str` class that helps write valid JSON strings to an `std::ostream`:
```cpp
std::string s;
s.append("/好\0.", sizeof("/好\0.") - 1);
std::stringstream ss;
ss << str(s);
assert(ss.str() == "\"\\/好\\u0000.\"");
```
