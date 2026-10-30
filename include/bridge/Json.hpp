#pragma once

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <iomanip>
#include <initializer_list>

namespace deadlock::bridge {

enum class JsonType {
    Null,
    Boolean,
    Number,
    String,
    Array,
    Object
};

class JsonValue {
public:
    JsonType type{JsonType::Null};
    bool boolVal{false};
    double numVal{0.0};
    std::string strVal;
    std::vector<JsonValue> arrVal;
    std::map<std::string, JsonValue> objVal;

    JsonValue() : type(JsonType::Null) {}
    JsonValue(std::nullptr_t) : type(JsonType::Null) {}
    JsonValue(bool b) : type(JsonType::Boolean), boolVal(b) {}
    JsonValue(int n) : type(JsonType::Number), numVal(static_cast<double>(n)) {}
    JsonValue(long long n) : type(JsonType::Number), numVal(static_cast<double>(n)) {}
    JsonValue(size_t n) : type(JsonType::Number), numVal(static_cast<double>(n)) {}
    JsonValue(double d) : type(JsonType::Number), numVal(d) {}
    JsonValue(const char* s) : type(JsonType::String), strVal(s ? s : "") {}
    JsonValue(const std::string& s) : type(JsonType::String), strVal(s) {}
    JsonValue(std::vector<JsonValue> arr) : type(JsonType::Array), arrVal(std::move(arr)) {}
    JsonValue(std::map<std::string, JsonValue> obj) : type(JsonType::Object), objVal(std::move(obj)) {}

    static JsonValue array(std::initializer_list<JsonValue> list) {
        return JsonValue(std::vector<JsonValue>(list.begin(), list.end()));
    }

    static JsonValue object(std::initializer_list<std::pair<const std::string, JsonValue>> list) {
        std::map<std::string, JsonValue> m;
        for (const auto& kv : list) {
            m[kv.first] = kv.second;
        }
        return JsonValue(std::move(m));
    }

    bool isNull() const { return type == JsonType::Null; }
    bool isBool() const { return type == JsonType::Boolean; }
    bool isNumber() const { return type == JsonType::Number; }
    bool isString() const { return type == JsonType::String; }
    bool isArray() const { return type == JsonType::Array; }
    bool isObject() const { return type == JsonType::Object; }

    bool asBool(bool def = false) const {
        return (type == JsonType::Boolean) ? boolVal : def;
    }

    int asInt(int def = 0) const {
        return (type == JsonType::Number) ? static_cast<int>(numVal) : def;
    }

    double asDouble(double def = 0.0) const {
        return (type == JsonType::Number) ? numVal : def;
    }

    std::string asString(const std::string& def = "") const {
        return (type == JsonType::String) ? strVal : def;
    }

    const std::vector<JsonValue>& asArray() const {
        static const std::vector<JsonValue> emptyArr;
        return (type == JsonType::Array) ? arrVal : emptyArr;
    }

    const std::map<std::string, JsonValue>& asObject() const {
        static const std::map<std::string, JsonValue> emptyObj;
        return (type == JsonType::Object) ? objVal : emptyObj;
    }

    bool has(const std::string& key) const {
        if (type != JsonType::Object) return false;
        return objVal.find(key) != objVal.end();
    }

    JsonValue get(const std::string& key) const {
        if (type != JsonType::Object) return JsonValue();
        auto it = objVal.find(key);
        return (it != objVal.end()) ? it->second : JsonValue();
    }

    JsonValue& operator[](const std::string& key) {
        type = JsonType::Object;
        return objVal[key];
    }

    void push_back(JsonValue val) {
        if (type != JsonType::Array) {
            type = JsonType::Array;
            arrVal.clear();
        }
        arrVal.push_back(std::move(val));
    }

    std::string serialize(bool pretty = false, int indent = 0) const {
        std::ostringstream ss;
        serializeInternal(ss, pretty, indent);
        return ss.str();
    }

    static JsonValue parse(const std::string& str) {
        size_t idx = 0;
        skipWhitespace(str, idx);
        if (idx >= str.size()) return JsonValue();
        return parseValue(str, idx);
    }

private:
    void serializeInternal(std::ostringstream& ss, bool pretty, int indent) const {
        std::string pad(pretty ? indent * 2 : 0, ' ');
        std::string nextPad(pretty ? (indent + 1) * 2 : 0, ' ');
        std::string nl = pretty ? "\n" : "";
        std::string sp = pretty ? " " : "";

        switch (type) {
            case JsonType::Null:
                ss << "null";
                break;
            case JsonType::Boolean:
                ss << (boolVal ? "true" : "false");
                break;
            case JsonType::Number:
                if (numVal == static_cast<long long>(numVal)) {
                    ss << static_cast<long long>(numVal);
                } else {
                    ss << std::setprecision(6) << numVal;
                }
                break;
            case JsonType::String:
                ss << quote(strVal);
                break;
            case JsonType::Array: {
                if (arrVal.empty()) {
                    ss << "[]";
                    break;
                }
                ss << "[" << nl;
                for (size_t i = 0; i < arrVal.size(); ++i) {
                    ss << nextPad;
                    arrVal[i].serializeInternal(ss, pretty, indent + 1);
                    if (i + 1 < arrVal.size()) ss << ",";
                    ss << nl;
                }
                ss << pad << "]";
                break;
            }
            case JsonType::Object: {
                if (objVal.empty()) {
                    ss << "{}";
                    break;
                }
                ss << "{" << nl;
                size_t count = 0;
                for (auto it = objVal.begin(); it != objVal.end(); ++it, ++count) {
                    ss << nextPad << quote(it->first) << ":" << sp;
                    it->second.serializeInternal(ss, pretty, indent + 1);
                    if (count + 1 < objVal.size()) ss << ",";
                    ss << nl;
                }
                ss << pad << "}";
                break;
            }
        }
    }

    static std::string quote(const std::string& s) {
        std::ostringstream ss;
        ss << '"';
        for (char c : s) {
            switch (c) {
                case '"':  ss << "\\\""; break;
                case '\\': ss << "\\\\"; break;
                case '\b': ss << "\\b"; break;
                case '\f': ss << "\\f"; break;
                case '\n': ss << "\\n"; break;
                case '\r': ss << "\\r"; break;
                case '\t': ss << "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                    } else {
                        ss << c;
                    }
            }
        }
        ss << '"';
        return ss.str();
    }

    static void skipWhitespace(const std::string& s, size_t& idx) {
        while (idx < s.size() && (s[idx] == ' ' || s[idx] == '\t' || s[idx] == '\r' || s[idx] == '\n')) {
            idx++;
        }
    }

    static JsonValue parseValue(const std::string& s, size_t& idx) {
        skipWhitespace(s, idx);
        if (idx >= s.size()) return JsonValue();

        char c = s[idx];
        if (c == '{') return parseObject(s, idx);
        if (c == '[') return parseArray(s, idx);
        if (c == '"') return parseString(s, idx);
        if (c == 't' || c == 'f') return parseBool(s, idx);
        if (c == 'n') return parseNull(s, idx);
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parseNumber(s, idx);

        throw std::runtime_error(std::string("Unexpected character in JSON: ") + c);
    }

    static JsonValue parseObject(const std::string& s, size_t& idx) {
        idx++; // skip '{'
        std::map<std::string, JsonValue> obj;
        skipWhitespace(s, idx);

        if (idx < s.size() && s[idx] == '}') {
            idx++;
            return JsonValue(std::move(obj));
        }

        while (idx < s.size()) {
            skipWhitespace(s, idx);
            if (idx >= s.size() || s[idx] != '"') {
                throw std::runtime_error("Expected string key in object");
            }
            std::string key = parseRawString(s, idx);
            skipWhitespace(s, idx);
            if (idx >= s.size() || s[idx] != ':') {
                throw std::runtime_error("Expected ':' after key");
            }
            idx++; // skip ':'
            skipWhitespace(s, idx);
            JsonValue val = parseValue(s, idx);
            obj[key] = std::move(val);

            skipWhitespace(s, idx);
            if (idx < s.size() && s[idx] == ',') {
                idx++;
                continue;
            } else if (idx < s.size() && s[idx] == '}') {
                idx++;
                break;
            } else {
                throw std::runtime_error("Expected ',' or '}' in object");
            }
        }
        return JsonValue(std::move(obj));
    }

    static JsonValue parseArray(const std::string& s, size_t& idx) {
        idx++; // skip '['
        std::vector<JsonValue> arr;
        skipWhitespace(s, idx);

        if (idx < s.size() && s[idx] == ']') {
            idx++;
            return JsonValue(std::move(arr));
        }

        while (idx < s.size()) {
            skipWhitespace(s, idx);
            JsonValue val = parseValue(s, idx);
            arr.push_back(std::move(val));

            skipWhitespace(s, idx);
            if (idx < s.size() && s[idx] == ',') {
                idx++;
                continue;
            } else if (idx < s.size() && s[idx] == ']') {
                idx++;
                break;
            } else {
                throw std::runtime_error("Expected ',' or ']' in array");
            }
        }
        return JsonValue(std::move(arr));
    }

    static std::string parseRawString(const std::string& s, size_t& idx) {
        idx++; // skip opening '"'
        std::string res;
        while (idx < s.size()) {
            char c = s[idx++];
            if (c == '"') {
                return res;
            }
            if (c == '\\' && idx < s.size()) {
                char esc = s[idx++];
                switch (esc) {
                    case '"': res += '"'; break;
                    case '\\': res += '\\'; break;
                    case '/': res += '/'; break;
                    case 'b': res += '\b'; break;
                    case 'f': res += '\f'; break;
                    case 'n': res += '\n'; break;
                    case 'r': res += '\r'; break;
                    case 't': res += '\t'; break;
                    default: res += esc; break;
                }
            } else {
                res += c;
            }
        }
        throw std::runtime_error("Unterminated string in JSON");
    }

    static JsonValue parseString(const std::string& s, size_t& idx) {
        return JsonValue(parseRawString(s, idx));
    }

    static JsonValue parseBool(const std::string& s, size_t& idx) {
        if (s.compare(idx, 4, "true") == 0) {
            idx += 4;
            return JsonValue(true);
        }
        if (s.compare(idx, 5, "false") == 0) {
            idx += 5;
            return JsonValue(false);
        }
        throw std::runtime_error("Invalid boolean token");
    }

    static JsonValue parseNull(const std::string& s, size_t& idx) {
        if (s.compare(idx, 4, "null") == 0) {
            idx += 4;
            return JsonValue(nullptr);
        }
        throw std::runtime_error("Invalid null token");
    }

    static JsonValue parseNumber(const std::string& s, size_t& idx) {
        size_t start = idx;
        if (s[idx] == '-') idx++;
        while (idx < s.size() && std::isdigit(static_cast<unsigned char>(s[idx]))) {
            idx++;
        }
        if (idx < s.size() && s[idx] == '.') {
            idx++;
            while (idx < s.size() && std::isdigit(static_cast<unsigned char>(s[idx]))) {
                idx++;
            }
        }
        if (idx < s.size() && (s[idx] == 'e' || s[idx] == 'E')) {
            idx++;
            if (idx < s.size() && (s[idx] == '+' || s[idx] == '-')) idx++;
            while (idx < s.size() && std::isdigit(static_cast<unsigned char>(s[idx]))) {
                idx++;
            }
        }
        std::string numStr = s.substr(start, idx - start);
        double val = std::stod(numStr);
        return JsonValue(val);
    }
};

} // namespace deadlock::bridge
