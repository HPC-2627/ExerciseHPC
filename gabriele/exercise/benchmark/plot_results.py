#!/usr/bin/env python3
"""
Parses the output of benchmark.c runs (one .out file per PBS placement
strategy, as produced by run_benchmark_placements.sh) and plots
bandwidth (MB/s) vs. message size (bytes, log scale) for all strategies
on a single chart.

Usage:
    python3 plot_results.py [benchmark_pack.out benchmark_scatter.out ...]

With no arguments, it auto-discovers benchmark_*.out in the current directory.
"""

import glob
import re
import sys

import matplotlib.pyplot as plt
import matplotlib.ticker as ticker


def human_bytes(n, _pos=None):
    for unit, size in (("GB", 1 << 30), ("MB", 1 << 20), ("KB", 1 << 10)):
        if n >= size and n % size == 0:
            return f"{n // size}{unit}"
    return str(n)

LINE_RE = re.compile(r"^\s*(\d+)\s+([\d.]+)\s+([\d.]+)\s*$")


def parse_file(path):
    sizes, rates = [], []
    with open(path) as f:
        for line in f:
            m = LINE_RE.match(line)
            if m:
                sizes.append(int(m.group(1)))
                rates.append(float(m.group(3)))
    return sizes, rates


def strategy_label(path):
    name = path.split("/")[-1]
    name = name.removeprefix("benchmark_").removesuffix(".out")
    return name.replace("_", ":")


def main():
    files = sys.argv[1:] if len(sys.argv) > 1 else sorted(glob.glob("benchmark_*.out"))
    if not files:
        print("No benchmark_*.out files found. Pass paths explicitly or run from the job output dir.")
        sys.exit(1)

    plt.figure(figsize=(8, 5))
    for path in files:
        sizes, rates = parse_file(path)
        if not sizes:
            print(f"Warning: no data parsed from {path}")
            continue
        plt.plot(sizes, rates, marker="o", label=strategy_label(path))

    ax = plt.gca()
    ax.set_xscale("log", base=2)
    ax.set_yscale("log", base=10)
    ax.xaxis.set_major_formatter(ticker.FuncFormatter(human_bytes))
    ax.xaxis.set_minor_formatter(ticker.NullFormatter())
    plt.xlabel("Message size")
    plt.ylabel("Bandwidth (MB/s)")
    plt.title("MPI_Send/MPI_Recv bandwidth vs. message size")
    plt.legend(title="PBS place")
    plt.grid(True, which="both", linestyle="--", alpha=0.5)
    plt.tight_layout()

    out_path = "bandwidth_comparison.png"
    plt.savefig(out_path, dpi=150)
    print(f"Saved {out_path}")


if __name__ == "__main__":
    main()
