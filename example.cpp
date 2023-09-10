#include "jsrw.h"

#include <assert.h>
#include <sstream>

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

static void write_simple_values() {
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

static void write_vectors() {
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

static void write_maps() {
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

int main() {
    write_simple_values();
    write_vectors();
    write_maps();
}
