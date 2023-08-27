#pragma once

#include <functional>
#include <istream>
#include <map>
#include <ostream>
#include <string>
#include <type_traits>
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

template <size_t BUFF = 4096>
class Reader {
   private:
    std::istream &input_;
    char buff_[BUFF];
    char *data_;
    size_t size_;
    int current_;

    inline int read() {
        if (size_ == 0) {
            input_.read(buff_, BUFF);
            data_ = buff_;
            if ((size_ = input_.gcount()) == 0) {
                return (current_ = Empty);
            }
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
        char input[4];
        input[0] = read();
        input[1] = read();
        input[2] = read();
        input[3] = read();
        int cp = 0;
        for (int i = 0; i < 4; i++) {
            char c = input[i];
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
        char data[4];
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

   public:
    Reader(std::istream &input) : input_(input), size_(0) {
        read();
        parse();
    }

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

    template <class T>
    bool read(std::vector<T> *&v, std::function<bool(T &)> fn) {
        if (next_is(Null)) {
            v = nullptr;
            parse();
            return true;
        }
        v = new std::vector<T>();
        return read(*v, fn);
    }

    template <class T>
    bool read(std::vector<T> &v, std::function<bool(T &)> fn) {
        if (!consume('[')) {
            return false;
        }
        v.clear();
        while (!next_is(']')) {
            T t;
            if (!fn(t)) {
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
    bool read(std::map<std::string, T> *&m, std::function<bool(T &)> fn) {
        if (next_is(Null)) {
            m = nullptr;
            parse();
            return true;
        }
        m = new std::map<std::string, T>();
        return read(*m, fn);
    }

    template <class T>
    bool read(std::map<std::string, T> &m, std::function<bool(T &)> fn) {
        if (!consume('{')) {
            return false;
        }
        while (!next_is('}')) {
            std::string key;
            if (!read_key(key)) {
                return false;
            }
            T t;
            if (!fn(t)) {
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

class Writer {
   protected:
    std::ostream &out;

   public:
    Writer(std::ostream &out) : out(out) {}

    Writer &write(bool b) {
        out << (b ? "true" : "false");
        return *this;
    }

    Writer &write(int n) {
        out << n;
        return *this;
    }

    Writer &write(double d) {
        out << d;
        return *this;
    }

    Writer &write(const std::string &s) {
        return write(s.data(), s.length());
    }

    Writer &write(const char *s) {
        if (s == nullptr) {
            out << "null";
            return *this;
        }
        return write(s, strlen(s));
    }

    template <typename T>
    Writer &write(const T *p) {
        if (p == nullptr) {
            out << "null";
            return *this;
        } else {
            return write(*p);
        }
    }

    template <typename T>
    Writer &write(const std::vector<T> &v, std::function<void(const T &)> fn) {
        out << '[';
        size_t i = 0;
        for (const auto &p : v) {
            if (i++ > 0) {
                out << ',';
            }
            fn(p);
        }
        out << ']';
        return *this;
    }

    template <typename T>
    Writer &write(const std::map<std::string, T> &m, std::function<void(const T &)> fn) {
        out << '{';
        size_t i = 0;
        for (const auto &p : m) {
            if (i++ > 0) {
                out << ',';
            }
            write(p.first.data(), p.first.length());
            out << ':';
            fn(p.second);
        }
        out << '}';
        return *this;
    }

    Writer& write(const char *s, size_t len) {
        const char *end = s + len;

        out << '"';

        for (char c = *s; s < end; c = *++s) {
            switch (c) {
                case '\"':
                    out << "\\\"";
                    break;
                case '\\':
                    out << "\\\\";
                    break;
                case '/':
                    out << "\\/";
                    break;
                case '\b':
                    out << "\\b";
                    break;
                case '\f':
                    out << "\\f";
                    break;
                case '\n':
                    out << "\\n";
                    break;
                case '\r':
                    out << "\\r";
                    break;
                case '\t':
                    out << "\\t";
                    break;
                default:
                    if (c >= 0 && c <= 0x1F) {
                        char buf[8];
                        snprintf(buf, sizeof(buf), "\\u%04x", c);
                        out << buf;
                    } else {
                        out << c;
                    }
                    break;
            }
        }

        out << '"';

        return *this;
    }
};

}  // namespace jsrw
