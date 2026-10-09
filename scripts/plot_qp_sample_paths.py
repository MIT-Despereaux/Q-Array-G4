#!/usr/bin/env python3
"""Plot every logged QP position from a qp_example run's qp_samples.csv.gz."""

from __future__ import annotations

import argparse
import gzip
import json
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from matplotlib.collections import PolyCollection

# Validated categorical order (fixed, never cycled); tracks beyond 8 fold into gray.
SERIES = ["#2a78d6", "#eb6834", "#1baf7a", "#eda100",
          "#e87ba4", "#008300", "#4a3aa7", "#e34948"]
OTHER = "#8a8a85"
INK, MUTED, GRID = "#1f1f1e", "#6b6a64", "#e4e3dd"
SAMPLE_NS = 100.0  # overwritten from the data in main()
INTERP_NS = 1000.0
CHIP_HALF_MM = 4.0  # Stock 8 mm x 8 mm chip and ground plane, centered on the origin.


DEFAULT_GEOMETRY = (Path(__file__).resolve().parents[1]
                    / "output/qp-example/geometry-dump-al250/geometry_top_faces.jsonl")
# Film layer: every Al volume (ground plane, resonator bases, conductors) is the
# same continuous film; G4_AIR daughters are the etched gaps.
FILM, GAP, GAP_EDGE = "#e2e1da", "#ffffff", "#9a998f"


def draw_geometry(ax, path: Path) -> None:
    rows = [json.loads(line) for line in path.open()]
    film = [r for r in rows if 4.999 < r["z_mm"] < 5.001 and r["material"] in ("G4_Al", "G4_AIR")]
    film.sort(key=lambda r: r["depth"])
    for r in film:
        gap = r["material"] == "G4_AIR"
        ax.add_collection(PolyCollection([r["polygon"]], facecolors=GAP if gap else FILM,
                                         edgecolors=GAP_EDGE if gap else "none",
                                         linewidths=0.3, zorder=1))


def load(run_dir: Path) -> tuple[pd.DataFrame, pd.DataFrame]:
    with gzip.open(run_dir / "qp_samples.csv.gz", "rt") as f:
        samples = pd.read_csv(f)
    with gzip.open(run_dir / "steps.csv.gz", "rt") as f:
        steps = pd.read_csv(f)
    return samples, steps


def track_info(samples: pd.DataFrame, steps: pd.DataFrame) -> pd.DataFrame:
    rows = []
    for track, g in samples.groupby("track"):
        birth = g[g.record_kind == "birth"].iloc[0]
        end = g[g.record_kind == "terminal"]
        last = steps[(steps.track == track) & (steps.particle == "BogoliubovQP")]
        rows.append({
            "track": track,
            "life_us": (end.time_ns.iloc[0] - birth.time_ns) / 1e3 if len(end) else float("nan"),
            "process": last.process.iloc[-1] if len(last) else "running",
            "geant4_steps": int(end.source_step.iloc[0]) if len(end) else -1,
        })
    return pd.DataFrame(rows).set_index("track")


def draw(ax, samples, colors, zoom: bool):
    for track, g in samples.groupby("track"):
        g = g.sort_values("time_ns")
        c = colors[track]
        # Samples inside one Geant4 step lasting >= INTERP_NS are straight-line
        # interpolations between that step's endpoints, not resolved positions.
        per_step = g.groupby("source_step").time_ns.transform("count")
        interp = (per_step * SAMPLE_NS >= INTERP_NS).to_numpy()
        x, y = g.x_mm.to_numpy(), g.y_mm.to_numpy()
        ax.plot(x, y, color=c, lw=0.9, ls=(0, (3, 3)), alpha=0.7, zorder=2)
        keep = ~(interp | np.roll(interp, 1))
        ax.plot(np.where(keep, x, np.nan), np.where(keep, y, np.nan),
                color=c, lw=1.0 if zoom else 0.8, zorder=3)
        b, e = g.iloc[0], g.iloc[-1]
        ax.scatter([b.x_mm], [b.y_mm], s=36, marker="o", facecolor="white",
                   edgecolor=c, linewidth=1.6, zorder=4)
        ax.scatter([e.x_mm], [e.y_mm], s=44, marker="X", color=c,
                   edgecolor="white", linewidth=0.8, zorder=5)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run_dir", type=Path)
    parser.add_argument("-o", "--output", type=Path)
    parser.add_argument("--geometry", type=Path, default=DEFAULT_GEOMETRY,
                        help="geometry_top_faces.jsonl from run_qp_example_smoke.sh geometry-dump-al250")
    args = parser.parse_args()

    samples, steps = load(args.run_dir)
    info = track_info(samples, steps)
    tracks = sorted(samples.track.unique())
    global SAMPLE_NS
    sampled = samples[samples.record_kind == "sample"].sort_values(["track", "time_ns"])
    SAMPLE_NS = float(sampled.groupby("track").time_ns.diff().mode().iloc[0])
    colors = {t: SERIES[i] if i < len(SERIES) else OTHER for i, t in enumerate(tracks)}

    plt.rcParams.update({"font.size": 9, "axes.edgecolor": MUTED, "axes.labelcolor": INK,
                         "xtick.color": MUTED, "ytick.color": MUTED})
    fig, (full, zoom) = plt.subplots(1, 2, figsize=(14, 5.6),
                                     gridspec_kw={"width_ratios": [1, 1.35]})
    has_geometry = args.geometry.exists()
    for ax in (full, zoom):
        ax.set_facecolor("#fcfcfb")
        ax.grid(color=GRID, lw=0.6, zorder=0)
        if has_geometry:
            draw_geometry(ax, args.geometry)
        ax.set_aspect("equal")
        ax.set_xlabel("x (mm)")
        ax.set_ylabel("y (mm)")
        for s in ax.spines.values():
            s.set_linewidth(0.6)

    chip = plt.Rectangle((-CHIP_HALF_MM, -CHIP_HALF_MM), 2 * CHIP_HALF_MM, 2 * CHIP_HALF_MM,
                         fill=False, ec=MUTED, lw=1.0, ls="--", zorder=1)
    full.add_patch(chip)
    full.set_xlim(-4.3, 4.3)
    full.set_ylim(-4.3, 4.3)
    full.set_title("Whole chip (8 mm × 8 mm ground plane, dashed)", color=INK, loc="left")
    draw(full, samples, colors, zoom=False)

    pad = 0.15
    x0, x1 = samples.x_mm.min() - pad, samples.x_mm.max() + pad
    y0, y1 = samples.y_mm.min() - pad, samples.y_mm.max() + pad
    zoom.set_xlim(x0, x1)
    zoom.set_ylim(y0, y1)
    zoom.set_title("Zoom on the QP region", color=INK, loc="left")
    draw(zoom, samples, colors, zoom=True)
    full.add_patch(plt.Rectangle((x0, y0), x1 - x0, y1 - y0, fill=False, ec=INK, lw=0.8, zorder=3))

    # Legend doubles as the per-track direct label (identity is never color-alone).
    handles = []
    for t in tracks:
        r = info.loc[t]
        proc = {"qpRecombination": "recombined", "qpLocalTrapping": "trapped"}.get(r.process, r.process)
        label = f"QP {t}: {r.life_us:.0f} µs, {proc}, {r.geant4_steps:,} steps"
        handles.append(plt.Line2D([], [], color=colors[t], lw=1.6, label=label))
    if has_geometry:
        handles += [plt.Rectangle((0, 0), 1, 1, fc=FILM, ec="none", label="250 nm Al film"),
                    plt.Rectangle((0, 0), 1, 1, fc=GAP, ec=GAP_EDGE, lw=0.5, label="etched gap (no film)")]
    handles += [plt.Line2D([], [], color=INK, lw=1.2, label="resolved diffusion"),
                plt.Line2D([], [], color=INK, lw=1.0, ls=(0, (3, 3)),
                           label=f"interpolated inside one Geant4 step ≥ {INTERP_NS/1e3:.0f} µs"),
                plt.Line2D([], [], ls="", marker="o", mfc="white", mec=INK, label="birth"),
                plt.Line2D([], [], ls="", marker="X", color=INK, label="end")]
    zoom.legend(handles=handles, loc="upper left", bbox_to_anchor=(1.02, 1.0), frameon=False,
                fontsize=8.5, labelcolor=INK)

    n = len(samples)
    fig.suptitle(f"Logged QP positions — {args.run_dir.name} ({len(tracks)} QPs, {n:,} rows, "
                 f"{SAMPLE_NS:.0f} ns sampling)", x=0.01, ha="left", color=INK, fontsize=11)
    fig.tight_layout()
    out = args.output or args.run_dir / "qp_sample_paths.png"
    fig.savefig(out, dpi=160, facecolor="white")
    print(out)


if __name__ == "__main__":
    main()
