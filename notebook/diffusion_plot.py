import sys
import re
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors

# =====================================================================
# CONFIGURATION
# =====================================================================
# Scale types supported: 'power' (highly recommended!), 'log', or 'linear'
SCALE_TYPE = 'power'

# If using 'power' scale, gamma < 1.0 stretches low-energy differences.
# (0.2 to 0.5 is usually the sweet spot for quasiparticles)
POWER_GAMMA = 0.25  

# Colormap: 'coolwarm' goes from Light Blue (low energy) to Red (high energy)
COLOR_MAP = 'coolwarm'
# =====================================================================

def parse_log_file(filepath):
    """
    Parses the log file read-only, extracting only '[QP STEP REGISTERED]' lines.
    """
    pattern = re.compile(
        r"TrackID:\s*(\d+).*?Pos:\s*\(([\-\d\.eE]+),([\-\d\.eE]+),[\-\d\.eE]+\).*?KE:\s*([\-\d\.eE]+)"
    )

    tracks = {}

    with open(filepath, 'r') as f:
        for line in f:
            if "[QP STEP REGISTERED]" in line:
                match = pattern.search(line)
                if match:
                    track_id = int(match.group(1))
                    x = float(match.group(2))
                    y = float(match.group(3))
                    ke = float(match.group(4))

                    if track_id not in tracks:
                        tracks[track_id] = []
                    tracks[track_id].append({'x': x, 'y': y, 'ke': ke})

    return tracks

def plot_data(tracks):
    birth_x, birth_y, birth_ke = [], [], []
    death_x, death_y, death_ke = [], [], []

    for track_id, data in tracks.items():
        # First point is Birth
        birth = data[0]
        birth_x.append(birth['x'])
        birth_y.append(birth['y'])
        birth_ke.append(birth['ke'])

        # Second point is Death (if tracking ended)
        if len(data) > 1:
            death = data[1]
            death_x.append(death['x'])
            death_y.append(death['y'])
            death_ke.append(death['ke'])

    if not birth_x:
        print("\n[!] No valid [QP STEP REGISTERED] data found in this log file yet.")
        return

    # Calculate statistics
    avg_birth_ke = np.mean(birth_ke)
    min_birth_ke = np.min(birth_ke)
    max_birth_ke = np.max(birth_ke)
    
    avg_death_ke = np.mean(death_ke) if death_ke else 0.0
    min_death_ke = np.min(death_ke) if death_ke else 0.0
    max_death_ke = np.max(death_ke) if death_ke else 0.0

    print("\n" + "="*50)
    print(f"Total Quasiparticles Birthed: {len(birth_x)}")
    print(f"Total Quasiparticles Died:    {len(death_x)}")
    print(f"Average Birth Energy:         {avg_birth_ke:.6e} eV")
    print(f"Average Death Energy:         {avg_death_ke:.6e} eV")
    print("="*50 + "\n")

    # Establish common color scaling limits based strictly on births
    vmin = min_birth_ke
    vmax = max_birth_ke

    # Handle edge case of single value
    if vmin == vmax:
        norm = mcolors.Normalize(vmin=vmin, vmax=vmax)
    elif SCALE_TYPE == 'log':
        # Ensure vmin is strictly positive for log scales
        safe_vmin = vmin if vmin > 0 else 1e-6
        norm = mcolors.LogNorm(vmin=safe_vmin, vmax=vmax)
    elif SCALE_TYPE == 'power':
        norm = mcolors.PowerNorm(gamma=POWER_GAMMA, vmin=vmin, vmax=vmax)
    else:  # Default to 'linear'
        norm = mcolors.Normalize(vmin=vmin, vmax=vmax)

    # --- Setup 2x2 Plot Figure Layout ---
    fig, axs = plt.subplots(2, 2, figsize=(14, 10))
    fig.subplots_adjust(right=0.80, hspace=0.35, wspace=0.3)

    # Unpack axis grid
    ((ax_birth_map, ax_death_map), (ax_birth_hist, ax_death_hist)) = axs

    # --- 1. SPATIAL PLOT: BIRTHS (Top Left) ---
    sc1 = ax_birth_map.scatter(birth_x, birth_y, c=birth_ke, cmap=COLOR_MAP, norm=norm, s=12, alpha=0.8)
    ax_birth_map.set_title("Quasiparticle Birth Locations", fontweight='bold')
    ax_birth_map.set_xlabel("X Position (mm)")
    ax_birth_map.set_ylabel("Y Position (mm)")
    ax_birth_map.set_aspect('equal')
    fig.colorbar(sc1, ax=ax_birth_map, label='Kinetic Energy (eV)', shrink=0.8)

    # --- 2. SPATIAL PLOT: DEATHS (Top Right) ---
    if death_x:
        sc2 = ax_death_map.scatter(death_x, death_y, c=death_ke, cmap=COLOR_MAP, norm=norm, s=12, alpha=0.8)
        ax_death_map.set_title("Quasiparticle Death Locations", fontweight='bold')
        ax_death_map.set_xlabel("X Position (mm)")
        ax_death_map.set_ylabel("Y Position (mm)")
        ax_death_map.set_aspect('equal')
        fig.colorbar(sc2, ax=ax_death_map, label='Kinetic Energy (eV)', shrink=0.8)
    else:
        ax_death_map.set_title("Quasiparticle Death Locations (No Data)", fontweight='bold')
        ax_death_map.set_aspect('equal')

    # --- HISTOGRAM BINNING CONFIGURATION ---
    # Merge both populations to build consistent log-spaced bins across the entire energy range
    all_ke = birth_ke + (death_ke if death_ke else [])
    hist_vmin = min(all_ke) if min(all_ke) > 0 else 1e-6
    hist_vmax = max(all_ke)
    
    # Use log-spaced bins because QP energy spans orders of magnitude
    bins = np.logspace(np.log10(hist_vmin), np.log10(hist_vmax), 40)

    # --- 3. HISTOGRAM: BIRTH ENERGIES (Bottom Left) ---
    ax_birth_hist.hist(birth_ke, bins=bins, color='crimson', alpha=0.7, edgecolor='black')
    ax_birth_hist.set_xscale('log')
    ax_birth_hist.set_title("Birth Energy Distribution", fontweight='bold')
    ax_birth_hist.set_xlabel("Kinetic Energy (eV)")
    ax_birth_hist.set_ylabel("Quasiparticle Count")
    ax_birth_hist.grid(True, which="both", ls="--", alpha=0.4)

    # --- 4. HISTOGRAM: DEATH ENERGIES (Bottom Right) ---
    if death_ke:
        ax_death_hist.hist(death_ke, bins=bins, color='royalblue', alpha=0.7, edgecolor='black')
        ax_death_hist.set_xscale('log')
        ax_death_hist.set_title("Death Energy Distribution", fontweight='bold')
        ax_death_hist.set_xlabel("Kinetic Energy (eV)")
        ax_death_hist.set_ylabel("Quasiparticle Count")
        ax_death_hist.grid(True, which="both", ls="--", alpha=0.4)
    else:
        ax_death_hist.set_title("Death Energy Distribution (No Data)", fontweight='bold')

    # --- SIDE PANEL: Floating Statistics TextBox ---
    stats_text = (
        "RUN STATISTICS\n"
        "====================\n"
        f"Total QPs:  {len(birth_x)}\n\n"
        "QP BIRTHS:\n"
        f" Avg: {avg_birth_ke:.4e} eV\n"
        f" Min: {min_birth_ke:.4e} eV\n"
        f" Max: {max_birth_ke:.4e} eV\n\n"
        "QP DEATHS:\n"
    )
    if death_x:
        stats_text += (
            f" Avg: {avg_death_ke:.4e} eV\n"
            f" Min: {min_death_ke:.4e} eV\n"
            f" Max: {max_death_ke:.4e} eV"
        )
    else:
        stats_text += " Avg: N/A\n Min: N/A\n Max: N/A"

    # Place the textbox nicely aligned in the right margin gap
    fig.text(0.825, 0.5, stats_text, fontsize=10, family='monospace',
             verticalalignment='center', 
             bbox=dict(boxstyle='round,pad=1', facecolor='#fafafa', alpha=0.95, edgecolor='#cccccc'))

    plt.suptitle(f"G4CMP Quasiparticle Tracking ({SCALE_TYPE.upper()} color scale)", fontsize=16, weight='bold', y=0.96)
    plt.savefig("quasiparticle_histogram.png", transparent=True, format="png", dpi=300)
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python plot_qps_dashboard.py <path_to_logfile>")
        sys.exit(1)
    
    log_filepath = sys.argv[1]
    track_data = parse_log_file(log_filepath)
    plot_data(track_data)