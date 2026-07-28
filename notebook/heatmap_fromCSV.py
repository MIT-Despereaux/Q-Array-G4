import os
import glob
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors

# =========================================================================
# CONFIGURATION & PARAMETERS
# =========================================================================
MATRIX_DIR = "/home/tclassen/projects/output/matrices"
OUTPUT_DIR = "/home/tclassen/projects/output"

X_LIMITS = (-2.5, 2.5)  # Physical bounds (mm)
Y_LIMITS = (-2.5, 2.5)  # Physical bounds (mm)

# =========================================================================
# CUSTOM SIGMOID NORMALIZER
# =========================================================================
class SigmoidNorm(mcolors.Normalize):
    def __init__(self, vmin=None, vmax=None, sharpness=10, midpoint=0.5, clip=False):
        super().__init__(vmin, vmax, clip)
        self.sharpness = sharpness
        self.midpoint = midpoint

    def __call__(self, value, clip=None):
        x, is_scalar = self.process_value(value)
        self.autoscale_None(x)
        if self.vmin == self.vmax:
            return np.ma.masked_array(np.zeros_like(x), mask=np.ma.getmask(x))
        
        x_norm = (x - self.vmin) / (self.vmax - self.vmin)
        shifted = (x_norm - self.midpoint) * self.sharpness
        result = 1.0 / (1.0 + np.ma.exp(-shifted))
        
        min_val = 1.0 / (1.0 + np.exp((0.0 - self.midpoint) * self.sharpness))
        max_val = 1.0 / (1.0 + np.exp((1.0 - self.midpoint) * self.sharpness))
        
        result = (result - min_val) / (max_val - min_val)
        
        if is_scalar:
            result = result[0]
        return result

# =========================================================================
# MODULAR FUNCTIONS
# =========================================================================

def load_and_aggregate_matrices(matrix_dir):
    matrix_files = glob.glob(os.path.join(matrix_dir, "matrix_task_*.csv"))
    if not matrix_files:
        raise FileNotFoundError(f"No matrix CSV files found in {matrix_dir}")

    print(f"Found {len(matrix_files)} matrix file(s). Aggregating...")
    master_matrix = None

    for fpath in matrix_files:
        data = np.loadtxt(fpath, delimiter=",")
        if master_matrix is None:
            master_matrix = data
        else:
            master_matrix += data

    master_path = os.path.join(OUTPUT_DIR, "master_heatmap_1000x1000.csv")
    np.savetxt(master_path, master_matrix, delimiter=",", fmt="%.6e")
    print(f"Master aggregated matrix saved to: {master_path}")

    return master_matrix

def create_color_norm(heatmap_data, scale_type='log', upper_percentile=99.9):
    """
    Generates normalization.
    - Captures ALL non-zero data (vmin = absolute minimum of valid data).
    - upper_percentile clips extreme outliers to keep the warm scale legible.
    """
    masked_data = np.ma.masked_where(heatmap_data <= 0, heatmap_data)
    valid_data = masked_data.compressed()
    
    if len(valid_data) == 0:
        return mcolors.Normalize(vmin=0, vmax=1)

    # vmin is strictly the lowest non-zero value so every hit registers as warm.
    vmin = valid_data.min()
    vmax = np.percentile(valid_data, upper_percentile)
    
    if vmin >= vmax:
        vmax = valid_data.max()

    if scale_type == 'sigmoid':
        return SigmoidNorm(vmin=vmin, vmax=vmax, sharpness=10, midpoint=0.5)
    elif scale_type == 'log':
        return mcolors.LogNorm(vmin=vmin, vmax=vmax)
    else: # linear
        return mcolors.Normalize(vmin=vmin, vmax=vmax)

def render_heatmap(matrix, x_bounds, y_bounds, output_path, scale_type='log'):
    x_edges = np.linspace(x_bounds[0], x_bounds[1], matrix.shape[0] + 1)
    y_edges = np.linspace(y_bounds[0], y_bounds[1], matrix.shape[1] + 1)
    
    # ---------------------------------------------------------
    # COLORMAP SETUP
    # ---------------------------------------------------------
    # Create a warm gradient: Light Peach -> Orange -> Pink/Red -> Deep Crimson
    colors = ["#FFDAB9", "#FFA500", "#FF4500", "#DC143C", "#8B0000"]
    cmap = mcolors.LinearSegmentedColormap.from_list("WarmIntensity", colors)
    
    # Mask strictly sets any value <= 0 to the background blue
    matrix_masked = np.ma.masked_where(matrix <= 0, matrix)
    cmap.set_bad(color="#215FAC") 

    fig, ax = plt.subplots(figsize=(10, 8), dpi=300)
    extent = [x_edges[0], x_edges[-1], y_edges[0], y_edges[-1]]

    norm = create_color_norm(matrix_masked, scale_type=scale_type, upper_percentile=99.9)

    im = ax.imshow(
        matrix_masked.T, 
        extent=extent, 
        origin='lower', 
        cmap=cmap, 
        norm=norm, 
        interpolation='nearest',
        aspect='equal'
    )

    cbar = fig.colorbar(im, ax=ax, pad=0.02)
    cbar.set_label(f'Path Length (Scale: {scale_type.capitalize()})', rotation=270, labelpad=20, fontsize=12)

    ax.set_xlabel('X Position (mm)', fontsize=12)
    ax.set_ylabel('Y Position (mm)', fontsize=12)
    ax.set_title('Quasiparticle Trajectory Heatmap (1000x1000 Grid)', fontsize=14, fontweight='bold')

    # Match the axes frame to the background blue for aesthetics
    for spine in ax.spines.values():
        spine.set_edgecolor('#215FAC')
        spine.set_linewidth(1.5)

    plt.tight_layout()
    plt.savefig(output_path, dpi=300)
    plt.close(fig)
    print(f"Heatmap figure successfully rendered to: {output_path}")

# =========================================================================
# MAIN EXECUTION PIPELINE
# =========================================================================
if __name__ == "__main__":
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    master_grid = load_and_aggregate_matrices(MATRIX_DIR)

    # Automatically generate Log and Linear versions for comparison
    out_figure_log = os.path.join(OUTPUT_DIR, "quasiparticle_heatmap_log.png")
    render_heatmap(master_grid, X_LIMITS, Y_LIMITS, out_figure_log, scale_type='log')

    out_figure_linear = os.path.join(OUTPUT_DIR, "quasiparticle_heatmap_linear.png")
    render_heatmap(master_grid, X_LIMITS, Y_LIMITS, out_figure_linear, scale_type='linear')