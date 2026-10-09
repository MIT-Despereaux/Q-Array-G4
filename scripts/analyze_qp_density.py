#!/usr/bin/env python3
"""Plot weighted quasiparticle first-step density from the compact smoke-run CSV."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.colors as colors
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


REQUIRED_COLUMNS = {
    "event",
    "track",
    "step",
    "particle",
    "time_ns",
    "x_mm",
    "y_mm",
    "pre_volume",
    "weight",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Create a weighted QP first-step density visualization from steps.csv."
    )
    parser.add_argument("steps_csv", type=Path, help="compact step CSV from qp_example_smoke")
    parser.add_argument(
        "--output-dir",
        type=Path,
        help="output directory (default: density/ beside the input CSV)",
    )
    parser.add_argument("--bins", type=int, default=80, help="bins along each chip axis")
    parser.add_argument(
        "--chip-half-width-mm",
        type=float,
        default=4.0,
        help="half-width of the square chip",
    )
    return parser.parse_args()


def weighted_quantile(values: np.ndarray, weights: np.ndarray, quantile: float) -> float:
    order = np.argsort(values)
    sorted_values = values[order]
    sorted_weights = weights[order]
    midpoint = np.cumsum(sorted_weights) - 0.5 * sorted_weights
    return float(np.interp(quantile * sorted_weights.sum(), midpoint, sorted_values))


def volume_group(volume: str) -> str:
    if volume == "GroundPlane":
        return "Ground plane"
    if volume.startswith("TransmissionLine"):
        return "Transmission line"
    match = re.match(r"ResonatorAssembly_(\d+)", volume)
    if match:
        return f"Resonator {match.group(1)}"
    return "Other"


def load_births(path: Path) -> pd.DataFrame:
    columns = pd.read_csv(path, nrows=0).columns
    missing = REQUIRED_COLUMNS.difference(columns)
    if missing:
        raise ValueError(f"{path} is missing columns: {', '.join(sorted(missing))}")

    births = []
    for chunk in pd.read_csv(path, usecols=sorted(REQUIRED_COLUMNS), chunksize=100_000):
        selected = chunk[(chunk["particle"] == "BogoliubovQP") & (chunk["step"] == 1)]
        if not selected.empty:
            births.append(selected)
    if not births:
        raise ValueError(f"No step-1 BogoliubovQP rows found in {path}")

    result = pd.concat(births, ignore_index=True)
    duplicate = result.duplicated(["event", "track"])
    if duplicate.any():
        raise ValueError(f"Found {int(duplicate.sum())} duplicate QP birth rows")
    return result


def make_analysis(
    births: pd.DataFrame, bins: int, half_width: float
) -> tuple[plt.Figure, pd.DataFrame, dict[str, object]]:
    if bins <= 0 or half_width <= 0:
        raise ValueError("bins and chip half-width must be positive")

    x = births["x_mm"].to_numpy(float)
    y = births["y_mm"].to_numpy(float)
    time = births["time_ns"].to_numpy(float)
    weight = births["weight"].to_numpy(float)
    extent = [[-half_width, half_width], [-half_width, half_width]]
    outside = (np.abs(x) > half_width) | (np.abs(y) > half_width)
    if outside.any():
        raise ValueError(f"{int(outside.sum())} QP births lie outside the configured chip")

    counts, x_edges, y_edges = np.histogram2d(
        x, y, bins=bins, range=extent, weights=weight
    )
    bin_width = 2.0 * half_width / bins
    bin_area = bin_width**2
    density = counts / bin_area
    nonzero_density = density[density > 0]

    x_centers = 0.5 * (x_edges[:-1] + x_edges[1:])
    y_centers = 0.5 * (y_edges[:-1] + y_edges[1:])
    grid_x, grid_y = np.meshgrid(x_centers, y_centers, indexing="ij")
    matrix = pd.DataFrame(
        {
            "x_center_mm": grid_x.ravel(),
            "y_center_mm": grid_y.ravel(),
            "weighted_qp_tracks": counts.ravel(),
            "surface_density_qp_per_mm2": density.ravel(),
        }
    )

    radius = np.hypot(x, y)
    radial_edges = np.linspace(0.0, np.sqrt(2.0) * half_width, 61)
    radial_counts, _ = np.histogram(radius, bins=radial_edges, weights=weight)
    annulus_area = np.pi * (radial_edges[1:] ** 2 - radial_edges[:-1] ** 2)
    radial_density = radial_counts / annulus_area
    radial_centers = 0.5 * (radial_edges[:-1] + radial_edges[1:])

    positive_time = time[time > 0]
    time_min = positive_time.min()
    time_max = positive_time.max()
    time_edges = np.geomspace(time_min, time_max, 55)
    time_counts, _ = np.histogram(time, bins=time_edges, weights=weight)
    time_rate = time_counts / np.diff(time_edges)
    time_centers = np.sqrt(time_edges[:-1] * time_edges[1:])

    grouped = births.assign(group=births["pre_volume"].map(volume_group)).groupby("group")[
        "weight"
    ].sum()
    group_order = ["Ground plane", "Transmission line"] + [
        f"Resonator {index}" for index in range(6)
    ]
    grouped = grouped.reindex(group_order, fill_value=0.0)

    figure, axes = plt.subplots(2, 2, figsize=(13.5, 10.5), constrained_layout=True)
    density_axis, radial_axis, time_axis, volume_axis = axes.ravel()

    masked_density = np.ma.masked_less_equal(density.T, 0.0)
    color_map = plt.get_cmap("magma").copy()
    color_map.set_bad("#eeeeee")
    image = density_axis.pcolormesh(
        x_edges,
        y_edges,
        masked_density,
        cmap=color_map,
        norm=colors.LogNorm(vmin=nonzero_density.min(), vmax=nonzero_density.max()),
        shading="flat",
    )
    density_axis.plot(0, 0, marker="+", color="cyan", markersize=10, markeredgewidth=1.5)
    density_axis.set(
        title=f"QP first-step surface density ({bin_width * 1_000:.0f} µm bins)",
        xlabel="x (mm)",
        ylabel="y (mm)",
        aspect="equal",
        xlim=(-half_width, half_width),
        ylim=(-half_width, half_width),
    )
    figure.colorbar(image, ax=density_axis, label="weighted QP tracks / mm²")

    valid_radial = radial_density > 0
    radial_axis.semilogy(
        radial_centers[valid_radial], radial_density[valid_radial], color="#5e3c99", linewidth=2
    )
    radial_axis.set(
        title="Radial first-step density from impact point",
        xlabel="radius (mm)",
        ylabel="weighted QP tracks / mm²",
    )
    radial_axis.grid(alpha=0.25)

    valid_time = time_rate > 0
    time_axis.loglog(
        time_centers[valid_time], time_rate[valid_time], color="#e66101", linewidth=2
    )
    time_axis.set(
        title="QP first-step completion rate",
        xlabel="first-step time after primary (ns)",
        ylabel="weighted QP tracks / ns",
    )
    time_axis.grid(alpha=0.25, which="both")

    volume_axis.barh(grouped.index, grouped.to_numpy(), color="#3288bd")
    volume_axis.invert_yaxis()
    volume_axis.set(
        title="QP tracks by initial film component",
        xlabel="weighted QP tracks",
        ylabel="",
    )
    volume_axis.grid(axis="x", alpha=0.25)

    weighted_total = float(weight.sum())
    figure.suptitle(
        "10 eV electron in stock G4CMP quasiparticle geometry\n"
        f"{len(births):,} QP tracks; weighted total {weighted_total:,.1f}",
        fontsize=15,
    )

    summary: dict[str, object] = {
        "analysis": "weighted quasiparticle first-step surface density",
        "raw_qp_tracks": int(len(births)),
        "weighted_qp_tracks": weighted_total,
        "chip_half_width_mm": half_width,
        "bins_per_axis": bins,
        "bin_width_mm": bin_width,
        "bin_area_mm2": bin_area,
        "occupied_bins": int(nonzero_density.size),
        "peak_surface_density_qp_per_mm2": float(nonzero_density.max()),
        "median_nonzero_surface_density_qp_per_mm2": float(np.median(nonzero_density)),
        "weighted_median_radius_mm": weighted_quantile(radius, weight, 0.5),
        "weighted_median_first_step_time_ns": weighted_quantile(time, weight, 0.5),
        "maximum_first_step_time_ns": float(time.max()),
        "weighted_tracks_by_component": {
            name: float(value) for name, value in grouped.items()
        },
        "interpretation": (
            "This maps each QP at the end of its first transport step. It is a creation-location "
            "proxy, not instantaneous QP population density; the compact smoke output omits "
            "the creation vertex and intermediate trajectory steps."
        ),
    }
    return figure, matrix, summary


def main() -> None:
    args = parse_args()
    input_path = args.steps_csv.resolve()
    output_dir = (args.output_dir or input_path.parent / "density").resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    births = load_births(input_path)
    figure, matrix, summary = make_analysis(births, args.bins, args.chip_half_width_mm)
    summary["input_csv"] = str(input_path)

    figure_path = output_dir / "qp_first_step_density.png"
    matrix_path = output_dir / "qp_first_step_density.csv"
    summary_path = output_dir / "qp_first_step_density_summary.json"
    figure.savefig(figure_path, dpi=220)
    plt.close(figure)
    matrix.to_csv(matrix_path, index=False)
    summary_path.write_text(json.dumps(summary, indent=2) + "\n")

    print(f"Wrote {figure_path}")
    print(f"Wrote {matrix_path}")
    print(f"Wrote {summary_path}")


if __name__ == "__main__":
    main()
