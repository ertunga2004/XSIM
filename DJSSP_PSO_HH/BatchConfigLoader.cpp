#include "BatchConfigLoader.h"

namespace djssp {

namespace {

void set_error(std::string* error_message, const std::string& message) {
    if (error_message != nullptr) {
        *error_message = message;
    }
}

void skip_ws(const std::string& text, size_t& pos) {
    while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
        ++pos;
    }
}

bool parse_json_string(const std::string& text, size_t& pos, std::string& out) {
    skip_ws(text, pos);
    if (pos >= text.size() || text[pos] != '"') {
        return false;
    }

    ++pos;
    out.clear();
    while (pos < text.size()) {
        const char ch = text[pos++];
        if (ch == '"') {
            return true;
        }
        if (ch != '\\') {
            out.push_back(ch);
            continue;
        }
        if (pos >= text.size()) {
            return false;
        }
        const char esc = text[pos++];
        switch (esc) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            default: out.push_back(esc); break;
        }
    }
    return false;
}

bool skip_json_string(const std::string& text, size_t& pos) {
    std::string ignored;
    return parse_json_string(text, pos, ignored);
}

bool skip_json_value(const std::string& text, size_t& pos) {
    skip_ws(text, pos);
    if (pos >= text.size()) {
        return false;
    }

    if (text[pos] == '"') {
        return skip_json_string(text, pos);
    }

    if (text[pos] == '{' || text[pos] == '[') {
        const char open = text[pos];
        const char close = (open == '{') ? '}' : ']';
        int depth = 0;
        while (pos < text.size()) {
            if (text[pos] == '"') {
                if (!skip_json_string(text, pos)) {
                    return false;
                }
                continue;
            }
            if (text[pos] == open) {
                ++depth;
            } else if (text[pos] == close) {
                --depth;
                ++pos;
                if (depth == 0) {
                    return true;
                }
                continue;
            }
            ++pos;
        }
        return false;
    }

    while (pos < text.size() && text[pos] != ',' && text[pos] != '}' && text[pos] != ']') {
        ++pos;
    }
    return true;
}

bool object_bounds(const std::string& text, size_t value_begin, size_t& object_begin, size_t& object_end) {
    size_t pos = value_begin;
    skip_ws(text, pos);
    if (pos >= text.size() || text[pos] != '{') {
        return false;
    }
    object_begin = pos;
    if (!skip_json_value(text, pos)) {
        return false;
    }
    object_end = pos;
    return true;
}

bool find_member_range(
    const std::string& text,
    const std::string& key,
    size_t& value_begin,
    size_t& value_end
) {
    size_t object_begin = 0;
    size_t object_end = 0;
    if (!object_bounds(text, 0, object_begin, object_end)) {
        return false;
    }

    size_t pos = object_begin + 1;
    while (pos < object_end) {
        skip_ws(text, pos);
        if (pos >= object_end || text[pos] == '}') {
            return false;
        }

        std::string current_key;
        if (!parse_json_string(text, pos, current_key)) {
            return false;
        }
        skip_ws(text, pos);
        if (pos >= object_end || text[pos] != ':') {
            return false;
        }
        ++pos;
        skip_ws(text, pos);

        const size_t candidate_begin = pos;
        if (!skip_json_value(text, pos)) {
            return false;
        }
        const size_t candidate_end = pos;

        if (current_key == key) {
            value_begin = candidate_begin;
            value_end = candidate_end;
            return true;
        }

        skip_ws(text, pos);
        if (pos < object_end && text[pos] == ',') {
            ++pos;
        }
    }

    return false;
}

bool read_string_member(const std::string& text, const std::string& key, std::string& out) {
    size_t begin = 0;
    size_t end = 0;
    if (!find_member_range(text, key, begin, end)) {
        return false;
    }
    size_t pos = begin;
    return parse_json_string(text, pos, out);
}

bool read_string_array_member(
    const std::string& text,
    const std::string& key,
    std::vector<std::string>& out,
    std::string* error_message
) {
    size_t begin = 0;
    size_t end = 0;
    if (!find_member_range(text, key, begin, end)) {
        set_error(error_message, "Batch config must contain runs string array.");
        return false;
    }

    size_t pos = begin;
    skip_ws(text, pos);
    if (pos >= end || text[pos] != '[') {
        set_error(error_message, "Batch config runs must be a string array.");
        return false;
    }
    ++pos;

    std::vector<std::string> values;
    while (pos < end) {
        skip_ws(text, pos);
        if (pos < end && text[pos] == ']') {
            ++pos;
            out = std::move(values);
            return true;
        }

        std::string value;
        if (!parse_json_string(text, pos, value)) {
            set_error(error_message, "Batch config runs must contain only strings.");
            return false;
        }
        values.push_back(std::move(value));

        skip_ws(text, pos);
        if (pos < end && text[pos] == ',') {
            ++pos;
            continue;
        }
        if (pos < end && text[pos] == ']') {
            ++pos;
            out = std::move(values);
            return true;
        }

        set_error(error_message, "Batch config runs array has invalid JSON syntax.");
        return false;
    }

    set_error(error_message, "Batch config runs array is not closed.");
    return false;
}

}  // namespace

bool BatchConfigLoader::load_file(
    const std::filesystem::path& path,
    BatchConfig& out_config,
    std::string* error_message
) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        set_error(error_message, "Could not open batch config file: " + path.string());
        return false;
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();
    const std::string text = buffer.str();
    if (text.empty()) {
        set_error(error_message, "Batch config file is empty: " + path.string());
        return false;
    }

    BatchConfig config;
    config.source_path = path;
    config.original_json = text;

    read_string_member(text, "schema_version", config.schema_version);
    read_string_member(text, "suite_name", config.suite_name);
    if (!read_string_array_member(text, "runs", config.runs, error_message)) {
        return false;
    }

    if (config.schema_version != "1.0") {
        set_error(error_message, "Unsupported batch schema_version: " + config.schema_version);
        return false;
    }
    if (config.suite_name.empty()) {
        set_error(error_message, "Batch config suite_name must not be empty.");
        return false;
    }
    if (config.runs.empty()) {
        set_error(error_message, "Batch config runs must contain at least one config path.");
        return false;
    }
    for (const auto& run_path : config.runs) {
        if (run_path.empty()) {
            set_error(error_message, "Batch config runs must not contain empty config paths.");
            return false;
        }
    }

    out_config = std::move(config);
    return true;
}

}  // namespace djssp
