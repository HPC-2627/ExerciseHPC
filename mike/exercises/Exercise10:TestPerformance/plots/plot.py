#!/usr/bin/env python3
"""
Plots for the P2P benchmark (HPC4DS, Lecture 12).

Usage:  python3 plot.py [csv/results_*.csv ...] [--split 65536] [--outdir .]
Default: reads ../csv/results_*.csv and writes the PNGs next to this script.

Produces (PNG, in --outdir):
  1_bandwidth.png         MB/s vs size, X axis in log scale   (the plot requested by the slides)
  2_bandwidth_loglog.png  as above, but log-log (shows the latency-dominated region)
  3_time_model.png        one-way time vs size (error bars = std dev) + piecewise fit T(n) = alpha + n/beta
  4_latency_zoom.png      small messages only (latency), log Y axis
  5_<placement>_bw.png    one bandwidth plot per placement, with mean +/- std dev band and peak (minimum time)
and prints a table with alpha (latency), beta (asymptotic bandwidth), n_1/2 for each fit segment.
"""
import sys, glob, os, argparse
import numpy as np
import matplotlib
matplotlib.use("Agg")   # windowless backend: only saves to file (suitable for the cluster)
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter, LogLocator, NullLocator


def load(path):
    """Reads a CSV produced by the benchmark.
    Returns (meta, d): meta = dictionary of the metadata from the '# key=value' lines,
    d = dictionary of numpy arrays, one per CSV column."""
    meta, rows = {}, []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if line.startswith("#"):
                # metadata line: split it into key=value pairs
                for tok in line[1:].split():
                    if "=" in tok:
                        k, v = tok.split("=", 1)
                        meta[k] = v
            elif line and not line.startswith("n_bytes"):
                # data line (the column header is skipped)
                rows.append([float(x) for x in line.split(",")])
    a = np.array(rows)
    d = dict(n=a[:, 0], reps=a[:, 1], t_mean=a[:, 2], t_std=a[:, 3], t_min=a[:, 4],
             t_med=a[:, 5], bw=a[:, 6], bw_peak=a[:, 7])
    return meta, d


def fit_model(n, t):
    """Weighted least squares (relative error) of the model T = alpha + n*(1/beta).
    Returns alpha [s] (latency) and beta [B/s] (asymptotic bandwidth)."""
    slope, alpha = np.polyfit(n, t, 1, w=1.0 / t)
    return alpha, 1.0 / slope


def time_err(d, m=slice(None)):
    """Asymmetric error bars [below, above] for the mean time:
    above = std dev; below = std dev but without going under the minimum measured time
    (with outliers the std dev can exceed the mean -> negative bar, invisible on a log scale)."""
    mean, std, tmin = d["t_mean"][m], d["t_std"][m], d["t_min"][m]
    lower = mean - np.maximum(mean - std, tmin)
    return np.vstack([lower, std])


def size_fmt(x, _):
    """Formats a value in bytes with the most suitable unit (B, KiB, MiB, GiB) for the X axis labels."""
    if x < 1:
        return ""
    x = int(round(x))
    for unit, f in (("GiB", 2**30), ("MiB", 2**20), ("KiB", 2**10)):
        if x >= f:
            return f"{x // f}{unit}"
    return f"{x}B"


def style_x(ax):
    """Applies the common style to the X axis: log scale in base 2, one tick per power of 2
    (so 1B shows up too), labels in readable units, grid."""
    ax.set_xscale("log", base=2)
    ax.xaxis.set_major_locator(LogLocator(base=2, numticks=64))
    ax.xaxis.set_minor_locator(NullLocator())
    ax.xaxis.set_major_formatter(FuncFormatter(size_fmt))
    ax.tick_params(axis="x", rotation=60)
    ax.grid(True, which="both", alpha=0.3)
    ax.set_xlabel("Message size")


def series_style(i, lab):
    """Style of each series: different markers and dashed line for the 'excl' placements,
    so overlapping curves (e.g. scatter and scatter_excl) remain distinguishable."""
    color = plt.rcParams["axes.prop_cycle"].by_key()["color"][i % 10]
    marker = "osD^"[i % 4]
    ls = "--" if "excl" in lab else "-"
    return dict(color=color, marker=marker, ls=ls, mfc="none" if "excl" in lab else color)


def main():
    # ---- Command-line arguments ----
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="*")
    ap.add_argument("--split", type=int, default=65536,
                    help="threshold (bytes) of the piecewise fit: two models for n < split and n >= split "
                         "(eager vs rendezvous); 0 = a single model over the whole range")
    ap.add_argument("--outdir", default=os.path.dirname(os.path.abspath(__file__)))
    args = ap.parse_args()
    files = args.files or sorted(glob.glob(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "csv", "results_*.csv")))
    if not files:
        sys.exit("Nessun csv/results_*.csv trovato.")
    os.makedirs(args.outdir, exist_ok=True)

    # ---- Data loading: one label per file, derived from the name ----
    data = {}
    for f in files:
        label = os.path.basename(f)[len("results_"):-len(".csv")]   # es. scatter_excl
        data[label] = load(f)

    # ---- 1) bandwidth, log X axis (requested by the slides)
    fig, ax = plt.subplots(figsize=(9, 5.5))
    for i, (lab, (meta, d)) in enumerate(data.items()):
        where = "same node" if meta.get("same_node") == "yes" else "different nodes"
        ax.plot(d["n"], d["bw"], ms=5, label=f"{lab} ({where})", **series_style(i, lab))
    style_x(ax); ax.set_ylabel("Bandwidth (MB/s)"); ax.set_title("P2P bandwidth (MPI_Send/MPI_Recv)")
    ax.legend(); fig.tight_layout(); fig.savefig(f"{args.outdir}/1_bandwidth.png", dpi=150); plt.close(fig)

    # ---- 2) bandwidth, log-log (Y axis in base 10, more readable than powers of 2)
    fig, ax = plt.subplots(figsize=(9, 5.5))
    for i, (lab, (_, d)) in enumerate(data.items()):
        ax.plot(d["n"], d["bw"], ms=5, label=lab, **series_style(i, lab))
    style_x(ax); ax.set_yscale("log")
    ax.set_ylabel("Bandwidth (MB/s)"); ax.set_title("Bandwidth, log-log")
    ax.legend(); fig.tight_layout(); fig.savefig(f"{args.outdir}/2_bandwidth_loglog.png", dpi=150); plt.close(fig)

    # ---- 3) time + model fit (piecewise if --split > 0); table printing
    fig, ax = plt.subplots(figsize=(9, 5.5))
    print(f"{'placement':<24}{'alpha (us)':>12}{'beta (MB/s)':>14}{'n_1/2 (B)':>12}")
    for i, (lab, (_, d)) in enumerate(data.items()):
        st = series_style(i, lab)
        # measured points with asymmetric error bars (see time_err)
        ax.errorbar(d["n"], d["t_mean"] * 1e6, yerr=time_err(d) * 1e6, fmt=st["marker"], ms=4, capsize=2,
                    color=st["color"], mfc=st["mfc"], label=lab)
        # fit segments: [1, split) and [split, max], or the whole range if split = 0
        if args.split > 0:
            parts = [(d["n"] < args.split, f"n<{args.split}"), (d["n"] >= args.split, f"n>={args.split}")]
        else:
            parts = [(np.ones_like(d["n"], dtype=bool), "all")]
        for m, tag in parts:
            if m.sum() < 2:
                continue
            # model curve T(n) = alpha + n/beta, drawn only over its segment
            a, b = fit_model(d["n"][m], d["t_mean"][m])
            xs = np.logspace(np.log2(d["n"][m].min()), np.log2(d["n"][m].max()), 100, base=2)
            ax.plot(xs, (a + xs / b) * 1e6, ls=st["ls"], color=st["color"], alpha=0.7)
            # n_1/2 = alpha*beta: size at which half of the asymptotic bandwidth is reached
            print(f"{lab + ' (' + tag + ')':<24}{a * 1e6:>12.2f}{b / 1e6:>14.1f}{a * b:>12.0f}")
    if args.split > 0:
        ax.axvline(args.split, color="grey", ls=":", lw=1)
    style_x(ax); ax.set_yscale("log")
    ax.set_ylabel("One-way time (us)"); ax.set_title("Time vs size and model  T(n) = alpha + n/beta")
    ax.legend(); fig.tight_layout(); fig.savefig(f"{args.outdir}/3_time_model.png", dpi=150); plt.close(fig)

    # ---- 4) latency zoom (n <= 1 KiB), log Y axis: otherwise pack (~0.3 us) and scatter (~20 us)
    #         do not fit in the same plot and the pack curves are squashed against zero
    fig, ax = plt.subplots(figsize=(9, 5.5))
    for i, (lab, (_, d)) in enumerate(data.items()):
        m = d["n"] <= 1024
        st = series_style(i, lab)
        ax.errorbar(d["n"][m], d["t_mean"][m] * 1e6, yerr=time_err(d, m) * 1e6, ms=5, capsize=2,
                    marker=st["marker"], ls=st["ls"], color=st["color"], mfc=st["mfc"], label=lab)
    style_x(ax); ax.set_yscale("log")
    ax.set_ylabel("One-way time (us)"); ax.set_title("Latency (small messages)")
    ax.legend(); fig.tight_layout(); fig.savefig(f"{args.outdir}/4_latency_zoom.png", dpi=150); plt.close(fig)

    # ---- 5) for each placement: mean +/- std dev and peak
    for lab, (_, d) in data.items():
        fig, ax = plt.subplots(figsize=(9, 5.5))
        ax.plot(d["n"], d["bw"], "o-", ms=4, label="mean")
        ax.plot(d["n"], d["bw_peak"], "s--", ms=3, alpha=0.7, label="peak (min time)")
        # bandwidth band corresponding to mean time +/- 1 std dev; the lower time does not go
        # below the measured minimum, so the band never exceeds the peak bandwidth
        lo = d["n"] / 1e6 / (d["t_mean"] + d["t_std"])
        hi = d["n"] / 1e6 / np.maximum(d["t_mean"] - d["t_std"], d["t_min"])
        ax.fill_between(d["n"], lo, hi, alpha=0.2, label="mean +/- 1 std (time)")
        style_x(ax); ax.set_ylabel("Bandwidth (MB/s)"); ax.set_title(f"Placement: {lab}")
        ax.legend(); fig.tight_layout()
        fig.savefig(f"{args.outdir}/5_{lab}_bw.png", dpi=150); plt.close(fig)

    print(f"\nGrafici salvati in {args.outdir}/")


if __name__ == "__main__":
    main()
