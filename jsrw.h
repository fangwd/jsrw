#pragma once

#include <functional>
#include <map>
#include <ostream>
#include <string>
#include <vector>

namespace jsrw {

enum TokenType {
    Empty = 256,
    Null,
    Bool,
    Integer,
    Number,
    String,
    Error = 400,
};

struct Slice {
    const char *data;
    size_t length;
    Slice() : data(nullptr), length(0) {}
    Slice(const char *data, size_t length) : data(data), length(length) {}
    Slice(const char *data) : Slice(data, strlen(data)) {}
    Slice(const std::string &s) : Slice(s.data(), s.size()) {}
    inline char operator[](size_t index) const { return index < length ? data[index] : '\0'; }
    bool operator==(const char *) const;
    bool operator==(const std::string &) const;
    bool operator==(const Slice &) const;
};

class Reader {
  private:
    const char *input_end_;
    const char *input_;

    struct Token {
        int type;
        union {
            bool bval;
            long lval;
            double dval;
        };
    };

    Token next_;

    void skip_space();
    void skip_string();

    void parse();
    int parse_int(long &num, bool &neg);
    int parse_num(Token &);
    int parse_hex(std::string &);

  public:
    Reader(const std::string &);
    Reader(const char *begin, const char *end);
    Reader(const char *begin);

    inline bool next_is(int type) const {
        return (next_.type == type);
    }

    void consume() {
        if (next_.type == String) {
            skip_string();
        }
        parse();
    }

    bool consume(int type) {
        if (next_.type == type) {
            if (type == String) {
                skip_string();
            }
            parse();
            return true;
        }
        return false;
    }

    template <typename T>
    T read() {
        T t;
        read(t);
        return t;
    }

    bool read(double &value) {
        if (next_.type == Number) {
            value = next_.dval;
            parse();
            return true;
        } else if (next_.type == Integer) {
            value = (double)next_.lval;
            parse();
            return true;
        }
        return false;
    }

    bool read(float &value) {
        double d;
        if (!read(d)) {
            return false;
        }
        value = (float)d;
        return true;
    }

    bool read(bool &value) {
        if (next_.type == Bool) {
            value = next_.bval;
            parse();
            return true;
        }
        return false;
    }

    template <typename T>
    bool read(T &value) {
        if (next_.type == Integer) {
            value = (T)next_.lval;
            parse();
            return true;
        }
        return false;
    }

    template <class T>
    bool read(std::vector<T> &v, std::function<bool(Reader &, T &)> fn) {
        if (!consume('[')) {
            return false;
        }
        v.clear();
        while (!next_is(']')) {
            T t;
            if (!fn(*this, t)) {
                v.push_back(std::move(t));
                return false;
            }
            v.push_back(std::move(t));
            if (!consume(',')) {
                break;
            }
        }
        return consume(']');
    }

    template<class T=Slice>
    bool read(std::function<bool(const T &)> fn) {
        if (!consume('{')) {
            return false;
        }
        T key;
        while (!next_is('}')) {
            if (!read_key(key)) {
                return false;
            }
            if (!fn(key)) {
                return false;
            }
            if (!consume(',')) {
                break;
            }
        }
        return consume('}');
    }

    template <class T>
    bool read(std::map<std::string, T> &m, std::function<bool(Reader &, T &)> fn) {
        if (!consume('{')) {
            return false;
        }
        while (!next_is('}')) {
            std::string key;
            if (!read_key(key)) {
                return false;
            }
            T t;
            if (!fn(*this, t)) {
                m[std::move(key)] = std::move(t);
                return false;
            }
            m[std::move(key)] = std::move(t);
            if (!consume(',')) {
                break;
            }
        }
        return consume('}');
    }

    bool read(std::string &val);
    bool read_key(std::string &key);
    bool read_key(Slice &s);
};

struct str {
    const std::string &s;
    str(const std::string &s) : s(s) {}
    friend std::ostream &operator<<(std::ostream &os, const str &s);
};

}  // namespace jsrw
