(function () {
  "use strict";

  const AGGREGATES = ["mean", "median", "stddev", "cv"];
  const AGGREGATE_ORDER = new Map(AGGREGATES.map((name, index) => [name, index]));
  const COLORS = [
    "#087f71",
    "#2f67b1",
    "#c64f2a",
    "#8d4e95",
    "#a67c00",
    "#c23b63",
    "#4f6f52",
    "#72573d",
  ];

  function splitAggregate(value) {
    for (const aggregate of AGGREGATES) {
      const suffix = `_${aggregate}`;
      if (value.endsWith(suffix)) {
        return { base: value.slice(0, -suffix.length) || "result", metric: aggregate };
      }
    }
    return { base: value || "result", metric: "value" };
  }

  function parseBenchmarkName(name) {
    const parts = String(name || "Unnamed").split("/").filter(Boolean);
    if (parts.length === 1) {
      const parsed = splitAggregate(parts[0]);
      return {
        caseName: parsed.base,
        series: "default",
        parameter: "result",
        metric: parsed.metric,
        fullName: name,
      };
    }

    const caseName = parts.shift() || "Uncategorized";
    const parsed = splitAggregate(parts.pop() || "result");
    return {
      caseName,
      series: parts.join(" / ") || "default",
      parameter: parsed.base,
      metric: parsed.metric,
      fullName: name,
    };
  }

  function naturalCompare(left, right) {
    return String(left).localeCompare(String(right), undefined, {
      numeric: true,
      sensitivity: "base",
    });
  }

  function metricCompare(left, right) {
    const leftOrder = AGGREGATE_ORDER.has(left) ? AGGREGATE_ORDER.get(left) : 100;
    const rightOrder = AGGREGATE_ORDER.has(right) ? AGGREGATE_ORDER.get(right) : 100;
    return leftOrder - rightOrder || naturalCompare(left, right);
  }

  function displayName(value) {
    return String(value)
      .replace(/[_-]+/g, " ")
      .replace(/([a-z0-9])([A-Z])/g, "$1 $2")
      .replace(/\b\w/g, (character) => character.toUpperCase());
  }

  function platformLabel(value) {
    const match = String(value).match(/\((.+)\)$/);
    return match ? match[1] : value;
  }

  function formatValue(value) {
    if (!Number.isFinite(value)) {
      return "-";
    }
    return new Intl.NumberFormat(undefined, {
      maximumSignificantDigits: 5,
    }).format(value);
  }

  function buildCatalog(data) {
    const cases = new Map();
    const metricsByCase = new Map();
    for (const entries of Object.values(data.entries || {})) {
      for (const entry of entries) {
        for (const bench of entry.benches || []) {
          const parsed = parseBenchmarkName(bench.name);
          if (!cases.has(parsed.caseName)) {
            cases.set(parsed.caseName, new Set());
            metricsByCase.set(parsed.caseName, new Set());
          }
          cases.get(parsed.caseName).add(`${parsed.series}\u0000${parsed.parameter}`);
          metricsByCase.get(parsed.caseName).add(parsed.metric);
        }
      }
    }
    return {
      platforms: Object.keys(data.entries || {}).sort(naturalCompare),
      cases: [...cases.keys()].sort(naturalCompare),
      caseCounts: new Map([...cases].map(([name, items]) => [name, items.size])),
      metricsByCase,
    };
  }

  function pointsForEntry(entry, caseName, metric) {
    if (!entry) {
      return [];
    }
    return (entry.benches || [])
      .map((bench) => ({ ...parseBenchmarkName(bench.name), ...bench }))
      .filter((bench) => bench.caseName === caseName && bench.metric === metric);
  }

  function keyedPoints(points) {
    return new Map(points.map((point) => [`${point.series}\u0000${point.parameter}`, point]));
  }

  function latestEntry(entries) {
    return entries && entries.length ? entries[entries.length - 1] : null;
  }

  function previousEntry(entries) {
    return entries && entries.length > 1 ? entries[entries.length - 2] : null;
  }

  function availableMetrics(catalog, caseName) {
    return [...(catalog.metricsByCase.get(caseName) || new Set(["value"]))].sort(metricCompare);
  }

  function initialState(data, catalog) {
    const params = new URLSearchParams(window.location.search);
    const platform = catalog.platforms.includes(params.get("platform"))
      ? params.get("platform")
      : catalog.platforms[0];
    const caseName = catalog.cases.includes(params.get("case"))
      ? params.get("case")
      : catalog.cases[0];
    const metrics = availableMetrics(catalog, caseName);
    const metric = metrics.includes(params.get("metric"))
      ? params.get("metric")
      : metrics.includes("mean") ? "mean" : metrics[0];
    return {
      platform,
      caseName,
      metric,
      view: params.get("view") === "history" ? "history" : "latest",
      scale: params.get("scale") === "logarithmic" ? "logarithmic" : "linear",
      parameter: params.get("parameter") || "",
      filter: "",
    };
  }

  function updateLocation(state) {
    const params = new URLSearchParams();
    params.set("platform", state.platform);
    params.set("case", state.caseName);
    params.set("metric", state.metric);
    params.set("view", state.view);
    params.set("scale", state.scale);
    if (state.view === "history" && state.parameter) {
      params.set("parameter", state.parameter);
    }
    try {
      window.history.replaceState(null, "", `${window.location.pathname}?${params}`);
    } catch (_) {
      // Local file previews may not allow URL replacement.
    }
  }

  function createDashboard(data) {
    const catalog = buildCatalog(data);
    const state = initialState(data, catalog);
    let chart = null;

    const elements = {
      platformTabs: document.getElementById("platform-tabs"),
      caseList: document.getElementById("case-list"),
      caseSearch: document.getElementById("case-search"),
      mobileCaseSelect: document.getElementById("mobile-case-select"),
      metricSelect: document.getElementById("metric-select"),
      parameterControl: document.getElementById("parameter-control"),
      parameterSelect: document.getElementById("parameter-select"),
      caseTitle: document.getElementById("case-title"),
      casePath: document.getElementById("case-path"),
      caseMeta: document.getElementById("case-meta"),
      chartCanvas: document.getElementById("benchmark-chart"),
      chartEmpty: document.getElementById("chart-empty"),
      resultsBody: document.getElementById("results-body"),
      commitLabel: document.getElementById("commit-label"),
      visibleCaseCount: document.getElementById("visible-case-count"),
    };

    function selectedEntries() {
      return data.entries[state.platform] || [];
    }

    function selectedLatestPoints() {
      return pointsForEntry(latestEntry(selectedEntries()), state.caseName, state.metric);
    }

    function renderHeader() {
      document.getElementById("last-update").textContent = `Updated ${new Date(data.lastUpdate).toLocaleString()}`;
      const repositoryLink = document.getElementById("repository-link");
      repositoryLink.href = data.repoUrl;
      document.getElementById("platform-count").textContent = catalog.platforms.length;
      document.getElementById("case-count").textContent = catalog.cases.length;
      document.getElementById("run-count").textContent = selectedEntries().length;
    }

    function renderPlatformTabs() {
      elements.platformTabs.replaceChildren();
      for (const platform of catalog.platforms) {
        const button = document.createElement("button");
        button.type = "button";
        button.textContent = platformLabel(platform);
        button.setAttribute("aria-pressed", String(platform === state.platform));
        button.addEventListener("click", () => {
          state.platform = platform;
          state.parameter = "";
          renderAll();
        });
        elements.platformTabs.appendChild(button);
      }
    }

    function selectCase(caseName) {
      state.caseName = caseName;
      const metrics = availableMetrics(catalog, caseName);
      if (!metrics.includes(state.metric)) {
        state.metric = metrics.includes("mean") ? "mean" : metrics[0];
      }
      state.parameter = "";
      renderAll();
    }

    function renderCases() {
      const needle = state.filter.trim().toLocaleLowerCase();
      const visibleCases = catalog.cases.filter((name) => name.toLocaleLowerCase().includes(needle));
      elements.visibleCaseCount.textContent = `${visibleCases.length}/${catalog.cases.length}`;
      elements.caseList.replaceChildren();
      for (const caseName of visibleCases) {
        const button = document.createElement("button");
        button.type = "button";
        button.title = caseName;
        button.setAttribute("aria-pressed", String(caseName === state.caseName));
        const label = document.createElement("span");
        label.className = "case-list__name";
        label.textContent = displayName(caseName);
        const count = document.createElement("span");
        count.className = "case-list__count";
        count.textContent = catalog.caseCounts.get(caseName);
        button.append(label, count);
        button.addEventListener("click", () => selectCase(caseName));
        elements.caseList.appendChild(button);
      }

      elements.mobileCaseSelect.replaceChildren();
      for (const caseName of catalog.cases) {
        const option = document.createElement("option");
        option.value = caseName;
        option.textContent = displayName(caseName);
        option.selected = caseName === state.caseName;
        elements.mobileCaseSelect.appendChild(option);
      }
    }

    function renderControls() {
      const metrics = availableMetrics(catalog, state.caseName);
      elements.metricSelect.replaceChildren();
      for (const metric of metrics) {
        const option = document.createElement("option");
        option.value = metric;
        option.textContent = displayName(metric);
        option.selected = metric === state.metric;
        elements.metricSelect.appendChild(option);
      }

      const parameters = [...new Set(selectedLatestPoints().map((point) => point.parameter))].sort(naturalCompare);
      if (!parameters.includes(state.parameter)) {
        state.parameter = parameters[0] || "";
      }
      elements.parameterSelect.replaceChildren();
      for (const parameter of parameters) {
        const option = document.createElement("option");
        option.value = parameter;
        option.textContent = parameter;
        option.selected = parameter === state.parameter;
        elements.parameterSelect.appendChild(option);
      }
      elements.parameterControl.hidden = state.view !== "history";

      for (const button of document.querySelectorAll("[data-view]")) {
        button.setAttribute("aria-pressed", String(button.dataset.view === state.view));
      }
      for (const button of document.querySelectorAll("[data-scale]")) {
        button.setAttribute("aria-pressed", String(button.dataset.scale === state.scale));
      }
    }

    function chartOptions(unit, historyEntries) {
      return {
        responsive: true,
        maintainAspectRatio: false,
        interaction: { mode: "nearest", intersect: false },
        animation: { duration: window.matchMedia("(prefers-reduced-motion: reduce)").matches ? 0 : 300 },
        plugins: {
          legend: {
            position: "bottom",
            labels: { boxWidth: 12, boxHeight: 12, padding: 18, color: "#3e4947" },
          },
          tooltip: {
            callbacks: {
              afterTitle: (items) => {
                if (!historyEntries || !items.length) {
                  return "";
                }
                const entry = historyEntries[items[0].dataIndex];
                return entry ? entry.commit.message : "";
              },
              label: (context) => `${context.dataset.label}: ${formatValue(context.parsed.y)} ${unit}`,
            },
          },
        },
        scales: {
          x: {
            grid: { color: "#edf1f0" },
            ticks: { color: "#64706e", maxRotation: 0, autoSkip: true },
          },
          y: {
            type: state.scale,
            beginAtZero: state.scale === "linear",
            grid: { color: "#e3e9e7" },
            ticks: { color: "#64706e" },
            title: { display: Boolean(unit), text: unit, color: "#64706e" },
          },
        },
      };
    }

    function renderChart() {
      if (chart) {
        chart.destroy();
        chart = null;
      }

      const points = selectedLatestPoints();
      elements.chartEmpty.hidden = true;
      elements.chartCanvas.hidden = false;
      if (!points.length) {
        elements.chartEmpty.textContent = "No results are available for this selection.";
        elements.chartEmpty.hidden = false;
        elements.chartCanvas.hidden = true;
        return;
      }
      if (typeof window.Chart === "undefined") {
        elements.chartEmpty.textContent = "The chart library could not be loaded. Latest values remain available below.";
        elements.chartEmpty.hidden = false;
        elements.chartCanvas.hidden = true;
        return;
      }

      const unit = points[0].unit || "";
      const series = [...new Set(points.map((point) => point.series))].sort(naturalCompare);
      let labels;
      let datasets;
      let historyEntries = null;

      if (state.view === "latest") {
        labels = [...new Set(points.map((point) => point.parameter))].sort(naturalCompare);
        const pointMap = keyedPoints(points);
        datasets = series.map((seriesName, index) => ({
          label: seriesName,
          data: labels.map((parameter) => pointMap.get(`${seriesName}\u0000${parameter}`)?.value ?? null),
          borderColor: COLORS[index % COLORS.length],
          backgroundColor: COLORS[index % COLORS.length],
          borderWidth: 2,
          pointRadius: 4,
          pointHoverRadius: 6,
          tension: 0.16,
          spanGaps: true,
        }));
      } else {
        historyEntries = selectedEntries();
        labels = historyEntries.map((entry) => entry.commit.id.slice(0, 7));
        datasets = series.map((seriesName, index) => ({
          label: seriesName,
          data: historyEntries.map((entry) => {
            const match = pointsForEntry(entry, state.caseName, state.metric)
              .find((point) => point.series === seriesName && point.parameter === state.parameter);
            return match ? match.value : null;
          }),
          borderColor: COLORS[index % COLORS.length],
          backgroundColor: COLORS[index % COLORS.length],
          borderWidth: 2,
          pointRadius: 4,
          pointHoverRadius: 6,
          tension: 0.16,
          spanGaps: true,
        }));
      }

      chart = new window.Chart(elements.chartCanvas, {
        type: "line",
        data: { labels, datasets },
        options: chartOptions(unit, historyEntries),
      });
    }

    function renderCaseHeading() {
      const points = selectedLatestPoints();
      const seriesCount = new Set(points.map((point) => point.series)).size;
      const parameterCount = new Set(points.map((point) => point.parameter)).size;
      const unit = points[0]?.unit || "no unit";
      elements.casePath.textContent = platformLabel(state.platform);
      elements.caseTitle.textContent = displayName(state.caseName);
      elements.caseMeta.textContent = `${seriesCount} series / ${parameterCount} parameters / ${unit}`;
    }

    function renderTable() {
      const entries = selectedEntries();
      const currentEntry = latestEntry(entries);
      const previous = keyedPoints(pointsForEntry(previousEntry(entries), state.caseName, state.metric));
      const points = selectedLatestPoints().sort((left, right) =>
        naturalCompare(left.series, right.series) || naturalCompare(left.parameter, right.parameter));
      elements.resultsBody.replaceChildren();
      elements.commitLabel.textContent = currentEntry
        ? `${currentEntry.commit.id.slice(0, 7)} / ${new Date(currentEntry.date).toLocaleDateString()}`
        : "";

      if (!points.length) {
        const row = document.createElement("tr");
        const cell = document.createElement("td");
        cell.colSpan = 4;
        cell.className = "table-empty";
        cell.textContent = "No latest values available.";
        row.appendChild(cell);
        elements.resultsBody.appendChild(row);
        return;
      }

      for (const point of points) {
        const row = document.createElement("tr");
        const oldPoint = previous.get(`${point.series}\u0000${point.parameter}`);
        const delta = oldPoint && oldPoint.value !== 0
          ? ((point.value - oldPoint.value) / Math.abs(oldPoint.value)) * 100
          : null;
        const values = [point.series, point.parameter, `${formatValue(point.value)} ${point.unit || ""}`.trim()];
        for (const [index, value] of values.entries()) {
          const cell = document.createElement("td");
          cell.textContent = value;
          if (index === 2) {
            cell.className = "value-cell";
          }
          row.appendChild(cell);
        }
        const deltaCell = document.createElement("td");
        deltaCell.textContent = delta === null ? "-" : `${delta > 0 ? "+" : ""}${delta.toFixed(2)}%`;
        if (delta !== null && delta !== 0) {
          deltaCell.className = delta < 0 ? "delta--better" : "delta--worse";
        }
        row.appendChild(deltaCell);
        elements.resultsBody.appendChild(row);
      }
    }

    function renderAll() {
      renderHeader();
      renderPlatformTabs();
      renderCases();
      renderControls();
      renderCaseHeading();
      renderChart();
      renderTable();
      updateLocation(state);
    }

    elements.caseSearch.addEventListener("input", (event) => {
      state.filter = event.target.value;
      renderCases();
    });
    elements.mobileCaseSelect.addEventListener("change", (event) => selectCase(event.target.value));
    elements.metricSelect.addEventListener("change", (event) => {
      state.metric = event.target.value;
      state.parameter = "";
      renderAll();
    });
    elements.parameterSelect.addEventListener("change", (event) => {
      state.parameter = event.target.value;
      renderAll();
    });
    for (const button of document.querySelectorAll("[data-view]")) {
      button.addEventListener("click", () => {
        state.view = button.dataset.view;
        renderAll();
      });
    }
    for (const button of document.querySelectorAll("[data-scale]")) {
      button.addEventListener("click", () => {
        state.scale = button.dataset.scale;
        renderAll();
      });
    }
    document.getElementById("download-button").addEventListener("click", () => {
      const blob = new Blob([JSON.stringify(data, null, 2)], { type: "application/json" });
      const url = URL.createObjectURL(blob);
      const link = document.createElement("a");
      link.href = url;
      link.download = "cpp-perf-benchmarks.json";
      link.click();
      URL.revokeObjectURL(url);
    });

    renderAll();
  }

  const exported = {
    splitAggregate,
    parseBenchmarkName,
    naturalCompare,
    buildCatalog,
    pointsForEntry,
  };
  if (typeof module !== "undefined" && module.exports) {
    module.exports = exported;
  }

  if (typeof window !== "undefined") {
    window.BenchmarkDashboard = exported;
    window.addEventListener("DOMContentLoaded", () => {
      if (!window.BENCHMARK_DATA) {
        document.getElementById("chart-empty").hidden = false;
        document.getElementById("chart-empty").textContent = "Benchmark data could not be loaded.";
        return;
      }
      createDashboard(window.BENCHMARK_DATA);
    });
  }
})();
