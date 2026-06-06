#!/usr/bin/env python3
"""Generate a static XSIM results explorer from runs/ output files."""

from __future__ import annotations

import argparse
import html
import json
from pathlib import Path
from typing import Any

from dashboard_data_loader import load_dashboard_data


def _json_for_script(data: dict[str, Any]) -> str:
    return json.dumps(data, ensure_ascii=True, separators=(",", ":")).replace("</", "<\\/")


def _render_index(data: dict[str, Any], title: str) -> str:
    safe_title = html.escape(title)
    data_json = _json_for_script(data)
    return f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{safe_title}</title>
<style>
:root {{
  color-scheme: light;
  --bg: #f7f8fa;
  --panel: #ffffff;
  --ink: #18202f;
  --muted: #637083;
  --line: #dce2ea;
  --soft: #eef2f6;
  --blue: #2459a8;
  --green: #157347;
  --red: #b42318;
  --amber: #946200;
  --teal: #0f766e;
  --violet: #6842a3;
  font-family: "Segoe UI", Arial, sans-serif;
}}
* {{ box-sizing: border-box; }}
body {{
  margin: 0;
  background: var(--bg);
  color: var(--ink);
}}
a {{ color: var(--blue); text-decoration: none; }}
a:hover {{ text-decoration: underline; }}
.shell {{
  max-width: 1480px;
  margin: 0 auto;
  padding: 20px;
}}
header {{
  display: flex;
  justify-content: space-between;
  align-items: flex-end;
  gap: 18px;
  margin-bottom: 18px;
}}
h1 {{
  margin: 0 0 6px;
  font-size: 28px;
  font-weight: 700;
  letter-spacing: 0;
}}
h2 {{
  margin: 0 0 12px;
  font-size: 18px;
  letter-spacing: 0;
}}
h3 {{
  margin: 0 0 10px;
  font-size: 15px;
  letter-spacing: 0;
}}
.subtle {{
  color: var(--muted);
  font-size: 13px;
}}
.toolbar {{
  display: grid;
  grid-template-columns: repeat(6, minmax(130px, 1fr));
  gap: 10px;
  margin: 14px 0 16px;
}}
.filter {{
  display: flex;
  flex-direction: column;
  gap: 5px;
}}
.filter label {{
  font-size: 12px;
  color: var(--muted);
  font-weight: 600;
}}
input, select {{
  width: 100%;
  border: 1px solid var(--line);
  border-radius: 6px;
  background: #fff;
  color: var(--ink);
  min-height: 34px;
  padding: 6px 8px;
  font: inherit;
  font-size: 13px;
}}
button {{
  border: 1px solid var(--line);
  border-radius: 6px;
  background: #fff;
  color: var(--ink);
  min-height: 34px;
  padding: 6px 10px;
  font: inherit;
  font-size: 13px;
  cursor: pointer;
}}
button:hover {{ border-color: #aeb8c6; }}
.overview {{
  display: grid;
  grid-template-columns: repeat(6, minmax(120px, 1fr));
  gap: 10px;
}}
.metric {{
  background: var(--panel);
  border: 1px solid var(--line);
  border-radius: 8px;
  padding: 12px;
}}
.metric .value {{
  font-size: 24px;
  font-weight: 700;
}}
.metric .label {{
  margin-top: 4px;
  color: var(--muted);
  font-size: 12px;
  font-weight: 600;
}}
.grid-2 {{
  display: grid;
  grid-template-columns: minmax(0, 1.4fr) minmax(360px, 0.6fr);
  gap: 14px;
}}
.section {{
  background: var(--panel);
  border: 1px solid var(--line);
  border-radius: 8px;
  padding: 14px;
  margin-top: 14px;
}}
.section-head {{
  display: flex;
  justify-content: space-between;
  gap: 12px;
  align-items: baseline;
}}
.table-wrap {{
  overflow: auto;
  border: 1px solid var(--line);
  border-radius: 8px;
}}
table {{
  width: 100%;
  border-collapse: collapse;
  font-size: 13px;
}}
th, td {{
  border-bottom: 1px solid var(--line);
  padding: 8px 9px;
  text-align: left;
  vertical-align: top;
  white-space: nowrap;
}}
th {{
  background: #f1f4f8;
  color: #344154;
  font-size: 12px;
  position: sticky;
  top: 0;
  z-index: 1;
}}
tbody tr:hover {{ background: #f8fafc; }}
tbody tr.selected {{ background: #eef4ff; }}
.wrap-cell {{
  white-space: normal;
  min-width: 160px;
}}
.badge {{
  display: inline-block;
  border-radius: 999px;
  border: 1px solid var(--line);
  padding: 2px 7px;
  font-size: 12px;
  line-height: 18px;
  margin: 0 4px 4px 0;
  background: #fff;
}}
.status-success {{ color: var(--green); border-color: #a9d6bf; background: #edf8f2; }}
.status-failed {{ color: var(--red); border-color: #efb4ae; background: #fff0ee; }}
.status-unknown {{ color: var(--amber); border-color: #e8d39b; background: #fff8df; }}
.charts {{
  display: grid;
  grid-template-columns: repeat(3, minmax(260px, 1fr));
  gap: 12px;
}}
.chart-box {{
  border: 1px solid var(--line);
  border-radius: 8px;
  padding: 12px;
  min-height: 260px;
}}
.chart-svg {{
  width: 100%;
  height: 210px;
  display: block;
}}
.empty {{
  color: var(--muted);
  font-size: 13px;
  padding: 14px 0;
}}
.details-grid {{
  display: grid;
  grid-template-columns: minmax(0, 1fr) minmax(0, 1fr);
  gap: 12px;
}}
.kv {{
  display: grid;
  grid-template-columns: 130px minmax(0, 1fr);
  gap: 6px 10px;
  font-size: 13px;
}}
.kv .k {{ color: var(--muted); }}
.links {{
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}}
.link-pill {{
  display: inline-flex;
  align-items: center;
  border: 1px solid var(--line);
  border-radius: 6px;
  min-height: 30px;
  padding: 5px 8px;
  background: #fff;
  font-size: 13px;
}}
.gantt-frame {{
  width: 100%;
  height: 360px;
  border: 1px solid var(--line);
  border-radius: 8px;
  background: #fff;
}}
.issues-list {{
  max-height: 240px;
  overflow: auto;
  border: 1px solid var(--line);
  border-radius: 8px;
}}
.issue {{
  padding: 8px 10px;
  border-bottom: 1px solid var(--line);
  font-size: 13px;
}}
.issue:last-child {{ border-bottom: 0; }}
.issue .sev {{
  font-weight: 700;
  color: var(--red);
  margin-right: 8px;
}}
.mono {{
  font-family: Consolas, "Courier New", monospace;
  font-size: 12px;
}}
@media (max-width: 1120px) {{
  .toolbar {{ grid-template-columns: repeat(3, minmax(130px, 1fr)); }}
  .overview {{ grid-template-columns: repeat(3, minmax(120px, 1fr)); }}
  .grid-2 {{ grid-template-columns: 1fr; }}
  .charts {{ grid-template-columns: 1fr; }}
  .details-grid {{ grid-template-columns: 1fr; }}
}}
@media (max-width: 680px) {{
  .shell {{ padding: 12px; }}
  header {{ display: block; }}
  .toolbar {{ grid-template-columns: 1fr; }}
  .overview {{ grid-template-columns: repeat(2, minmax(120px, 1fr)); }}
}}
</style>
</head>
<body>
<div class="shell">
  <header>
    <div>
      <h1>{safe_title}</h1>
      <div class="subtle" id="runRoot"></div>
    </div>
    <div class="subtle" id="generatedAt"></div>
  </header>

  <section class="overview" id="overview"></section>

  <section class="section">
    <div class="section-head">
      <h2>Run Filters</h2>
      <button id="resetFilters" type="button">Reset filters</button>
    </div>
    <div class="toolbar">
      <div class="filter"><label for="searchFilter">Search</label><input id="searchFilter" type="search" placeholder="run, config, rule"></div>
      <div class="filter"><label for="instanceFilter">Instance</label><select id="instanceFilter"></select></div>
      <div class="filter"><label for="batchFilter">Batch</label><select id="batchFilter"></select></div>
      <div class="filter"><label for="statusFilter">Status</label><select id="statusFilter"></select></div>
      <div class="filter"><label for="sgsFilter">SGS</label><select id="sgsFilter"></select></div>
      <div class="filter"><label for="ruleFilter">Rule</label><select id="ruleFilter"></select></div>
      <div class="filter"><label for="featureFilter">Feature</label><select id="featureFilter"></select></div>
      <div class="filter"><label for="configFilter">Config path</label><select id="configFilter"></select></div>
    </div>
  </section>

  <section class="section">
    <div class="section-head">
      <h2>Comparison Charts</h2>
      <div class="subtle" id="chartCount"></div>
    </div>
    <div class="charts">
      <div class="chart-box"><h3>Cmax Comparison</h3><div id="cmaxChart"></div></div>
      <div class="chart-box"><h3>Runtime Comparison</h3><div id="runtimeChart"></div></div>
      <div class="chart-box"><h3>SGS Comparison</h3><div id="sgsChart"></div></div>
    </div>
  </section>

  <div class="grid-2">
    <section class="section">
      <div class="section-head">
        <h2>Run Table</h2>
        <div class="subtle" id="runCount"></div>
      </div>
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>run_id</th>
              <th>instance</th>
              <th>cmax</th>
              <th>runtime_sec</th>
              <th>feasibility</th>
              <th>sgs</th>
              <th>active_rules</th>
              <th>active_features</th>
              <th>batch_id</th>
              <th>config_path</th>
            </tr>
          </thead>
          <tbody id="runRows"></tbody>
        </table>
      </div>
    </section>

    <section class="section">
      <h2>Selected Run</h2>
      <div id="runDetails"></div>
    </section>
  </div>

  <section class="section">
    <div class="section-head">
      <h2>Batch Summary</h2>
      <div class="subtle" id="batchCount"></div>
    </div>
    <div class="table-wrap">
      <table>
        <thead>
          <tr>
            <th>batch_id</th>
            <th>suite_name</th>
            <th>run_count</th>
            <th>success</th>
            <th>failed</th>
            <th>started_at</th>
            <th>finished_at</th>
            <th>config_path</th>
          </tr>
        </thead>
        <tbody id="batchRows"></tbody>
      </table>
    </div>
  </section>

  <section class="section">
    <h2>Convergence Viewer</h2>
    <div id="convergenceViewer"></div>
  </section>

  <section class="section">
    <h2>Schedule / Gantt Viewer</h2>
    <div id="scheduleViewer"></div>
  </section>

  <section class="section">
    <div class="section-head">
      <h2>Loader Issues</h2>
      <div class="subtle" id="issueCount"></div>
    </div>
    <div id="issuesPanel"></div>
  </section>
</div>

<script>
const DASHBOARD_DATA = {data_json};
const state = {{
  search: "",
  instance: "",
  batch: "",
  status: "",
  sgs: "",
  rule: "",
  feature: "",
  config: "",
  selectedRunKey: ""
}};

function escapeHtml(value) {{
  return String(value ?? "").replace(/[&<>"']/g, char => ({{
    "&": "&amp;",
    "<": "&lt;",
    ">": "&gt;",
    '"': "&quot;",
    "'": "&#39;"
  }}[char]));
}}

function formatValue(value, digits = 3) {{
  if (value === null || value === undefined || value === "") return "";
  if (typeof value === "number") {{
    if (!Number.isFinite(value)) return "";
    return Number.isInteger(value) ? String(value) : value.toFixed(digits).replace(/0+$/, "").replace(/\\.$/, "");
  }}
  return String(value);
}}

function asArray(value) {{
  return Array.isArray(value) ? value : [];
}}

function joinBadges(values) {{
  const list = asArray(values);
  if (!list.length) return "";
  return list.map(value => `<span class="badge">${{escapeHtml(value)}}</span>`).join("");
}}

function statusBadge(status) {{
  const normalized = String(status || "unknown").toLowerCase();
  const cls = normalized === "success" ? "status-success" : normalized === "failed" ? "status-failed" : "status-unknown";
  return `<span class="badge ${{cls}}">${{escapeHtml(status || "unknown")}}</span>`;
}}

function uniqueSorted(values) {{
  return [...new Set(values.filter(value => value !== null && value !== undefined && String(value) !== "").map(String))].sort((a, b) => a.localeCompare(b));
}}

function fillSelect(id, values, allLabel) {{
  const select = document.getElementById(id);
  select.innerHTML = `<option value="">${{escapeHtml(allLabel)}}</option>` + values.map(value => `<option value="${{escapeHtml(value)}}">${{escapeHtml(value)}}</option>`).join("");
}}

function setupFilters() {{
  const runs = DASHBOARD_DATA.runs || [];
  fillSelect("instanceFilter", uniqueSorted(runs.map(run => run.instance)), "All instances");
  fillSelect("batchFilter", uniqueSorted(runs.flatMap(run => asArray(run.batch_ids))), "All batches");
  fillSelect("statusFilter", uniqueSorted(runs.map(run => run.status)), "All statuses");
  fillSelect("sgsFilter", uniqueSorted(runs.map(run => run.sgs)), "All SGS");
  fillSelect("ruleFilter", uniqueSorted(runs.flatMap(run => asArray(run.active_rules))), "All rules");
  fillSelect("featureFilter", uniqueSorted(runs.flatMap(run => asArray(run.active_features))), "All features");
  fillSelect("configFilter", uniqueSorted(runs.map(run => run.config_path)), "All configs");

  const bindings = [
    ["searchFilter", "search"],
    ["instanceFilter", "instance"],
    ["batchFilter", "batch"],
    ["statusFilter", "status"],
    ["sgsFilter", "sgs"],
    ["ruleFilter", "rule"],
    ["featureFilter", "feature"],
    ["configFilter", "config"]
  ];
  for (const [id, key] of bindings) {{
    document.getElementById(id).addEventListener("input", event => {{
      state[key] = event.target.value;
      render();
    }});
  }}
  document.getElementById("resetFilters").addEventListener("click", () => {{
    for (const [id, key] of bindings) {{
      state[key] = "";
      document.getElementById(id).value = "";
    }}
    render();
  }});
}}

function runMatches(run) {{
  const haystack = [
    run.run_id,
    run.display_run_id,
    run.instance,
    run.status,
    run.sgs,
    run.config_path,
    ...asArray(run.batch_ids),
    ...asArray(run.active_rules),
    ...asArray(run.active_features)
  ].join(" ").toLowerCase();
  const searchOk = !state.search || haystack.includes(state.search.toLowerCase());
  return searchOk
    && (!state.instance || run.instance === state.instance)
    && (!state.batch || asArray(run.batch_ids).includes(state.batch))
    && (!state.status || run.status === state.status)
    && (!state.sgs || run.sgs === state.sgs)
    && (!state.rule || asArray(run.active_rules).includes(state.rule))
    && (!state.feature || asArray(run.active_features).includes(state.feature))
    && (!state.config || run.config_path === state.config);
}}

function filteredRuns() {{
  return (DASHBOARD_DATA.runs || []).filter(runMatches);
}}

function metric(label, value) {{
  return `<div class="metric"><div class="value">${{escapeHtml(value)}}</div><div class="label">${{escapeHtml(label)}}</div></div>`;
}}

function renderOverview() {{
  const overview = DASHBOARD_DATA.overview || {{}};
  document.getElementById("overview").innerHTML = [
    metric("total runs", overview.run_count ?? 0),
    metric("success", overview.success_count ?? 0),
    metric("failed", overview.failed_count ?? 0),
    metric("unique instances", overview.unique_instance_count ?? 0),
    metric("batches", overview.batch_count ?? 0),
    metric("loader issues", overview.issue_count ?? 0)
  ].join("");
  document.getElementById("runRoot").textContent = `runs root: ${{DASHBOARD_DATA.runs_root || ""}}`;
  document.getElementById("generatedAt").textContent = `generated: ${{DASHBOARD_DATA.generated_at || ""}}`;
}}

function selectRun(runKey) {{
  state.selectedRunKey = runKey;
  render();
}}

function renderRunTable(runs) {{
  document.getElementById("runCount").textContent = `${{runs.length}} shown`;
  const rows = runs.map(run => {{
    const selected = run.run_key === state.selectedRunKey ? "selected" : "";
    return `<tr class="${{selected}}" data-run-key="${{escapeHtml(run.run_key)}}">`
      + `<td class="mono">${{escapeHtml(run.display_run_id || run.run_id || "")}}</td>`
      + `<td>${{escapeHtml(run.instance || "")}}</td>`
      + `<td>${{formatValue(run.cmax, 2)}}</td>`
      + `<td>${{formatValue(run.runtime_sec, 6)}}</td>`
      + `<td>${{formatValue(run.feasibility_valid)}}</td>`
      + `<td>${{escapeHtml(run.sgs || "")}}</td>`
      + `<td class="wrap-cell">${{joinBadges(run.active_rules)}}</td>`
      + `<td class="wrap-cell">${{joinBadges(run.active_features)}}</td>`
      + `<td class="wrap-cell mono">${{escapeHtml(asArray(run.batch_ids).join(", "))}}</td>`
      + `<td class="wrap-cell mono">${{escapeHtml(run.config_path || "")}}</td>`
      + `</tr>`;
  }}).join("");
  document.getElementById("runRows").innerHTML = rows || `<tr><td colspan="10" class="empty">No runs match the current filters.</td></tr>`;
  document.querySelectorAll("#runRows tr[data-run-key]").forEach(row => {{
    row.addEventListener("click", () => selectRun(row.getAttribute("data-run-key")));
  }});
}}

function renderBatchTable() {{
  const batches = DASHBOARD_DATA.batches || [];
  document.getElementById("batchCount").textContent = `${{batches.length}} batches`;
  document.getElementById("batchRows").innerHTML = batches.map(batch => (
    `<tr>`
    + `<td class="mono">${{escapeHtml(batch.batch_id)}}</td>`
    + `<td>${{escapeHtml(batch.suite_name)}}</td>`
    + `<td>${{formatValue(batch.run_count, 0)}}</td>`
    + `<td>${{formatValue(batch.success_count, 0)}}</td>`
    + `<td>${{formatValue(batch.failed_count, 0)}}</td>`
    + `<td>${{escapeHtml(batch.started_at || "")}}</td>`
    + `<td>${{escapeHtml(batch.finished_at || "")}}</td>`
    + `<td class="wrap-cell mono">${{escapeHtml(batch.config_path || "")}}</td>`
    + `</tr>`
  )).join("") || `<tr><td colspan="8" class="empty">No batch summaries found.</td></tr>`;
}}

function renderBarChart(containerId, rows, valueKey, labelKey, colorKey, valueLabel) {{
  const container = document.getElementById(containerId);
  const chartRows = rows
    .filter(row => typeof row[valueKey] === "number" && Number.isFinite(row[valueKey]))
    .slice()
    .sort((a, b) => (a.instance || "").localeCompare(b.instance || "") || a[valueKey] - b[valueKey])
    .slice(0, 24);
  if (!chartRows.length) {{
    container.innerHTML = `<div class="empty">No numeric ${{escapeHtml(valueLabel)}} values for the current filters.</div>`;
    return;
  }}
  const width = 720;
  const height = 210;
  const left = 42;
  const right = 12;
  const top = 16;
  const bottom = 54;
  const plotW = width - left - right;
  const plotH = height - top - bottom;
  const maxValue = Math.max(...chartRows.map(row => row[valueKey]), 1);
  const barW = Math.max(5, plotW / chartRows.length - 4);
  const palette = ["#2459a8", "#157347", "#946200", "#0f766e", "#6842a3", "#b42318"];
  const bars = chartRows.map((row, index) => {{
    const value = row[valueKey];
    const x = left + index * (plotW / chartRows.length) + 2;
    const h = Math.max(1, (value / maxValue) * plotH);
    const y = top + plotH - h;
    const label = row[labelKey] || row.instance || row.run_id || "";
    const colorSource = colorKey ? String(row[colorKey] || "") : String(index);
    const colorIndex = Math.abs([...colorSource].reduce((acc, char) => acc + char.charCodeAt(0), 0)) % palette.length;
    return `<rect x="${{x.toFixed(2)}}" y="${{y.toFixed(2)}}" width="${{barW.toFixed(2)}}" height="${{h.toFixed(2)}}" fill="${{palette[colorIndex]}}">`
      + `<title>${{escapeHtml(label)}} | ${{escapeHtml(valueLabel)}}: ${{formatValue(value, 6)}}</title></rect>`
      + `<text x="${{(x + barW / 2).toFixed(2)}}" y="${{height - 22}}" text-anchor="end" transform="rotate(-35 ${{(x + barW / 2).toFixed(2)}} ${{height - 22}})" font-size="10" fill="#637083">${{escapeHtml(String(label).slice(0, 18))}}</text>`;
  }}).join("");
  const ticks = [0, maxValue / 2, maxValue].map(value => {{
    const y = top + plotH - (value / maxValue) * plotH;
    return `<line x1="${{left}}" x2="${{width - right}}" y1="${{y.toFixed(2)}}" y2="${{y.toFixed(2)}}" stroke="#e6ebf2"/>`
      + `<text x="${{left - 8}}" y="${{(y + 4).toFixed(2)}}" text-anchor="end" font-size="10" fill="#637083">${{formatValue(value, 2)}}</text>`;
  }}).join("");
  container.innerHTML = `<svg class="chart-svg" viewBox="0 0 ${{width}} ${{height}}" role="img" aria-label="${{escapeHtml(valueLabel)}} chart">`
    + ticks
    + `<line x1="${{left}}" x2="${{width - right}}" y1="${{top + plotH}}" y2="${{top + plotH}}" stroke="#aeb8c6"/>`
    + bars
    + `</svg>`;
}}

function renderSgsChart(runs) {{
  const groups = new Map();
  for (const run of runs) {{
    if (!run.sgs || typeof run.cmax !== "number") continue;
    if (!groups.has(run.sgs)) groups.set(run.sgs, {{sgs: run.sgs, total: 0, count: 0}});
    const group = groups.get(run.sgs);
    group.total += run.cmax;
    group.count += 1;
  }}
  const rows = [...groups.values()].map(group => ({{
    label: `${{group.sgs}} (n=${{group.count}})`,
    avg_cmax: group.count ? group.total / group.count : null,
    sgs: group.sgs
  }}));
  renderBarChart("sgsChart", rows, "avg_cmax", "label", "sgs", "average cmax");
}}

function renderCharts(runs) {{
  document.getElementById("chartCount").textContent = `${{runs.length}} filtered runs`;
  const labeled = runs.map(run => ({{
    ...run,
    chart_label: `${{run.instance || "unknown"}} | ${{run.sgs || "sgs"}} | ${{run.config_path ? run.config_path.split(/[\\\\/]/).pop() : run.display_run_id}}`
  }}));
  renderBarChart("cmaxChart", labeled, "cmax", "chart_label", "sgs", "cmax");
  renderBarChart("runtimeChart", labeled, "runtime_sec", "chart_label", "status", "runtime_sec");
  renderSgsChart(runs);
}}

function selectedRun(runs) {{
  if (!runs.length) return null;
  const stillVisible = runs.find(run => run.run_key === state.selectedRunKey);
  if (stillVisible) return stillVisible;
  state.selectedRunKey = runs[0].run_key;
  return runs[0];
}}

function fileLink(run, fileName, label) {{
  const entry = run.files && run.files[fileName];
  if (!entry || !entry.exists || !entry.href) return "";
  return `<a class="link-pill" href="${{escapeHtml(entry.href)}}" target="_blank" rel="noreferrer">${{escapeHtml(label)}}</a>`;
}}

function renderRunDetails(run) {{
  const root = document.getElementById("runDetails");
  if (!run) {{
    root.innerHTML = `<div class="empty">No run selected.</div>`;
    return;
  }}
  const links = [
    fileLink(run, "result.json", "result.json"),
    fileLink(run, "metadata.json", "metadata.json"),
    fileLink(run, "config.resolved.json", "config.resolved.json"),
    fileLink(run, "schedule.csv", "schedule.csv"),
    fileLink(run, "convergence.csv", "convergence.csv"),
    fileLink(run, "gantt.csv", "gantt.csv"),
    fileLink(run, "gantt.html", "gantt.html")
  ].filter(Boolean).join("");
  root.innerHTML = `
    <div class="kv">
      <div class="k">run_id</div><div class="mono">${{escapeHtml(run.display_run_id || run.run_id || "")}}</div>
      <div class="k">status</div><div>${{statusBadge(run.status)}}</div>
      <div class="k">instance</div><div>${{escapeHtml(run.instance || "")}}</div>
      <div class="k">cmax</div><div>${{formatValue(run.cmax, 3)}}</div>
      <div class="k">runtime_sec</div><div>${{formatValue(run.runtime_sec, 6)}}</div>
      <div class="k">feasibility</div><div>${{formatValue(run.feasibility_valid)}}</div>
      <div class="k">sgs</div><div>${{escapeHtml(run.sgs || "")}}</div>
      <div class="k">rules</div><div>${{joinBadges(run.active_rules) || ""}}</div>
      <div class="k">features</div><div>${{joinBadges(run.active_features) || ""}}</div>
      <div class="k">batch</div><div class="mono">${{escapeHtml(asArray(run.batch_ids).join(", "))}}</div>
      <div class="k">config</div><div class="mono">${{escapeHtml(run.config_path || "")}}</div>
    </div>
    <h3 style="margin-top:14px;">Files</h3>
    <div class="links">${{links || '<span class="empty">No linked files for this row.</span>'}}</div>
  `;
}}

function findNumericColumn(columns, preferred) {{
  const lowered = columns.map(column => String(column).toLowerCase());
  for (const name of preferred) {{
    const index = lowered.indexOf(name.toLowerCase());
    if (index >= 0) return columns[index];
  }}
  return columns.find(column => !["iteration", "iter"].includes(String(column).toLowerCase()));
}}

function renderLineChart(container, points, columns) {{
  const xCol = findNumericColumn(columns, ["Iteration", "iteration", "iter"]);
  const yCol = findNumericColumn(columns, ["BestCmax", "best_cmax", "IterBest", "iter_best"]);
  const numeric = points
    .map(point => ({{x: Number(point[xCol]), y: Number(point[yCol])}}))
    .filter(point => Number.isFinite(point.x) && Number.isFinite(point.y));
  if (!numeric.length) {{
    container.innerHTML = `<div class="empty">Convergence rows are present but no numeric series was detected.</div>`;
    return;
  }}
  const width = 900;
  const height = 260;
  const left = 48;
  const right = 18;
  const top = 18;
  const bottom = 34;
  const plotW = width - left - right;
  const plotH = height - top - bottom;
  const minX = Math.min(...numeric.map(point => point.x));
  const maxX = Math.max(...numeric.map(point => point.x));
  const minY = Math.min(...numeric.map(point => point.y));
  const maxY = Math.max(...numeric.map(point => point.y));
  const spanX = Math.max(1, maxX - minX);
  const spanY = Math.max(1, maxY - minY);
  const coords = numeric.map(point => {{
    const x = left + ((point.x - minX) / spanX) * plotW;
    const y = top + plotH - ((point.y - minY) / spanY) * plotH;
    return `${{x.toFixed(2)}},${{y.toFixed(2)}}`;
  }}).join(" ");
  const ticks = [minY, minY + spanY / 2, maxY].map(value => {{
    const y = top + plotH - ((value - minY) / spanY) * plotH;
    return `<line x1="${{left}}" x2="${{width - right}}" y1="${{y.toFixed(2)}}" y2="${{y.toFixed(2)}}" stroke="#e6ebf2"/>`
      + `<text x="${{left - 8}}" y="${{(y + 4).toFixed(2)}}" text-anchor="end" font-size="10" fill="#637083">${{formatValue(value, 2)}}</text>`;
  }}).join("");
  container.innerHTML = `<svg class="chart-svg" viewBox="0 0 ${{width}} ${{height}}" role="img" aria-label="convergence chart">`
    + ticks
    + `<line x1="${{left}}" x2="${{width - right}}" y1="${{top + plotH}}" y2="${{top + plotH}}" stroke="#aeb8c6"/>`
    + `<polyline fill="none" stroke="#2459a8" stroke-width="2.5" points="${{coords}}"/>`
    + `<text x="${{left}}" y="${{height - 8}}" font-size="11" fill="#637083">${{escapeHtml(xCol)}} -> ${{escapeHtml(yCol)}}</text>`
    + `</svg>`;
}}

function renderConvergence(run) {{
  const root = document.getElementById("convergenceViewer");
  if (!run) {{
    root.innerHTML = `<div class="empty">No run selected.</div>`;
    return;
  }}
  const convergence = run.convergence || {{}};
  if (!convergence.available) {{
    root.innerHTML = `<div class="empty">No convergence.csv for the selected run.</div>`;
    return;
  }}
  root.innerHTML = `<div class="subtle">${{convergence.row_count || 0}} rows${{convergence.truncated ? ", sampled for display" : ""}}</div><div id="convergenceChart"></div>`;
  renderLineChart(document.getElementById("convergenceChart"), convergence.points || [], convergence.columns || []);
}}

function renderSchedule(run) {{
  const root = document.getElementById("scheduleViewer");
  if (!run) {{
    root.innerHTML = `<div class="empty">No run selected.</div>`;
    return;
  }}
  const schedule = run.schedule || {{}};
  const gantt = run.files && run.files["gantt.html"];
  const links = [
    fileLink(run, "schedule.csv", "schedule.csv"),
    fileLink(run, "gantt.html", "gantt.html"),
    fileLink(run, "gantt.csv", "gantt.csv")
  ].filter(Boolean).join("");
  let preview = `<div class="empty">No schedule.csv for the selected run.</div>`;
  if (schedule.available && schedule.preview && schedule.preview.length) {{
    const columns = schedule.columns || Object.keys(schedule.preview[0] || {{}});
    const head = columns.map(column => `<th>${{escapeHtml(column)}}</th>`).join("");
    const body = schedule.preview.map(row => `<tr>${{columns.map(column => `<td>${{escapeHtml(row[column] ?? "")}}</td>`).join("")}}</tr>`).join("");
    preview = `<div class="subtle">${{schedule.row_count}} operations, machines: ${{asArray(schedule.machines).join(", ")}}, schedule cmax: ${{formatValue(schedule.cmax, 3)}}${{schedule.truncated ? ", preview truncated" : ""}}</div>`
      + `<div class="table-wrap" style="margin-top:10px;"><table><thead><tr>${{head}}</tr></thead><tbody>${{body}}</tbody></table></div>`;
  }}
  const frame = gantt && gantt.exists && gantt.href
    ? `<iframe class="gantt-frame" src="${{escapeHtml(gantt.href)}}" title="Gantt chart"></iframe>`
    : `<div class="empty">No gantt.html for the selected run.</div>`;
  root.innerHTML = `<div class="details-grid"><div><h3>Files</h3><div class="links">${{links || '<span class="empty">No schedule or Gantt links.</span>'}}</div><h3 style="margin-top:14px;">Schedule Preview</h3>${{preview}}</div><div><h3>Gantt</h3>${{frame}}</div></div>`;
}}

function renderIssues() {{
  const issues = DASHBOARD_DATA.issues || [];
  document.getElementById("issueCount").textContent = `${{issues.length}} issues`;
  if (!issues.length) {{
    document.getElementById("issuesPanel").innerHTML = `<div class="empty">No loader issues.</div>`;
    return;
  }}
  document.getElementById("issuesPanel").innerHTML = `<div class="issues-list">` + issues.map(issue => (
    `<div class="issue"><span class="sev">${{escapeHtml(issue.severity || "warning")}}</span>`
    + `<span class="mono">${{escapeHtml(issue.path || "")}}</span><br>`
    + `${{escapeHtml(issue.message || "")}}</div>`
  )).join("") + `</div>`;
}}

function render() {{
  const runs = filteredRuns();
  const run = selectedRun(runs);
  renderOverview();
  renderCharts(runs);
  renderRunTable(runs);
  renderRunDetails(run);
  renderBatchTable();
  renderConvergence(run);
  renderSchedule(run);
  renderIssues();
}}

setupFilters();
render();
</script>
</body>
</html>
"""


def generate_dashboard(runs_root: Path, output_dir: Path, title: str) -> dict[str, Any]:
    data = load_dashboard_data(runs_root, output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    data_dir = output_dir / "data"
    data_dir.mkdir(parents=True, exist_ok=True)

    data_path = data_dir / "dashboard_data.json"
    index_path = output_dir / "index.html"
    data_path.write_text(json.dumps(data, ensure_ascii=True, indent=2) + "\n", encoding="utf-8")
    index_path.write_text(_render_index(data, title), encoding="utf-8")
    return {
        "index_path": index_path,
        "data_path": data_path,
        "run_count": len(data.get("runs", [])),
        "batch_count": len(data.get("batches", [])),
        "issue_count": len(data.get("issues", [])),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate a static XSIM results dashboard.")
    parser.add_argument("--runs-root", type=Path, default=Path("runs"), help="Path to the XSIM runs root.")
    parser.add_argument("--output", type=Path, default=Path("dashboard"), help="Directory where the dashboard is written.")
    parser.add_argument("--title", default="XSIM Results Explorer", help="Dashboard title.")
    args = parser.parse_args()

    summary = generate_dashboard(args.runs_root, args.output, args.title)
    print(f"Wrote {summary['index_path']}")
    print(f"Wrote {summary['data_path']}")
    print(
        "Loaded "
        f"{summary['run_count']} run rows, "
        f"{summary['batch_count']} batches, "
        f"{summary['issue_count']} loader issues."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
