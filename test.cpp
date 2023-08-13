#include <assert.h>

#include <iostream>
#include <sstream>

#include "jsrw.h"

using namespace jsrw;

#define dequal(d, x) (abs(d - x) < 0.001)

static void test_read_empty() {
    std::string input("  ");
    Reader reader(input);
    assert(reader.next_is(Empty));
}

static void test_read_symbol() {
    const std::string input("{ } []:, null'");
    Reader reader(input);
    int expected[] = {'{', '}', '[', ']', ':', ',', Null, Error, 0};
    for (int i = 0; expected[i]; i++) {
        assert(reader.next_is(expected[i]));
        reader.consume(expected[i]);
    }
    assert(reader.next_is(Error));
}

static void test_read_bool() {
    const std::string input = "true false";

    Reader reader(input);
    assert(reader.next_is(Bool));

    bool value;

    reader.read(value);  // consumes
    assert(value);
    assert(reader.next_is(Bool));

    reader.read(value);  // consumes
    assert(!value);

    assert(reader.next_is(Empty));
}

static void test_read_number() {
    {
        const std::string input = "123 -456 0 -0";
        Reader reader(input);
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
    }
    {
        Reader reader("1");
        assert(reader.next_is(Integer));
        float dval;
        assert(reader.read(dval));
        assert(dequal(dval, 1));
    }
    {
        Reader reader("2");
        assert(reader.next_is(Integer));
        assert(dequal(reader.read<float>(), 2));
    }

    {
        const std::string input = "123.456";
        Reader reader(input);
        assert(reader.next_is(Number));

        double val;

        assert(reader.read(val));
        assert(dequal(val, 123.456));
    }

    {
        const std::string input = "-123.456";
        Reader reader(input);
        assert(reader.next_is(Number));

        float val;

        assert(reader.read(val));
        assert(dequal(val, -123.456));
    }

    {
        const std::string input = "-1.";
        Reader reader(input);
        double val;
        assert(reader.read(val));
        assert(dequal(val, -1));
    }

    {
        const std::string input = "0.";
        Reader reader(input);
        double val;
        assert(reader.read(val));
        assert(dequal(val, 0));
    }

    {
        const std::string input = "-.456";
        Reader reader(input);
        double val;
        assert(reader.read(val));
        assert(dequal(val, -0.456));
    }

    {
        const std::string input = "-.456e+1";
        Reader reader(input);
        double val;
        assert(reader.read(val));
        assert(dequal(val, -4.56));
    }

    {
        const std::string input = ".456e+10";
        Reader reader(input);
        double val;
        assert(reader.read(val));
        assert(dequal(val, 4560000000));
    }

    {
        const std::string input = "-.456e-2";
        Reader reader(input);
        double val;
        assert(reader.read(val));
        assert(dequal(val, -0.00456));
    }

    {
        const std::string input = ".";
        Reader reader(input);
        assert(reader.next_is(Error));
        double val;
        assert(!reader.read(val));
    }

    {
        const std::string input = "-.";
        Reader reader(input);
        assert(reader.next_is(Error));
        double val;
        assert(!reader.read(val));
    }

    {
        const std::string input = "-1.E";
        Reader reader(input);
        assert(reader.next_is(Error));
        double val;
        assert(!reader.read(val));
    }

    {
        const std::string input = "-1.e0";
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
        const std::string input = "\"\",";
        Reader reader(input);
        assert(reader.next_is(String));
        assert(reader.read(s));
        assert(s == "");
        assert(reader.next_is(','));
    }
    {
        const std::string input = "\"1\"";
        Reader reader(input);
        assert(reader.read(s));
        assert(s == "1");
    }
    {
        const std::string input = "\"\\r\\n\"";
        Reader reader(input);
        assert(reader.read(s));
        assert(s == "\r\n");
    }
    {
        const std::string input = "\"\\u597d\"";
        Reader reader(input);
        assert(reader.read(s));
        assert(s == "好");
    }
    {
        const std::string input = "\"\\u597\"";
        Reader reader(input);
        assert(!reader.read(s));
    }
    {
        const std::string input = "\"\\u597x\"";
        Reader reader(input);
        assert(!reader.read(s));
    }
    {
        const std::string input = "\"\\u597d\\u597dx\"";
        Reader reader(input);
        assert(reader.read(s));
        assert(s == "好好x");
    }
    {
        const std::string input = "\"好\"";
        Reader reader(input);
        assert(reader.read(s));
        assert(s == "好");
    }
    {
        const std::string input = "";
        Reader reader(input);
        assert(!reader.read(s));
    }
}

static void test_skip_string() {
    std::string s;
    {
        const std::string input = "\"\",";
        Reader reader(input);
        assert(reader.consume(String));
        assert(reader.next_is(','));
    }
    {
        const std::string input = "\"\\n\\\"\",";
        Reader reader(input);
        assert(reader.consume(String));
        assert(reader.next_is(','));
    }
}

static void test_read_key() {
    std::string key;
    {
        const std::string input = " \"success\": true";
        Reader reader(input);
        assert(reader.read_key(key));
        assert(key == "success");
        assert(reader.next_is(Bool));
        bool bval;
        assert(reader.read(bval));
        assert(reader.next_is(Empty));
    }
    {
        const std::string input = " 123";
        Reader reader(input);
        assert(!reader.read_key(key));
    }

    {
        const std::string input = " \"success\"";
        Reader reader(input);
        assert(!reader.read_key(key));
    }
}

static void test_read_mix() {
    const std::string input = " { \"success\":true, \"message\": \"正确!\" }";
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
    jsrw::Reader reader("[1,2,3]");

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
    assert(reader.consume('}'));

    assert(product.id == 1);
    assert(product.name == "product");
    assert(dequal(product.price, 1));
}

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
}
