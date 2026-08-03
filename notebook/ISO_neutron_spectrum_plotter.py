import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

"""
ISO AmBe spectrum versus simulated, less useful.
"""

# 1. Load and parse the raw simulation data (handling keV vs MeV)
def load_simulated_energies(filename):
    energies_mev = []
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or ',' not in line:
                continue
            
            # Split by the comma (e.g., "389.5,keV" -> "389.5" and "keV")
            val_str, unit_str = line.split(',')
            try:
                val = float(val_str)
                unit = unit_str.strip().lower()
                
                # Convert keV to MeV if necessary
                if 'kev' in unit:
                    val = val / 1000.0
                
                energies_mev.append(val)
            except ValueError:
                # Skips header lines or malformed lines gracefully
                continue
                
    return np.array(energies_mev)

# Load your simulated gammas (adjust filename if needed)
sim_energies = load_simulated_energies('./output/initial_data/primary_neutrons.csv')

# 2. Load the reference spectrum data
ref_df = pd.read_csv('./scripts/ISO_neutron_spectrum.csv')
ref_df.columns = [c.strip() for c in ref_df.columns]  # Clean up any accidental spaces

E_ref = ref_df.iloc[:, 0].values  # First column: Energy points
B_ref = ref_df.iloc[:, 1].values  # Second column: Weights/Rates

# 3. Normalize the reference data by sum to yield fractional weights
B_ref_fractional = B_ref

# 4. Generate the Comparison Plot
plt.figure(figsize=(10, 6))

# Define uniform binning starting at 0
bin_width = 0.2
max_E = 11.0 
num_bins = int(max_E / bin_width) + 1
bins = np.linspace(0, max_E, num_bins)
bin_centers = bins[:-1] + (bin_width / 2)

# Compute histogram arrays for simulated data
counts, edges = np.histogram(sim_energies, bins=bins)
total_particles = len(sim_energies)
fractions = counts / total_particles

# Assuming Poisson statistics for error bars: error = sqrt(N) / N_total
fraction_errors = np.sqrt(counts) / total_particles

# Plot Simulated Data as a bar chart with explicit error bars
plt.bar(edges[:-1], fractions, width=bin_width, align='edge', 
        alpha=0.35, color='#ed7b7b', edgecolor='#ed7b7b', 
        yerr=fraction_errors, ecolor='#EC2A8C', capsize=2, 
        label='Simulated Data (Fraction of Total)')

# Interpolate and plot reference data aligned to bin centers
ref_binned = np.interp(bin_centers, E_ref, B_ref_fractional)
plt.plot(bin_centers, ref_binned, color='royalblue', linewidth=2, 
         zorder=10, label='Reference Spectrum')

# 5. Formatting the Plot
plt.xlabel('Energy (MeV)')
plt.ylabel(f'Fraction of total particles (per {bin_width} MeV bin)')
plt.title('Comparison of Simulated Data vs. Reference: Gammas')
plt.xlim(0, max_E) 
plt.grid(True, linestyle='--', alpha=0.5)
plt.legend(loc='upper right')

# ---- SAVING FEATURE RESTORED ----
plt.savefig('./notebook/image/neutron_rose_graph.png', transparent=True, format="png", dpi=300)
plt.show()