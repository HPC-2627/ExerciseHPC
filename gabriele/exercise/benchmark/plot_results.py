#!/usr/bin/env python3
"""
Plots for the P2P ping-pong benchmark (benchmark.c). Reads one benchmark_*.out
file per PBS placement strategy (as produced by run_benchmark_placements.sh;
non-CSV lines such as the UCX wrapper messages are ignored).

Usage:
    python3 plot_results.py [benchmark_pack.out ...] [--split 65536] [--outdir .]

Produces (PNG, in --outdir):
  1_bandwidth.png         MB/s vs. message size, X axis log scale
  2_bandwidth_loglog.png  same, log-log (shows the latency-dominated region)
  3_time_model.png        one-way time vs. size (error bars = std dev) + piecewise
                          fit T(n) = alpha + n/beta
  4_latency_zoom.png      small messages only (latency), Y axis log
  5_<placement>_bw.png    one bandwidth plot per placement: mean +/- std band and peak
and prints a table with alpha (latency), beta (asymptotic bandwidth), n_1/2.
"""

import argparse
import glob
import os
import re
import sys

import matplotlib
matplotlib.use("Agg")   # no GUI backend: only saves to file (suitable for the cluster)
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FuncFormatter, LogLocator, NullLocator

ROW_RE = re.compile(r"^\d+(,[-+\d.eE]+){7}$")   # n_bytes + 7 numeric CSV columns


def parse_file(path):
    """Reads one benchmark_*.out file. Returns dict of numpy arrays, one per CSV column."""
    rows = []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if ROW_RE.match(line):
                rows.append([float(x) for x in line.split(",")])
    a = np.array(rows).reshape(-1, 8)
    return dict(n=a[:, 0], reps=a[:, 1], t_mean=a[:, 2], t_std=a[:, 3], t_min=a[:, 4],
                t_med=a[:, 5], bw=a[:, 6], bw_peak=a[:, 7])


def time_err(d, m=slice(None)):
    """Asymmetric error bars [below, above] for the mean time: above = std dev;
    below = std dev but never under the measured minimum time (with outliers the
    std can exceed the mean -> negative bar, invisible on a log scale)."""
    mean, std, tmin = d["t_mean"][m], d["t_std"][m], d["t_min"][m]
    return np.vstack([mean - np.maximum(mean - std, tmin), std])


def strategy_label(path):
    """benchmark_scatter_excl.out -> scatter_excl"""
    name = os.path.basename(path)
    return name.removeprefix("benchmark_").removesuffix(".out")


def fit_model(n, t):
    """Weighted least squares (relative error) of T = alpha + n*(1/beta).
    Returns alpha [s] (latency) and beta [B/s] (asymptotic bandwidth)."""
    slope, alpha = np.polyfit(n, t, 1, w=1.0 / t)
    return alpha, 1.0 / slope


def size_fmt(x, _pos=None):
    """Formats a byte count with the most suitable unit (B, KiB, MiB, GiB)."""
    if x < 1:
        return ""
    x = int(round(x))
    for unit, f in (("GiB", 2**30), ("MiB", 2**20), ("KiB", 2**10)):
        if x >= f:
            return f"{x // f}{unit}"
    return f"{x}B"


def style_x(ax):
    """Common X axis style: log base 2, one tick per power of 2, readable units, grid."""
    ax.set_xscale("log", base=2)
    ax.xaxis.set_major_locator(LogLocator(base=2, numticks=64))
    ax.xaxis.set_minor_locator(NullLocator())
    ax.xaxis.set_major_formatter(FuncFormatter(size_fmt))
    ax.tick_params(axis="x", rotation=60)
    ax.grid(True, which="both", alpha=0.3)
    ax.set_xlabel("Message size")


def series_style(i, lab):
    """Different marker per series, dashed line for 'excl' placements so
    overlapping curves (e.g. scatter and scatter_excl) stay distinguishable."""
    color = plt.rcParams["axes.prop_cycle"].by_key()["color"][i % 10]
    marker = "osD^"[i % 4]
    ls = "--" if "excl" in lab else "-"
    return dict(color=color, marker=marker, ls=ls, mfc="none" if "excl" in lab else color)


def save(fig, outdir, name):
    fig.tight_layout()
    fig.savefig(os.path.join(outdir, name), dpi=150)
    plt.close(fig)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="*")
    ap.add_argument("--split", type=int, default=65536,
                    help="piecewise-fit threshold (bytes): two models for n < split and "
                         "n >= split (eager vs rendezvous); 0 = single model")
    ap.add_argument("--outdir", default=".")
    args = ap.parse_args()

    files = args.files or sorted(glob.glob("benchmark_*.out"))
    if not files:
        sys.exit("No benchmark_*.out files found. Pass paths explicitly or run from the job output dir.")
    os.makedirs(args.outdir, exist_ok=True)

    data = {}
    for path in files:
        d = parse_file(path)
        if len(d["n"]) == 0:
            print(f"Warning: no data parsed from {path}")
            continue
        data[strategy_label(path)] = d

    # ---- 1) bandwidth, X axis log
    fig, ax = plt.subplots(figsize=(9, 5.5))
    for i, (lab, d) in enumerate(data.items()):
        ax.plot(d["n"], d["bw"], ms=5, label=lab, **series_style(i, lab))
    style_x(ax)
    ax.set_ylabel("Bandwidth (MB/s)")
    ax.set_title("P2P bandwidth (MPI_Send/MPI_Recv)")
    ax.legend(title="PBS place")
    save(fig, args.outdir, "1_bandwidth.png")

    # ---- 2) bandwidth, log-log
    fig, ax = plt.subplots(figsize=(9, 5.5))
    for i, (lab, d) in enumerate(data.items()):
        ax.plot(d["n"], d["bw"], ms=5, label=lab, **series_style(i, lab))
    style_x(ax)
    ax.set_yscale("log")
    ax.set_ylabel("Bandwidth (MB/s)")
    ax.set_title("Bandwidth, log-log")
    ax.legend(title="PBS place")
    save(fig, args.outdir, "2_bandwidth_loglog.png")

    # ---- 3) time + (piecewise) model fit T(n) = alpha + n/beta, and parameter table
    fig, ax = plt.subplots(figsize=(9, 5.5))
    print(f"{'placement':<24}{'alpha (us)':>12}{'beta (MB/s)':>14}{'n_1/2 (B)':>12}")
    for i, (lab, d) in enumerate(data.items()):
        st = series_style(i, lab)
        ax.errorbar(d["n"], d["t_mean"] * 1e6, yerr=time_err(d) * 1e6, fmt=st["marker"], ms=4,
                    capsize=2, color=st["color"], mfc=st["mfc"], label=lab)
        if args.split > 0:
            parts = [(d["n"] < args.split, f"n<{args.split}"), (d["n"] >= args.split, f"n>={args.split}")]
        else:
            parts = [(np.ones_like(d["n"], dtype=bool), "all")]
        for m, tag in parts:
            if m.sum() < 2:
                continue
            a, b = fit_model(d["n"][m], d["t_mean"][m])
            xs = np.logspace(np.log2(d["n"][m].min()), np.log2(d["n"][m].max()), 100, base=2)
            ax.plot(xs, (a + xs / b) * 1e6, ls=st["ls"], color=st["color"], alpha=0.7)
            # n_1/2 = alpha*beta: size at which half of the asymptotic bandwidth is reached
            print(f"{lab + ' (' + tag + ')':<24}{a * 1e6:>12.2f}{b / 1e6:>14.1f}{a * b:>12.0f}")
    if args.split > 0:
        ax.axvline(args.split, color="grey", ls=":", lw=1)
    style_x(ax)
    ax.set_yscale("log")
    ax.set_ylabel("One-way time (us)")
    ax.set_title("Time vs size and model  T(n) = alpha + n/beta")
    ax.legend(title="PBS place")
    save(fig, args.outdir, "3_time_model.png")

    # ---- 4) latency zoom (n <= 1 KiB), Y axis log: pack (~0.3 us) and scatter (~20 us)
    #         would otherwise not fit one linear plot
    fig, ax = plt.subplots(figsize=(9, 5.5))
    for i, (lab, d) in enumerate(data.items()):
        m = d["n"] <= 1024
        st = series_style(i, lab)
        ax.errorbar(d["n"][m], d["t_mean"][m] * 1e6, yerr=time_err(d, m) * 1e6, ms=5, capsize=2,
                    marker=st["marker"], ls=st["ls"], color=st["color"], mfc=st["mfc"], label=lab)
    style_x(ax)
    ax.set_yscale("log")
    ax.set_ylabel("One-way time (us)")
    ax.set_title("Latency (small messages)")
    ax.legend(title="PBS place")
    save(fig, args.outdir, "4_latency_zoom.png")

    # ---- 5) per placement: mean +/- std band and peak
    for lab, d in data.items():
        fig, ax = plt.subplots(figsize=(9, 5.5))
        ax.plot(d["n"], d["bw"], "o-", ms=4, label="mean")
        ax.plot(d["n"], d["bw_peak"], "s--", ms=3, alpha=0.7, label="peak (min time)")
        # band = bandwidth for mean time +/- 1 std; lower time is clipped at the measured
        # minimum, so the band never exceeds the peak bandwidth
        lo = d["n"] / 1e6 / (d["t_mean"] + d["t_std"])
        hi = d["n"] / 1e6 / np.maximum(d["t_mean"] - d["t_std"], d["t_min"])
        ax.fill_between(d["n"], lo, hi, alpha=0.2, label="mean +/- 1 std (time)")
        style_x(ax)
        ax.set_ylabel("Bandwidth (MB/s)")
        ax.set_title(f"Placement: {lab}")
        ax.legend()
        save(fig, args.outdir, f"5_{lab}_bw.png")

    print(f"\nPlots saved in {args.outdir}/")


if __name__ == "__main__":
    main()
