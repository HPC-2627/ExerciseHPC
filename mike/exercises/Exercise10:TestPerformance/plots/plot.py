#!/usr/bin/env python3
"""
Grafici per il benchmark P2P (HPC4DS, Lezione 12).

Uso:  python3 plot.py [csv/results_*.csv ...] [--split 65536] [--outdir .]
Default: legge ../csv/results_*.csv e scrive i PNG accanto a questo script.

Produce (PNG, in --outdir):
  1_bandwidth.png         MB/s in funzione della dimensione, asse X in scala log   (il grafico richiesto dalle slide)
  2_bandwidth_loglog.png  come sopra, ma log-log (mostra la zona dominata dalla latenza)
  3_time_model.png        tempo di sola andata vs dimensione (barre di errore = dev. std) + fit a tratti T(n) = alpha + n/beta
  4_latency_zoom.png      solo messaggi piccoli (latenza), asse Y log
  5_<placement>_bw.png    un grafico di banda per ogni posizionamento, con fascia media +/- dev. std e picco (tempo minimo)
e stampa una tabella con alpha (latenza), beta (banda asintotica), n_1/2 per ciascun tratto del fit.
"""
import sys, glob, os, argparse
import numpy as np
import matplotlib
matplotlib.use("Agg")   # backend senza finestre: salva solo su file (adatto al cluster)
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter, LogLocator, NullLocator


def load(path):
    """Legge un CSV prodotto dal benchmark.
    Restituisce (meta, d): meta = dizionario dei metadati dalle righe '# chiave=valore',
    d = dizionario di array numpy, uno per colonna del CSV."""
    meta, rows = {}, []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if line.startswith("#"):
                # riga di metadati: la scompone in coppie chiave=valore
                for tok in line[1:].split():
                    if "=" in tok:
                        k, v = tok.split("=", 1)
                        meta[k] = v
            elif line and not line.startswith("n_bytes"):
                # riga di dati (si salta l'intestazione delle colonne)
                rows.append([float(x) for x in line.split(",")])
    a = np.array(rows)
    d = dict(n=a[:, 0], reps=a[:, 1], t_mean=a[:, 2], t_std=a[:, 3], t_min=a[:, 4],
             t_med=a[:, 5], bw=a[:, 6], bw_peak=a[:, 7])
    return meta, d


def fit_model(n, t):
    """Minimi quadrati pesati (errore relativo) del modello T = alpha + n*(1/beta).
    Restituisce alpha [s] (latenza) e beta [B/s] (banda asintotica)."""
    slope, alpha = np.polyfit(n, t, 1, w=1.0 / t)
    return alpha, 1.0 / slope


def time_err(d, m=slice(None)):
    """Barre di errore asimmetriche [sotto, sopra] per il tempo medio:
    sopra = dev. std; sotto = dev. std ma senza scendere sotto il tempo minimo misurato
    (con outlier la dev. std puo' superare la media -> barra negativa, invisibile in scala log)."""
    mean, std, tmin = d["t_mean"][m], d["t_std"][m], d["t_min"][m]
    lower = mean - np.maximum(mean - std, tmin)
    return np.vstack([lower, std])


def size_fmt(x, _):
    """Formatta un valore in byte con l'unita' piu' adatta (B, KiB, MiB, GiB) per le etichette dell'asse X."""
    if x < 1:
        return ""
    x = int(round(x))
    for unit, f in (("GiB", 2**30), ("MiB", 2**20), ("KiB", 2**10)):
        if x >= f:
            return f"{x // f}{unit}"
    return f"{x}B"


def style_x(ax):
    """Applica all'asse X lo stile comune: scala log in base 2, un tick per ogni potenza di 2
    (cosi' compare anche 1B), etichette in unita' leggibili, griglia."""
    ax.set_xscale("log", base=2)
    ax.xaxis.set_major_locator(LogLocator(base=2, numticks=64))
    ax.xaxis.set_minor_locator(NullLocator())
    ax.xaxis.set_major_formatter(FuncFormatter(size_fmt))
    ax.tick_params(axis="x", rotation=60)
    ax.grid(True, which="both", alpha=0.3)
    ax.set_xlabel("Message size")


def series_style(i, lab):
    """Stile di ogni serie: marker diversi e linea tratteggiata per i posizionamenti 'excl',
    cosi' le curve sovrapposte (es. scatter e scatter_excl) restano distinguibili."""
    color = plt.rcParams["axes.prop_cycle"].by_key()["color"][i % 10]
    marker = "osD^"[i % 4]
    ls = "--" if "excl" in lab else "-"
    return dict(color=color, marker=marker, ls=ls, mfc="none" if "excl" in lab else color)


def main():
    # ---- Argomenti da riga di comando ----
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="*")
    ap.add_argument("--split", type=int, default=65536,
                    help="soglia (byte) del fit a tratti: due modelli per n < split e n >= split "
                         "(eager vs rendezvous); 0 = un solo modello su tutto l'intervallo")
    ap.add_argument("--outdir", default=os.path.dirname(os.path.abspath(__file__)))
    args = ap.parse_args()
    files = args.files or sorted(glob.glob(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "csv", "results_*.csv")))
    if not files:
        sys.exit("Nessun csv/results_*.csv trovato.")
    os.makedirs(args.outdir, exist_ok=True)

    # ---- Caricamento dati: un'etichetta per file, ricavata dal nome ----
    data = {}
    for f in files:
        label = os.path.basename(f)[len("results_"):-len(".csv")]   # es. scatter_excl
        data[label] = load(f)

    # ---- 1) banda, asse X log (richiesto dalle slide)
    fig, ax = plt.subplots(figsize=(9, 5.5))
    for i, (lab, (meta, d)) in enumerate(data.items()):
        where = "same node" if meta.get("same_node") == "yes" else "different nodes"
        ax.plot(d["n"], d["bw"], ms=5, label=f"{lab} ({where})", **series_style(i, lab))
    style_x(ax); ax.set_ylabel("Bandwidth (MB/s)"); ax.set_title("P2P bandwidth (MPI_Send/MPI_Recv)")
    ax.legend(); fig.tight_layout(); fig.savefig(f"{args.outdir}/1_bandwidth.png", dpi=150); plt.close(fig)

    # ---- 2) banda, log-log (asse Y in base 10, piu' leggibile delle potenze di 2)
    fig, ax = plt.subplots(figsize=(9, 5.5))
    for i, (lab, (_, d)) in enumerate(data.items()):
        ax.plot(d["n"], d["bw"], ms=5, label=lab, **series_style(i, lab))
    style_x(ax); ax.set_yscale("log")
    ax.set_ylabel("Bandwidth (MB/s)"); ax.set_title("Bandwidth, log-log")
    ax.legend(); fig.tight_layout(); fig.savefig(f"{args.outdir}/2_bandwidth_loglog.png", dpi=150); plt.close(fig)

    # ---- 3) tempo + fit del modello (a tratti se --split > 0) ; stampa della tabella
    fig, ax = plt.subplots(figsize=(9, 5.5))
    print(f"{'placement':<24}{'alpha (us)':>12}{'beta (MB/s)':>14}{'n_1/2 (B)':>12}")
    for i, (lab, (_, d)) in enumerate(data.items()):
        st = series_style(i, lab)
        # punti misurati con barre di errore asimmetriche (vedi time_err)
        ax.errorbar(d["n"], d["t_mean"] * 1e6, yerr=time_err(d) * 1e6, fmt=st["marker"], ms=4, capsize=2,
                    color=st["color"], mfc=st["mfc"], label=lab)
        # tratti del fit: [1, split) e [split, max], oppure tutto l'intervallo se split = 0
        if args.split > 0:
            parts = [(d["n"] < args.split, f"n<{args.split}"), (d["n"] >= args.split, f"n>={args.split}")]
        else:
            parts = [(np.ones_like(d["n"], dtype=bool), "all")]
        for m, tag in parts:
            if m.sum() < 2:
                continue
            # curva del modello T(n) = alpha + n/beta, disegnata solo nel suo tratto
            a, b = fit_model(d["n"][m], d["t_mean"][m])
            xs = np.logspace(np.log2(d["n"][m].min()), np.log2(d["n"][m].max()), 100, base=2)
            ax.plot(xs, (a + xs / b) * 1e6, ls=st["ls"], color=st["color"], alpha=0.7)
            # n_1/2 = alpha*beta: dimensione per cui si raggiunge meta' della banda asintotica
            print(f"{lab + ' (' + tag + ')':<24}{a * 1e6:>12.2f}{b / 1e6:>14.1f}{a * b:>12.0f}")
    if args.split > 0:
        ax.axvline(args.split, color="grey", ls=":", lw=1)
    style_x(ax); ax.set_yscale("log")
    ax.set_ylabel("One-way time (us)"); ax.set_title("Time vs size and model  T(n) = alpha + n/beta")
    ax.legend(); fig.tight_layout(); fig.savefig(f"{args.outdir}/3_time_model.png", dpi=150); plt.close(fig)

    # ---- 4) zoom sulla latenza (n <= 1 KiB), asse Y log: altrimenti pack (~0.3 us) e scatter (~20 us)
    #         non stanno nello stesso grafico e le curve pack restano schiacciate sullo zero
    fig, ax = plt.subplots(figsize=(9, 5.5))
    for i, (lab, (_, d)) in enumerate(data.items()):
        m = d["n"] <= 1024
        st = series_style(i, lab)
        ax.errorbar(d["n"][m], d["t_mean"][m] * 1e6, yerr=time_err(d, m) * 1e6, ms=5, capsize=2,
                    marker=st["marker"], ls=st["ls"], color=st["color"], mfc=st["mfc"], label=lab)
    style_x(ax); ax.set_yscale("log")
    ax.set_ylabel("One-way time (us)"); ax.set_title("Latency (small messages)")
    ax.legend(); fig.tight_layout(); fig.savefig(f"{args.outdir}/4_latency_zoom.png", dpi=150); plt.close(fig)

    # ---- 5) per ogni posizionamento: media +/- dev. std e picco
    for lab, (_, d) in data.items():
        fig, ax = plt.subplots(figsize=(9, 5.5))
        ax.plot(d["n"], d["bw"], "o-", ms=4, label="mean")
        ax.plot(d["n"], d["bw_peak"], "s--", ms=3, alpha=0.7, label="peak (min time)")
        # fascia di banda corrispondente a tempo medio +/- 1 dev. std; il tempo inferiore non scende
        # sotto il minimo misurato, quindi la fascia non supera mai la banda di picco
        lo = d["n"] / 1e6 / (d["t_mean"] + d["t_std"])
        hi = d["n"] / 1e6 / np.maximum(d["t_mean"] - d["t_std"], d["t_min"])
        ax.fill_between(d["n"], lo, hi, alpha=0.2, label="mean +/- 1 std (time)")
        style_x(ax); ax.set_ylabel("Bandwidth (MB/s)"); ax.set_title(f"Placement: {lab}")
        ax.legend(); fig.tight_layout()
        fig.savefig(f"{args.outdir}/5_{lab}_bw.png", dpi=150); plt.close(fig)

    print(f"\nGrafici salvati in {args.outdir}/")


if __name__ == "__main__":
    main()
