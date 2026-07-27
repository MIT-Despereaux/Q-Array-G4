import pandas as pd
import matplotlib
matplotlib.use('Agg')  # Required for cluster environments without a display
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import numpy as np
import sys

# ==========================================
# CONFIGURATION
# ==========================================
FILE_PATH = "your_data_file.csv"  # CHANGE THIS to your actual output file name
OUTPUT_IMG = "energy_deposition_map.png"

def parse_geant4_csv(filepath):
    """
    Parses the custom Geant4 CSV format. Extracts headers from lines 
    starting with '#column' and then loads the data.
    """
    headers = []
    data_start_line = 0
    
    # 1. Extract the column names from the metadata
    with open(filepath, 'r') as f:
        for i, line in enumerate(f):
            if line.startswith('#column'):
                # Split "#column double edep" -> "edep"
                col_name = line.strip().split()[-1]
                headers.append(col_name)
            elif not line.startswith('#'):
                data_start_line = i
                break
                
    print(f"Detected {len(headers)} columns. Loading data...")
    
    # 2. Load the actual comma-separated data
    try:
        df = pd.read_csv(filepath, skiprows=data_start_line, names=headers, on_bad_lines='skip')
    except Exception as e:
        print(f"Error loading CSV: {e}")
        sys.exit(1)
        
    return df

def plot_spatial_energy(df, save_path):
    """
    Filters data and generates a 3-panel scatter plot (Gamma, Neutron, Combined)
    color-coded by energy deposition using a log scale.
    """
    # 1. Filter for rows where energy was actually deposited
    df_hits = df[df['edep'] > 0].copy()
    
    if df_hits.empty:
        print("No energy deposition (edep > 0) found in the file.")
        return

    print(f"Total valid hits: {len(df_hits)}")

    # 2. Separate by particle type 
    # (Checking both incidentParticle and particleName in case of secondary generation)
    gamma_mask = df_hits['incidentParticle'].str.lower().str.contains('gamma') | \
                 df_hits['particleName'].str.lower().str.contains('gamma')
                 
    neutron_mask = df_hits['incidentParticle'].str.lower().str.contains('neutron') | \
                   df_hits['particleName'].str.lower().str.contains('neutron')

    df_gamma = df_hits[gamma_mask]
    df_neutron = df_hits[neutron_mask]

    print(f"Gamma hits: {len(df_gamma)} | Neutron hits: {len(df_neutron)}")

    # 3. Setup the 1x3 Plotting Figure
    fig, axes = plt.subplots(1, 3, figsize=(24, 7), dpi=200)
    titles = ['Gamma Energy Deposition', 'Neutron Energy Deposition', 'Combined Energy Deposition']
    dataframes = [df_gamma, df_neutron, df_hits]

    # Find global min/max for a consistent colorbar scale across all plots
    min_edep = df_hits['edep'].min()
    max_edep = df_hits['edep'].max()
    norm = mcolors.LogNorm(vmin=min_edep, vmax=max_edep)
    cmap = matplotlib.colormaps['coolwarm']

    # 4. Generate each subplot
    for ax, data, title in zip(axes, dataframes, titles):
        if not data.empty:
            scatter = ax.scatter(
                data['x1'], 
                data['y1'], 
                c=data['edep'], 
                cmap=cmap, 
                norm=norm, 
                s=10,       # Point size
                alpha=0.8,  # Slight transparency to see overlapping hits
                edgecolors='none'
            )
        
        ax.set_title(title, fontsize=16, fontweight='bold')
        ax.set_xlabel('X Position (mm)', fontsize=12)
        ax.set_ylabel('Y Position (mm)', fontsize=12)
        ax.grid(True, linestyle='--', alpha=0.5)
        ax.set_aspect('equal', adjustable='box') # Keeps geometry proportional

    # Add a single colorbar for the whole figure
    cbar = fig.colorbar(scatter, ax=axes.ravel().tolist(), pad=0.02)
    cbar.set_label('Energy Deposited (edep) - Log Scale', rotation=270, labelpad=20, fontsize=14)

    # 5. Save the figure
    plt.tight_layout()
    plt.savefig(save_path, bbox_inches='tight')
    plt.close()
    print(f"Chart successfully saved to: {save_path}")

if __name__ == "__main__":
    data_frame = parse_geant4_csv(FILE_PATH)
    plot_spatial_energy(data_frame, OUTPUT_IMG)