import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

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

# ---- 2. LOAD SIMULATED DATASETS WITH CLARIFIED FILENAMES ----
# audit_gammas.csv contains the signal + background noise
total_energies = load_simulated_energies('./output/initial_data/audited_gammas.csv')

# audit_gamma_environmental.csv contains the environmental noise alone
bg_energies = load_simulated_energies('./output/initial_data/audited_gammas_environmental.csv')

# ---- 3. LOAD REFERENCE SPECTRUM ----
ref_df = pd.read_csv('./scripts/Gamma_Spectrum_v2.csv')
ref_df.columns = [c.strip() for c in ref_df.columns]  # Clean up any accidental spaces

E_ref = ref_df.iloc[:, 0].values  # First column: Energy points
B_ref = ref_df.iloc[:, 1].values  # Second column: Weights/Rates

# Normalize the reference data by sum to yield fractional weights
B_ref_fractional = B_ref / np.sum(B_ref)

# ---- 4. COMPUTE HISTOGRAMS AND SUBTRACT BACKGROUND ----
# Compute raw counts for both datasets using identical reference bins (E_ref)
total_counts, _ = np.histogram(total_energies, bins=E_ref)
bg_counts, _ = np.histogram(bg_energies, bins=E_ref)

# IMPORTANT NOTE ON SCALING:
# If both of your simulations ran for the exact same amount of time / same number 
# of incident particles, leave scale_factor = 1.0.
# If the background simulation ran longer or shorter than the main simulation, 
# set scale_factor = (total_sim_time / bg_sim_time) to equalize them.
scale_factor = 1.0  

# Subtract environmental background from total data
signal_counts = total_counts - (scale_factor * bg_counts)

# Convert the subtracted counts to fractional weights relative to the total particles
signal_fraction = signal_counts / len(total_energies)

# ---- 5. GENERATE THE COMPARISON PLOT ----
fig, ax = plt.subplots(figsize=(10, 6))

# Calculate midpoints of the energy bins to use with plt.hist weights trick
bin_centers = (E_ref[:-1] + E_ref[1:]) / 2.0

# Plot Background-Subtracted Data
ax.hist(bin_centers, bins=E_ref, weights=signal_fraction, alpha=0.35, 
        color='royalblue', edgecolor='blue', label='Background-Subtracted Data (audit_gammas - environmental)')

# Plot Reference Data on top
ax.plot(E_ref, B_ref_fractional, color='crimson', linewidth=2, 
        zorder=10, label='Reference Spectrum (Gamma_Spectrum_v2)')

# ---- 6. FORMATTING THE PLOT ----
ax.set_xlabel('Energy ({MeV})')
ax.set_ylabel('Fraction of Total Particles')
ax.set_title('Background-Subtracted Data vs. Reference Spectrum')
ax.set_xlim(0, max(E_ref)) 
ax.grid(True, linestyle='--', alpha=0.5)
ax.legend(loc='upper right')

# Save the updated chart
plt.savefig('./notebook/image/chart/gammas_subtracted_comparison.png', dpi=300)