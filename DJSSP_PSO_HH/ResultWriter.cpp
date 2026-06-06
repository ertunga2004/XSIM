#include "ResultWriter.h"

#include <ctime>
#include <iomanip>

namespace djssp {

namespace {

std::string json_escape(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 8);
    for (char ch : value) {
        switch (ch) {
            case '\\': escaped += "\\\\"; break;
            case '"': escaped += "\\\""; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20) {
                    std::ostringstream hex;
                    hex << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<int>(static_cast<unsigned char>(ch));
                    escaped += hex.str();
                } else {
                    escaped += ch;
                }
                break;
        }
    }
    return escaped;
}

std::string json_string(const std::string& value) {
    return "\"" + json_escape(value) + "\"";
}

std::string json_optional_string(const std::string& value) {
    if (value.empty()) {
        return "null";
    }
    return json_string(value);
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
    if (!std::isfinite(value)) {
        return "null";
    }

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

std::string sanitize_instance_for_run_id(const std::string& instance) {
    std::string safe;
    safe.reserve(instance.size());
    for (char ch : instance) {
        const unsigned char uch = static_cast<unsigned char>(ch);
        if (std::isalnum(uch) || ch == '_' || ch == '-') {
            safe.push_back(ch);
        } else {
            safe.push_back('_');
        }
    }
    return safe.empty() ? "instance" : safe;
}

std::string offset_string(int offset_seconds) {
    char sign = '+';
    if (offset_seconds < 0) {
        sign = '-';
        offset_seconds = -offset_seconds;
    }
    const int hours = offset_seconds / 3600;
    const int minutes = (offset_seconds % 3600) / 60;

    std::ostringstream out;
    out << sign << std::setw(2) << std::setfill('0') << hours
        << ":" << std::setw(2) << std::setfill('0') << minutes;
    return out.str();
}

void write_solver_json(std::ostream& out, const SolverRunConfig& solver, const std::string& indent) {
    out << indent << "\"sgs\": " << json_string(solver.sgs) << ",\n"
        << indent << "\"iters\": " << solver.iters << ",\n"
        << indent << "\"swarm\": " << solver.swarm << ",\n"
        << indent << "\"evalk\": " << solver.evalk << ",\n"
        << indent << "\"finalk\": " << solver.finalk << ",\n"
        << indent << "\"eps0\": " << json_number(solver.eps0) << ",\n"
        << indent << "\"epsmin\": " << json_number(solver.epsmin) << ",\n"
        << indent << "\"fitavg\": " << json_bool(solver.fitavg) << ",\n"
        << indent << "\"traindet\": " << json_bool(solver.traindet) << "\n";
}

void write_improvement_json(std::ostream& out, const ImprovementRunConfig& improvement, const std::string& indent) {
    out << indent << "\"enabled\": " << json_bool(improvement.enabled) << ",\n"
        << indent << "\"type\": " << json_optional_string(improvement.type) << ",\n"
        << indent << "\"slsiters\": " << improvement.slsiters << ",\n"
        << indent << "\"tsiters\": " << improvement.tsiters << ",\n"
        << indent << "\"tabu\": " << improvement.tabu << ",\n"
        << indent << "\"tsmove\": " << json_optional_string(improvement.tsmove) << "\n";
}

bool write_schedule_csv(const fs::path& output_path, const ScheduleResult& schedule, std::string* error_message) {
    std::ofstream out(output_path);
    if (!out) {
        if (error_message != nullptr) {
            *error_message = "Could not open schedule CSV for writing: " + output_path.string();
        }
        return false;
    }

    out << "run_id,instance,job_id,operation_id,machine_id,start,end,processing_time,sequence_index\n";
    for (const auto& row : schedule.operations) {
        out << row.run_id << ","
            << row.instance << ","
            << row.job_id << ","
            << row.operation_id << ","
            << row.machine_id << ","
            << json_number(row.start) << ","
            << json_number(row.end) << ","
            << json_number(row.processing_time) << ","
            << row.sequence_index << "\n";
    }
    return true;
}

void write_config_snapshot(std::ostream& out, const RunContext& context) {
    out << "{\n"
        << "  \"schema_version\": " << json_string(context.schema_version) << ",\n"
        << "  \"source\": " << json_string(context.config_source) << ",\n"
        << "  \"instance\": " << json_string(context.instance) << ",\n"
        << "  \"instance_resolved\": {\n"
        << "    \"source\": " << json_string("orlib") << ",\n"
        << "    \"name\": " << json_string(context.instance) << ",\n"
        << "    \"path\": " << json_string(context.instance_source) << "\n"
        << "  },\n"
        << "  \"seed\": " << static_cast<unsigned long long>(context.seed) << ",\n"
        << "  \"rules\": ";
    write_string_array(out, context.active_rules);
    out << ",\n"
        << "  \"features\": ";
    write_string_array(out, context.active_features);
    out << ",\n"
        << "  \"active_rules\": ";
    write_string_array(out, context.active_rules);
    out << ",\n"
        << "  \"active_features\": ";
    write_string_array(out, context.active_features);
    out << ",\n"
        << "  \"solver\": {\n";
    write_solver_json(out, context.solver, "    ");
    out << "  },\n"
        << "  \"improvement\": {\n";
    write_improvement_json(out, context.improvement, "    ");
    out << "  }\n"
        << "}\n";
}

bool write_config_file(const fs::path& output_path, const RunContext& context, std::string* error_message) {
    std::ofstream out(output_path);
    if (!out) {
        if (error_message != nullptr) {
            *error_message = "Could not open config snapshot for writing: " + output_path.string();
        }
        return false;
    }
    write_config_snapshot(out, context);
    return true;
}

bool write_text_file(const fs::path& output_path, const std::string& text, std::string* error_message) {
    std::ofstream out(output_path);
    if (!out) {
        if (error_message != nullptr) {
            *error_message = "Could not open file for writing: " + output_path.string();
        }
        return false;
    }
    out << text;
    return true;
}

bool write_metadata_json(const fs::path& output_path, const RunContext& context, std::string* error_message) {
    std::ofstream out(output_path);
    if (!out) {
        if (error_message != nullptr) {
            *error_message = "Could not open metadata JSON for writing: " + output_path.string();
        }
        return false;
    }

    out << "{\n"
        << "  \"schema_version\": " << json_string(context.schema_version) << ",\n"
        << "  \"run_id\": " << json_string(context.run_id) << ",\n"
        << "  \"timestamp_local\": " << json_string(context.timestamp_local) << ",\n"
        << "  \"xsim_version\": " << json_string(context.xsim_version) << ",\n"
        << "  \"git_commit\": " << json_string(context.git_commit) << ",\n"
        << "  \"build\": {\n"
        << "    \"compiler\": " << json_string(context.compiler) << ",\n"
        << "    \"cpp_standard\": " << json_string(context.cpp_standard) << ",\n"
        << "    \"build_type\": " << json_string(context.build_type) << "\n"
        << "  },\n"
        << "  \"machine\": {\n"
        << "    \"os\": " << json_string(context.os) << ",\n"
        << "    \"cpu\": " << json_string(context.cpu) << ",\n"
        << "    \"threads\": " << context.threads << "\n"
        << "  },\n"
        << "  \"inputs\": {\n"
        << "    \"instance_source\": " << json_string(context.instance_source) << ",\n"
        << "    \"config_source\": " << json_string(context.config_source) << "\n"
        << "  },\n"
        << "  \"command\": " << json_string(context.command) << "\n"
        << "}\n";
    return true;
}

bool write_result_json(
    const fs::path& output_path,
    const RunContext& context,
    const FeasibilityResult& feasibility,
    double objective_value,
    std::string* error_message
) {
    std::ofstream out(output_path);
    if (!out) {
        if (error_message != nullptr) {
            *error_message = "Could not open result JSON for writing: " + output_path.string();
        }
        return false;
    }

    out << "{\n"
        << "  \"schema_version\": " << json_string(context.schema_version) << ",\n"
        << "  \"run_id\": " << json_string(context.run_id) << ",\n"
        << "  \"instance\": " << json_string(context.instance) << ",\n"
        << "  \"method\": " << json_string(context.method) << ",\n"
        << "  \"status\": " << json_string(context.status) << ",\n"
        << "  \"objective\": " << json_string(context.objective) << ",\n"
        << "  \"objective_value\": " << json_number(objective_value) << ",\n"
        << "  \"metrics\": {\n"
        << "    \"cmax\": " << json_number(objective_value) << "\n"
        << "  },\n"
        << "  \"seed\": " << static_cast<unsigned long long>(context.seed) << ",\n"
        << "  \"active_rules\": ";
    write_string_array(out, context.active_rules);
    out << ",\n"
        << "  \"active_features\": ";
    write_string_array(out, context.active_features);
    out << ",\n"
        << "  \"config_source\": " << json_string(context.config_source) << ",\n"
        << "  \"inputs\": {\n"
        << "    \"instance_source\": " << json_string(context.instance_source) << ",\n"
        << "    \"config_source\": " << json_string(context.config_source) << "\n"
        << "  },\n"
        << "  \"solver\": {\n";
    write_solver_json(out, context.solver, "    ");
    out << "  },\n"
        << "  \"improvement\": {\n";
    write_improvement_json(out, context.improvement, "    ");
    out << "  },\n"
        << "  \"feasibility\": {\n"
        << "    \"valid\": " << json_bool(feasibility.valid) << ",\n"
        << "    \"operation_count_ok\": " << json_bool(feasibility.operation_count_ok) << ",\n"
        << "    \"duplicate_operation_ok\": " << json_bool(feasibility.duplicate_operation_ok) << ",\n"
        << "    \"precedence_ok\": " << json_bool(feasibility.precedence_ok) << ",\n"
        << "    \"machine_capacity_ok\": " << json_bool(feasibility.machine_capacity_ok) << ",\n"
        << "    \"start_end_valid\": " << json_bool(feasibility.start_end_valid) << ",\n"
        << "    \"cmax_consistent\": " << json_bool(feasibility.cmax_consistent) << ",\n"
        << "    \"expected_operation_count\": " << feasibility.expected_operation_count << ",\n"
        << "    \"actual_operation_count\": " << feasibility.actual_operation_count << ",\n"
        << "    \"schedule_cmax\": " << json_number(feasibility.schedule_cmax) << ",\n"
        << "    \"violations\": [";

    for (size_t i = 0; i < feasibility.violations.size(); ++i) {
        if (i > 0) {
            out << ", ";
        }
        out << json_string(feasibility.violations[i]);
    }

    out << "]\n"
        << "  },\n"
        << "  \"runtime_sec\": " << json_number(context.runtime_sec) << ",\n"
        << "  \"outputs\": {\n"
        << "    \"schedule_csv\": " << json_string("schedule.csv") << ",\n"
        << "    \"metadata_json\": " << json_string("metadata.json") << ",\n"
        << "    \"convergence_csv\": " << json_string("convergence.csv") << ",\n"
        << "    \"gantt_html\": " << json_string("gantt.html") << ",\n"
        << "    \"config_original_json\": " << json_string("config.original.json") << ",\n"
        << "    \"config_resolved_json\": " << json_string("config.resolved.json") << "\n"
        << "  },\n"
        << "  \"legacy_stdout\": {\n"
        << "    \"best_cmax_line\": " << json_string(context.best_cmax_line) << "\n"
        << "  }\n"
        << "}\n";
    return true;
}

}  // namespace

LocalTimestamp make_local_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t raw = std::chrono::system_clock::to_time_t(now);

    std::tm local{};
    std::tm utc{};
#ifdef _WIN32
    localtime_s(&local, &raw);
    gmtime_s(&utc, &raw);
#else
    localtime_r(&raw, &local);
    gmtime_r(&raw, &utc);
#endif

    std::tm local_copy = local;
    std::tm utc_copy = utc;
    const std::time_t local_epoch = std::mktime(&local_copy);
    const std::time_t utc_epoch_as_local = std::mktime(&utc_copy);
    const int offset_seconds = static_cast<int>(std::difftime(local_epoch, utc_epoch_as_local));

    std::ostringstream compact;
    compact << std::put_time(&local, "%Y%m%d_%H%M%S");

    std::ostringstream iso;
    iso << std::put_time(&local, "%Y-%m-%dT%H:%M:%S") << offset_string(offset_seconds);

    return LocalTimestamp{compact.str(), iso.str()};
}

std::string build_command_line(int argc, char** argv) {
    std::ostringstream out;
    for (int i = 0; i < argc; ++i) {
        if (i > 0) {
            out << ' ';
        }

        const std::string arg = argv[i] == nullptr ? "" : argv[i];
        const bool needs_quotes = arg.find_first_of(" \t\"") != std::string::npos;
        if (!needs_quotes) {
            out << arg;
            continue;
        }

        out << '"';
        for (char ch : arg) {
            if (ch == '"') {
                out << "\\\"";
            } else {
                out << ch;
            }
        }
        out << '"';
    }
    return out.str();
}

std::string build_config_fingerprint(
    const std::string& instance,
    uint64_t seed,
    const SolverRunConfig& solver,
    const ImprovementRunConfig& improvement,
    const std::vector<std::string>& active_rules,
    const std::vector<std::string>& active_features
) {
    std::ostringstream data;
    data << "instance=" << instance
        << ";seed=" << static_cast<unsigned long long>(seed)
        << ";sgs=" << solver.sgs
        << ";iters=" << solver.iters
        << ";swarm=" << solver.swarm
        << ";evalk=" << solver.evalk
        << ";finalk=" << solver.finalk
        << ";eps0=" << json_number(solver.eps0)
        << ";epsmin=" << json_number(solver.epsmin)
        << ";fitavg=" << solver.fitavg
        << ";traindet=" << solver.traindet
        << ";improvement=" << improvement.enabled
        << ";type=" << improvement.type
        << ";slsiters=" << improvement.slsiters
        << ";tsiters=" << improvement.tsiters
        << ";tabu=" << improvement.tabu
        << ";tsmove=" << improvement.tsmove;
    for (const auto& rule_name : active_rules) {
        data << ";rule=" << rule_name;
    }
    for (const auto& feature_name : active_features) {
        data << ";feature=" << feature_name;
    }

    uint64_t hash = 1469598103934665603ULL;
    const std::string text = data.str();
    for (unsigned char ch : text) {
        hash ^= static_cast<uint64_t>(ch);
        hash *= 1099511628211ULL;
    }

    std::ostringstream out;
    out << std::uppercase << std::hex << std::setw(8) << std::setfill('0')
        << static_cast<unsigned int>(hash & 0xffffffffULL);
    return out.str();
}

std::string make_run_id(
    const LocalTimestamp& timestamp,
    const std::string& instance,
    uint64_t seed,
    const std::string& short_hash
) {
    return timestamp.compact + "_" + sanitize_instance_for_run_id(instance) +
        "_seed" + std::to_string(static_cast<unsigned long long>(seed)) +
        "_cfg" + short_hash;
}

std::string detect_os_name() {
#ifdef _WIN32
    return "Windows";
#elif defined(__APPLE__)
    return "macOS";
#elif defined(__linux__)
    return "Linux";
#else
    return "unknown";
#endif
}

ScheduleResult build_schedule_result(
    const std::string& run_id,
    const std::string& instance,
    const std::vector<GanttLogger::Entry>& rows
) {
    std::vector<GanttLogger::Entry> sorted = rows;
    std::sort(sorted.begin(), sorted.end(), [](const GanttLogger::Entry& a, const GanttLogger::Entry& b) {
        if (a.machine_id != b.machine_id) return a.machine_id < b.machine_id;
        if (a.start != b.start) return a.start < b.start;
        if (a.end != b.end) return a.end < b.end;
        if (a.job_id != b.job_id) return a.job_id < b.job_id;
        return a.op_index < b.op_index;
    });

    ScheduleResult result;
    result.operations.reserve(sorted.size());

    int current_machine = std::numeric_limits<int>::min();
    int sequence_index = 0;
    for (const auto& row : sorted) {
        if (row.machine_id != current_machine) {
            current_machine = row.machine_id;
            sequence_index = 0;
        }

        result.cmax = std::max(result.cmax, row.end);
        result.operations.push_back(ScheduleOperation{
            run_id,
            instance,
            row.job_id,
            row.op_index,
            row.machine_id,
            row.start,
            row.end,
            row.proc_time,
            sequence_index++
        });
    }

    return result;
}

bool write_run_contract_outputs(
    const RunContext& context,
    const ScheduleResult& schedule,
    const FeasibilityResult& feasibility,
    double objective_value,
    std::string* error_message
) {
    std::error_code ec;
    fs::create_directories(context.output_dir, ec);
    if (ec) {
        if (error_message != nullptr) {
            *error_message = "Could not create run output directory: " + context.output_dir.string() +
                " (" + ec.message() + ")";
        }
        return false;
    }

    if (!write_schedule_csv(context.output_dir / "schedule.csv", schedule, error_message)) {
        return false;
    }
    if (!write_metadata_json(context.output_dir / "metadata.json", context, error_message)) {
        return false;
    }
    if (!context.config_original_json.empty()) {
        if (!write_text_file(context.output_dir / "config.original.json", context.config_original_json, error_message)) {
            return false;
        }
    } else if (!write_config_file(context.output_dir / "config.original.json", context, error_message)) {
        return false;
    }
    if (!context.config_resolved_json.empty()) {
        if (!write_text_file(context.output_dir / "config.resolved.json", context.config_resolved_json, error_message)) {
            return false;
        }
    } else if (!write_config_file(context.output_dir / "config.resolved.json", context, error_message)) {
        return false;
    }
    if (!write_result_json(context.output_dir / "result.json", context, feasibility, objective_value, error_message)) {
        return false;
    }

    return true;
}

}  // namespace djssp
