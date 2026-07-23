import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors

def process_large_log_file(file_path, x_bounds, y_bounds, grid_res=(1000, 1000), chunksize=5_000_000):
    """
    Streams a massive log file in chunks and accumulates total step time into a 2D grid.
    
    Parameters:
        file_path (str): Path to the log file.
        x_bounds (tuple): (x_min, x_max) physical sample bounds.
        y_bounds (tuple): (y_min, y_max) physical sample bounds.
        grid_res (tuple): (nx, ny) resolution of the output grid.
        chunksize (int): Number of lines to parse per batch.
        
    Returns:
        heatmap (2D numpy array), x_edges, y_edges
    """
    # 1. Define grid bin edges
    x_edges = np.linspace(x_bounds[0], x_bounds[1], grid_res[0] + 1)
    y_edges = np.linspace(y_bounds[0], y_bounds[1], grid_res[1] + 1)
    
    # 2. Pre-allocate accumulator grid (nx, ny)
    heatmap = np.zeros((grid_res[0], grid_res[1]), dtype=np.float64)
    
    print(f"Processing '{file_path}' in chunks of {chunksize:,} lines...")
    
    # 3. Stream through the file in chunks
    # Adjust `sep`, `comment`, and `usecols` based on your exact G4CMP log format.
    # Assuming columns: x, y, z, dt (or x, y, dt)
    chunk_count = 0
    total_steps = 0
    
    for chunk in pd.read_csv(
        file_path, 
        sep=r'\s+',              # Whitespace separated (adjust if CSV uses comma)
        comment='#',             # Skip Geant4 header comments
        usecols=[0, 1, 3],       # e.g., col 0=x, col 1=y, col 3=dt
        names=['x', 'y', 'dt'], 
        header=None,
        engine='c',              # Fast C parser engine
        chunksize=chunksize
    ):
        chunk_count += 1
        total_steps += len(chunk)
        
        # Accumulate 2D histogram weighted by step duration 'dt'
        h_chunk, _, _ = np.histogram2d(
            chunk['x'].values, 
            chunk['y'].values, 
            bins=[x_edges, y_edges], 
            weights=chunk['dt'].values
        )
        
        heatmap += h_chunk
        print(f"Processed chunk {chunk_count} ({total_steps:,} total steps processed)")
        
    return heatmap, x_edges, y_edges


def plot_quasiparticle_heatmap(heatmap, x_edges, y_edges, norm_type='log'):
    """
    Visualizes the accumulator grid with coolwarm colormap and white unvisited background.
    """
    # 1. Create a masked array for unvisited pixels (heatmap == 0)
    heatmap_masked = np.ma.masked_where(heatmap == 0, heatmap)
    
    # 2. Prepare custom colormap: coolwarm + white for masked/bad values
    cmap = plt.cm.get_cmap('coolwarm').copy()
    cmap.set_bad(color='white')
    
    fig, ax = plt.subplots(figsize=(10, 8), dpi=150)
    
    extent = [x_edges[0], x_edges[-1], y_edges[0], y_edges[-1]]
    
    # 3. Select normalization to handle wide dynamic range
    if norm_type == 'log':
        norm = mcolors.LogNorm(vmin=heatmap_masked.min(), vmax=heatmap_masked.max())
    elif norm_type == 'power':
        norm = mcolors.PowerNorm(gamma=0.5, vmin=heatmap_masked.min(), vmax=heatmap_masked.max())
    else:
        norm = mcolors.Normalize(vmin=heatmap_masked.min(), vmax=heatmap_masked.max())
        
    # Note: np.histogram2d output needs to be transposed (.T) for imshow spatial alignment
    im = ax.imshow(
        heatmap_masked.T, 
        extent=extent, 
        origin='lower', 
        cmap=cmap, 
        norm=norm, 
        interpolation='nearest',
        aspect='equal'
    )
    
    # Colorbar configuration
    cbar = fig.colorbar(im, ax=ax, pad=0.02)
    cbar.set_label('Total Time Spent ($\mu$s or ns)', rotation=270, labelpad=20, fontsize=12)
    
    ax.set_xlabel('X Position (mm)', fontsize=12)
    ax.set_ylabel('Y Position (mm)', fontsize=12)
    ax.set_title('Quasiparticle Dwell Time Heatmap', fontsize=14, fontweight='bold')
    ax.grid(False)
    
    plt.tight_layout()
    plt.show()

# ==========================================
# EXAMPLE EXECUTION
# ==========================================
if __name__ == "__main__":
    # Define physical detector/sample boundaries (e.g., -10mm to +10mm)
    X_LIMITS = (-10.0, 10.0)
    Y_LIMITS = (-10.0, 10.0)
    GRID_RESOLUTION = (1000, 1000) # 1000x1000 pixels grid
    
    LOG_FILE = "geant4_g4cmp_track.log"
    
    # Process & aggregate data
    # heatmap, x_edges, y_edges = process_large_log_file(
    #     LOG_FILE, 
    #     x_bounds=X_LIMITS, 
    #     y_bounds=Y_LIMITS, 
    #     grid_res=GRID_RESOLUTION
    # )
    
    # plot_quasiparticle_heatmap(heatmap, x_edges, y_edges, norm_type='log')
    pass