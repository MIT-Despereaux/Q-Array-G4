import os
import glob
import re
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

# ==========================================
# Configuration & Setup
# ==========================================
CSV_DIRECTORY = '/home/tclassen/projects/output/g4sim/q_array_cmp/'
FILE_PATTERN = 'ISO_AmBe*.csv'

# Column names present in your Geant4 Ntuples
PARTICLE_COL = 'particleName' 
ENERGY_COL = 'edep'

# Binning settings (energies in MeV)
MIN_ENERGY = 0.0001  # Ignores zero/near-zero energy hits
MAX_ENERGY = 0.05    # Adjust based on maximum particle energy expected
NUM_BINS = 1000

# Create histogram bin edges
bins = np.linspace(MIN_ENERGY, MAX_ENERGY, NUM_BINS + 1)
bin_centers = (bins[:-1] + bins[1:]) / 2

particle_histograms = {}
total_histogram = np.zeros(NUM_BINS)

# ==========================================
# Helper Functions
# ==========================================
def get_g4_csv_columns(filepath):
    """Parses `#column <type> <name>` metadata lines from Geant4 CSV files."""
    columns = []
    with open(filepath, 'r') as f:
        for line in f:
            if line.startswith('#column'):
                columns.append(line.strip().split()[-1])
            elif not line.startswith('#'):
                break  # Stop checking once data rows start
    return columns

def generalize_particle_name(name):
    """
    Groups isotopes by stripping atomic mass numbers. 
    Examples: 'Sn119' -> 'Sn', 'Cu63[0.0]' -> 'Cu', 'Pb208' -> 'Pb'
    Standard particles ('gamma', 'e-', 'neutron') are returned unchanged.
    """
    # Regex: Looks for 1 capital letter, 0-1 lowercase letter, followed by 1+ digits
    match = re.match(r'^([A-Z][a-z]?)(\d+)', str(name))
    if match:
        return match.group(1) # Returns just the element symbol
    return name

# ==========================================
# 1. Parse and Bin Data Efficiently
# ==========================================
search_path = os.path.join(CSV_DIRECTORY, FILE_PATTERN)
file_list = glob.glob(search_path)

print(f"Searched path: {search_path}")
print(f"SUCCESS: Found {len(file_list)} matching files out of the directory.")
print("Beginning processing...")

if len(file_list) == 0:
    raise ValueError("No files found! Check your CSV_DIRECTORY path and file naming pattern.")

for file_idx, filepath in enumerate(file_list):
    print(f"Processing file {file_idx + 1}/{len(file_list)}: {os.path.basename(filepath)}")
    
    # Extract column names from the header comments
    columns = get_g4_csv_columns(filepath)
    
    if PARTICLE_COL not in columns or ENERGY_COL not in columns:
        print(f"Warning: Columns {PARTICLE_COL} or {ENERGY_COL} not found in {filepath}. Skipping file.")
        continue

    # Read CSV skipping '#' lines and supplying column names
    chunk_iterator = pd.read_csv(
        filepath, 
        comment='#', 
        names=columns, 
        usecols=[PARTICLE_COL, ENERGY_COL], 
        chunksize=1_000_000
    )
    
    for chunk in chunk_iterator:
        # Filter out rows where energy deposited is less than MIN_ENERGY (removes 0 edep)
        # Using .copy() to avoid Pandas SettingWithCopy warnings when we modify the particle names later
        valid_hits = chunk[chunk[ENERGY_COL] >= MIN_ENERGY].copy()
        
        if valid_hits.empty:
            continue
            
        # Optimize regex by only applying it to unique particle names in the current chunk
        unique_particles = valid_hits[PARTICLE_COL].unique()
        particle_mapping = {p: generalize_particle_name(p) for p in unique_particles}
        
        # Apply the mapping to overwrite the specific isotopes with general element names
        valid_hits[PARTICLE_COL] = valid_hits[PARTICLE_COL].map(particle_mapping)
            
        # Update Total Histogram
        counts, _ = np.histogram(valid_hits[ENERGY_COL], bins=bins)
        total_histogram += counts
        
        # Group by the newly generalized particle type and update particle-specific histograms
        grouped = valid_hits.groupby(PARTICLE_COL)
        for particle, group_data in grouped:
            if particle not in particle_histograms:
                particle_histograms[particle] = np.zeros(NUM_BINS)
                
            p_counts, _ = np.histogram(group_data[ENERGY_COL], bins=bins)
            particle_histograms[particle] += p_counts

print("Data processing complete. Generating plots...")

# ==========================================
# 2. Reference Spectrum Loading
# ==========================================
try:
    ref_df = pd.read_csv('./scripts/ISO_neutron_spectrum.csv')
    ref_df.columns = [c.strip() for c in ref_df.columns]
    E_ref = ref_df.iloc[:, 0].values
    B_ref_fractional = ref_df.iloc[:, 1].values
    has_reference = True
except FileNotFoundError:
    print("Reference spectrum not found. Plotting simulated data only.")
    has_reference = False

# ==========================================
# 3. Plot Total Energy Deposited Spectrum
# ==========================================
plt.figure(figsize=(10, 6))

total_fractional = total_histogram / np.sum(total_histogram) if np.sum(total_histogram) > 0 else total_histogram

plt.stairs(total_fractional, bins, fill=True, alpha=0.4, color='purple', label='Total Simulated Data (All Particles)')

plt.xlabel('Energy Deposited (MeV)')
plt.ylabel('Fraction of Total Hits')
plt.title('Total Energy Deposited Spectrum (All Particles)')
plt.xlim(MIN_ENERGY, MAX_ENERGY)
plt.grid(True, linestyle='--', alpha=0.5)
plt.legend(loc='upper right')

os.makedirs('./notebook/image/', exist_ok=True)
plt.savefig('./notebook/image/ISO_AmBe_Total_energy_spectrum.png', transparent=False, format="png", dpi=300)
plt.show()

# ==========================================
# 4. Plot Individual Particle Spectra
# ==========================================
colors = plt.cm.tab10.colors 

for i, (particle_type, counts) in enumerate(particle_histograms.items()):
    plt.figure(figsize=(10, 6))
    
    total_particle_hits = np.sum(counts)
    if total_particle_hits == 0:
        continue
        
    fractional_counts = counts / total_particle_hits
    
    c = colors[i % len(colors)]
    plt.stairs(fractional_counts, bins, fill=True, alpha=0.4, color=c, 
               label=f'Simulated Data: {particle_type}')
    
    if has_reference:
        plt.plot(E_ref, B_ref_fractional, color='black', linewidth=2, 
                 zorder=10, label='Reference Spectrum')

    plt.xlabel('Energy Deposited (MeV)')
    plt.ylabel('Fraction of Total Hits')
    plt.title(f'Energy Deposited Spectrum: {particle_type}')
    plt.xlim(MIN_ENERGY, MAX_ENERGY)
    plt.grid(True, linestyle='--', alpha=0.5)
    plt.legend(loc='upper right')
    
    safe_name = str(particle_type).replace('/', '_').replace(' ', '_')
    save_path = f'./notebook/image/ISO_AmBe_{safe_name}_energy_graph.png'
    plt.savefig(save_path, transparent=False, format="png", dpi=300)
    plt.show()
    
print("All charts generated successfully.")