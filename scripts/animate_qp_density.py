#!/usr/bin/env python3
"""Animate weighted quasiparticle surface density from complete step histories."""

from __future__ import annotations

import argparse
import json
from dataclasses import dataclass
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
    "step",
    "particle",
    "pre_time_ns",
    "time_ns",
    "pre_x_mm",
    "pre_y_mm",
    "x_mm",
    "y_mm",
    "status",
    "weight",
}


@dataclass
class Trajectory:
    pre_time: np.ndarray
    post_time: np.ndarray
    pre_x: np.ndarray
    pre_y: np.ndarray
    post_x: np.ndarray
    post_y: np.ndarray
    weight: float

    def position_at(self, time_ns: float) -> tuple[float, float] | None:
        if time_ns < self.pre_time[0] or time_ns > self.post_time[-1]:
            return None
        index = int(np.searchsorted(self.pre_time, time_ns, side="right") - 1)
        index = max(0, min(index, len(self.pre_time) - 1))
        if time_ns > self.post_time[index] + 1e-12:
            return None
        duration = self.post_time[index] - self.pre_time[index]
        fraction = 1.0 if duration <= 0 else (time_ns - self.pre_time[index]) / duration
        fraction = float(np.clip(fraction, 0.0, 1.0))
        x = self.pre_x[index] + fraction * (self.post_x[index] - self.pre_x[index])
        y = self.pre_y[index] + fraction * (self.post_y[index] - self.pre_y[index])
        return float(x), float(y)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Animate weighted QP surface density from a fully recorded steps.csv."
    )
    parser.add_argument("steps_csv", type=Path)
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument("--frames", type=int, default=72)
    parser.add_argument("--fps", type=int, default=8)
    parser.add_argument("--bins", type=int, default=80)
    parser.add_argument("--chip-half-width-mm", type=float, default=4.0)
    return parser.parse_args()


def verify_run_summary(csv_path: Path) -> dict[str, object]:
    summary_path = csv_path.parent / "summary.json"
    if not summary_path.exists():
        raise ValueError(f"Missing run summary: {summary_path}")
    summary = json.loads(summary_path.read_text())
    if not summary.get("record_all_qp_steps"):
        raise ValueError("Run summary does not confirm full QP step recording")
    if not summary.get("passed") or summary.get("aborted"):
        raise ValueError("Simulation did not complete successfully")
    return summary


def load_trajectories(path: Path) -> tuple[list[Trajectory], dict[str, int]]:
    columns = pd.read_csv(path, nrows=0).columns
    missing = REQUIRED_COLUMNS.difference(columns)
    if missing:
        raise ValueError(f"{path} is missing columns: {', '.join(sorted(missing))}")
    chunks = []
    for chunk in pd.read_csv(path, usecols=sorted(REQUIRED_COLUMNS), chunksize=100_000):
        selected = chunk[chunk["particle"] == "BogoliubovQP"]
        if not selected.empty:
            chunks.append(selected)
    if not chunks:
        raise ValueError("No BogoliubovQP rows found")
    qp_steps = pd.concat(chunks, ignore_index=True)

    trajectories = []
    incomplete_tracks = 0
    nonterminal_tracks = 0
    for _, track in qp_steps.groupby(["event", "track"], sort=False):
        track = track.sort_values("step")
        steps = track["step"].to_numpy(int)
        if not np.array_equal(steps, np.arange(1, len(steps) + 1)):
            incomplete_tracks += 1
            continue
        if int(track.iloc[-1]["status"]) == 0:
            nonterminal_tracks += 1
        trajectories.append(
            Trajectory(
                pre_time=track["pre_time_ns"].to_numpy(float),
                post_time=track["time_ns"].to_numpy(float),
                pre_x=track["pre_x_mm"].to_numpy(float),
                pre_y=track["pre_y_mm"].to_numpy(float),
                post_x=track["x_mm"].to_numpy(float),
                post_y=track["y_mm"].to_numpy(float),
                weight=float(track.iloc[0]["weight"]),
            )
        )
    if incomplete_tracks:
        raise ValueError(f"Found {incomplete_tracks} QP tracks with missing step rows")
    return trajectories, {
        "qp_step_rows": int(len(qp_steps)),
        "qp_tracks": len(trajectories),
        "nonterminal_tracks": nonterminal_tracks,
    }


def format_time(time_ns: float) -> str:
    if time_ns < 1.0:
        return f"{time_ns * 1_000:.2f} ps"
    if time_ns < 1_000.0:
        return f"{time_ns:.2f} ns"
    if time_ns < 1_000_000.0:
        return f"{time_ns / 1_000:.2f} µs"
    return f"{time_ns / 1_000_000:.2f} ms"


def frame_times(trajectories: list[Trajectory], count: int) -> np.ndarray:
    if count < 2:
        raise ValueError("frames must be at least 2")
    start = min(track.pre_time[0] for track in trajectories)
    stop = max(track.post_time[-1] for track in trajectories)
    positive = [track.post_time[0] for track in trajectories if track.post_time[0] > 0]
    log_start = max(start, min(positive), stop * 1e-12)
    if start < log_start:
        return np.concatenate(([start], np.geomspace(log_start, stop, count - 1)))
    return np.geomspace(log_start, stop, count)


def density_frames(
    trajectories: list[Trajectory], times: np.ndarray, bins: int, half_width: float
) -> tuple[list[np.ndarray], list[dict[str, float]]]:
    extent = [[-half_width, half_width], [-half_width, half_width]]
    bin_area = (2.0 * half_width / bins) ** 2
    densities = []
    statistics = []
    for time_ns in times:
        positions = []
        weights = []
        for track in trajectories:
            position = track.position_at(float(time_ns))
            if position is not None:
                positions.append(position)
                weights.append(track.weight)
        if positions:
            position_array = np.asarray(positions)
            density, _, _ = np.histogram2d(
                position_array[:, 0],
                position_array[:, 1],
                bins=bins,
                range=extent,
                weights=np.asarray(weights),
            )
            density /= bin_area
        else:
            density = np.zeros((bins, bins), dtype=float)
        densities.append(density)
        statistics.append(
            {
                "time_ns": float(time_ns),
                "active_tracks": len(positions),
                "weighted_active_tracks": float(sum(weights)),
                "peak_density_qp_per_mm2": float(density.max()),
            }
        )
    return densities, statistics


def save_animation(
    densities: list[np.ndarray],
    statistics: list[dict[str, float]],
    half_width: float,
    fps: int,
    output_path: Path,
    title: str,
) -> None:
    positive_frames = [density[density > 0] for density in densities]
    positive_frames = [values for values in positive_frames if values.size]
    if not positive_frames:
        raise ValueError("No active QPs occur at the selected frame times")
    positive = np.concatenate(positive_frames)
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
    axis.set(
        title=title,
        xlabel="x (mm)",
        ylabel="y (mm)",
        aspect="equal",
    )
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
    movie.save(output_path, writer=animation.PillowWriter(fps=fps), dpi=110)
    plt.close(figure)


def main() -> None:
    args = parse_args()
    csv_path = args.steps_csv.resolve()
    output_dir = (args.output_dir or csv_path.parent / "density-animation").resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    run_summary = verify_run_summary(csv_path)
    trajectories, tracking_summary = load_trajectories(csv_path)
    times = frame_times(trajectories, args.frames)
    densities, statistics = density_frames(
        trajectories, times, args.bins, args.chip_half_width_mm
    )

    gif_path = output_dir / "qp_density_over_time.gif"
    frames_path = output_dir / "qp_density_frames.csv"
    summary_path = output_dir / "qp_density_animation_summary.json"
    ground_plane = run_summary.get("ground_plane") or {}
    ground_material = ground_plane.get("material")
    ground_thickness = ground_plane.get("thickness_nm")
    primary = run_summary.get("primary") or {}
    primary_energy_mev = float(primary.get("energy_eV", 0.0)) * 1_000.0
    title = f"{primary_energy_mev:g} meV electron: QP surface density"
    if ground_material and ground_thickness:
        title += f"\n{ground_material} ground plane, {ground_thickness:.0f} nm"
    save_animation(
        densities, statistics, args.chip_half_width_mm, args.fps, gif_path, title
    )
    pd.DataFrame(statistics).to_csv(frames_path, index=False)
    summary = {
        "input_csv": str(csv_path),
        "primary": run_summary.get("primary"),
        "record_all_qp_steps": run_summary.get("record_all_qp_steps"),
        "max_qp_steps": run_summary.get("max_qp_steps"),
        "limited_qp_tracks": run_summary.get("limited_qp_tracks"),
        "phonon_bounces": run_summary.get("phonon_bounces"),
        "qp_bounces": run_summary.get("qp_bounces"),
        **tracking_summary,
        "frames": len(times),
        "fps": args.fps,
        "bins_per_axis": args.bins,
        "bin_width_mm": 2.0 * args.chip_half_width_mm / args.bins,
        "first_frame_time_ns": float(times[0]),
        "last_frame_time_ns": float(times[-1]),
        "maximum_weighted_active_tracks": max(
            frame["weighted_active_tracks"] for frame in statistics
        ),
        "interpretation": (
            "Positions are linearly interpolated within every recorded Geant4 QP step. "
            "Frames use logarithmically spaced simulation times and track weights. "
            "Tracks counted by limited_qp_tracks end at the configured per-track step window."
        ),
    }
    summary_path.write_text(json.dumps(summary, indent=2) + "\n")
    print(f"Wrote {gif_path}")
    print(f"Wrote {frames_path}")
    print(f"Wrote {summary_path}")


if __name__ == "__main__":
    main()
