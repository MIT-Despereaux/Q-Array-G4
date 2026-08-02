import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# ==========================================
# 1. READ DATA
# ==========================================
# --- Simulated Data ---
sim_file = '../build-dspx/NoCMP_Edep_SpectrumEnergySpectrum.csv'
df_sim = pd.read_csv(sim_file, names=['particle', 'initial_energy', 'deposited_energy'])
df_sim['energy_keV'] = df_sim['deposited_energy'] / 1000.0

is_neutron = df_sim['particle'].str.contains('neutron', case=False, na=False)
is_gamma = df_sim['particle'].str.contains('gamma', case=False, na=False)

# --- Experimental Data ---
exp_file = '../build-dspx/MIT_Sn_Au_Feb2026_Subtracted_Corrected_1D.csv'
df_exp = pd.read_csv(exp_file)

# ==========================================
# 2. DYNAMIC BINNING (Matching Experimental)
# ==========================================
# Extract experimental bin centers directly
exp_centers = df_exp['energy_keV'].values

# Calculate the precise bin edges from the experimental centers
bin_width = exp_centers[1] - exp_centers[0]
bins = np.append(exp_centers - (bin_width / 2), exp_centers[-1] + (bin_width / 2))

# ==========================================
# 3. SCALING FACTORS 
# ==========================================
activity_ci = 0.01          # 10 mCi
n_per_sec_1Ci = 2.2e6       # AmBe nominal yield per Curie
n_per_sec = activity_ci * n_per_sec_1Ci  # 22,200 neutrons/s
simulated_events = 25e6     # 25 million runs
sim_time = simulated_events / n_per_sec  # ~1126 seconds

efficiency = 0.77 

# Scale factor: (1 / (time * bin_width)) * efficiency
scale_factor = (1.0 / (sim_time * bin_width)) * efficiency

# Function to calculate manually scaled rates and errors
def get_scaled_rates(data):
    counts, _ = np.histogram(data, bins=bins)
    errors = np.sqrt(counts)
    return exp_centers, counts * scale_factor, errors * scale_factor

bin_centers, rate_total, err_total = get_scaled_rates(df_sim['energy_keV'])
_, rate_n, err_n = get_scaled_rates(df_sim[is_neutron]['energy_keV'])
_, rate_g, err_g = get_scaled_rates(df_sim[is_gamma]['energy_keV'])

# ==========================================
# 4. PLOTTING SETUP
# ==========================================
c_total   = '#ED7B7B' 
c_neutron = '#215FAC' 
c_gamma   = "#EC2A8C" 
c_accent  = '#9E6ED0' 
c_exp     = '#FF8C00' 

def plot_spectrum(x_min, x_max, filename, log_scale=False):
    fig, ax = plt.subplots(figsize=(10, 6))
    fig.patch.set_facecolor('white')

    ax.set_title(f'Deposited Energy Spectrum Rate ({x_min} - {x_max} keV)', fontsize=14, fontweight='bold')

    # --- Plot Simulated Data ---
    ax.bar(bin_centers, rate_total, width=bin_width, color=c_total, alpha=0.4, 
           yerr=err_total, ecolor=c_accent, capsize=3, label='Sim Total')
    
    w_n = np.ones(len(df_sim[is_neutron])) * scale_factor
    ax.hist(df_sim[is_neutron]['energy_keV'], bins=bins, weights=w_n,
            color=c_neutron, histtype='step', linewidth=1.5, label='Sim Neutrons')
    ax.errorbar(bin_centers, rate_n, yerr=err_n, fmt='none', 
                ecolor=c_neutron, capsize=2, alpha=0.7)

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
    
    # --- Dynamic Y-Axis Scaling ---
    sim_mask = (bin_centers >= x_min) & (bin_centers <= x_max)
    max_sim_y = np.max(rate_total[sim_mask] + err_total[sim_mask]) if sim_mask.any() else 0
    min_sim_y = np.min(rate_total[sim_mask] - err_total[sim_mask]) if sim_mask.any() else 0

    if not exp_subset.empty:
        max_exp_y = np.max(exp_subset['subtracted_rate'] + exp_subset['subtracted_rate_err'])
        min_exp_y = np.min(exp_subset['subtracted_rate'] - exp_subset['subtracted_rate_err'])
    else:
        max_exp_y, min_exp_y = 0, 0

    max_y = max(max_sim_y, max_exp_y)

    if log_scale:
        ax.set_yscale('log')
        # Find the smallest positive value to set a clean bottom bound for the log axis
        sim_pos = rate_total[sim_mask]
        min_pos_sim = np.min(sim_pos[sim_pos > 0]) if np.any(sim_pos > 0) else 1e-5
        
        exp_pos = exp_subset['subtracted_rate']
        min_pos_exp = np.min(exp_pos[exp_pos > 0]) if np.any(exp_pos > 0) else 1e-5
        
        min_y = min(min_pos_sim, min_pos_exp)
        ax.set_ylim(min_y * 0.5, max_y * 2.0) # slightly wider buffer for log view
    else:
        min_y = min(0, min(min_sim_y, min_exp_y)) 
        ax.set_ylim(min_y, max_y * 1.05)

    ax.set_xlim(x_min, x_max)
    ax.set_xlabel('Deposited Energy (keV)', fontsize=12)
    ax.set_ylabel('Rate (Counts / s / keV)', fontsize=12)
    ax.grid(color=c_accent, linestyle=':', alpha=0.3)
    ax.legend(loc='upper right')

    plt.tight_layout()
    plt.savefig(filename, transparent=True, dpi=300)
    plt.show()

# ==========================================
# 5. GENERATE PLOTS
# ==========================================
# Plot 1: Full Range (0 - 1400 keV) - LOG SCALE
plot_spectrum(0, 1400, 'Spectrum_Overlay_0_1400_Log.png', log_scale=True)

# Plot 2: Low Energy Focus (24 - 200 keV) - LINEAR SCALE
plot_spectrum(24, 200, 'Spectrum_Overlay_0_200_Linear.png', log_scale=False)