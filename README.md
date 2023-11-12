A stream-oriented, header-only library for parsing JSON data in C++.

Main features:
- Memory efficient: instead of parsing the whole JSON data into a structure in memory, the library provides a `Reader` class with methods to check the type of the next token in the stream and to parse different types of tokens one at a time.
- Minimal writing support: provides an overloaded `<<` operator to help write JSON strings
- UTF-8 support: capable of encoding/decoding UTF-8 characters
- Light-weight and header only: the whole library has only 539 lines of code

# Usage

## Parsing
### Parsing objects
```cpp
struct Product {
    int id;
    std::string name;
    float price;
};

std::istringstream input("{\"id\": 1, \"sku\": \"p1\", \"name\": \"product\", \"price\": 10}");
jsrw::Reader reader(input);

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
        // std::cout << "Ignored key '" << key << "'\n";
        reader.consume();
    }
}

assert(reader.consume('}'));

```

### Parsing vectors
```cpp
std::istringstream input("[1,2,3]");
jsrw::Reader reader(input);
std::vector<int> values;
bool ok = reader.read<int>(values, [&](int& value) { return reader.read(value); });
assert(ok);
assert(values == std::vector<int>({1, 2, 3}));
```

### Parsing maps
```cpp
std::istringstream input("{\"x\": 1, \"y\":2}");
jsrw::Reader reader(input);
std::map<std::string, int> values;
bool ok = reader.read<int>(values, [&](int& value) { return reader.read(value); });
assert(ok);
std::map<std::string, int> expected = {{"x", 1}, {"y", 2}};
assert(values == expected);
```

### Parsing a vector of objects
```cpp
struct Person {
    std::string name;
    bool decode(jsrw::Reader<>& reader) {
        return reader.read([&](const std::string& key) {
            if (key == "name") {
                return reader.read(name);
            }
            return false;
        });
    }
};

std::istringstream input("[{\"name\": \"person1\"}, {\"name\": \"person2\",} ]");
jsrw::Reader reader(input);
auto fn = [&](Person& person) { return person.decode(reader); };
std::vector<Person> people;
bool ok = reader.read<Person>(people, fn);
assert(ok);
assert(people.size() == 2);
assert(people[0].name == "person1");
assert(people[1].name == "person2");
```

## Writing JSON data
The library provides an overloaded `<<` operator to help write strings to an `std::ostream` correctly. 

Example to write a `std::string` to `std::stringstream`:
```cpp
std::stringstream ss;
std::string s = "hello";
ss << s;
assert(ss.str() == "\"hello\"");
```

See [test.cpp](https://github.com/fangwd/jsrw/blob/main/test.cpp) for more examples (including a `Writer` class to help write different data types to a `std::ostream`) and [jsrw.h](https://github.com/fangwd/jsrw/blob/main/jsrw.h) for implementation details.

# Changelog

- 12/11/2023 v0.1.0

