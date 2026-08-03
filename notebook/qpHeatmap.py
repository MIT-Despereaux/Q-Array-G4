import os
import glob
import multiprocessing as mp
import numpy as np

# 1. CRITICAL FOR CLUSTERS: Disable GUI display before importing pyplot
import matplotlib
matplotlib.use('Agg') 
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors

"""
Heatmap of Immortal QP, good for showing if diffusion is getting stuck on internal boundaries.
"""


# ==========================================
# CONFIGURATION
# ==========================================
INPUT_DIR = "/home/tclassen/projects/build-dspx/logs"       # Directory containing your .log run files
OUTPUT_DIR = "/home/tclassen/projects/output"     # Directory where PNGs will be saved

X_LIMITS = (-2.5, 2.5)  # Physical bounds in mm
Y_LIMITS = (-2.5, 2.5)  # Physical bounds in mm
# Reduced resolution to make the individual pixels slightly larger and more visible on a poster
GRID_RES = (500, 500) 

# Dictionary for fast unit conversion to millimeters (mm)
UNIT_TO_MM = {
    'fm': 1e-12,
    'pm': 1e-9,
    'nm': 1e-6,
    'um': 1e-3,
    'mm': 1.0,
    'cm': 10.0,
    'm': 1000.0
}

def process_chunk(args):
    """
    Worker function to parse a specific byte range of the log file.
    Accumulates step lengths into a local 2D histogram.
    """
    filename, start_byte, end_byte, x_edges, y_edges = args
    
    local_heatmap = np.zeros((len(x_edges)-1, len(y_edges)-1), dtype=np.float64)
    
    xs, ys, sls = [], [], []
    batch_size = 100_000  
    
    with open(filename, 'rb') as f:
        f.seek(start_byte)
        
        if start_byte != 0:
            f.readline()
            
        while f.tell() < end_byte:
            line = f.readline()
            if not line:
                break
                
            try:
                line_str = line.decode('utf-8')
                
                if line_str.startswith('G4WT'):
                    parts = line_str.split()
                    
                    if len(parts) >= 15 and parts[1] == '>' and parts[2].isdigit():
                        x_val = float(parts[3]) * UNIT_TO_MM[parts[4]]
                        y_val = float(parts[5]) * UNIT_TO_MM[parts[6]]
                        sl_val = float(parts[13]) * UNIT_TO_MM[parts[14]]
                        
                        xs.append(x_val)
                        ys.append(y_val)
                        sls.append(sl_val)
                        
                        if len(xs) >= batch_size:
                            h_chunk, _, _ = np.histogram2d(
                                xs, ys, bins=[x_edges, y_edges], weights=sls
                            )
                            local_heatmap += h_chunk
                            xs, ys, sls = [], [], []
                            
            except (ValueError, KeyError, UnicodeDecodeError):
                continue
                
        if xs:
            h_chunk, _, _ = np.histogram2d(
                xs, ys, bins=[x_edges, y_edges], weights=sls
            )
            local_heatmap += h_chunk
            
    return local_heatmap

def generate_heatmap_parallel(file_path, x_bounds, y_bounds, grid_res):
    """
    Splits the file into byte chunks and maps them to multiple CPU cores.
    """
    x_edges = np.linspace(x_bounds[0], x_bounds[1], grid_res[0] + 1)
    y_edges = np.linspace(y_bounds[0], y_bounds[1], grid_res[1] + 1)
    
    file_size = os.path.getsize(file_path)
    
    # Cap cores at 8 to prevent broken pipe errors on login nodes
    num_cores = min(mp.cpu_count(), 8)
    
    if file_size == 0 or num_cores == 0:
        return np.zeros(grid_res, dtype=np.float64), x_edges, y_edges
        
    chunk_size = file_size // num_cores
    
    tasks = []
    for i in range(num_cores):
        start_byte = i * chunk_size
        end_byte = start_byte + chunk_size if i < num_cores - 1 else file_size
        tasks.append((file_path, start_byte, end_byte, x_edges, y_edges))
        
    print(f"File size: {file_size / (1024**3):.2f} GB. Spawning {num_cores} parallel workers...")
    
    master_heatmap = np.zeros(grid_res, dtype=np.float64)
    
    with mp.Pool(num_cores) as pool:
        for local_result in pool.imap_unordered(process_chunk, tasks):
            master_heatmap += local_result
            
    return master_heatmap, x_edges, y_edges

def plot_quasiparticle_heatmap(heatmap, x_edges, y_edges, save_path, title_name):
    """
    Visualizes the grid with a custom high-contrast PowerNorm scale.
    """
    # Create the custom pastel/contrasty colormap requested
    colors = ["#215FAC", "#ED7B7B"]
    cmap = mcolors.LinearSegmentedColormap.from_list("PastelHeat", colors)
    
    # Mask unvisited areas so we can paint them solidly with the background color
    heatmap_masked = np.ma.masked_where(heatmap <= 0, heatmap)
    cmap.set_bad(color="#215FAC")
    
    fig, ax = plt.subplots(figsize=(10, 8), dpi=150)
    extent = [x_edges[0], x_edges[-1], y_edges[0], y_edges[-1]]
    
    # Use PowerNorm (gamma correction). A gamma < 1 pushes lower values up the color scale,
    # making sparse scattering highly visible against the background.
    if heatmap_masked.count() > 0:
        norm = mcolors.PowerNorm(gamma=0.25, vmin=heatmap_masked.min(), vmax=heatmap_masked.max())
    else:
        norm = mcolors.Normalize()
        
    im = ax.imshow(
        heatmap_masked.T, 
        extent=extent, 
        origin='lower', 
        cmap=cmap, 
        norm=norm, 
        interpolation='nearest',
        aspect='equal'
    )
    
    # Adjust colorbar to reflect the custom setup
    cbar = fig.colorbar(im, ax=ax, pad=0.02)
    cbar.set_label('Distance Traveled (Gamma Scaled)', rotation=270, labelpad=20, fontsize=12)
    
    ax.set_xlabel('X Position (mm)', fontsize=12)
    ax.set_ylabel('Y Position (mm)', fontsize=12)
    ax.set_title(f'Quasiparticle Heatmap: {title_name}', fontsize=14, fontweight='bold')
    
    # Make the plot frame blend well with the aesthetic
    for spine in ax.spines.values():
        spine.set_edgecolor('#215FAC')
        spine.set_linewidth(1.5)
    
    plt.tight_layout()
    plt.savefig(save_path, dpi=300)
    plt.close(fig)

# ==========================================
# EXECUTION
# ==========================================
if __name__ == "__main__":
    mp.freeze_support()
    
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    
    log_files = sorted(glob.glob(os.path.join(INPUT_DIR, "*.log")))
    
    if not log_files:
        print(f"No '.log' files found in directory '{INPUT_DIR}'. Please check your path.")
    else:
        print(f"Found {len(log_files)} run files to process.\n")
        
        for idx, file_path in enumerate(log_files, 1):
            file_basename = os.path.splitext(os.path.basename(file_path))[0]
            output_png_path = os.path.join(OUTPUT_DIR, f"{file_basename}_heatmap.png")
            
            print(f"[{idx}/{len(log_files)}] Processing file: {file_path}")
            
            heatmap, x_edges, y_edges = generate_heatmap_parallel(
                file_path, 
                x_bounds=X_LIMITS, 
                y_bounds=Y_LIMITS, 
                grid_res=GRID_RES
            )
            
            plot_quasiparticle_heatmap(
                heatmap, 
                x_edges, 
                y_edges, 
                save_path=output_png_path, 
                title_name=file_basename
            )
            print(f" Saved heatmap to: {output_png_path}\n")
            
        print("All heatmaps generated successfully.")