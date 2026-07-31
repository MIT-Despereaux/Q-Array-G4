import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# ==========================================
# 1. READ DATA
# ==========================================
# --- Simulated Data ---
sim_file = '../build-dspx/NoCMP_Edep_SpectrumEnergySpectrum.csv'
df_sim = pd.read_csv(sim_file, names=['particle', 'initial_energy', 'deposited_energy'])

# Convert deposited energy from eV to keV
df_sim['energy_keV'] = df_sim['deposited_energy'] / 1000.0

# Define particle filters
is_neutron = df_sim['particle'].str.contains('neutron', case=False, na=False)
is_gamma = df_sim['particle'].str.contains('gamma', case=False, na=False)

# --- Experimental Data ---
exp_file = '../build-dspx/MIT_Sn_Au_Feb2026_Subtracted_Corrected_1D.csv'
df_exp = pd.read_csv(exp_file)

# ==========================================
# 2. SCALING FACTORS & BINNING (5 keV)
# ==========================================
bin_width = 5
max_energy = 1400 
bins = np.arange(0, max_energy + bin_width, bin_width)

# Calculate Simulated Time & Scaling Factor (10 mCi Pathway)
activity_ci = 0.01          # 10 mCi
n_per_sec_1Ci = 2.2e6       #  AmBe nominal yield per Curie
n_per_sec = activity_ci * n_per_sec_1Ci  # 22,200 neutrons/s
simulated_events = 25e6     # 25 million runs
sim_time = simulated_events / n_per_sec  # ~1126 seconds

# Scale factor to convert raw counts per bin to Counts / s / keV
scale_factor = 1.0 / (sim_time * bin_width)

# Function to calculate manually scaled rates and errors
def get_scaled_rates(data):
    counts, edges = np.histogram(data, bins=bins)
    # The relative error (sqrt(N)/N) stays the same, so absolute error scales linearly
    errors = np.sqrt(counts)
    centers = (edges[:-1] + edges[1:]) / 2
    return centers, counts * scale_factor, errors * scale_factor

# Calculate rates for bars/errorbars
bin_centers, rate_total, err_total = get_scaled_rates(df_sim['energy_keV'])
_, rate_n, err_n = get_scaled_rates(df_sim[is_neutron]['energy_keV'])
_, rate_g, err_g = get_scaled_rates(df_sim[is_gamma]['energy_keV'])

# ==========================================
# 3. PLOTTING SETUP (Unified Axis)
# ==========================================
c_total   = '#ED7B7B' # Simulated Total
c_neutron = '#215FAC' # Neutrons
c_gamma   = "#EC2A8C" # Gammas
c_accent  = '#9E6ED0' # Sim errors and grid
c_exp     = '#FF8C00' # Bright Orange for Experimental Scatter

def plot_spectrum(x_min, x_max, filename):
    fig, ax = plt.subplots(figsize=(10, 6))
    fig.patch.set_facecolor('white')

    ax.set_title(f'Deposited Energy Spectrum Rate ({x_min} - {x_max} keV)', fontsize=14, fontweight='bold')

    # --- Plot Simulated Data ---
    # Total (Bars + Errors)
    ax.bar(bin_centers, rate_total, width=bin_width, color=c_total, alpha=0.4, 
           yerr=err_total, ecolor=c_accent, capsize=3, label='Sim Total')
    
    # Neutrons (Step outline + Error bars)
    # We use weights to scale the hist plot internally to match the errors
    w_n = np.ones(len(df_sim[is_neutron])) * scale_factor
    ax.hist(df_sim[is_neutron]['energy_keV'], bins=bins, weights=w_n,
            color=c_neutron, histtype='step', linewidth=1.5, label='Sim Neutrons')
    ax.errorbar(bin_centers, rate_n, yerr=err_n, fmt='none', 
                ecolor=c_neutron, capsize=2, alpha=0.7)

    # Gammas (Step outline + Error bars)
    w_g = np.ones(len(df_sim[is_gamma])) * scale_factor
    ax.hist(df_sim[is_gamma]['energy_keV'], bins=bins, weights=w_g,
            color=c_gamma, histtype='step', linewidth=1.5, label='Sim Gammas')
    ax.errorbar(bin_centers, rate_g, yerr=err_g, fmt='none', 
                ecolor=c_gamma, capsize=2, alpha=0.7)

    # --- Plot Experimental Data ---
    mask = (df_exp['energy_keV'] >= x_min) & (df_exp['energy_keV'] <= x_max)
    exp_subset = df_exp[mask]

    ax.errorbar(exp_subset['energy_keV'], exp_subset['subtracted_rate'], 
                yerr=exp_subset['subtracted_rate_err'], fmt='o', markersize=4,
                color=c_exp, ecolor=c_exp, capsize=3, label='Exp Rate')
    
    # --- Axis Formatting ---
    ax.set_xlim(x_min, x_max)
    ax.set_xlabel('Deposited Energy (keV)', fontsize=12)
    ax.set_ylabel('Rate (Counts / s / keV)', fontsize=12)
    ax.grid(color=c_accent, linestyle=':', alpha=0.3)
    ax.legend(loc='upper right')

    plt.tight_layout()
    plt.savefig(filename, dpi=300)
    plt.show()

# ==========================================
# 4. GENERATE PLOTS
# ==========================================
# Plot 1: Full Range (0 - 1400 keV)
plot_spectrum(0, 1400, 'Spectrum_Overlay_0_1400_3.png')

# Plot 2: Low Energy Focus (0 - 200 keV)
plot_spectrum(24, 200, 'Spectrum_Overlay_0_200_3.png')