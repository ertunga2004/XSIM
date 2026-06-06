#include "ConfigLoader.h"

#include <iomanip>

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
    size_t object_begin,
    size_t object_end,
    const std::string& key,
    size_t& value_begin,
    size_t& value_end
) {
    if (object_begin >= object_end || text[object_begin] != '{') {
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

bool find_path_range(
    const std::string& text,
    const std::vector<std::string>& path,
    size_t& value_begin,
    size_t& value_end
) {
    size_t object_begin = 0;
    size_t object_end = 0;
    if (!object_bounds(text, 0, object_begin, object_end)) {
        return false;
    }

    for (size_t i = 0; i < path.size(); ++i) {
        if (!find_member_range(text, object_begin, object_end, path[i], value_begin, value_end)) {
            return false;
        }
        if (i + 1 == path.size()) {
            return true;
        }
        if (!object_bounds(text, value_begin, object_begin, object_end)) {
            return false;
        }
    }

    return false;
}

bool read_string_path(const std::string& text, const std::vector<std::string>& path, std::string& out) {
    size_t begin = 0;
    size_t end = 0;
    if (!find_path_range(text, path, begin, end)) {
        return false;
    }

    size_t pos = begin;
    skip_ws(text, pos);
    if (text.compare(pos, 4, "null") == 0) {
        out.clear();
        return true;
    }
    return parse_json_string(text, pos, out);
}

bool read_string_array_path(
    const std::string& text,
    const std::vector<std::string>& path,
    std::vector<std::string>& out,
    std::string* error_message,
    const std::string& field_name
) {
    size_t begin = 0;
    size_t end = 0;
    if (!find_path_range(text, path, begin, end)) {
        return false;
    }

    size_t pos = begin;
    skip_ws(text, pos);
    if (pos >= end || text[pos] != '[') {
        set_error(error_message, "Config " + field_name + " must be a string array.");
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
            set_error(error_message, "Config " + field_name + " must contain only strings.");
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

        set_error(error_message, "Config " + field_name + " array has invalid JSON syntax.");
        return false;
    }

    set_error(error_message, "Config " + field_name + " array is not closed.");
    return false;
}

bool read_bool_path(const std::string& text, const std::vector<std::string>& path, bool& out) {
    size_t begin = 0;
    size_t end = 0;
    if (!find_path_range(text, path, begin, end)) {
        return false;
    }

    size_t pos = begin;
    skip_ws(text, pos);
    if (text.compare(pos, 4, "true") == 0) {
        out = true;
        return true;
    }
    if (text.compare(pos, 5, "false") == 0) {
        out = false;
        return true;
    }
    return false;
}

bool read_int_path(const std::string& text, const std::vector<std::string>& path, int& out) {
    size_t begin = 0;
    size_t end = 0;
    if (!find_path_range(text, path, begin, end)) {
        return false;
    }

    try {
        out = std::stoi(text.substr(begin, end - begin));
        return true;
    } catch (...) {
        return false;
    }
}

bool read_uint64_path(const std::string& text, const std::vector<std::string>& path, uint64_t& out) {
    size_t begin = 0;
    size_t end = 0;
    if (!find_path_range(text, path, begin, end)) {
        return false;
    }

    try {
        out = static_cast<uint64_t>(std::stoull(text.substr(begin, end - begin)));
        return true;
    } catch (...) {
        return false;
    }
}

bool read_double_path(const std::string& text, const std::vector<std::string>& path, double& out) {
    size_t begin = 0;
    size_t end = 0;
    if (!find_path_range(text, path, begin, end)) {
        return false;
    }

    try {
        out = std::stod(text.substr(begin, end - begin));
        return true;
    } catch (...) {
        return false;
    }
}

std::string json_escape(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 8);
    for (char ch : value) {
        switch (ch) {
            case '\\': escaped += "\\\\"; break;
            case '"': escaped += "\\\""; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped.push_back(ch); break;
        }
    }
    return escaped;
}

std::string json_string(const std::string& value) {
    return "\"" + json_escape(value) + "\"";
}

std::string json_optional_string(const std::string& value) {
    return value.empty() ? "null" : json_string(value);
}

std::string json_bool(bool value) {
    return value ? "true" : "false";
}

void write_string_array(std::ostream& out, const std::vector<std::string>& values) {
    out << "[";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            out << ", ";
        }
        out << json_string(values[i]);
    }
    out << "]";
}

std::string json_number(double value) {
    const double rounded = std::round(value);
    if (std::fabs(value - rounded) <= 1e-9) {
        return std::to_string(static_cast<long long>(rounded));
    }

    std::ostringstream out;
    out << std::fixed << std::setprecision(6) << value;
    std::string text = out.str();
    while (!text.empty() && text.back() == '0') {
        text.pop_back();
    }
    if (!text.empty() && text.back() == '.') {
        text.pop_back();
    }
    return text.empty() ? "0" : text;
}

}  // namespace

bool ConfigLoader::load_file(
    const fs::path& path,
    XSimConfig& out_config,
    std::string* error_message
) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        set_error(error_message, "Could not open config file: " + path.string());
        return false;
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();
    const std::string text = buffer.str();
    if (text.empty()) {
        set_error(error_message, "Config file is empty: " + path.string());
        return false;
    }

    XSimConfig config;
    config.source_path = path.generic_string();
    config.original_json = text;

    read_string_path(text, {"schema_version"}, config.schema_version);
    read_string_path(text, {"run", "name"}, config.run.name);
    read_uint64_path(text, {"run", "seed"}, config.run.seed);
    read_string_path(text, {"run", "output_root"}, config.run.output_root);
    read_bool_path(text, {"run", "write_reports"}, config.run.write_reports);

    read_string_path(text, {"instance", "source"}, config.instance.source);
    read_string_path(text, {"instance", "name"}, config.instance.name);
    read_string_path(text, {"instance", "path"}, config.instance.path);

    read_string_path(text, {"objective"}, config.objective);
    read_string_path(text, {"solver", "method"}, config.solver.method);
    read_string_path(text, {"solver", "sgs"}, config.solver.sgs);
    read_int_path(text, {"solver", "iters"}, config.solver.iters);
    read_int_path(text, {"solver", "swarm"}, config.solver.swarm);
    read_int_path(text, {"solver", "evalk"}, config.solver.evalk);
    read_int_path(text, {"solver", "finalk"}, config.solver.finalk);
    read_double_path(text, {"solver", "eps0"}, config.solver.eps0);
    read_double_path(text, {"solver", "epsmin"}, config.solver.epsmin);
    read_bool_path(text, {"solver", "fitavg"}, config.solver.fitavg);
    read_bool_path(text, {"solver", "traindet"}, config.solver.traindet);

    const bool rules_read = read_string_array_path(text, {"rules"}, config.rules, error_message, "rules");
    if (!rules_read && error_message != nullptr && !error_message->empty()) {
        return false;
    }
    config.rules_specified = rules_read;

    const bool features_read = read_string_array_path(text, {"features"}, config.features, error_message, "features");
    if (!features_read && error_message != nullptr && !error_message->empty()) {
        return false;
    }
    config.features_specified = features_read;

    read_bool_path(text, {"improvement", "enabled"}, config.improvement.enabled);
    read_string_path(text, {"improvement", "type"}, config.improvement.type);
    read_int_path(text, {"improvement", "slsiters"}, config.improvement.slsiters);
    read_int_path(text, {"improvement", "tsiters"}, config.improvement.tsiters);
    read_int_path(text, {"improvement", "tabu"}, config.improvement.tabu);
    read_string_path(text, {"improvement", "tsmove"}, config.improvement.tsmove);

    read_bool_path(text, {"outputs", "write_result_json"}, config.outputs.write_result_json);
    read_bool_path(text, {"outputs", "write_schedule_csv"}, config.outputs.write_schedule_csv);
    read_bool_path(text, {"outputs", "write_metadata_json"}, config.outputs.write_metadata_json);
    read_bool_path(text, {"outputs", "write_convergence_csv"}, config.outputs.write_convergence_csv);
    read_bool_path(text, {"outputs", "write_gantt_html"}, config.outputs.write_gantt_html);

    if (config.schema_version != "1.0") {
        set_error(error_message, "Unsupported config schema_version: " + config.schema_version);
        return false;
    }
    if (config.instance.source != "orlib") {
        set_error(error_message, "Unsupported instance.source: " + config.instance.source);
        return false;
    }
    if (config.instance.name.empty()) {
        set_error(error_message, "Config instance.name must not be empty.");
        return false;
    }
    if (config.instance.path.empty()) {
        set_error(error_message, "Config instance.path must not be empty.");
        return false;
    }
    if (config.objective != "cmax") {
        set_error(error_message, "Unsupported objective: " + config.objective);
        return false;
    }
    if (config.solver.method != "pso_hh") {
        set_error(error_message, "Unsupported solver.method: " + config.solver.method);
        return false;
    }
    if (config.solver.sgs != "gt" && config.solver.sgs != "event") {
        set_error(error_message, "Unsupported solver.sgs: " + config.solver.sgs);
        return false;
    }
    if (config.solver.iters < 1 || config.solver.swarm < 1 ||
        config.solver.evalk < 1 || config.solver.finalk < 1) {
        set_error(error_message, "Config solver iteration/swarm counts must be positive.");
        return false;
    }
    if (config.rules_specified && config.rules.empty()) {
        set_error(error_message, "Config rules must contain at least one rule when provided.");
        return false;
    }
    if (!config.rules.empty()) {
        for (const auto& rule_name : config.rules) {
            if (rule_name.empty()) {
                set_error(error_message, "Config rules must not contain empty rule names.");
                return false;
            }
        }
    }
    if (config.features_specified && config.features.empty()) {
        set_error(error_message, "Config features must contain at least one feature when provided.");
        return false;
    }
    if (!config.features.empty()) {
        for (const auto& feature_name : config.features) {
            if (feature_name.empty()) {
                set_error(error_message, "Config features must not contain empty feature names.");
                return false;
            }
        }
    }
    if (config.improvement.slsiters < 0 || config.improvement.tsiters < 0 ||
        config.improvement.tabu < 0) {
        set_error(error_message, "Config improvement counters must not be negative.");
        return false;
    }
    if (!config.improvement.tsmove.empty() &&
        config.improvement.tsmove != "swap" &&
        config.improvement.tsmove != "insert" &&
        config.improvement.tsmove != "mixed") {
        set_error(error_message, "Unsupported improvement.tsmove: " + config.improvement.tsmove);
        return false;
    }

    out_config = std::move(config);
    return true;
}

std::string ConfigLoader::to_resolved_json(const XSimConfig& config) {
    std::ostringstream out;
    out << "{\n"
        << "  \"schema_version\": " << json_string(config.schema_version) << ",\n"
        << "  \"run\": {\n"
        << "    \"name\": " << json_string(config.run.name) << ",\n"
        << "    \"seed\": " << static_cast<unsigned long long>(config.run.seed) << ",\n"
        << "    \"output_root\": " << json_string(config.run.output_root) << ",\n"
        << "    \"write_reports\": " << json_bool(config.run.write_reports) << "\n"
        << "  },\n"
        << "  \"instance\": {\n"
        << "    \"source\": " << json_string(config.instance.source) << ",\n"
        << "    \"name\": " << json_string(config.instance.name) << ",\n"
        << "    \"path\": " << json_string(config.instance.path) << "\n"
        << "  },\n"
        << "  \"objective\": " << json_string(config.objective) << ",\n"
        << "  \"solver\": {\n"
        << "    \"method\": " << json_string(config.solver.method) << ",\n"
        << "    \"sgs\": " << json_string(config.solver.sgs) << ",\n"
        << "    \"iters\": " << config.solver.iters << ",\n"
        << "    \"swarm\": " << config.solver.swarm << ",\n"
        << "    \"evalk\": " << config.solver.evalk << ",\n"
        << "    \"finalk\": " << config.solver.finalk << ",\n"
        << "    \"eps0\": " << json_number(config.solver.eps0) << ",\n"
        << "    \"epsmin\": " << json_number(config.solver.epsmin) << ",\n"
        << "    \"fitavg\": " << json_bool(config.solver.fitavg) << ",\n"
        << "    \"traindet\": " << json_bool(config.solver.traindet) << "\n"
        << "  },\n"
        << "  \"rules\": ";
    write_string_array(out, config.rules);
    out << ",\n"
        << "  \"features\": ";
    write_string_array(out, config.features);
    out << ",\n"
        << "  \"improvement\": {\n"
        << "    \"enabled\": " << json_bool(config.improvement.enabled) << ",\n"
        << "    \"type\": " << json_optional_string(config.improvement.type) << ",\n"
        << "    \"slsiters\": " << config.improvement.slsiters << ",\n"
        << "    \"tsiters\": " << config.improvement.tsiters << ",\n"
        << "    \"tabu\": " << config.improvement.tabu << ",\n"
        << "    \"tsmove\": " << json_optional_string(config.improvement.tsmove) << "\n"
        << "  },\n"
        << "  \"outputs\": {\n"
        << "    \"write_result_json\": " << json_bool(config.outputs.write_result_json) << ",\n"
        << "    \"write_schedule_csv\": " << json_bool(config.outputs.write_schedule_csv) << ",\n"
        << "    \"write_metadata_json\": " << json_bool(config.outputs.write_metadata_json) << ",\n"
        << "    \"write_convergence_csv\": " << json_bool(config.outputs.write_convergence_csv) << ",\n"
        << "    \"write_gantt_html\": " << json_bool(config.outputs.write_gantt_html) << "\n"
        << "  }\n"
        << "}\n";
    return out.str();
}

}  // namespace djssp
