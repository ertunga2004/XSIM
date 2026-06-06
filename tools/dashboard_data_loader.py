#!/usr/bin/env python3
"""Load XSIM run and batch outputs for the static results dashboard."""

from __future__ import annotations

import csv
import json
import os
from datetime import datetime
from pathlib import Path
from typing import Any


EXPECTED_RUN_FILES = (
    "result.json",
    "metadata.json",
    "config.resolved.json",
    "config.original.json",
    "schedule.csv",
    "convergence.csv",
    "gantt.csv",
    "gantt.html",
)

MAX_CONVERGENCE_POINTS = 2000
MAX_SCHEDULE_PREVIEW_ROWS = 80


def _issue(path: Path, message: str, severity: str = "warning") -> dict[str, str]:
    return {"severity": severity, "path": str(path), "message": message}


def _read_json(path: Path) -> tuple[dict[str, Any] | None, list[dict[str, str]]]:
    if not path.exists():
        return None, [_issue(path, "File is missing.")]
    try:
        loaded = json.loads(path.read_text(encoding="utf-8-sig"))
    except OSError as exc:
        return None, [_issue(path, f"Could not read JSON: {exc}", "error")]
    except json.JSONDecodeError as exc:
        return None, [_issue(path, f"Invalid JSON at line {exc.lineno}: {exc.msg}", "error")]
    if not isinstance(loaded, dict):
        return None, [_issue(path, "JSON root is not an object.", "error")]
    return loaded, []


def _read_csv(path: Path) -> tuple[list[dict[str, str]], list[str], list[dict[str, str]]]:
    if not path.exists():
        return [], [], [_issue(path, "File is missing.")]
    try:
        with path.open(newline="", encoding="utf-8-sig") as handle:
            reader = csv.DictReader(handle)
            if reader.fieldnames is None:
                return [], [], [_issue(path, "CSV header is missing.", "error")]
            rows = [dict(row) for row in reader]
            return rows, list(reader.fieldnames), []
    except csv.Error as exc:
        return [], [], [_issue(path, f"Invalid CSV: {exc}", "error")]
    except OSError as exc:
        return [], [], [_issue(path, f"Could not read CSV: {exc}", "error")]


def _as_list(value: Any) -> list[str]:
    if isinstance(value, list):
        return [str(item) for item in value if item is not None and str(item) != ""]
    if value is None or value == "":
        return []
    return [str(value)]


def _as_float(value: Any) -> float | None:
    if value is None or value == "":
        return None
    try:
        return float(value)
    except (TypeError, ValueError):
        return None


def _as_bool(value: Any) -> bool | None:
    if isinstance(value, bool):
        return value
    if value is None or value == "":
        return None
    normalized = str(value).strip().lower()
    if normalized in {"true", "1", "yes", "y"}:
        return True
    if normalized in {"false", "0", "no", "n"}:
        return False
    return None


def _nested(data: dict[str, Any] | None, *keys: str) -> Any:
    current: Any = data
    for key in keys:
        if not isinstance(current, dict) or key not in current:
            return None
        current = current[key]
    return current


def _coalesce(*values: Any) -> Any:
    for value in values:
        if value is not None and value != "":
            return value
    return None


def _format_float(value: float | None) -> float | None:
    if value is None:
        return None
    return round(value, 6)


def _rel_url(path: Path, output_dir: Path | None) -> str:
    source = path.resolve()
    if output_dir is None:
        rel = source
    else:
        rel = Path(os.path.relpath(source, output_dir.resolve()))
    return rel.as_posix()


def _file_links(run_dir: Path, output_dir: Path | None) -> dict[str, dict[str, Any]]:
    files: dict[str, dict[str, Any]] = {}
    for name in EXPECTED_RUN_FILES:
        path = run_dir / name
        files[name] = {
            "exists": path.exists(),
            "path": str(path),
            "href": _rel_url(path, output_dir) if path.exists() else None,
        }
    return files


def _numeric_row(row: dict[str, str]) -> dict[str, Any]:
    converted: dict[str, Any] = {}
    for key, value in row.items():
        number = _as_float(value)
        converted[key] = number if number is not None else value
    return converted


def _downsample(rows: list[dict[str, Any]], max_rows: int) -> tuple[list[dict[str, Any]], bool]:
    if max_rows <= 0 or len(rows) <= max_rows:
        return rows, False
    if max_rows == 1:
        return [rows[-1]], True
    last_index = len(rows) - 1
    indexes = sorted({round(index * last_index / (max_rows - 1)) for index in range(max_rows)})
    return [rows[index] for index in indexes], True


def _parse_convergence(path: Path) -> tuple[dict[str, Any], list[dict[str, str]]]:
    if not path.exists():
        return {"available": False, "columns": [], "row_count": 0, "points": [], "truncated": False}, []
    rows, columns, issues = _read_csv(path)
    if issues:
        return {"available": False, "columns": columns, "row_count": 0, "points": [], "truncated": False}, issues
    points = [_numeric_row(row) for row in rows]
    points, truncated = _downsample(points, MAX_CONVERGENCE_POINTS)
    return {
        "available": True,
        "columns": columns,
        "row_count": len(rows),
        "points": points,
        "truncated": truncated,
    }, []


def _parse_schedule(path: Path) -> tuple[dict[str, Any], list[dict[str, str]]]:
    if not path.exists():
        return {
            "available": False,
            "columns": [],
            "row_count": 0,
            "preview": [],
            "machines": [],
            "cmax": None,
            "truncated": False,
        }, []
    rows, columns, issues = _read_csv(path)
    if issues:
        return {
            "available": False,
            "columns": columns,
            "row_count": 0,
            "preview": [],
            "machines": [],
            "cmax": None,
            "truncated": False,
        }, issues

    machines = sorted(
        {
            int(value)
            for row in rows
            for value in [row.get("machine_id")]
            if value is not None and str(value).strip().lstrip("-").isdigit()
        }
    )
    cmax_values = [_as_float(row.get("end")) for row in rows]
    cmax = max((value for value in cmax_values if value is not None), default=None)
    preview = rows[:MAX_SCHEDULE_PREVIEW_ROWS]
    return {
        "available": True,
        "columns": columns,
        "row_count": len(rows),
        "preview": preview,
        "machines": machines,
        "cmax": _format_float(cmax),
        "truncated": len(rows) > len(preview),
    }, []


def _run_summary_from_files(
    run_dir: Path,
    runs_root: Path,
    output_dir: Path | None,
) -> tuple[dict[str, Any], list[dict[str, str]]]:
    result, result_issues = _read_json(run_dir / "result.json")
    metadata, metadata_issues = _read_json(run_dir / "metadata.json") if (run_dir / "metadata.json").exists() else (None, [])
    config, config_issues = (
        _read_json(run_dir / "config.resolved.json") if (run_dir / "config.resolved.json").exists() else (None, [])
    )
    convergence, convergence_issues = _parse_convergence(run_dir / "convergence.csv")
    schedule, schedule_issues = _parse_schedule(run_dir / "schedule.csv")

    issues = result_issues + metadata_issues + config_issues + convergence_issues + schedule_issues
    if result is None:
        result = {}

    active_rules = _as_list(_coalesce(result.get("active_rules"), result.get("rules"), config.get("rules") if config else None))
    active_features = _as_list(
        _coalesce(result.get("active_features"), result.get("features"), config.get("features") if config else None)
    )

    run_id = str(_coalesce(result.get("run_id"), metadata.get("run_id") if metadata else None, run_dir.name))
    objective_value = _as_float(_coalesce(_nested(result, "metrics", "cmax"), result.get("objective_value")))
    status = str(_coalesce(result.get("status"), "unknown"))
    feasibility_valid = _as_bool(_nested(result, "feasibility", "valid"))

    instance = _coalesce(
        result.get("instance"),
        _nested(config, "instance", "name"),
        config.get("instance") if isinstance(config, dict) else None,
        "unknown",
    )
    if isinstance(instance, dict):
        instance = instance.get("name") or instance.get("path") or "unknown"

    config_path = _coalesce(
        result.get("config_source"),
        _nested(result, "inputs", "config_source"),
        _nested(metadata, "inputs", "config_source"),
        config.get("source") if config else None,
        _nested(config, "run", "config_source"),
    )

    files = _file_links(run_dir, output_dir)
    run_rel_dir = Path(os.path.relpath(run_dir.resolve(), runs_root.resolve())).as_posix()

    run = {
        "source": "run_dir",
        "run_key": run_id,
        "run_id": run_id,
        "display_run_id": run_id,
        "run_dir": run_rel_dir,
        "status": status,
        "instance": str(instance),
        "cmax": _format_float(objective_value),
        "runtime_sec": _format_float(_as_float(result.get("runtime_sec"))),
        "feasibility_valid": feasibility_valid,
        "sgs": str(_coalesce(_nested(result, "solver", "sgs"), _nested(config, "solver", "sgs"), "")),
        "seed": _coalesce(result.get("seed"), _nested(config, "run", "seed"), config.get("seed") if config else None),
        "method": str(_coalesce(result.get("method"), _nested(config, "solver", "method"), "")),
        "objective": str(_coalesce(result.get("objective"), config.get("objective") if config else "")),
        "active_rules": active_rules,
        "active_features": active_features,
        "config_path": str(config_path) if config_path is not None else "",
        "timestamp_local": str(_coalesce(_nested(metadata, "timestamp_local"), "")),
        "batch_ids": [],
        "batch_suite_names": [],
        "files": files,
        "convergence": convergence,
        "schedule": schedule,
        "issues": issues,
    }
    return run, issues


def _load_batches(
    batches_root: Path,
) -> tuple[list[dict[str, Any]], dict[str, list[dict[str, Any]]], list[dict[str, str]]]:
    batches: list[dict[str, Any]] = []
    run_rows_by_id: dict[str, list[dict[str, Any]]] = {}
    issues: list[dict[str, str]] = []

    if not batches_root.exists():
        return batches, run_rows_by_id, issues

    for batch_dir in sorted(path for path in batches_root.iterdir() if path.is_dir()):
        metadata, metadata_issues = (
            _read_json(batch_dir / "batch_metadata.json") if (batch_dir / "batch_metadata.json").exists() else (None, [])
        )
        rows, columns, csv_issues = _read_csv(batch_dir / "batch_summary.csv")
        batch_issues = metadata_issues + csv_issues
        issues.extend(batch_issues)

        batch_id = str(_coalesce(_nested(metadata, "batch_id"), batch_dir.name))
        suite_name = str(_coalesce(_nested(metadata, "suite_name"), rows[0].get("suite_name") if rows else ""))
        success_count = sum(1 for row in rows if str(row.get("status", "")).lower() == "success")
        failed_count = sum(1 for row in rows if str(row.get("status", "")).lower() == "failed")
        run_count = _coalesce(_nested(metadata, "run_count"), len(rows))

        batch = {
            "batch_id": batch_id,
            "batch_dir": batch_dir.name,
            "suite_name": suite_name,
            "run_count": int(run_count) if str(run_count).isdigit() else len(rows),
            "success_count": success_count,
            "failed_count": failed_count,
            "started_at": str(_coalesce(_nested(metadata, "started_at"), "")),
            "finished_at": str(_coalesce(_nested(metadata, "finished_at"), "")),
            "config_path": str(_coalesce(_nested(metadata, "config_path"), "")),
            "columns": columns,
            "rows": rows,
            "issues": batch_issues,
        }
        batches.append(batch)

        for index, row in enumerate(rows, start=1):
            enriched = dict(row)
            enriched["_batch_id"] = batch_id
            enriched["_suite_name"] = suite_name
            enriched["_row_index"] = index
            run_id = str(row.get("run_id") or "")
            if run_id:
                run_rows_by_id.setdefault(run_id, []).append(enriched)
            else:
                run_rows_by_id.setdefault(f"{batch_id}::row{index}", []).append(enriched)

    return batches, run_rows_by_id, issues


def _batch_only_run(row: dict[str, Any]) -> dict[str, Any]:
    batch_id = str(row.get("_batch_id", ""))
    row_index = row.get("_row_index", "")
    run_key = f"{batch_id}::row{row_index}"
    status = str(row.get("status") or "unknown")
    return {
        "source": "batch_summary",
        "run_key": run_key,
        "run_id": str(row.get("run_id") or ""),
        "display_run_id": f"(batch row {row_index})",
        "run_dir": "",
        "status": status,
        "instance": str(row.get("instance") or ""),
        "cmax": _format_float(_as_float(row.get("cmax"))),
        "runtime_sec": _format_float(_as_float(row.get("runtime_sec"))),
        "feasibility_valid": _as_bool(row.get("feasibility_valid")),
        "sgs": "",
        "seed": None,
        "method": "",
        "objective": "cmax",
        "active_rules": [],
        "active_features": [],
        "config_path": str(row.get("config_path") or ""),
        "timestamp_local": "",
        "batch_ids": [batch_id] if batch_id else [],
        "batch_suite_names": [str(row.get("_suite_name") or "")],
        "files": {},
        "convergence": {"available": False, "columns": [], "row_count": 0, "points": [], "truncated": False},
        "schedule": {
            "available": False,
            "columns": [],
            "row_count": 0,
            "preview": [],
            "machines": [],
            "cmax": None,
            "truncated": False,
        },
        "issues": [],
    }


def load_dashboard_data(runs_root: Path, output_dir: Path | None = None) -> dict[str, Any]:
    """Return dashboard-ready data from an XSIM runs root."""

    runs_root = runs_root.resolve()
    output_dir = output_dir.resolve() if output_dir is not None else None
    issues: list[dict[str, str]] = []
    runs: list[dict[str, Any]] = []

    if not runs_root.exists():
        issues.append(_issue(runs_root, "Runs root does not exist.", "error"))
    elif not runs_root.is_dir():
        issues.append(_issue(runs_root, "Runs root is not a directory.", "error"))
    else:
        for run_dir in sorted(path for path in runs_root.iterdir() if path.is_dir() and path.name != "batches"):
            has_expected_output = any((run_dir / name).exists() for name in EXPECTED_RUN_FILES)
            if not has_expected_output:
                continue
            run, run_issues = _run_summary_from_files(run_dir, runs_root, output_dir)
            runs.append(run)
            issues.extend(run_issues)

    batches, batch_rows_by_id, batch_issues = _load_batches(runs_root / "batches")
    issues.extend(batch_issues)

    runs_by_id = {run["run_id"]: run for run in runs if run.get("run_id")}
    known_batch_only_keys = set()
    for key, rows_for_key in batch_rows_by_id.items():
        first_row = rows_for_key[0]
        run = runs_by_id.get(key)
        if run is None:
            run = _batch_only_run(first_row)
            runs.append(run)
            known_batch_only_keys.add(run["run_key"])
        for row in rows_for_key:
            batch_id = str(row.get("_batch_id") or "")
            suite_name = str(row.get("_suite_name") or "")
            if batch_id and batch_id not in run["batch_ids"]:
                run["batch_ids"].append(batch_id)
            if suite_name and suite_name not in run["batch_suite_names"]:
                run["batch_suite_names"].append(suite_name)
            if not run.get("config_path") and row.get("config_path"):
                run["config_path"] = str(row["config_path"])
            if run["run_key"] in known_batch_only_keys:
                run["status"] = str(row.get("status") or run["status"])

    unique_instances = sorted({run["instance"] for run in runs if run.get("instance")})
    status_counts: dict[str, int] = {}
    sgs_counts: dict[str, int] = {}
    for run in runs:
        status = str(run.get("status") or "unknown")
        status_counts[status] = status_counts.get(status, 0) + 1
        sgs = str(run.get("sgs") or "")
        if sgs:
            sgs_counts[sgs] = sgs_counts.get(sgs, 0) + 1

    overview = {
        "run_count": len(runs),
        "success_count": sum(1 for run in runs if str(run.get("status", "")).lower() == "success"),
        "failed_count": sum(1 for run in runs if str(run.get("status", "")).lower() == "failed"),
        "unique_instance_count": len(unique_instances),
        "batch_count": len(batches),
        "issue_count": len(issues),
        "status_counts": status_counts,
        "sgs_counts": sgs_counts,
    }

    return {
        "schema_version": "1.0",
        "generated_at": datetime.now().isoformat(timespec="seconds"),
        "runs_root": str(runs_root),
        "overview": overview,
        "runs": runs,
        "batches": batches,
        "issues": issues,
    }


__all__ = ["load_dashboard_data"]
