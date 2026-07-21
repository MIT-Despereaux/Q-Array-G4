import sys
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
from matplotlib.colors import LinearSegmentedColormap

# =====================================================================
# CONFIGURATION
# =====================================================================
SCALE_TYPE = 'power'
POWER_GAMMA = 0.25  

color_low = '#215FAC'  
color_high = '#F0B6D3' 

COLOR_MAP = LinearSegmentedColormap.from_list("custom_coolwarm", [color_low, color_high])

# =====================================================================

def parse_csv_file(filepath):
    """
    Parses the G4CMP CSV file. Dynamically reads headers to find columns,
    then extracts BogoliubovQP tracks.
    """
    tracks = {}
    col_map = {}
    col_idx = 0
    
    print(f"Reading {filepath}...")
    
    with open(filepath, 'r') as f:
        for line in f:
            # 1. Parse Headers dynamically
            if line.startswith('#column '):
                parts = line.strip().split()
                if len(parts) >= 3:
                    col_name = parts[2]
                    col_map[col_name] = col_idx
                    col_idx += 1
                continue
            
            # Skip any other header/comment lines
            if line.startswith('#'):
                continue
                
            # 2. Parse Data Rows
            parts = line.strip().split(',')
            if len(parts) < len(col_map):
                continue # Skip malformed lines
                
            particle_name = parts[col_map['particleName']]
            
            # Only track Quasiparticles
            if 'BogoliubovQP' not in particle_name and 'QP' not in particle_name:
                continue
                
            event_id = int(parts[col_map['eventID']])
            track_id = int(parts[col_map['trackID']])
            ke = float(parts[col_map['ke']])
            x = float(parts[col_map['x1']])
            y = float(parts[col_map['y1']])
            
            # Use a composite key of (event_id, track_id) since track IDs reset every event
            key = (event_id, track_id)
            
            if key not in tracks:
                tracks[key] = []
            tracks[key].append({'x': x, 'y': y, 'ke': ke})

    return tracks

def plot_data(tracks):
    birth_x, birth_y, birth_ke = [], [], []
    death_x, death_y, death_ke = [], [], []

    for track_key, data in tracks.items():
        # First point is Birth
        birth = data[0]
        birth_x.append(birth['x'])
        birth_y.append(birth['y'])
        birth_ke.append(birth['ke'])

        # Second point is Death (Last recorded step for this track)
        if len(data) > 1:
            death = data[-1]
            death_x.append(death['x'])
            death_y.append(death['y'])
            death_ke.append(death['ke'])

    if not birth_x:
        print("\n[!] No Quasiparticle data found in this CSV file.")
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

    vmin = min_birth_ke
    vmax = max_birth_ke

    if vmin == vmax:
        norm = mcolors.Normalize(vmin=vmin, vmax=vmax)
    elif SCALE_TYPE == 'log':
        safe_vmin = vmin if vmin > 0 else 1e-6
        norm = mcolors.LogNorm(vmin=safe_vmin, vmax=vmax)
    elif SCALE_TYPE == 'power':
        norm = mcolors.PowerNorm(gamma=POWER_GAMMA, vmin=vmin, vmax=vmax)
    else:  
        norm = mcolors.Normalize(vmin=vmin, vmax=vmax)

    fig, axs = plt.subplots(2, 2, figsize=(14, 10))
    fig.subplots_adjust(right=0.80, hspace=0.35, wspace=0.3)
    ((ax_birth_map, ax_death_map), (ax_birth_hist, ax_death_hist)) = axs

    sc1 = ax_birth_map.scatter(birth_x, birth_y, c=birth_ke, cmap=COLOR_MAP, norm=norm, s=12, alpha=0.8)
    ax_birth_map.set_title("Quasiparticle Birth Locations", fontweight='bold')
    ax_birth_map.set_xlabel("X Position (mm)")
    ax_birth_map.set_ylabel("Y Position (mm)")
    ax_birth_map.set_aspect('equal')
    fig.colorbar(sc1, ax=ax_birth_map, label='Kinetic Energy (eV)', shrink=0.8)

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

    all_ke = birth_ke + (death_ke if death_ke else [])
    hist_vmin = min(all_ke) if min(all_ke) > 0 else 1e-4
    hist_vmax = max(all_ke)
    bins = np.logspace(np.log10(hist_vmin), np.log10(hist_vmax), 40)

    ax_birth_hist.hist(birth_ke, bins=bins, color='#ED7B7B', alpha=0.7, edgecolor='black')
    ax_birth_hist.set_xscale('log')
    ax_birth_hist.set_title("Birth Energy Distribution", fontweight='bold')
    ax_birth_hist.set_xlabel("Kinetic Energy (eV)")
    ax_birth_hist.set_ylabel("Quasiparticle Count")
    ax_birth_hist.grid(True, which="both", ls="--", alpha=0.4)

    if death_ke:
        ax_death_hist.hist(death_ke, bins=bins, color='#F89548', alpha=0.7, edgecolor='black')
        ax_death_hist.set_xscale('log')
        ax_death_hist.set_title("Death Energy Distribution", fontweight='bold')
        ax_death_hist.set_xlabel("Kinetic Energy (eV)")
        ax_death_hist.set_ylabel("Quasiparticle Count")
        ax_death_hist.grid(True, which="both", ls="--", alpha=0.4)
    else:
        ax_death_hist.set_title("Death Energy Distribution (No Data)", fontweight='bold')

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

    fig.text(0.825, 0.5, stats_text, fontsize=10, family='monospace',
             verticalalignment='center', 
             bbox=dict(boxstyle='round,pad=1', facecolor='#fafafa', alpha=0.95, edgecolor='#cccccc'))

    plt.suptitle(f"G4CMP Quasiparticle Tracking ({SCALE_TYPE.upper()} color scale)", fontsize=16, weight='bold', y=0.96)
    plt.savefig("quasiparticle_histogram.png", transparent=True, format="png", dpi=300)
    print("[SUCCESS] Plot saved as quasiparticle_histogram.png")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python diffusion_csv.py <path_to_csv_file>")
        sys.exit(1)
    
    csv_filepath = sys.argv[1]
    track_data = parse_csv_file(csv_filepath)
    plot_data(track_data)