#pragma once

#include <ostream>
#include <string>

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

    inline bool next_is(int type) const { return (next_.type == type); }

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

    bool read(std::string &val);
    bool read_key(std::string &key);
};

struct str {
    const std::string &s;
    str(const std::string &s) : s(s) {}
    friend std::ostream &operator<<(std::ostream &os, const str &s);
};

}  // namespace jsrw
