#!/usr/bin/env python3
"""Animate QP density from fixed-time samples written by qp_example_smoke."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.animation as animation
import matplotlib.colors as colors
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


REQUIRED_COLUMNS = {
    "event",
    "track",
    "record_kind",
    "time_ns",
    "x_mm",
    "y_mm",
    "weight",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Animate fixed-time QP density samples")
    parser.add_argument("samples_csv", type=Path)
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument("--frames", type=int, default=72)
    parser.add_argument("--fps", type=int, default=8)
    parser.add_argument("--bins", type=int, default=80)
    parser.add_argument("--chip-half-width-mm", type=float, default=4.0)
    return parser.parse_args()


def load_samples(path: Path) -> pd.DataFrame:
    columns = pd.read_csv(path, nrows=0).columns
    missing = REQUIRED_COLUMNS.difference(columns)
    if missing:
        raise ValueError(f"{path} is missing columns: {', '.join(sorted(missing))}")
    chunks = []
    for chunk in pd.read_csv(path, usecols=sorted(REQUIRED_COLUMNS), chunksize=100_000):
        selected = chunk[chunk["record_kind"] == "sample"]
        if not selected.empty:
            chunks.append(selected)
    if not chunks:
        raise ValueError("No fixed-time QP sample rows found")
    return pd.concat(chunks, ignore_index=True)


def select_times(samples: pd.DataFrame, frame_count: int) -> np.ndarray:
    times = np.sort(samples["time_ns"].unique())
    if len(times) <= frame_count:
        return times
    indices = np.geomspace(1, len(times), frame_count)
    indices = np.unique(np.rint(indices).astype(int) - 1)
    if indices[-1] != len(times) - 1:
        indices = np.append(indices, len(times) - 1)
    return times[indices]


def make_density_frames(
    samples: pd.DataFrame, times: np.ndarray, bins: int, half_width: float
) -> tuple[list[np.ndarray], list[dict[str, float]]]:
    area = (2.0 * half_width / bins) ** 2
    densities = []
    statistics = []
    grouped = {time: frame for time, frame in samples.groupby("time_ns", sort=False)}
    for time in times:
        frame = grouped[time]
        density, _, _ = np.histogram2d(
            frame["x_mm"],
            frame["y_mm"],
            bins=bins,
            range=[[-half_width, half_width], [-half_width, half_width]],
            weights=frame["weight"],
        )
        density /= area
        densities.append(density)
        statistics.append(
            {
                "time_ns": float(time),
                "active_tracks": int(len(frame)),
                "weighted_active_tracks": float(frame["weight"].sum()),
                "peak_density_qp_per_mm2": float(density.max()),
            }
        )
    return densities, statistics


def format_time(time_ns: float) -> str:
    if time_ns < 1_000.0:
        return f"{time_ns:.0f} ns"
    if time_ns < 1_000_000.0:
        return f"{time_ns / 1_000:.2f} µs"
    return f"{time_ns / 1_000_000:.2f} ms"


def save_gif(
    densities: list[np.ndarray],
    statistics: list[dict[str, float]],
    half_width: float,
    fps: int,
    title: str,
    path: Path,
) -> None:
    positives = [density[density > 0] for density in densities]
    positive = np.concatenate([values for values in positives if values.size])
    norm = colors.LogNorm(vmin=float(positive.min()), vmax=float(positive.max()))
    color_map = plt.get_cmap("magma").copy()
    color_map.set_bad("#eeeeee")

    figure, axis = plt.subplots(figsize=(7.2, 6.5), constrained_layout=True)
    image = axis.imshow(
        np.ma.masked_less_equal(densities[0].T, 0.0),
        origin="lower",
        extent=(-half_width, half_width, -half_width, half_width),
        cmap=color_map,
        norm=norm,
        interpolation="nearest",
    )
    axis.plot(0, 0, marker="+", color="cyan", markersize=10, markeredgewidth=1.5)
    axis.set(title=title, xlabel="x (mm)", ylabel="y (mm)", aspect="equal")
    figure.colorbar(image, ax=axis, label="weighted active QP tracks / mm²")
    annotation = axis.text(
        0.02,
        0.98,
        "",
        transform=axis.transAxes,
        ha="left",
        va="top",
        color="white",
        bbox={"boxstyle": "round", "facecolor": "black", "alpha": 0.7},
    )

    def update(index: int) -> tuple[object, object]:
        image.set_data(np.ma.masked_less_equal(densities[index].T, 0.0))
        frame = statistics[index]
        annotation.set_text(
            f"t = {format_time(frame['time_ns'])}\n"
            f"active QPs = {frame['active_tracks']:,}\n"
            f"weighted = {frame['weighted_active_tracks']:,.1f}"
        )
        return image, annotation

    movie = animation.FuncAnimation(
        figure, update, frames=len(densities), interval=1_000 / fps, blit=False
    )
    movie.save(path, writer=animation.PillowWriter(fps=fps), dpi=110)
    plt.close(figure)


def main() -> None:
    args = parse_args()
    sample_path = args.samples_csv.resolve()
    run_summary_path = sample_path.parent / "summary.json"
    run_summary = json.loads(run_summary_path.read_text())
    interval = float(run_summary.get("qp_sample_interval_ns", 0.0))
    if interval <= 0 or not run_summary.get("passed"):
        raise ValueError("Run summary does not describe a completed fixed-time sample run")

    samples = load_samples(sample_path)
    times = select_times(samples, args.frames)
    densities, statistics = make_density_frames(
        samples, times, args.bins, args.chip_half_width_mm
    )
    ground_plane = run_summary.get("ground_plane") or {}
    primary = run_summary.get("primary") or {}
    energy_mev = float(primary.get("energy_eV", 0.0)) * 1_000.0
    title = (
        f"{energy_mev:g} meV electron: QP density sampled every {interval:g} ns\n"
        f"{ground_plane.get('material')} ground plane, "
        f"{float(ground_plane.get('thickness_nm')):.0f} nm"
    )

    output_dir = (args.output_dir or sample_path.parent / "density-animation-100ns").resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    gif_path = output_dir / "qp_density_over_time.gif"
    frames_path = output_dir / "qp_density_frames.csv"
    summary_path = output_dir / "qp_density_animation_summary.json"
    save_gif(densities, statistics, args.chip_half_width_mm, args.fps, title, gif_path)
    pd.DataFrame(statistics).to_csv(frames_path, index=False)
    analysis_summary = {
        "input_csv": str(sample_path),
        "primary": primary,
        "ground_plane": ground_plane,
        "sample_interval_ns": interval,
        "simulation_qp_tracks": run_summary.get("qp_tracks"),
        "sample_rows": run_summary.get("qp_sample_rows"),
        "frames": len(times),
        "bins_per_axis": args.bins,
        "bin_width_mm": 2.0 * args.chip_half_width_mm / args.bins,
        "first_frame_time_ns": float(times[0]),
        "last_frame_time_ns": float(times[-1]),
        "maximum_weighted_active_tracks": max(
            frame["weighted_active_tracks"] for frame in statistics
        ),
    }
    summary_path.write_text(json.dumps(analysis_summary, indent=2) + "\n")
    print(f"Wrote {gif_path}")
    print(f"Wrote {frames_path}")
    print(f"Wrote {summary_path}")


if __name__ == "__main__":
    main()
