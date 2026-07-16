import sys
import re
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors

# =====================================================================
# CONFIGURATION TOGGLES
# =====================================================================
USE_LOG_SCALE = False

# The color map to use: 'coolwarm' goes from Light Blue (low) to Red (high)
COLOR_MAP = 'coolwarm' 
# =====================================================================

def parse_log_file(filepath):
    """
    Parses the log file line-by-line. 
    Only extracts lines starting containing '[QP STEP REGISTERED]'.
    """
    # Regex to extract TrackID, X, Y, and Kinetic Energy (KE)
    # Example matching line:
    # G4WT0 > [QP STEP REGISTERED] TrackID: 532168 | Time: 0.157128 ns | Pos: (-0.00139147,0.00110606,5.50015) mm | KE: 0.000282057 eV | Vol: ...
    pattern = re.compile(
        r"TrackID:\s*(\d+).*?Pos:\s*\(([\-\d\.eE]+),([\-\d\.eE]+),[\-\d\.eE]+\).*?KE:\s*([\-\d\.eE]+)"
    )

    tracks = {}

    with open(filepath, 'r') as f:
        for line in f:
            # STRICT FILTER: Ignore "[background hit]" or any other standard Geant4 output.
            # Only process lines that specifically register a Quasiparticle step.
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
    """
    Splits track data into Births and Deaths and plots them side-by-side.
    """
    birth_x, birth_y, birth_ke = [], [], []
    death_x, death_y, death_ke = [], [], []

    for track_id, data in tracks.items():
        # The first time the TrackID is seen, it is registered as the Birth
        birth = data[0]
        birth_x.append(birth['x'])
        birth_y.append(birth['y'])
        birth_ke.append(birth['ke'])

        # Since we only output Birth and Death (no intermediate steps), 
        # the second data point (if it exists) is registered as the Death
        if len(data) > 1:
            death = data[1]
            death_x.append(death['x'])
            death_y.append(death['y'])
            death_ke.append(death['ke'])

    if not birth_x:
        print("\n[!] No valid [QP STEP REGISTERED] data found in this log file yet.")
        print("    If the simulation is still running, it may not have written any steps yet.\n")
        return

    # Scale coloring bounds strictly based on the MIN and MAX Quasiparticle BIRTH energies
    vmin = min(birth_ke)
    vmax = max(birth_ke)
    
    print("\n" + "="*50)
    print(f"Total Quasiparticles Birthed: {len(birth_x)}")
    print(f"Total Quasiparticles Died:    {len(death_x)}")
    print(f"Energy Range (Births):        {vmin:.6e} eV to {vmax:.6e} eV")
    print(f"Scaling Mode:                 {'LOGARITHMIC' if USE_LOG_SCALE else 'LINEAR'}")
    print("="*50 + "\n")

    # Set up the figure with 2 subplots side-by-side
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

    # Configure the normalization according to our toggle
    if USE_LOG_SCALE:
        # Ensure vmin is strictly greater than 0 to prevent math errors in log space
        safe_vmin = vmin if vmin > 0 else 1e-6
        norm = mcolors.LogNorm(vmin=safe_vmin, vmax=vmax)
        scatter_kwargs = {"norm": norm}
    else:
        scatter_kwargs = {"vmin": vmin, "vmax": vmax}

    # --- 1. Plot Births ---
    sc1 = ax1.scatter(birth_x, birth_y, c=birth_ke, cmap=COLOR_MAP, s=15, alpha=0.8, **scatter_kwargs)
    ax1.set_title("Quasiparticle Births")
    ax1.set_xlabel("X Position (mm)")
    ax1.set_ylabel("Y Position (mm)")
    ax1.set_aspect('equal')  # Enforces 1:1 scale so your physical chip layout isn't distorted
    
    cbar1 = fig.colorbar(sc1, ax=ax1)
    cbar1.set_label('Kinetic Energy (eV)')

    # --- 2. Plot Deaths ---
    if death_x:
        sc2 = ax2.scatter(death_x, death_y, c=death_ke, cmap=COLOR_MAP, s=15, alpha=0.8, **scatter_kwargs)
        ax2.set_title("Quasiparticle Deaths")
        ax2.set_xlabel("X Position (mm)")
        ax2.set_ylabel("Y Position (mm)")
        ax2.set_aspect('equal')
        
        cbar2 = fig.colorbar(sc2, ax=ax2)
        cbar2.set_label('Kinetic Energy (eV)')
    else:
        ax2.set_title("Quasiparticle Deaths (None recorded yet)")
        ax2.set_aspect('equal')

    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Error: Missing log file path.")
        print("Usage: python plot_qps.py <path_to_your_log_file>")
        sys.exit(1)
    
    log_filepath = sys.argv[1]
    track_data = parse_log_file(log_filepath)
    plot_data(track_data)