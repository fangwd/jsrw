#include "jsrw.h"

#include <ctype.h>

namespace jsrw {

Reader::Reader(const std::string &input) : Reader(input.data(), input.data() + input.size()) {}

Reader::Reader(const char *begin, const char *end) : input_end_(end), input_(begin) {
    parse();
}
Reader::Reader(const char *begin) : input_end_(begin + strlen(begin)), input_(begin) {
    parse();
}

void Reader::parse() {
    Token &token = next_;

    skip_space();

    if (input_ >= input_end_) {
        token.type = Empty;
    } else {
        int ch = *input_;

        switch (ch) {
        case '{':
        case '}':
        case '[':
        case ']':
        case ':':
        case ',':
            input_ += 1;
            token.type = ch;
            break;
        case '"':
            input_ += 1;
            token.type = String;
            break;
        case 'n':
            if (input_ + 3 < input_end_ && input_[1] == 'u' && input_[2] == 'l' && input_[3] == 'l') {
                input_ += 4;
                token.type = Null;
            } else {
                token.type = Error;
            }
            break;
        case 't':
            if (input_ + 3 < input_end_ && input_[1] == 'r' && input_[2] == 'u' && input_[3] == 'e') {
                input_ += 4;
                token.bval = true;
                token.type = Bool;
            } else {
                token.type = Error;
            }
            break;
        case 'f':
            if (input_ + 4 < input_end_ && input_[1] == 'a' && input_[2] == 'l' && input_[3] == 's' &&
                    input_[4] == 'e') {
                input_ += 5;
                token.bval = false;
                token.type = Bool;
            } else {
                token.type = Error;
            }
            break;
        default:
            token.type = parse_num(token);
            break;
        }
    }
}

int Reader::parse_int(long &val, bool &neg) {
    int digits = 0;
    if ((neg = *input_ == '-') || *input_ == '+') {
        input_++;
    }
    val = 0;
    while (isdigit(*input_)) {
        val = val * 10 + (*input_ - '0');
        input_++;
        digits++;
    }
    return digits;
}

int Reader::parse_num(Token &val) {
    long lval;
    bool neg;
    int digits = parse_int(lval, neg);

    double scale = 1.0;
    double dval = lval;
    bool is_float = false;

    if (*input_ == '.') {
        input_++;
        while (isdigit(*input_)) {
            dval += (scale /= 10) * (*input_ - '0');
            digits++;
            input_++;
        }
        is_float = true;
    }

    if (*input_ == 'e' || *input_ == 'E') {
        input_++;
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

int32_t encode_utf8(int32_t ch, char *buffer) {
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

bool Reader::read(std::string &s) {
    s.clear();

    while (input_ != input_end_) {
        char c = *input_++;
        if (c == '"') {
            parse();
            return true;
        } else if (c == '\\') {
            if (input_ == input_end_) {
                return false;
            }
            switch (*input_++) {
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
            s.push_back(c);
        }
    }

    return false;
}

bool Reader::read(Slice &s) {
    if (next_.type != String) {
        return false;
    }

    s.data = input_;

    skip_string();

    s.length = input_ - s.data - 1;

    parse();

    return true;
}

int Reader::parse_hex(std::string &s) {
    if (input_ + 4 >= input_end_) {
        return Error;
    }
    int cp = 0;
    for (int i = 0; i < 4; i++) {
        char c = input_[i];
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
    input_ += 4;
    return String;
}

bool Reader::read_key(std::string &key) {
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

bool Reader::read_key(Slice& s) {
    if (next_.type != String) {
        return false;
    }

    s.data = input_;

    skip_string();

    s.length = input_ - s.data - 1;

    parse();

    if (!next_is(':')) {
        return false;
    }

    parse();

    return true;
}

void Reader::skip_space() {
    while (input_ < input_end_ && isspace(*input_)) input_++;
}

void Reader::skip_string() {
    while (input_ != input_end_) {
        char c = *input_++;
        if (c == '"') {
            break;
        } else if (c == '\\') {
            input_++;
        }
    }
}

std::ostream &operator<<(std::ostream &os, const str &s) {
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

bool Slice::operator==(const char *s) const {
    size_t offset = 0;
    while (*s) {
        if (offset >= length || *s != data[offset]) {
            return false;
        }
        s++;
        offset++;
    }
    return offset == length;
}

bool Slice::operator==(const std::string& s) const {
    return operator==(Slice(s));
}

bool Slice::operator==(const Slice &other) const {
    if (length != other.length) {
        return false;
    }
    for (size_t i = 0; i < length; i++) {
        if (data[i] != other.data[i]) {
        return false;
        }
    }
    return true;
}

}  // namespace jsrw
