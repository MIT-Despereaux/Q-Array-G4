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

# Load your simulated gammas (adjust filename if needed)
sim_energies = load_simulated_energies('./output/initial_data/primary_gammas.csv')

# 2. Load the reference spectrum data
#ref_df = pd.read_csv('./scripts/Gamma_Spectrum_v2.csv')
# 2. Load the reference spectrum data
ref_df = pd.read_csv('./scripts/Gamma_Spectrum_v2.csv')
ref_df.columns = [c.strip() for c in ref_df.columns]  # Clean up any accidental spaces

E_ref = ref_df.iloc[:, 0].values  # First column: Energy points
B_ref = ref_df.iloc[:, 1].values  # Second column: Weights/Rates

# 3. Normalize the reference data by sum to yield fractional weights
B_ref_fractional = B_ref / np.sum(B_ref)
#B_ref_fractional = B_ref

# 4. Generate the Comparison Plot
plt.figure(figsize=(10, 6))

# Calculate fractional weights for simulated data so height equals relative frequency
sim_weights = np.ones_like(sim_energies) / len(sim_energies)

# Plot Simulated Data
plt.hist(sim_energies, bins=E_ref, weights=sim_weights, alpha=0.35, 
         color='royalblue', edgecolor='blue', label='Simulated Data (Fraction of Total)')

# Plot Reference Data on top
plt.plot(E_ref, B_ref_fractional, color='crimson', linewidth=2, 
         zorder=10, label='Reference Spectrum (Fractional Weights)')

# 5. Formatting the Plot
plt.xlabel('Energy (MeV)')
plt.ylabel('Fraction of Total Particles')
plt.title('Comparison of Simulated Data vs. Reference: Gammas')
plt.xlim(0, max(E_ref)) 
plt.grid(True, linestyle='--', alpha=0.5)
plt.legend(loc='upper right')

# ---- SAVING FEATURE RESTORED ----
plt.savefig('./notebook/image/gamma_comparison_primary.png', dpi=300)
plt.show()