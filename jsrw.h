// Copyright (c) 2023 Weidong Fang (wdfang@gmail.com)
#pragma once

#include <functional>
#include <istream>
#include <map>
#include <ostream>
#include <string>
#include <type_traits>
#include <vector>

#define JSRW_VERSION "0.2.0"

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

template <size_t BUFF = 4096>
class Reader {
   private:
    std::istream *input_;
    char buff_[BUFF];
    const char *data_;
    size_t size_;
    int current_;

    inline int read() {
        if (size_ == 0 && input_) {
            input_->read(buff_, BUFF);
            data_ = buff_;
            size_ = input_->gcount();
        }
        if (size_ == 0) {
            return (current_ = Empty);
        }
        size_--;
        return (current_ = *data_++);
    }

    struct Token {
        int type;
        union {
            bool bval;
            long lval;
            double dval;
        };
    };

    Token next_;
    std::string key_;

    void parse() {
        skip_space();

        if (current_ == Empty) {
            next_.type = Empty;
            return;
        }

        switch (current_) {
            case '{':
            case '}':
            case '[':
            case ']':
            case ':':
            case ',':
                next_.type = current_;
                read();
                break;
            case '"':
                next_.type = String;
                break;
            case 'n':
                if (read() == 'u' && read() == 'l' && read() == 'l') {
                    next_.type = Null;
                    read();
                } else {
                    next_.type = Error;
                }
                break;
            case 't':
                if (read() == 'r' && read() == 'u' && read() == 'e') {
                    next_.bval = true;
                    next_.type = Bool;
                    read();
                } else {
                    next_.type = Error;
                }
                break;
            case 'f':
                if (read() == 'a' && read() == 'l' && read() == 's' && read() == 'e') {
                    next_.bval = false;
                    next_.type = Bool;
                    read();
                } else {
                    next_.type = Error;
                }
                break;
            default:
                next_.type = parse_num(next_);
                break;
        }
    }

    void skip_space() {
        while (isspace(current_)) {
            read();
        }
    }

    void skip_string() {
        while (current_ != Empty) {
            read();
            if (current_ == '"') {
                read();
                break;
            } else if (current_ == '\\') {
                read();
            }
        }
    }

    int parse_hex(std::string &s) {
        char data[4];
        data[0] = read();
        data[1] = read();
        data[2] = read();
        data[3] = read();
        int cp = 0;
        for (int i = 0; i < 4; i++) {
            char c = data[i];
            cp *= 16;
            if (c >= '0' && c <= '9')
                cp += c - '0';
            else if (c >= 'a' && c <= 'f')
                cp += c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                cp += c - 'A' + 10;
            else
                return Error;
        }
        int n = encode_utf8(cp, data);
        for (int i = 0; i < n; i++) {
            s.push_back(data[i]);
        }
        return String;
    }

    static int32_t encode_utf8(int32_t ch, char *buffer) {
        if (ch <= 0x7f) {
            buffer[0] = (char)ch;
            return 1;
        }

        if (ch <= 0x7ff) {
            buffer[0] = (char)(0xc0 | (ch >> 6));
            buffer[1] = (char)(0x80 | (ch & 0x03f));
            return 2;
        }

        if (ch <= 0xffff) {
            buffer[0] = (char)(0xe0 | (ch >> 12));
            buffer[1] = (char)(0x80 | ((ch >> 6) & 0x3f));
            buffer[2] = (char)(0x80 | (ch & 0x3f));
            return 3;
        }

        if (ch <= 0x10ffff) {
            buffer[0] = (char)(0xf0 | (ch >> 18));
            buffer[1] = (char)(0x80 | ((ch >> 12) & 0x3f));
            buffer[2] = (char)(0x80 | ((ch >> 6) & 0x3f));
            buffer[3] = (char)(0x80 | (ch & 0x3f));
            return 4;
        }

        return -1;
    }

    int parse_int(long &val, bool &neg) {
        int digits = 0;
        if ((neg = current_ == '-') || current_ == '+') {
            read();
        }
        val = 0;
        while (isdigit(current_)) {
            val = val * 10 + (current_ - '0');
            read();
            digits++;
        }
        return digits;
    }

    int parse_num(Token &val) {
        long lval;
        bool neg;
        int digits = parse_int(lval, neg);

        double scale = 1.0;
        double dval = lval;
        bool is_float = false;

        if (current_ == '.') {
            read();
            while (isdigit(current_)) {
                dval += (scale /= 10) * (current_ - '0');
                digits++;
                read();
            }
            is_float = true;
        }

        if (current_ == 'e' || current_ == 'E') {
            read();
            bool neg;
            if (parse_int(lval, neg) == 0) {
                return Error;
            }
            scale = neg ? 0.1 : 10.0;
            while (lval > 0) {
                dval *= scale;
                lval--;
            }
            is_float = true;
        }

        if (digits == 0) {
            return Error;
        }

        if (!is_float) {
            val.lval = neg ? -lval : lval;
            return Integer;
        }

        val.dval = neg ? -dval : dval;

        return Number;
    }

    void start() {
        read();
        parse();
    }

   public:
    Reader(std::istream &input) : input_(&input), size_(0) { start(); }
    Reader(const std::string &s) : input_(nullptr), data_(s.data()), size_(s.length()) { start(); }
    Reader(const char *s, size_t len = 0) : input_(nullptr), data_(s), size_(len ? len : strlen(s)) { start(); }

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

    bool read(bool &value) {
        if (next_.type == Bool) {
            value = next_.bval;
            parse();
            return true;
        }
        return false;
    }

    template <typename Type, std::enable_if_t<std::is_integral<Type>::value, bool> = true>
    bool read(Type &value) {
        if (next_.type == Integer) {
            value = (double)next_.lval;
            parse();
            return true;
        }
        return false;
    }

    template <typename Type, std::enable_if_t<std::is_floating_point<Type>::value, bool> = true>
    bool read(Type &value) {
        if (next_.type == Number) {
            value = next_.dval;
            parse();
            return true;
        } else if (next_.type == Integer) {
            value = (Type)next_.lval;
            parse();
            return true;
        }
        return false;
    }

    template <typename Type, std::enable_if_t<std::is_pointer<Type>::value, bool> = true>
    bool read(Type &value, std::function<bool()> fn = {}) {
        if (next_is(Null)) {
            value = nullptr;
            parse();
            return true;
        }
        if (fn) {
            return fn();
        }
        value = new typename std::remove_pointer<Type>::type();
        return read(*value);
    }

    template <class T>
    bool read(std::vector<T> &values) {
        if (!consume('[')) {
            return false;
        }
        while (!next_is(']')) {
            values.emplace_back(T());
            if (!read(values.back())) {
                return false;
            }
            if (!consume(',')) {
                break;
            }
        }
        return consume(']');
    }

    template <class T>
    bool read(std::vector<T> &values, std::function<bool()> fn) {
        if (!consume('[')) {
            return false;
        }
        while (!next_is(']')) {
            if (!fn()) {
                return false;
            }
            if (!consume(',')) {
                break;
            }
        }
        return consume(']');
    }

    bool read(std::function<bool(const std::string &)> fn) {
        if (!consume('{')) {
            return false;
        }
        while (!next_is('}')) {
            if (!read_key(key_)) {
                return false;
            }
            if (!fn(key_)) {
                return false;
            }
            if (!consume(',')) {
                break;
            }
        }
        return consume('}');
    }

    template <class T>
    bool read(std::map<std::string, T> &m) {
        return read([this, &m](const std::string &key) { return read(m[key]); });
    }

    bool read(std::string &s) {
        s.clear();

        while (current_ != Empty) {
            read();
            if (current_ == '"') {
                read();
                parse();
                return true;
            } else if (current_ == '\\') {
                read();
                switch (current_) {
                    case '"':
                        s.push_back('"');
                        break;
                    case '\\':
                        s.push_back('\\');
                        break;
                    case '/':
                        s.push_back('/');
                        break;
                    case 'b':
                        s.push_back('\b');
                        break;
                    case 'f':
                        s.push_back('\f');
                        break;
                    case 'n':
                        s.push_back('\n');
                        break;
                    case 'r':
                        s.push_back('\r');
                        break;
                    case 't':
                        s.push_back('\t');
                        break;
                    case 'u':
                        if (parse_hex(s) == Error) {
                            return false;
                        }
                        break;
                    default:
                        return false;
                }
            } else {
                s.push_back(current_);
            }
        }

        return false;
    }

    bool read_key(std::string &key) {
        if (next_.type != String) {
            return false;
        }

        if (!read(key)) {
            return false;
        }

        if (!next_is(':')) {
            return false;
        }

        parse();

        return true;
    }
};

struct str {
    const char *s;
    size_t len;
    str(const std::string &s) : str(s.data(), s.length()) {}
    str(const char *s) : str(s, strlen(s)) {}
    str(const char *s, size_t len) : s(s), len(len) {}
};

inline std::ostream &operator<<(std::ostream &os, const str &s) {
    const char *p = s.s;
    const char *q = s.s + s.len;

    os << '"';

    for (char c = *p; p < q; c = *++p) {
        switch (c) {
            case '\"':
                os << "\\\"";
                break;
            case '\\':
                os << "\\\\";
                break;
            case '/':
                os << "\\/";
                break;
            case '\b':
                os << "\\b";
                break;
            case '\f':
                os << "\\f";
                break;
            case '\n':
                os << "\\n";
                break;
            case '\r':
                os << "\\r";
                break;
            case '\t':
                os << "\\t";
                break;
            default:
                if (c >= 0 && c <= 0x1F) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    os << buf;
                } else {
                    os << c;
                }
                break;
        }
    }

    os << '"';

    return os;
}

}  // namespace jsrw
