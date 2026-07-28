import sys
import os
import argparse
import signal
import numpy as np

# Fast conversion lookup to millimeters
UNIT_TO_MM = {
    'fm': 1e-12, 'pm': 1e-9, 'nm': 1e-6,
    'um': 1e-3,  'mm': 1.0,  'cm': 10.0, 'm': 1000.0
}

# Global variables for safe signal handling
heatmap = None
output_path = ""

def save_matrix():
    """Flushes the current 1000x1000 grid state to a CSV file."""
    global heatmap, output_path
    if heatmap is not None and output_path:
        os.makedirs(os.path.dirname(output_path), exist_ok=True)
        # Saves 1000x1000 grid as scientific notation CSV (~8MB)
        np.savetxt(output_path, heatmap, delimiter=",", fmt="%.6e")
        print(f"\n[STREAM PIPELINE] Matrix safely saved to: {output_path}", flush=True)

def handle_slurm_timeout(signum, frame):
    """Triggered by SLURM 60 seconds before hard kill."""
    print("\n[STREAM PIPELINE] Received SLURM SIGTERM timeout signal! Saving matrix progress...", flush=True)
    save_matrix()
    sys.exit(0)

def process_stream(x_bounds=(-2.5, 2.5), y_bounds=(-2.5, 2.5), res=(1000, 1000)):
    global heatmap
    x_edges = np.linspace(x_bounds[0], x_bounds[1], res[0] + 1)
    y_edges = np.linspace(y_bounds[0], y_bounds[1], res[1] + 1)
    heatmap = np.zeros(res, dtype=np.float64)
    
    xs, ys, sls = [], [], []
    batch_size = 100_000
    lines_parsed = 0

    # Read line-by-line directly from Geant4 standard output pipe
    for line in sys.stdin:
        # Silently ignore non-tracking lines, vacuum errors, or initialization text
        if not (line.startswith('G4WT') or line.startswith('   ')):
            continue
            
        parts = line.split()
        # Validate tracking format: expecting step number, position coordinates, units, and step length
        if len(parts) >= 15 and parts[1] == '>' and parts[2].isdigit():
            try:
                x_val = float(parts[3]) * UNIT_TO_MM[parts[4]]
                y_val = float(parts[5]) * UNIT_TO_MM[parts[6]]
                sl_val = float(parts[13]) * UNIT_TO_MM[parts[14]]

                xs.append(x_val)
                ys.append(y_val)
                sls.append(sl_val)

                if len(xs) >= batch_size:
                    h_chunk, _, _ = np.histogram2d(xs, ys, bins=[x_edges, y_edges], weights=sls)
                    heatmap += h_chunk
                    xs, ys, sls = [], [], []
                    lines_parsed += batch_size
                    
                    # Auto-checkpoint every 10 million steps as insurance against node power loss
                    if lines_parsed % 10_000_000 == 0:
                        save_matrix()

            except (ValueError, KeyError, IndexingError):
                # Drops vacuum error lines or unformatted text gracefully
                continue

    # Flush remaining steps if particle run finishes normally before timeout
    if xs:
        h_chunk, _, _ = np.histogram2d(xs, ys, bins=[x_edges, y_edges], weights=sls)
        heatmap += h_chunk
    
    save_matrix()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Stream Geant4 tracking stdout directly to a 1000x1000 CSV matrix.")
    parser.add_argument("--output", required=True, help="Path to save output CSV matrix")
    args = parser.parse_args()

    output_path = args.output

    # Register SLURM cancellation / timeout signals
    signal.signal(signal.SIGTERM, handle_slurm_timeout)
    signal.signal(signal.SIGINT, handle_slurm_timeout)

    process_stream(res=(1000, 1000))