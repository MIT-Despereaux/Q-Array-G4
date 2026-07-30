import pandas as pd
import numpy as np
import re
import matplotlib.pyplot as plt
import os

class QuasiparticleAnalyzer:
    def __init__(self, slurm_file, jj_csv_file, zone_radius_mm=0.1, time_bin_size_ns=100.0):
        """
        Initializes the analyzer with file paths and parameters.
        """
        self.slurm_file = slurm_file
        self.jj_csv_file = jj_csv_file
        self.intervals_csv = "qp_intervals.csv"
        self.binned_csv = "qp_binned_counts.csv"
        
        self.zone_radius_mm = zone_radius_mm
        self.time_bin_size_ns = time_bin_size_ns
        
        # Pre-compile regex for performance
        # Extracts: TrackID, Time(ns), X(mm), Y(mm)
        self.regex = re.compile(
            r"\[QP STEP REGISTERED\]\s+TrackID:\s+(\d+)\s+\|\s+Time:\s+([\d\.]+)\s+ns\s+\|\s+Pos:\s+\(([\-\d\.]+),([\-\d\.]+),[\-\d\.]+\)\s+mm"
        )
        
        self.jj_coords = self._load_jj_coordinates()

    def _load_jj_coordinates(self):
        """
        Loads the coordinates for the Josephson Junctions.
        """
        if os.path.exists(self.jj_csv_file):
            return pd.read_csv(self.jj_csv_file)
        else:
            print(f"Warning: {self.jj_csv_file} not found. Using mock data for testing.")
            return pd.DataFrame({
                'JJ_ID': [1, 2, 3, 4, 5, 6, 7, 8],
                'X_mm': [0.0, 0.5, -0.5, 1.0, -1.0, 0.0, 0.0, 1.5],
                'Y_mm': [1.0, 1.0, 1.0, 1.5, 1.5, 0.5, 1.5, 2.0]
            })

    def check_segment_intersection(self, pos_start, pos_end, zone_center):
        """
        Checks if a straight line segment intersects a circular zone.
        Returns (True, entry_ratio, exit_ratio) or (False, None, None)
        """
        pos_start = np.array(pos_start)
        pos_end = np.array(pos_end)
        center = np.array(zone_center)
        
        step_vector = pos_end - pos_start
        start_offset = pos_start - center
        
        quadratic_a = np.dot(step_vector, step_vector)
        quadratic_b = 2 * np.dot(start_offset, step_vector)
        quadratic_c = np.dot(start_offset, start_offset) - self.zone_radius_mm**2
        
        if quadratic_a == 0:
            if quadratic_c <= 0:
                return True, 0.0, 1.0
            return False, None, None
            
        discriminant = quadratic_b**2 - 4 * quadratic_a * quadratic_c
        
        if discriminant < 0:
            return False, None, None
            
        ratio_1 = (-quadratic_b - np.sqrt(discriminant)) / (2 * quadratic_a)
        ratio_2 = (-quadratic_b + np.sqrt(discriminant)) / (2 * quadratic_a)
        
        if (ratio_1 >= 0 and ratio_1 <= 1) or (ratio_2 >= 0 and ratio_2 <= 1) or (ratio_1 < 0 and ratio_2 > 1):
            entry_ratio = max(0.0, min(ratio_1, ratio_2))
            exit_ratio = min(1.0, max(ratio_1, ratio_2))
            return True, entry_ratio, exit_ratio
        
        return False, None, None

    def parse_simulation_data(self):
        """
        Parses the SLURM file to extract birth and death events for each quasiparticle.
        """
        print("Parsing SLURM file for birth and death events...")
        qp_data = {}
        
        with open(self.slurm_file, 'r') as file_handle:
            for line in file_handle:
                if line.startswith("G4WT") and "[QP STEP REGISTERED]" in line:
                    match = self.regex.search(line)
                    if match:
                        track_id = int(match.group(1))
                        time_ns = float(match.group(2))
                        x_mm = float(match.group(3))
                        y_mm = float(match.group(4))
                        
                        if track_id not in qp_data:
                            # First time seeing this ID: log as birth
                            qp_data[track_id] = {'birth': (time_ns, x_mm, y_mm)}
                        else:
                            # ID already exists: log as death
                            qp_data[track_id]['death'] = (time_ns, x_mm, y_mm)
                            
        return qp_data

    def calculate_zone_intervals(self, qp_data):
        """
        Calculates when each particle intersected with any JJ zone.
        """
        print("Calculating intersections with JJ Zones...")
        intervals = []

        for track_id, lifecycle in qp_data.items():
            # Sanity check: Ensure particle has both birth and death recorded
            if 'death' not in lifecycle:
                continue 
                
            time_birth, x_birth, y_birth = lifecycle['birth']
            time_death, x_death, y_death = lifecycle['death']
            
            pos_birth = (x_birth, y_birth)
            pos_death = (x_death, y_death)
            time_delta = time_death - time_birth
            
            for _, row in self.jj_coords.iterrows():
                jj_id = int(row['JJ_ID'])
                zone_center = (row['X_mm'], row['Y_mm'])
                
                intersected, entry_ratio, exit_ratio = self.check_segment_intersection(
                    pos_start=pos_birth, 
                    pos_end=pos_death, 
                    zone_center=zone_center
                )
                
                if intersected:
                    start_time = time_birth + (time_delta * entry_ratio)
                    stop_time = time_birth + (time_delta * exit_ratio)
                    
                    intervals.append({
                        "Start": start_time,
                        "Stop": stop_time,
                        "Zone #": jj_id,
                        "Particle ID #": track_id
                    })

        df_intervals = pd.DataFrame(intervals)
        df_intervals.to_csv(self.intervals_csv, index=False)
        print(f"Saved {len(intervals)} interval events to {self.intervals_csv}")
        return df_intervals

    def bin_time_intervals(self, df_intervals):
        """
        Bins the interval data into discrete time steps and counts unique particles.
        """
        print("Binning data by time...")
        if df_intervals.empty:
            return pd.DataFrame()

        max_time = df_intervals['Stop'].max()
        time_bins = np.arange(0, max_time + self.time_bin_size_ns, self.time_bin_size_ns)
        
        binned_data = []
        
        for i in range(len(time_bins) - 1):
            bin_start = time_bins[i]
            bin_end = time_bins[i+1]
            mid_time = (bin_start + bin_end) / 2
            
            overlapping_intervals = df_intervals[
                (df_intervals['Start'] < bin_end) & (df_intervals['Stop'] > bin_start)
            ]
            
            for jj_id in self.jj_coords['JJ_ID']:
                zone_intervals = overlapping_intervals[overlapping_intervals['Zone #'] == jj_id]
                unique_particles = zone_intervals['Particle ID #'].nunique()
                
                binned_data.append({
                    "Time": mid_time,
                    "Number of QP": unique_particles,
                    "Zone #": jj_id
                })

        df_binned = pd.DataFrame(binned_data)
        df_binned.to_csv(self.binned_csv, index=False)
        print(f"Saved binned data to {self.binned_csv}")
        return df_binned

    def plot_density(self, df_binned):
        """
        Generates a scatter plot of QP density over time per JJ zone.
        """
        print("Generating Scatter Plot...")
        if df_binned.empty:
            print("No data available to plot.")
            return

        plt.figure(figsize=(12, 6))
        color_map = plt.cm.get_cmap('tab10', len(self.jj_coords))
        
        for idx, jj_id in enumerate(self.jj_coords['JJ_ID']):
            zone_subset = df_binned[df_binned['Zone #'] == jj_id]
            active_points = zone_subset[zone_subset['Number of QP'] > 0]
            
            plt.scatter(
                active_points['Time'], 
                active_points['Number of QP'], 
                label=f'JJ {jj_id}', 
                color=color_map(idx),
                alpha=0.7,
                edgecolors='k'
            )

        plt.title('Quasiparticle Density in JJ Zones Over Time')
        plt.xlabel('Time (ns)')
        plt.ylabel('QP Count per Bin')
        plt.grid(True, linestyle='--', alpha=0.5)
        plt.legend(title="Transmon JJs")
        plt.tight_layout()
        plt.savefig('qp_density_plot.png', dpi=300)
        print("Plot saved as 'qp_density_plot.png'")

    def run(self):
        """
        Executes the full analysis pipeline.
        """
        qp_data = self.parse_simulation_data()
        df_intervals = self.calculate_zone_intervals(qp_data)
        df_binned = self.bin_time_intervals(df_intervals)
        self.plot_density(df_binned)


if __name__ == "__main__":
    # Define file names
    SLURM_OUTPUT = "simulation_output.txt"
    JJ_COORDINATES = "jj_coordinates.csv"
    
    # Initialize and run the analyzer
    analyzer = QuasiparticleAnalyzer(
        slurm_file=SLURM_OUTPUT, 
        jj_csv_file=JJ_COORDINATES,
        zone_radius_mm=0.1,      # 100 um
        time_bin_size_ns=100.0   # Adjust based on expected diffusion timescale
    )
    
    analyzer.run()