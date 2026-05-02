#!/usr/bin/env python3
"""Render a Gantt CSV from djssp_pso_hh.exe as a self-contained HTML chart."""

from __future__ import annotations

import argparse
import csv
import html
from datetime import datetime
from pathlib import Path


COLORS = [
    "#2563eb",
    "#dc2626",
    "#16a34a",
    "#9333ea",
    "#ea580c",
    "#0891b2",
    "#be123c",
    "#4f46e5",
    "#65a30d",
    "#b45309",
    "#0f766e",
    "#7c3aed",
    "#c2410c",
    "#0369a1",
    "#a21caf",
]


def parse_rows(csv_path: Path) -> list[dict[str, float | int]]:
    rows: list[dict[str, float | int]] = []
    with csv_path.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        required = {"machine_id", "job_id", "op_index", "start", "end", "proc_time"}
        if reader.fieldnames is None or not required.issubset(reader.fieldnames):
            missing = ", ".join(sorted(required - set(reader.fieldnames or [])))
            raise SystemExit(f"Bad Gantt CSV header. Missing: {missing}")

        for raw in reader:
            rows.append(
                {
                    "machine_id": int(raw["machine_id"]),
                    "job_id": int(raw["job_id"]),
                    "op_index": int(raw["op_index"]),
                    "start": float(raw["start"]),
                    "end": float(raw["end"]),
                    "proc_time": float(raw["proc_time"]),
                }
            )
    return sorted(rows, key=lambda r: (r["machine_id"], r["start"], r["end"], r["job_id"], r["op_index"]))


def infer_instance(csv_path: Path) -> str:
    stem = csv_path.stem
    if stem.startswith("gantt_"):
        return stem[len("gantt_") :]
    return stem


def color_for_job(job_id: int) -> str:
    return COLORS[max(0, job_id) % len(COLORS)]


def render_html(rows: list[dict[str, float | int]], instance: str) -> str:
    cmax = max((float(row["end"]) for row in rows), default=0.0)
    machines = sorted({int(row["machine_id"]) for row in rows})
    generated = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    safe_instance = html.escape(instance)

    parts: list[str] = [
        "<!doctype html>",
        '<html lang="en">',
        "<head>",
        '<meta charset="utf-8">',
        '<meta name="viewport" content="width=device-width, initial-scale=1">',
        f"<title>Gantt {safe_instance}</title>",
        "<style>",
        ":root{color-scheme:light;font-family:Segoe UI,Arial,sans-serif;background:#f6f7f9;color:#151922;}",
        "body{margin:0;padding:24px;background:#f6f7f9;}",
        ".wrap{max-width:1280px;margin:0 auto;}",
        "header{display:flex;justify-content:space-between;gap:16px;align-items:flex-end;margin-bottom:18px;}",
        "h1{font-size:24px;line-height:1.2;margin:0;color:#111827;}",
        ".meta{font-size:13px;color:#4b5563;text-align:right;}",
        ".chart{background:#fff;border:1px solid #d8dde6;border-radius:8px;overflow:auto;box-shadow:0 1px 2px rgba(15,23,42,.06);}",
        ".axis{display:grid;grid-template-columns:86px minmax(760px,1fr);border-bottom:1px solid #e5e7eb;background:#f9fafb;position:sticky;top:0;z-index:2;}",
        ".axis-label{padding:10px 12px;font-size:12px;font-weight:600;color:#4b5563;border-right:1px solid #e5e7eb;}",
        ".ticks{position:relative;height:38px;min-width:760px;}",
        ".tick{position:absolute;top:0;bottom:0;border-left:1px solid #e5e7eb;font-size:11px;color:#64748b;padding-left:5px;}",
        ".row{display:grid;grid-template-columns:86px minmax(760px,1fr);border-bottom:1px solid #edf0f5;}",
        ".row:last-child{border-bottom:0;}",
        ".label{padding:18px 12px;font-size:13px;font-weight:600;color:#334155;border-right:1px solid #e5e7eb;background:#fbfcfe;}",
        ".track{position:relative;height:58px;min-width:760px;background:linear-gradient(to right,#eef2f7 1px,transparent 1px);background-size:10% 100%;}",
        ".bar{position:absolute;top:12px;height:34px;border-radius:5px;color:#fff;font-size:12px;line-height:34px;text-align:center;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;box-shadow:inset 0 -1px 0 rgba(0,0,0,.18);}",
        ".bar:hover{filter:brightness(.92);z-index:3;}",
        ".empty{padding:28px;color:#64748b;}",
        "</style>",
        "</head>",
        "<body>",
        '<div class="wrap">',
        "<header>",
        f"<div><h1>Gantt Chart - {safe_instance}</h1></div>",
        f'<div class="meta">Cmax: {cmax:.0f}<br>Generated: {html.escape(generated)}</div>',
        "</header>",
        '<div class="chart">',
    ]

    if not rows or cmax <= 0:
        parts.append('<div class="empty">No operations found.</div>')
    else:
        parts.append('<div class="axis"><div class="axis-label">Machine</div><div class="ticks">')
        for index in range(5):
            pct = index * 25.0
            tick_time = cmax * pct / 100.0
            parts.append(f'<div class="tick" style="left:{pct:.4f}%">{tick_time:.0f}</div>')
        parts.append("</div></div>")

        for machine_id in machines:
            parts.append(f'<div class="row"><div class="label">M{machine_id}</div><div class="track">')
            for row in rows:
                if int(row["machine_id"]) != machine_id:
                    continue
                start = float(row["start"])
                end = float(row["end"])
                left = 100.0 * start / cmax
                width = max(0.08, 100.0 * (end - start) / cmax)
                job_id = int(row["job_id"])
                op_index = int(row["op_index"])
                title = (
                    f"Machine {machine_id} | Job {job_id} | Op {op_index} | "
                    f"Start {start:.0f} | End {end:.0f} | Proc {float(row['proc_time']):.0f}"
                )
                parts.append(
                    '<div class="bar" '
                    f'title="{html.escape(title)}" '
                    f'style="left:{left:.4f}%;width:max(2px,{width:.4f}%);background:{color_for_job(job_id)}">'
                    f"J{job_id} O{op_index}</div>"
                )
            parts.append("</div></div>")

    parts.extend(["</div>", "</div>", "</body>", "</html>"])
    return "\n".join(parts) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description="Render a Gantt CSV as a standalone HTML chart.")
    parser.add_argument("csv_path", type=Path, help="Path to gantt_<instance>.csv")
    parser.add_argument("--output", "-o", type=Path, help="Output HTML path")
    parser.add_argument("--title", help="Instance/title to show in the chart")
    args = parser.parse_args()

    csv_path = args.csv_path
    if not csv_path.exists():
        raise SystemExit(f"CSV file not found: {csv_path}")

    output = args.output or csv_path.with_suffix(".html")
    output.parent.mkdir(parents=True, exist_ok=True)
    rows = parse_rows(csv_path)
    instance = args.title or infer_instance(csv_path)
    output.write_text(render_html(rows, instance), encoding="utf-8")
    print(f"Wrote {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
