import os
import multiprocessing as mp
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors

# ==========================================
# CONFIGURATION
# ==========================================
LOG_FILE = "geant4_g4cmp_track.log"
X_LIMITS = (-2.0, 2.0)  # Physical bounds in mm
Y_LIMITS = (-2.0, 2.0)  # Physical bounds in mm
GRID_RES = (1000, 1000)

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
    
    # Initialize a local grid to avoid cross-process memory locks
    local_heatmap = np.zeros((len(x_edges)-1, len(y_edges)-1), dtype=np.float64)
    
    xs, ys, sls = [], [], []
    batch_size = 100_000  # Batch size for vectorized histogramming
    
    with open(filename, 'rb') as f:
        f.seek(start_byte)
        
        # If we didn't start at the very beginning, skip the first partial line
        if start_byte != 0:
            f.readline()
            
        while f.tell() < end_byte:
            line = f.readline()
            if not line:
                break
                
            try:
                line_str = line.decode('utf-8')
                
                # Fast string checking (avoids slow Regex)
                if line_str.startswith('G4WT'):
                    parts = line_str.split()
                    
                    # Ensure it's a valid step line by checking length and structure
                    if len(parts) >= 15 and parts[1] == '>' and parts[2].isdigit():
                        x_val = float(parts[3]) * UNIT_TO_MM[parts[4]]
                        y_val = float(parts[5]) * UNIT_TO_MM[parts[6]]
                        sl_val = float(parts[13]) * UNIT_TO_MM[parts[14]]
                        
                        xs.append(x_val)
                        ys.append(y_val)
                        sls.append(sl_val)
                        
                        # Process batch using high-speed numpy histogram2d
                        if len(xs) >= batch_size:
                            h_chunk, _, _ = np.histogram2d(
                                xs, ys, bins=[x_edges, y_edges], weights=sls
                            )
                            local_heatmap += h_chunk
                            xs, ys, sls = [], [], []
                            
            except (ValueError, KeyError, UnicodeDecodeError):
                # Silently skip malformed lines or mid-process warnings
                continue
                
        # Process any remaining data in the buffers
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
    # 1. Define grid edges
    x_edges = np.linspace(x_bounds[0], x_bounds[1], grid_res[0] + 1)
    y_edges = np.linspace(y_bounds[0], y_bounds[1], grid_res[1] + 1)
    
    file_size = os.path.getsize(file_path)
    num_cores = mp.cpu_count()
    chunk_size = file_size // num_cores
    
    # 2. Create byte boundaries for each core
    tasks = []
    for i in range(num_cores):
        start_byte = i * chunk_size
        end_byte = start_byte + chunk_size if i < num_cores - 1 else file_size
        tasks.append((file_path, start_byte, end_byte, x_edges, y_edges))
        
    print(f"File size: {file_size / (1024**3):.2f} GB. Spawning {num_cores} parallel workers...")
    
    # 3. Execute in parallel and sum the resulting heatmaps
    master_heatmap = np.zeros(grid_res, dtype=np.float64)
    
    with mp.Pool(num_cores) as pool:
        for local_result in pool.imap_unordered(process_chunk, tasks):
            master_heatmap += local_result
            print("A worker finished its chunk.")
            
    return master_heatmap, x_edges, y_edges

def plot_quasiparticle_heatmap(heatmap, x_edges, y_edges):
    """
    Visualizes the accumulator grid with coolwarm colormap.
    """
    # Mask unvisited regions to keep them white
    heatmap_masked = np.ma.masked_where(heatmap == 0, heatmap)
    
    cmap = plt.cm.get_cmap('coolwarm').copy()
    cmap.set_bad(color='white')
    
    fig, ax = plt.subplots(figsize=(10, 8), dpi=150)
    extent = [x_edges[0], x_edges[-1], y_edges[0], y_edges[-1]]
    
    # Logarithmic colormap
    norm = mcolors.LogNorm(vmin=heatmap_masked.min(), vmax=heatmap_masked.max())
        
    im = ax.imshow(
        heatmap_masked.T, 
        extent=extent, 
        origin='lower', 
        cmap=cmap, 
        norm=norm, 
        interpolation='nearest',
        aspect='equal'
    )
    
    cbar = fig.colorbar(im, ax=ax, pad=0.02)
    cbar.set_label('Time Spent Correlation', rotation=270, labelpad=20, fontsize=12)
    
    ax.set_xlabel('X Position (mm)', fontsize=12)
    ax.set_ylabel('Y Position (mm)', fontsize=12)
    ax.set_title('Quasiparticle Dwell Distance Heatmap', fontsize=14, fontweight='bold')
    
    plt.tight_layout()
    plt.savefig('quasiparticle_distance_heatmap.png', dpi=300)
    plt.show()

# ==========================================
# EXECUTION
# ==========================================
if __name__ == "__main__":
    # Ensure the script runs properly under Windows/Linux multiprocessing
    mp.freeze_support()
    
    if os.path.exists(LOG_FILE):
        heatmap, x_edges, y_edges = generate_heatmap_parallel(
            LOG_FILE, 
            x_bounds=X_LIMITS, 
            y_bounds=Y_LIMITS, 
            grid_res=GRID_RES
        )
        plot_quasiparticle_heatmap(heatmap, x_edges, y_edges)
    else:
        print(f"Log file '{LOG_FILE}' not found.")