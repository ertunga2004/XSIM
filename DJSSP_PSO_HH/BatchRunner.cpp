#include "BatchRunner.h"

#include <chrono>
#include <ctime>
#include <iomanip>

namespace djssp {

namespace {

void set_error(std::string* error_message, const std::string& message) {
    if (error_message != nullptr) {
        *error_message = message;
    }
}

std::string trim_copy(const std::string& value) {
    size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) {
        ++begin;
    }
    size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(begin, end - begin);
}

std::string compact_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t raw = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &raw);
#else
    localtime_r(&raw, &local);
#endif

    std::ostringstream out;
    out << std::put_time(&local, "%Y%m%d_%H%M%S");
    return out.str();
}

std::string iso_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t raw = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &raw);
#else
    localtime_r(&raw, &local);
#endif

    std::ostringstream out;
    out << std::put_time(&local, "%Y-%m-%dT%H:%M:%S");
    return out.str();
}

std::string sanitize_for_id(const std::string& value) {
    std::string safe;
    safe.reserve(value.size());
    for (char ch : value) {
        const unsigned char uch = static_cast<unsigned char>(ch);
        if (std::isalnum(uch) || ch == '_' || ch == '-') {
            safe.push_back(ch);
        } else {
            safe.push_back('_');
        }
    }
    return safe.empty() ? "batch" : safe;
}

std::string quote_arg(const std::string& value) {
    std::string out = "\"";
    for (char ch : value) {
        if (ch == '"') {
            out += "\\\"";
        } else {
            out.push_back(ch);
        }
    }
    out += "\"";
    return out;
}

std::string csv_escape(const std::string& value) {
    const bool needs_quotes = value.find_first_of(",\"\r\n") != std::string::npos;
    if (!needs_quotes) {
        return value;
    }

    std::string out = "\"";
    for (char ch : value) {
        if (ch == '"') {
            out += "\"\"";
        } else {
            out.push_back(ch);
        }
    }
    out += "\"";
    return out;
}

std::string json_escape(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (char ch : value) {
        switch (ch) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(ch); break;
        }
    }
    return out;
}

std::string json_string(const std::string& value) {
    return "\"" + json_escape(value) + "\"";
}

bool read_text_file(const std::filesystem::path& path, std::string& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    out = buffer.str();
    return true;
}

void skip_ws(const std::string& text, size_t& pos) {
    while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
        ++pos;
    }
}

bool parse_json_string_at(const std::string& text, size_t& pos, std::string& out) {
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

bool find_json_value(const std::string& text, const std::string& key, size_t& value_pos) {
    const std::string needle = "\"" + key + "\"";
    const size_t key_pos = text.find(needle);
    if (key_pos == std::string::npos) {
        return false;
    }
    const size_t colon = text.find(':', key_pos + needle.size());
    if (colon == std::string::npos) {
        return false;
    }
    value_pos = colon + 1;
    skip_ws(text, value_pos);
    return value_pos < text.size();
}

bool extract_json_string(const std::string& text, const std::string& key, std::string& out) {
    size_t pos = 0;
    if (!find_json_value(text, key, pos)) {
        return false;
    }
    return parse_json_string_at(text, pos, out);
}

bool extract_json_number(const std::string& text, const std::string& key, std::string& out) {
    size_t pos = 0;
    if (!find_json_value(text, key, pos)) {
        return false;
    }
    const size_t begin = pos;
    while (pos < text.size() && text[pos] != ',' && text[pos] != '}' &&
           text[pos] != '\r' && text[pos] != '\n') {
        ++pos;
    }
    out = trim_copy(text.substr(begin, pos - begin));
    return !out.empty() && out != "null";
}

bool extract_json_bool(const std::string& text, const std::string& key, std::string& out) {
    size_t pos = 0;
    if (!find_json_value(text, key, pos)) {
        return false;
    }
    if (text.compare(pos, 4, "true") == 0) {
        out = "true";
        return true;
    }
    if (text.compare(pos, 5, "false") == 0) {
        out = "false";
        return true;
    }
    return false;
}

std::string find_run_output_dir(const std::string& log_text) {
    const std::string marker = "Run outputs written to ";
    const size_t pos = log_text.rfind(marker);
    if (pos == std::string::npos) {
        return "";
    }
    const size_t begin = pos + marker.size();
    size_t end = log_text.find_first_of("\r\n", begin);
    if (end == std::string::npos) {
        end = log_text.size();
    }
    return trim_copy(log_text.substr(begin, end - begin));
}

std::filesystem::path resolve_config_path(
    const std::filesystem::path& batch_config_path,
    const std::string& run_config_path
) {
    std::filesystem::path path(run_config_path);
    if (path.is_absolute() || std::filesystem::exists(path)) {
        return path;
    }

    const auto parent = batch_config_path.parent_path();
    if (!parent.empty()) {
        const auto relative_to_batch = parent / path;
        if (std::filesystem::exists(relative_to_batch)) {
            return relative_to_batch;
        }
    }

    return path;
}

BatchRunSummaryRow failed_row(
    const BatchConfig& config,
    const std::string& batch_id,
    const std::string& config_path
) {
    BatchRunSummaryRow row;
    row.batch_id = batch_id;
    row.suite_name = config.suite_name;
    row.config_path = config_path;
    row.status = "failed";
    row.feasibility_valid = "false";
    return row;
}

void write_summary_csv(const std::filesystem::path& path, const std::vector<BatchRunSummaryRow>& rows) {
    std::ofstream out(path);
    out << "batch_id,suite_name,run_id,config_path,instance,status,cmax,runtime_sec,feasibility_valid\n";
    for (const auto& row : rows) {
        out << csv_escape(row.batch_id) << ","
            << csv_escape(row.suite_name) << ","
            << csv_escape(row.run_id) << ","
            << csv_escape(row.config_path) << ","
            << csv_escape(row.instance) << ","
            << csv_escape(row.status) << ","
            << csv_escape(row.cmax) << ","
            << csv_escape(row.runtime_sec) << ","
            << csv_escape(row.feasibility_valid) << "\n";
    }
}

void write_batch_metadata(
    const std::filesystem::path& path,
    const BatchConfig& config,
    const std::string& batch_id,
    const std::string& started_at,
    const std::string& finished_at
) {
    std::ofstream out(path);
    out << "{\n"
        << "  \"schema_version\": \"1.0\",\n"
        << "  \"batch_id\": " << json_string(batch_id) << ",\n"
        << "  \"suite_name\": " << json_string(config.suite_name) << ",\n"
        << "  \"run_count\": " << config.runs.size() << ",\n"
        << "  \"started_at\": " << json_string(started_at) << ",\n"
        << "  \"finished_at\": " << json_string(finished_at) << ",\n"
        << "  \"config_path\": " << json_string(config.source_path.generic_string()) << "\n"
        << "}\n";
}

}  // namespace

bool BatchRunner::run(
    const std::filesystem::path& executable_path,
    const BatchConfig& config,
    BatchRunResult& out_result,
    std::string* error_message
) {
    const std::string started_at = iso_timestamp();
    std::string batch_id = compact_timestamp() + "_" + sanitize_for_id(config.suite_name) + "_batch";

    std::filesystem::path output_dir = std::filesystem::path("runs") / "batches" / batch_id;
    std::error_code ec;
    if (std::filesystem::exists(output_dir, ec)) {
        for (int suffix = 2; suffix < 1000; ++suffix) {
            batch_id = compact_timestamp() + "_" + sanitize_for_id(config.suite_name) +
                "_batch_" + std::to_string(suffix);
            output_dir = std::filesystem::path("runs") / "batches" / batch_id;
            if (!std::filesystem::exists(output_dir, ec)) {
                break;
            }
        }
    }

    std::filesystem::create_directories(output_dir, ec);
    if (ec) {
        set_error(error_message, "Could not create batch output directory: " +
            output_dir.string() + " (" + ec.message() + ")");
        return false;
    }

    std::vector<BatchRunSummaryRow> rows;
    rows.reserve(config.runs.size());

    for (size_t i = 0; i < config.runs.size(); ++i) {
        const std::filesystem::path run_config_path =
            resolve_config_path(config.source_path, config.runs[i]);
        const std::string config_path_text = run_config_path.generic_string();
        const std::filesystem::path log_path =
            output_dir / ("run_" + std::to_string(i + 1) + ".log");

        if (!std::filesystem::exists(run_config_path)) {
            BatchRunSummaryRow row = failed_row(config, batch_id, config_path_text);
            rows.push_back(std::move(row));
            std::ofstream log(log_path);
            log << "Missing run config path: " << config_path_text << "\n";
            continue;
        }

        const std::string command = executable_path.generic_string() +
            " --config " + quote_arg(run_config_path.generic_string()) +
            " > " + quote_arg(log_path.generic_string()) + " 2>&1";
        const int command_status = std::system(command.c_str());

        std::string log_text;
        read_text_file(log_path, log_text);

        BatchRunSummaryRow row;
        row.batch_id = batch_id;
        row.suite_name = config.suite_name;
        row.config_path = config_path_text;
        row.status = (command_status == 0) ? "success" : "failed";
        row.feasibility_valid = "false";

        const std::string run_output_dir = find_run_output_dir(log_text);
        if (!run_output_dir.empty()) {
            const std::filesystem::path result_path =
                std::filesystem::path(run_output_dir) / "result.json";
            std::string result_text;
            if (read_text_file(result_path, result_text)) {
                extract_json_string(result_text, "run_id", row.run_id);
                extract_json_string(result_text, "instance", row.instance);
                extract_json_string(result_text, "status", row.status);
                extract_json_number(result_text, "cmax", row.cmax);
                extract_json_number(result_text, "runtime_sec", row.runtime_sec);
                extract_json_bool(result_text, "valid", row.feasibility_valid);
            }
        }

        rows.push_back(std::move(row));
    }

    write_summary_csv(output_dir / "batch_summary.csv", rows);
    write_batch_metadata(output_dir / "batch_metadata.json", config, batch_id, started_at, iso_timestamp());

    out_result.batch_id = batch_id;
    out_result.output_dir = output_dir;
    out_result.rows = std::move(rows);
    return true;
}

}  // namespace djssp
