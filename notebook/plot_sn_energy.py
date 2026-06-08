import marimo

__generated_with = "0.23.6"
app = marimo.App(width="medium")


@app.cell
def _():
    from pathlib import Path

    import marimo as mo
    import matplotlib.pyplot as plt
    import pandas as pd

    return Path, mo, pd, plt


@app.cell
def _(Path):
    repo_root = Path(__file__).resolve().parents[1]
    output_dir = repo_root / "output" / "g4sim" / "v02x"
    image_dir = repo_root / "notebook" / "image"
    file_pattern = "dspx_cosmic_batch*DetectorSnCubeLogical*.csv"
    producer_command = "scripts/run_dspx_cosmic_batch.sh"
    return file_pattern, image_dir, output_dir, producer_command


@app.cell
def _(file_pattern, mo, output_dir, producer_command):
    mo.md(
        f"""
        # Sn Cube Energy Deposition

        This notebook analyzes only Sn-cube CSV files produced by
        `{producer_command}`.

        Input directory: `{output_dir}`

        File pattern: `{file_pattern}`
        """
    )
    return


@app.cell
def _(pd):
    def geant4_csv_columns(csv_path):
        columns = []
        with csv_path.open(encoding="utf-8") as csv_file:
            for line in csv_file:
                if not line.startswith("#"):
                    break
                if line.startswith("#column "):
                    parts = line.strip().split()
                    if len(parts) < 3:
                        raise ValueError(f"Malformed #column metadata in {csv_path}: {line!r}")
                    columns.append(parts[-1])

        if not columns:
            raise ValueError(f"No Geant4 column metadata found in {csv_path}")
        return columns


    def load_geant4_csv(csv_path):
        columns = geant4_csv_columns(csv_path)
        frame = pd.read_csv(csv_path, comment="#", names=columns)
        frame.insert(0, "source_file", csv_path.name)
        return frame


    def load_sn_cube_data(output_dir, file_pattern):
        csv_paths = sorted(output_dir.glob(file_pattern))
        if not csv_paths:
            raise FileNotFoundError(
                "No Sn-cube cosmic batch CSVs found. Run "
                "`scripts/run_dspx_cosmic_batch.sh` from the repository root."
            )

        frames = []
        for csv_path in csv_paths:
            frame = load_geant4_csv(csv_path)
            required_columns = {
                "edep",
                "eventID",
                "trackID",
                "parentTrackID",
                "particleName",
            }
            missing_columns = required_columns.difference(frame.columns)
            if missing_columns:
                missing = ", ".join(sorted(missing_columns))
                raise ValueError(f"Missing required columns in {csv_path}: {missing}")
            frames.append(frame)

        data = pd.concat(frames, ignore_index=True)
        data["edep"] = pd.to_numeric(data["edep"], errors="coerce")
        data["eventID"] = pd.to_numeric(data["eventID"], errors="coerce").astype("Int64")
        data["trackID"] = pd.to_numeric(data["trackID"], errors="coerce").astype("Int64")
        data["parentTrackID"] = pd.to_numeric(
            data["parentTrackID"],
            errors="coerce",
        ).astype("Int64")
        bad_edep_rows = int(data["edep"].isna().sum())
        if bad_edep_rows:
            raise ValueError(f"Found {bad_edep_rows} rows with non-numeric edep values")

        data = data.loc[data["edep"] > 0].copy()
        if data.empty:
            raise ValueError("Sn-cube cosmic batch CSVs contain no positive edep rows")
        return data


    return (load_sn_cube_data,)


@app.cell
def _(file_pattern, load_sn_cube_data, output_dir):
    sn_steps = load_sn_cube_data(output_dir, file_pattern)
    primary_muon_mask = (
        sn_steps["particleName"].isin(["mu+", "mu-"])
        & (sn_steps["trackID"] == 1)
        & (sn_steps["parentTrackID"] == 0)
    )
    sn_steps["deposit_source"] = primary_muon_mask.map(
        {True: "Primary muon", False: "Secondary particles"}
    )
    event_sources = (
        sn_steps.groupby(
            ["source_file", "eventID", "deposit_source"],
            dropna=False,
            as_index=False,
        )["edep"]
        .sum()
        .rename(columns={"edep": "event_edep"})
    )
    event_edep = (
        event_sources.pivot(
            index=["source_file", "eventID"],
            columns="deposit_source",
            values="event_edep",
        )
        .fillna(0.0)
        .reset_index()
    )
    for source_name in ("Primary muon", "Secondary particles"):
        if source_name not in event_edep.columns:
            event_edep[source_name] = 0.0
    return event_edep, sn_steps


@app.cell
def _(event_edep, output_dir, sn_steps):
    summary = {
        "input_directory": str(output_dir),
        "csv_files": sn_steps["source_file"].nunique(),
        "step_rows": len(sn_steps),
        "events_with_sn_deposit": len(event_edep),
        "total_edep_mev": sn_steps["edep"].sum(),
        "primary_muon_edep_mev": sn_steps.loc[
            sn_steps["deposit_source"] == "Primary muon",
            "edep",
        ].sum(),
        "secondary_edep_mev": sn_steps.loc[
            sn_steps["deposit_source"] == "Secondary particles",
            "edep",
        ].sum(),
        "mean_primary_event_edep_mev": event_edep["Primary muon"].mean(),
        "mean_secondary_event_edep_mev": event_edep["Secondary particles"].mean(),
    }
    return (summary,)


@app.cell
def _(mo, summary):
    mo.md(
        "\n".join(
            [
                "## Summary",
                "",
                f"- CSV files: `{summary['csv_files']}`",
                f"- Step rows with positive Sn deposition: `{summary['step_rows']}`",
                f"- Events with Sn deposition: `{summary['events_with_sn_deposit']}`",
                f"- Total Sn energy deposition: `{summary['total_edep_mev']:.6g} MeV`",
                f"- Primary-muon deposition: `{summary['primary_muon_edep_mev']:.6g} MeV`",
                f"- Secondary-particle deposition: `{summary['secondary_edep_mev']:.6g} MeV`",
                (
                    "- Mean primary-muon deposition per event: "
                    f"`{summary['mean_primary_event_edep_mev']:.6g} MeV`"
                ),
                (
                    "- Mean secondary-particle deposition per event: "
                    f"`{summary['mean_secondary_event_edep_mev']:.6g} MeV`"
                ),
            ]
        )
    )
    return


@app.cell
def _(mo):
    default_plot_settings = {
        "bins": 100,
        "x_max_mev": 15.0,
        "log_y": False,
    }
    plot_settings_form = (
        mo.md(
            """
            ## Plot Settings

            {bins}

            {x_max_mev}

            {log_y}
            """
        )
        .batch(
            bins=mo.ui.number(
                start=1,
                stop=1000,
                step=1,
                value=default_plot_settings["bins"],
                label="Histogram bins",
            ),
            x_max_mev=mo.ui.number(
                start=0.1,
                stop=100.0,
                step=0.1,
                value=default_plot_settings["x_max_mev"],
                label="Maximum energy [MeV]",
            ),
            log_y=mo.ui.switch(
                value=default_plot_settings["log_y"],
                label="Logarithmic y-axis",
            ),
        )
        .form(submit_button_label="Update plots")
    )
    plot_settings_form  # noqa: B018
    return default_plot_settings, plot_settings_form


@app.cell
def _(default_plot_settings, plot_settings_form):
    def numeric_form_value(value, name):
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            raise TypeError(f"{name} must be numeric")
        return value


    submitted_plot_settings = plot_settings_form.value
    if submitted_plot_settings is None:
        plot_settings = default_plot_settings
    else:
        bins_value = numeric_form_value(submitted_plot_settings["bins"], "Histogram bins")
        x_max_value = numeric_form_value(
            submitted_plot_settings["x_max_mev"],
            "Maximum energy",
        )
        log_y_value = submitted_plot_settings["log_y"]
        if not isinstance(log_y_value, bool):
            raise TypeError("Logarithmic y-axis must be boolean")

        plot_settings = {
            "bins": int(bins_value),
            "x_max_mev": float(x_max_value),
            "log_y": log_y_value,
        }
    return (plot_settings,)


@app.cell
def _(image_dir, plt):
    def save_histogram(series_by_source, output_path, title, xlabel, bins, x_max, log_y):
        output_path.parent.mkdir(parents=True, exist_ok=True)
        fig, ax = plt.subplots(figsize=(8, 5), constrained_layout=True)
        colors = {
            "Primary muon": "#3b6ea8",
            "Secondary particles": "#d97932",
        }
        source_names = list(series_by_source)
        ax.hist(
            [series_by_source[source_name] for source_name in source_names],
            bins=bins,
            range=(0, x_max),
            color=[colors[source_name] for source_name in source_names],
            edgecolor="black",
            alpha=0.85,
            label=source_names,
            stacked=True,
        )
        ax.set_title(title)
        ax.set_xlabel(xlabel)
        ax.set_ylabel("Count")
        ax.set_xlim(0, x_max)
        ax.set_yscale("log" if log_y else "linear")
        ax.grid(True, alpha=0.25)
        ax.legend()
        fig.savefig(output_path, dpi=160)
        return fig, output_path


    step_hist_path = image_dir / "sn_cube_step_edep_histogram.png"
    event_hist_path = image_dir / "sn_cube_event_edep_histogram.png"
    return event_hist_path, save_histogram, step_hist_path


@app.cell
def _(
    event_edep,
    event_hist_path,
    plot_settings,
    save_histogram,
    sn_steps,
    step_hist_path,
):
    step_hist_fig, saved_step_hist_path = save_histogram(
        {
            source_name: source_steps["edep"]
            for source_name, source_steps in sn_steps.groupby("deposit_source")
        },
        step_hist_path,
        "Sn Cube Step Energy Deposition",
        "Step energy deposition [MeV]",
        plot_settings["bins"],
        plot_settings["x_max_mev"],
        plot_settings["log_y"],
    )
    event_hist_fig, saved_event_hist_path = save_histogram(
        {
            "Primary muon": event_edep.loc[
                event_edep["Primary muon"] > 0,
                "Primary muon",
            ],
            "Secondary particles": event_edep.loc[
                event_edep["Secondary particles"] > 0,
                "Secondary particles",
            ],
        },
        event_hist_path,
        "Sn Cube Per-Event Energy Deposition",
        "Per-event energy deposition [MeV]",
        plot_settings["bins"],
        plot_settings["x_max_mev"],
        plot_settings["log_y"],
    )
    return (
        event_hist_fig,
        saved_event_hist_path,
        saved_step_hist_path,
        step_hist_fig,
    )


@app.cell
def _(mo, saved_event_hist_path, saved_step_hist_path):
    mo.md(
        "\n".join(
            [
                "## Saved Images",
                "",
                f"- `{saved_step_hist_path}`",
                f"- `{saved_event_hist_path}`",
            ]
        )
    )
    return


@app.cell
def _(event_hist_fig):
    event_hist_fig  # noqa: B018
    return


@app.cell
def _(step_hist_fig):
    step_hist_fig  # noqa: B018
    return


if __name__ == "__main__":
    app.run()
