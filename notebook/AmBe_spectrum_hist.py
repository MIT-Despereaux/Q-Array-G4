import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# ==========================================
# 1. READ DATA (Non-Destructive)
# ==========================================
# Read the CSV without altering the source file. 
file_path = '../build-dspx/SpectrumEnergySummary.csv'
df = pd.read_csv(file_path, names=['particle', 'initial_energy', 'deposited_energy'])

# Convert deposited energy from eV to keV for the histogram
df['energy_keV'] = df['deposited_energy'] / 1000.0

# Define particle filters
is_neutron = df['particle'].str.contains('neutron', case=False, na=False)
is_gamma = df['particle'].str.contains('gamma', case=False, na=False)

# ==========================================
# 2. BINNING & COUNTS (UNWEIGHTED)
# ==========================================
# Create bins every 25 keV up to the truncated maximum of 1400 keV
bin_width = 25
max_energy = 1400 
bins = np.arange(0, max_energy + bin_width, bin_width)

# Calculate Histogram & Errors Manually
counts, edges = np.histogram(df['energy_keV'], bins=bins)
# Standard Poisson statistical error for raw counts
errors = np.sqrt(counts) 
bin_centers = (edges[:-1] + edges[1:]) / 2

# ==========================================
# 3. PLOTTING SETUP (Colors & Styles)
# ==========================================
c_total   = '#ED7B7B' # Highest priority
c_neutron = '#215FAC' # Second priority
c_gamma   = "#EC2A8C" # Third priority
c_accent  = '#9E6ED0' # Purple accent for errors and grid

fig, ax = plt.subplots(figsize=(10, 6))
fig.patch.set_facecolor('white')

ax.set_title('Unweighted Deposited Energy Spectrum', fontsize=14, fontweight='bold')

# --- Plot Main Data (Bars with Errors) ---
ax.bar(bin_centers, counts, width=bin_width, color=c_total, alpha=0.4, 
       yerr=errors, ecolor=c_accent, capsize=3, label='Total Spectrum (Raw)')

# --- Plot Components (Thinner Lines) ---
ax.hist(df[is_neutron]['energy_keV'], bins=bins, 
        color=c_neutron, histtype='step', linewidth=1, label='Neutrons')
ax.hist(df[is_gamma]['energy_keV'], bins=bins, 
        color=c_gamma, histtype='step', linewidth=1, label='Gammas')

# --- Formatting ---
ax.set_xlim(0, max_energy)
ax.set_xlabel('Deposited Energy (keV)', fontsize=12)
ax.set_ylabel('Counts / 25 keV', fontsize=12)
ax.grid(color=c_accent, linestyle=':', alpha=0.3)
ax.legend(loc='upper right')

plt.tight_layout()
plt.savefig('Spectrum_Histogram_Truncated_Unweighted.png', dpi=300)
plt.show()