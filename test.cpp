#include <assert.h>

#include <iostream>
#include <sstream>

#include "jsrw.h"

using namespace jsrw;

#define dequal(d, x) (abs(d - x) < 0.001)

static void test_read_empty() {
    std::istringstream input("  ");
    Reader reader(input);
    assert(reader.next_is(Empty));
    {
        Reader reader("");
        assert(reader.next_is(Empty));
    }
}

static void test_read_symbol() {
    std::istringstream input("{ } []:, null'");
    Reader reader(input);
    int expected[] = {'{', '}', '[', ']', ':', ',', Null, Error, 0};
    for (int i = 0; expected[i]; i++) {
        assert(reader.next_is(expected[i]));
        reader.consume(expected[i]);
    }
    assert(reader.next_is(Error));
}

static void test_read_bool() {
    std::istringstream input("truetrue falsetrue");

    Reader reader(input);
    assert(reader.next_is(Bool));

    bool value;

    reader.read(value);  // consumes
    assert(value);
    assert(reader.next_is(Bool));

    reader.read(value);  // consumes
    assert(value);

    reader.read(value);  // consumes
    assert(!value);

    reader.read(value);  // consumes
    assert(value);

    assert(reader.next_is(Empty));
}

static void test_read_number() {
    {
        std::istringstream input("123 -456 0 -0,");
        Reader<2> reader(input);
        assert(reader.next_is(Integer));

        int val;

        reader.read(val);
        assert(val == 123);

        assert(reader.read(val));
        assert(val == -456);

        assert(reader.read(val));
        assert(val == 0);

        assert(reader.read(val));
        assert(val == -0);

        assert(reader.next_is(','));
    }
    {
        std::istringstream input("1 ");
        Reader reader(input);
        assert(reader.next_is(Integer));
        float dval;
        assert(reader.read(dval));
        assert(dequal(dval, 1));
        assert(reader.next_is(Empty));
    }
    {
        std::istringstream input(" 2");
        Reader reader(input);
        assert(reader.next_is(Integer));
        assert(dequal(reader.read<float>(), 2));
    }

    {
        std::istringstream input("123.456");
        Reader reader(input);
        assert(reader.next_is(Number));

        double val;

        assert(reader.read(val));
        assert(dequal(val, 123.456));
    }

    {
        std::istringstream input("-123.456");
        Reader reader(input);
        assert(reader.next_is(Number));

        float val;

        assert(reader.read(val));
        assert(dequal(val, -123.456));
    }

    {
        std::istringstream input("-1.");
        Reader reader(input);
        double val;
        assert(reader.read(val));
        assert(dequal(val, -1));
    }

    {
        std::istringstream input("0.");
        Reader reader(input);
        double val;
        assert(reader.read(val));
        assert(dequal(val, 0));
    }

    {
        std::istringstream input("-.456");
        Reader reader(input);
        double val;
        assert(reader.read(val));
        assert(dequal(val, -0.456));
    }

    {
        std::istringstream input("-.456e+1");
        Reader reader(input);
        double val;
        assert(reader.read(val));
        assert(dequal(val, -4.56));
    }

    {
        std::istringstream input(".456e+10");
        Reader reader(input);
        double val;
        assert(reader.read(val));
        assert(dequal(val, 4560000000));
    }

    {
        std::istringstream input("-.456e-2");
        Reader reader(input);
        double val;
        assert(reader.read(val));
        assert(dequal(val, -0.00456));
    }

    {
        std::istringstream input(".");
        Reader reader(input);
        assert(reader.next_is(Error));
        double val;
        assert(!reader.read(val));
    }

    {
        std::istringstream input("-.");
        Reader reader(input);
        assert(reader.next_is(Error));
        double val;
        assert(!reader.read(val));
    }

    {
        std::istringstream input("-1.E");
        Reader reader(input);
        assert(reader.next_is(Error));
        double val;
        assert(!reader.read(val));
    }

    {
        std::istringstream input("-1.e0");
        Reader reader(input);
        assert(reader.next_is(Number));
        float val;
        assert(reader.read(val));
        assert(dequal(val, -1));
    }
}

static void test_read_string() {
    std::string s;
    {
        std::istringstream input("\"\",");
        Reader reader(input);
        assert(reader.next_is(String));
        assert(reader.read(s));
        assert(s == "");
        assert(reader.next_is(','));
    }
    {
        std::istringstream input("\"1\",");
        Reader reader(input);
        assert(reader.read(s));
        assert(s == "1");
        assert(reader.next_is(','));
    }
    {
        std::istringstream input("\"\\r\\n\"");
        Reader reader(input);
        assert(reader.read(s));
        assert(s == "\r\n");
    }
    {
        std::istringstream input("\"\\u597d\"");
        Reader reader(input);
        assert(reader.read(s));
        assert(s == "好");
    }
    {
        std::istringstream input("\"\\u597\"");
        Reader reader(input);
        assert(!reader.read(s));
    }
    {
        std::istringstream input("\"\\u597x\"");
        Reader reader(input);
        assert(!reader.read(s));
    }
    {
        std::istringstream input("\"\\u597d\\u597dx\"");
        Reader reader(input);
        assert(reader.read(s));
        assert(s == "好好x");
    }
    {
        std::istringstream input("\"好\"");
        Reader reader(input);
        assert(reader.read(s));
        assert(s == "好");
    }
    {
        std::istringstream input("");
        Reader reader(input);
        assert(!reader.read(s));
    }
}

static void test_skip_string() {
    std::string s;
    {
        std::istringstream input("\"\",");
        Reader reader(input);
        assert(reader.consume(String));
        assert(reader.next_is(','));
    }
    {
        std::istringstream input("\"\\n\\\"\",");
        Reader reader(input);
        reader.consume();
        assert(reader.next_is(','));
    }
}

static void test_read_key() {
    std::string key;
    {
        std::istringstream input(" \"success\": true");
        Reader reader(input);
        assert(reader.read_key(key));
        assert(key == "success");
        assert(reader.next_is(Bool));
        bool bval;
        assert(reader.read(bval));
        assert(reader.next_is(Empty));
    }

    {
        std::istringstream input(" 123");
        Reader reader(input);
        assert(!reader.read_key(key));
    }

    {
        std::istringstream input(" \"success\"");
        Reader reader(input);
        assert(!reader.read_key(key));
    }

    {
        std::istringstream input(" \"success\": true");
        Reader reader(input);
        assert(reader.read_key(key));
        assert(key.length() == 7);
        assert(key == "success");
        assert(reader.next_is(Bool));
    }

    {
        std::istringstream input("\"success\" : true");
        Reader reader(input);
        assert(reader.read_key(key));
        assert(key.length() == 7);
        assert(memcmp(key.data(), "success", 7) == 0);
        assert(reader.next_is(Bool));
    }

    {
        std::istringstream input("\"\\nsu\\\"ccess\\\"\" : true");
        Reader reader(input);
        assert(reader.read_key(key));
        assert(key == "\nsu\"ccess\"");
        assert(reader.next_is(Bool));
    }
}

static void test_read_mix() {
    std::istringstream input(" { \"success\":true, \"message\": \"正确!\" }");
    Reader reader(input);

    std::string sval;
    bool bval;

    assert(reader.consume('{'));

    assert(reader.read(sval));
    assert(sval == "success");

    assert(reader.consume(':'));

    assert(reader.read(bval));
    assert(bval);

    assert(reader.next_is(','));
    assert(reader.consume(','));

    assert(reader.next_is(String));
    assert(reader.read(sval));
    assert(sval == "message");

    assert(reader.consume(':'));

    assert(reader.read(sval));
    assert(sval == "正确!");
    assert(reader.consume('}'));

    assert(reader.next_is(Empty));
}

static void test_stringify() {
    std::string s;
    s.append("/好\0.", sizeof("/好\0.") - 1);
    std::stringstream ss;
    ss << str(s);
    assert(ss.str() == "\"\\/好\\u0000.\"");
}

static void parse_vector() {
    std::istringstream input("[1,2,3]");
    jsrw::Reader reader(input);

    std::vector<int> parsed;
    for (reader.consume('['); !reader.next_is(']'); reader.consume(',')) {
        parsed.push_back(reader.read<int>());
    }

    assert(reader.consume(']'));
    assert(parsed == std::vector<int>({1, 2, 3}));
}

static void parse_map() {
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

    assert(product.id == 1);
    assert(product.name == "product");
    assert(dequal(product.price, 10));
}

static void parse_vector2() {
    {
        std::istringstream input("[1,2,3]");
        jsrw::Reader reader(input);
        std::vector<int> values;
        bool ok = reader.read<int>(values, [&](int& value) { return reader.read(value); });
        assert(ok);
        assert(values == std::vector<int>({1, 2, 3}));
    }

    {
        std::istringstream input("[1,]");
        jsrw::Reader reader(input);
        std::vector<int> values;
        bool ok = reader.read<int>(values, [&](int& value) { return reader.read(value); });
        assert(ok);
        assert(values == std::vector<int>({1}));
    }

    {
        std::istringstream input("[]");
        jsrw::Reader reader(input);
        std::vector<int> values;
        bool ok = reader.read<int>(values, [&](int& value) { return reader.read(value); });
        assert(ok);
        assert(values == std::vector<int>());
    }

    {
        std::istringstream input("[,]");
        jsrw::Reader reader(input);
        std::vector<int> values;
        bool ok = reader.read<int>(values, [&](int& value) { return reader.read(value); });
        assert(!ok);
    }

    {
        std::istringstream input("[1,,2]");
        jsrw::Reader reader(input);
        std::vector<int> values;
        bool ok = reader.read<int>(values, [&](int& n) -> bool { return reader.read(n); });
        assert(!ok);
    }
}

static void parse_vector_ptr() {
    {
        std::istringstream input("[1,2,3]");
        jsrw::Reader reader(input);
        std::vector<int>* values;
        bool ok = reader.read<int>(values, [&](int& value) { return reader.read(value); });
        assert(ok);
        assert(*values == std::vector<int>({1, 2, 3}));
        delete values;
    }

    {
        std::istringstream input("null,");
        jsrw::Reader reader(input);
        std::vector<int>* values;
        bool ok = reader.read<int>(values, [&](int& n) -> bool { return reader.read(n); });
        assert(ok);
        assert(values == nullptr);
        assert(reader.next_is(','));
    }
}

static void parse_map2() {
    {
        std::istringstream input("{\"x\": 1, \"y\":2}");
        jsrw::Reader reader(input);
        std::map<std::string, int> values;
        bool ok = reader.read<int>(values, [&](int& value) { return reader.read(value); });
        assert(ok);
        std::map<std::string, int> expected = {{"x", 1}, {"y", 2}};
        assert(values == expected);
    }

    {
        std::istringstream input("[1,]");
        jsrw::Reader reader(input);
        std::vector<int> values;
        bool ok = reader.read<int>(values, [&](int& value) { return reader.read(value); });
        assert(ok);
        assert(values == std::vector<int>({1}));
    }

    {
        std::istringstream input("[]");
        jsrw::Reader reader(input);
        std::vector<int> values;
        bool ok = reader.read<int>(values, [&](int& value) { return reader.read(value); });
        assert(ok);
        assert(values == std::vector<int>());
    }

    {
        std::istringstream input("[,]");
        jsrw::Reader reader(input);
        std::vector<int> values;
        bool ok = reader.read<int>(values, [&](int& value) { return reader.read(value); });
        assert(!ok);
    }

    {
        std::istringstream input("[1,,2]");
        jsrw::Reader reader(input);
        std::vector<int> values;
        bool ok = reader.read<int>(values, [&](int& value) { return reader.read(value); });
        assert(!ok);
    }
}

static void parse_map_ptr() {
    {
        std::istringstream input("{\"x\": 1, \"y\":2}");
        jsrw::Reader reader(input);
        std::map<std::string, int>* values;
        bool ok = reader.read<int>(values, [&](int& value) { return reader.read(value); });
        assert(ok);
        std::map<std::string, int> expected = {{"x", 1}, {"y", 2}};
        assert(*values == expected);
        delete values;
    }
    {
        std::istringstream input("null,");
        jsrw::Reader reader(input);
        std::map<std::string, int>* values;
        bool ok = reader.read<int>(values, [&](int& value) { return reader.read(value); });
        assert(ok);
        assert(values == nullptr);
        assert(reader.next_is(','));
    }
}

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

static void test_parse_objects() {
    {
        std::istringstream input("{\"name\": \"person\"}");
        jsrw::Reader reader(input);
        Person p;
        assert(p.decode(reader));
        assert(p.name == "person");
        assert(reader.next_is(Empty));
    }
    {
        std::istringstream input("{\"name\": 100");
        jsrw::Reader reader(input);
        Person p;
        assert(!p.decode(reader));
    }
    {
        // unknown field 'age'
        std::istringstream input("{\"name\": \"person\", \"age\": 1}");
        jsrw::Reader reader(input);
        Person p;
        assert(!p.decode(reader));
    }
    {
        std::istringstream input("{}");
        jsrw::Reader reader(input);
        Person p;
        assert(p.decode(reader));
        assert(p.name.empty());
    }

    {
        std::istringstream input("[{\"name\": \"person1\"}, {\"name\": \"person2\",} ]");
        jsrw::Reader reader(input);
        auto fn = [&](Person& person) { return person.decode(reader); };
        std::vector<Person> people;
        bool ok = reader.read<Person>(people, fn);
        assert(ok);
        assert(people.size() == 2);
        assert(people[0].name == "person1");
        assert(people[1].name == "person2");
    }

    {
        std::istringstream input("[{\"name\": \"person1\"}, null, {\"name\": \"person3\"} ]");
        jsrw::Reader reader(input);
        std::vector<Person*> people;
        bool ok = reader.read<Person*>(people, [&](Person*& person) {
            if (reader.next_is(Null)) {
                reader.consume();
                person = nullptr;
                return true;
            }
            person = new Person();
            return person->decode(reader);
        });
        assert(ok);
        assert(people.size() == 3);
        assert(people[0]->name == "person1");
        assert(people[1] == nullptr);
        assert(people[2]->name == "person3");
        for (auto p : people) {
            delete p;
        }
    }

    {
        std::istringstream input("[{\"name\": \"person1\"}, null, {\"code\": \"1\"} ]");
        jsrw::Reader reader(input);
        std::vector<Person*> people;
        bool ok = reader.read<Person*>(people, [&](Person*& person) {
            if (reader.next_is(Null)) {
                reader.consume();
                person = nullptr;
                return true;
            }
            person = new Person();
            return person->decode(reader);
        });
        assert(!ok);
        for (auto p : people) {
            delete p;
        }
    }
}

static void test_write_simple_values();
static void test_write_vectors();
static void test_write_maps();

int main() {
    test_read_empty();
    test_read_symbol();
    test_read_bool();
    test_read_number();
    test_read_string();
    test_skip_string();
    test_read_key();
    test_read_mix();
    test_stringify();
    parse_vector();
    parse_map();
    parse_vector2();
    parse_vector_ptr();
    parse_map2();
    parse_map_ptr();
    test_parse_objects();

    test_write_simple_values();
    test_write_vectors();
    test_write_maps();
}

struct OrderItem {
    int product_id;
    int quantity;
};

struct Order {
    int id;
    std::vector<OrderItem> items;
};

// An example JSON writer.
class Writer {
   public:
    // common:
    void write(std::ostream &out, bool b) { out << (b ? "true" : "false"); }

    template <typename T>
    void write(std::ostream &out, T t) {
        if constexpr (std::is_pointer<T>::value) {
            if (t == nullptr) {
                out << "null";
            } else {
                write(out, *t);
            }
        } else {
            out << t;
        }
    }

    void write(std::ostream &out, const std::string &s) { out << jsrw::str(s); }

    void write(std::ostream &out, const char *s) {
        if (s == nullptr) {
            out << "null";
        } else {
            out << jsrw::str(s);
        }
    }

    template <typename T>
    void write(std::ostream &out, const std::vector<T> &v) {
        out << '[';
        size_t i = 0;
        for (const auto &p : v) {
            if (i++ > 0) {
                out << ',';
            }
            write(out, p);
        }
        out << ']';
    }

    template <typename T>
    void write(std::ostream &out, const std::map<std::string, T> &m) {
        out << '{';
        size_t i = 0;
        for (const auto &p : m) {
            if (i++ > 0) {
                out << ',';
            }
            out << jsrw::str(p.first);
            out << ':';
            write(out, p.second);
        }
        out << '}';
    }

    // app
    void write(std::ostream &out, const OrderItem& item) {
        out << "{";
        out << jsrw::str("product_id") << ":";
        write(out, item.product_id);
        out << ',';
        out << jsrw::str("quantity") << ":";
        write(out, item.quantity);
        out << '}';
    }

    void write(std::ostream &out, const Order& order) {
        out << "{";
        out << jsrw::str("id") << ":";
        write(out, order.id);
        out << ',';
        out << jsrw::str("items") << ":";
        write(out, order.items);
        out << '}';
    }
};


template <typename T>
std::string write(const T& t) {
    std::stringstream ss;
    Writer writer;
    writer.write(ss, t);
    return ss.str();
}

static void test_write_simple_values() {
    bool b = true;
    int n = 100;
    float f = 1.0;
    std::string s = "hello";

    assert(write(100) == "100");
    assert(write(n) == "100");
    assert(write(&n) == "100");
    assert(write((int*)nullptr) == "null");
    assert(write((const char*)nullptr) == "null");
    assert(write((char*)nullptr) == "null");
    assert(write((std::string*)nullptr) == "null");
    assert(write((const char*)"hello") == "\"hello\"");
    assert(write(s) == "\"hello\"");
    assert(write<const std::string*>(&s) == "\"hello\"");
    assert(write(true) == "true");
    assert(write(false) == "false");
    assert(write(&b) == "true");
    assert(write(f) == "1");
    assert(write(&f) == "1");
}

static void test_write_vectors() {
    std::stringstream ss;
    Writer writer;

    {
        std::vector<int> values = {1, 2, 3};
        writer.write<int>(ss, values);
        assert(ss.str() == "[1,2,3]");
        ss.str("");
    }

    {
        int a = 1, b = 2;
        std::vector<int*> values = {&a, &b, nullptr};
        writer.write<int*>(ss, values);
        assert(ss.str() == "[1,2,null]");
        ss.str("");
    }

    {
        int a = 1, b = 2;
        std::vector<int*> values = {&a, &b, nullptr};
        writer.write<int*>(ss, values);
        assert(ss.str() == "[1,2,null]");
        ss.str("");
    }

    {
        std::vector<Order> orders{{
                                      .id = 1,
                                      .items = {{.product_id = 1, .quantity = 100}, {.product_id = 2, .quantity = 200}},
                                  },
                                  {
                                      .id = 2,
                                      .items = {{.product_id = 3, .quantity = 300}, {.product_id = 4, .quantity = 400}},
                                  }};
        writer.write(ss, orders);
        assert(ss.str() == R"([{"id":1,"items":[{"product_id":1,"quantity":100},{"product_id":2,"quantity":200}]},{"id":2,"items":[{"product_id":3,"quantity":300},{"product_id":4,"quantity":400}]}])");
        ss.str("");
     }
}

static void test_write_maps() {
    std::stringstream ss;
    Writer writer;

    {
        std::map<std::string, int> value = {{"x", 1}, {"y", 2}};
        writer.write<int>(ss, value);
        assert(ss.str() == "{\"x\":1,\"y\":2}");
        ss.str("");
    }

    {
        int a = 1, b = 2;
        std::map<std::string, int*> value = {{"x", &a}, {"y", &b}};
        writer.write<int*>(ss, value);
        assert(ss.str() == "{\"x\":1,\"y\":2}");
        ss.str("");
    }

    {
        int a = 1, b = 2;
        std::map<std::string, int*> value = {{"x", &a}, {"y", &b}, {"z", nullptr}};
        writer.write<int*>(ss, value);
        assert(ss.str() == "{\"x\":1,\"y\":2,\"z\":null}");
        ss.str("");
    }
}
