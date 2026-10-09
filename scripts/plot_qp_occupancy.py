#!/usr/bin/env python3
"""QP occupancy heat map and fixed-cadence GIF from a qp_example run's qp_samples.csv.gz.

The heat map bins every fixed-time sample, so each bin holds weighted QP-time
(QP·µs) spent there. The GIF draws one frame per sample time (the run's
qpSampleIntervalNs) over the requested window. Samples inside long Geant4 steps
are straight-line interpolations between step endpoints.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.animation as animation
import matplotlib.colors as colors
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

sys.path.insert(0, str(Path(__file__).resolve().parent))
from plot_qp_sample_paths import DEFAULT_GEOMETRY, draw_geometry  # noqa: E402

INK, MUTED = "#1f1f1e", "#6b6a64"
QP_COLOR = "#2a78d6"
CHIP_HALF_MM = 4.0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run_dir", type=Path)
    parser.add_argument("--geometry", type=Path, default=DEFAULT_GEOMETRY)
    parser.add_argument("--bin-um", type=float, default=20.0, help="heat-map bin size")
    parser.add_argument("--gif-start-us", type=float, default=0.0)
    parser.add_argument("--gif-end-us", type=float, help="default: last sample time")
    parser.add_argument("--frame-us", type=float, help="frame spacing; default: every sample")
    parser.add_argument("--fps", type=int, default=25)
    parser.add_argument("--dpi", type=int, default=80)
    parser.add_argument("--skip-heatmap", action="store_true")
    return parser.parse_args()


def setup_axis(ax, geometry: Path, title: str) -> None:
    ax.set_facecolor("#fcfcfb")
    if geometry.exists():
        draw_geometry(ax, geometry)
    ax.set_xlim(-CHIP_HALF_MM, CHIP_HALF_MM)
    ax.set_ylim(-CHIP_HALF_MM, CHIP_HALF_MM)
    ax.set_aspect("equal")
    ax.set_xlabel("x (mm)", color=INK)
    ax.set_ylabel("y (mm)", color=INK)
    ax.tick_params(colors=MUTED)
    ax.set_title(title, color=INK, loc="left", fontsize=10)


def heat_map(samples: pd.DataFrame, interval_ns: float, args, out: Path) -> None:
    edges = np.arange(-CHIP_HALF_MM, CHIP_HALF_MM + 1e-9, args.bin_um / 1000.0)
    qp_us = samples.weight * interval_ns / 1000.0
    hist, _, _ = np.histogram2d(samples.x_mm, samples.y_mm, bins=[edges, edges], weights=qp_us)
    fig, ax = plt.subplots(figsize=(7.6, 6.6), constrained_layout=True)
    setup_axis(ax, args.geometry,
               f"Where QPs spent time — {args.run_dir.name} "
               f"({samples.track.nunique()} QPs, {args.bin_um:.0f} µm bins)")
    masked = np.ma.masked_less_equal(hist.T, 0.0)
    image = ax.imshow(masked, origin="lower", extent=(edges[0], edges[-1], edges[0], edges[-1]),
                      cmap=colors.LinearSegmentedColormap.from_list(
                          "blues_from_mid", plt.get_cmap("Blues")(np.linspace(0.3, 1.0, 256))),
                      norm=colors.LogNorm(vmin=masked.min(), vmax=masked.max()),
                      interpolation="nearest", zorder=2, alpha=0.95)
    fig.colorbar(image, ax=ax, label="weighted QP time per bin (QP·µs)", shrink=0.85)
    fig.savefig(out, dpi=160, facecolor="white")
    plt.close(fig)


def geometry_image(geometry: Path, dpi: int) -> np.ndarray:
    """Rasterize the chip layout once so GIF frames only redraw the QPs."""
    fig = plt.figure(figsize=(6, 6), dpi=dpi * 2)
    ax = fig.add_axes((0, 0, 1, 1))
    ax.axis("off")
    if geometry.exists():
        draw_geometry(ax, geometry)
    ax.set_xlim(-CHIP_HALF_MM, CHIP_HALF_MM)
    ax.set_ylim(-CHIP_HALF_MM, CHIP_HALF_MM)
    fig.canvas.draw()
    image = np.asarray(fig.canvas.buffer_rgba()).copy()
    plt.close(fig)
    return image


def gif(samples: pd.DataFrame, interval_ns: float, args, out: Path) -> int:
    times = np.sort(samples.time_ns.unique())
    end_ns = args.gif_end_us * 1000.0 if args.gif_end_us is not None else times[-1]
    times = times[(times >= args.gif_start_us * 1000.0) & (times <= end_ns)]
    # Keep sample times on the frame grid; frames must be a multiple of the sample interval.
    frame_ns = args.frame_us * 1000.0 if args.frame_us else interval_ns
    times = times[np.isclose(np.mod(times + 0.5 * interval_ns, frame_ns), 0.5 * interval_ns)]
    groups = {t: g[["x_mm", "y_mm"]].to_numpy() for t, g in samples.groupby("time_ns")}

    fig, ax = plt.subplots(figsize=(6.4, 6.4), constrained_layout=True)
    setup_axis(ax, Path("/nonexistent"), f"QPs every {frame_ns / 1000.0:g} µs — {args.run_dir.name}")
    ax.imshow(geometry_image(args.geometry, args.dpi), zorder=1, interpolation="antialiased",
              extent=(-CHIP_HALF_MM, CHIP_HALF_MM, -CHIP_HALF_MM, CHIP_HALF_MM))
    dots = ax.scatter([], [], s=9, color=QP_COLOR, edgecolor="white", linewidth=0.3, zorder=3)
    label = ax.text(0.02, 0.98, "", transform=ax.transAxes, ha="left", va="top", color=INK,
                    fontsize=9, bbox={"boxstyle": "round", "facecolor": "white", "alpha": 0.85,
                                      "edgecolor": MUTED})

    def update(i: int):
        pts = groups.get(times[i], np.empty((0, 2)))
        dots.set_offsets(pts)
        label.set_text(f"t = {times[i] / 1000.0:8.1f} µs\nactive QPs = {len(pts)}")
        return dots, label

    movie = animation.FuncAnimation(fig, update, frames=len(times), blit=True)
    movie.save(out, writer=animation.PillowWriter(fps=args.fps), dpi=args.dpi)
    plt.close(fig)
    return len(times)


def main() -> None:
    args = parse_args()
    # Read only the plotted columns with compact types; large runs have >10^7 rows.
    all_rows = pd.read_csv(args.run_dir / "qp_samples.csv.gz",
                           usecols=["track", "record_kind", "time_ns", "x_mm", "y_mm", "weight"],
                           dtype={"track": "int32", "record_kind": "category", "time_ns": "float64",
                                  "x_mm": "float32", "y_mm": "float32", "weight": "float32"})
    samples = all_rows[all_rows.record_kind == "sample"]
    del all_rows
    interval_ns = float(samples.sort_values(["track", "time_ns"]).groupby("track")
                        .time_ns.diff().mode().iloc[0])
    heat_path = args.run_dir / "qp_occupancy_heatmap.png"
    if not args.skip_heatmap:
        heat_map(samples, interval_ns, args, heat_path)
        print(heat_path)
    end = f"{args.gif_end_us:g}" if args.gif_end_us is not None else "end"
    step = f"_every{args.frame_us:g}us" if args.frame_us else ""
    gif_path = args.run_dir / f"qp_positions_{args.gif_start_us:g}-{end}us{step}.gif"
    frames = gif(samples, interval_ns, args, gif_path)
    print(gif_path, f"{frames} frames")


if __name__ == "__main__":
    main()
