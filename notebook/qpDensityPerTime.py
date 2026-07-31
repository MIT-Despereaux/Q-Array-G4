import pandas as pd
import numpy as np
import re
import matplotlib.pyplot as plt
import os
import argparse

class QuasiparticleAnalyzer:
    def __init__(self, slurm_file, jj_csv_file, zone_radius_mm=0.1, time_bin_size_ns=100.0):
        self.slurm_file = slurm_file
        self.jj_csv_file = jj_csv_file
        self.intervals_csv = "qp_intervals.csv"
        self.binned_csv = "qp_binned_counts.csv"
        
        self.zone_radius_mm = zone_radius_mm
        self.time_bin_size_ns = time_bin_size_ns
        
        # Physics Constants for Velocity Calculation
        self.m_eff_kg = 9.109e-31  # Electron effective mass (adjust if using specific band mass for Al)
        self.ev_to_joules = 1.602e-19
        
        # Unit Conversion Dictionaries
        self.len_to_mm = {'fm': 1e-12, 'nm': 1e-6, 'um': 1e-3, 'mm': 1.0, 'cm': 10.0, 'm': 1e3}
        self.len_to_m = {'fm': 1e-15, 'nm': 1e-9, 'um': 1e-6, 'mm': 1e-3, 'cm': 1e-2, 'm': 1.0}
        self.e_to_ev = {'ueV': 1e-6, 'meV': 1e-3, 'eV': 1.0, 'keV': 1e3, 'MeV': 1e6}
        
        # Regex to catch the start of a track and the registration time
        self.track_regex = re.compile(r"\*\s+G4Track Information:\s+Particle\s+=\s+BogoliubovQP,\s+Track ID\s+=\s+(\d+)")
        self.reg_regex = re.compile(r"\[QP STEP REGISTERED\]\s+TrackID:\s+(\d+)\s+\|\s+Time:\s+([\d\.]+)\s+ns")
        
        self.jj_coords = self._load_jj_coordinates()

    def _load_jj_coordinates(self):
        if os.path.exists(self.jj_csv_file):
            return pd.read_csv(self.jj_csv_file)
        else:
            print(f"Warning: {self.jj_csv_file} not found. Using mock data.")
            return pd.DataFrame({
                'JJ_ID':[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
                'X_mm': [-1.81656, -1.55744, -0.76656, -0.50744, 0.28244, 0.54156, 1.33244, 1.59156, -1.81656, -1.55744, -0.76656, -0.50744, 0.28244, 0.54156, 1.33244, 1.59156],
                'Y_mm': [1.15, 1.15, 1.15, 1.15, 1.15, 1.15, 1.15, 1.15, -1.121, -1.121, -1.121, -1.121, -1.121, -1.121, -1.121, -1.121]
            })

    def check_segment_intersection(self, pos_start, pos_end, zone_center):
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
        print("Parsing random walk tracks from SLURM file...")
        qp_data = {}
        current_track_id = None
        
        track_banner_regex = re.compile(r"\*\s+G4Track Information:\s+Particle\s+=\s+(\w+),\s+Track ID\s+=\s+(\d+)")
        registration_regex = re.compile(r"\[QP STEP REGISTERED\]\s+TrackID:\s+(\d+)\s+\|\s+Time:\s+([\d\.]+)\s+ns")
        
        with open(self.slurm_file, 'r') as file_handle:
            for line in file_handle:
                if "G4Track Information" in line:
                    match = track_banner_regex.search(line)
                    if match:
                        particle_type = match.group(1)
                        track_id = int(match.group(2))
                        
                        if particle_type == "BogoliubovQP":
                            current_track_id = track_id
                            if current_track_id not in qp_data:
                                qp_data[current_track_id] = {'start_time_nanoseconds': None, 'steps': []}
                        else:
                            current_track_id = None
                
                elif "[QP STEP REGISTERED]" in line:
                    match = registration_regex.search(line)
                    if match:
                        registered_track_id = int(match.group(1))
                        time_nanoseconds = float(match.group(2))
                        
                        if registered_track_id in qp_data and qp_data[registered_track_id]['start_time_nanoseconds'] is None:
                            qp_data[registered_track_id]['start_time_nanoseconds'] = time_nanoseconds
                            
                elif current_track_id is not None and "G4WT" in line:
                    line_parts = line.split(">")[-1].strip().split()
                    
                    if len(line_parts) >= 15 and line_parts[0].isdigit():
                        try:
                            position_x_mm = float(line_parts[1]) * self.len_to_mm[line_parts[2]]
                            position_y_mm = float(line_parts[3]) * self.len_to_mm[line_parts[4]]
                            kinetic_energy_ev = float(line_parts[7]) * self.e_to_ev[line_parts[8]]
                            step_length_meters = float(line_parts[11]) * self.len_to_m[line_parts[12]]
                            
                            qp_data[current_track_id]['steps'].append({
                                'x_mm': position_x_mm,
                                'y_mm': position_y_mm,
                                'ke_ev': kinetic_energy_ev,
                                'step_len_m': step_length_meters
                            })
                        except (ValueError, KeyError):
                            continue

        return qp_data

    def calculate_absolute_step_times(self, qp_data):
        print("Calculating absolute times for random walk nodes...")
        
        for track_id, data in qp_data.items():
            if data['start_time_nanoseconds'] is None or not data['steps']:
                continue
                
            current_time_ns = data['start_time_nanoseconds']
            
            for step in data['steps']:
                ke_joules = step['ke_ev'] * self.ev_to_joules
                step['abs_time_ns'] = current_time_ns
                
                if ke_joules > 0:
                    velocity_m_s = np.sqrt((2 * ke_joules) / self.m_eff_kg)
                    dt_seconds = step['step_len_m'] / velocity_m_s
                    dt_ns = dt_seconds * 1e9
                    current_time_ns += dt_ns

        return qp_data

    def calculate_zone_intervals(self, qp_data):
        print("Evaluating random walk segments against JJ Zones...")
        intervals = []

        for track_id, data in qp_data.items():
            steps = data['steps']
            if len(steps) < 2:
                continue
                
            for i in range(1, len(steps)):
                prev_step = steps[i-1]
                curr_step = steps[i]
                
                if 'abs_time_ns' not in prev_step or 'abs_time_ns' not in curr_step:
                    continue

                pos_prev = (prev_step['x_mm'], prev_step['y_mm'])
                pos_curr = (curr_step['x_mm'], curr_step['y_mm'])
                time_delta = curr_step['abs_time_ns'] - prev_step['abs_time_ns']
                
                for _, row in self.jj_coords.iterrows():
                    jj_id = int(row['JJ_ID'])
                    zone_center = (row['X_mm'], row['Y_mm'])
                    
                    intersected, entry_ratio, exit_ratio = self.check_segment_intersection(
                        pos_start=pos_prev, 
                        pos_end=pos_curr, 
                        zone_center=zone_center
                    )
                    
                    if intersected:
                        start_time = prev_step['abs_time_ns'] + (time_delta * entry_ratio)
                        stop_time = prev_step['abs_time_ns'] + (time_delta * exit_ratio)
                        
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
            
            overlapping = df_intervals[(df_intervals['Start'] < bin_end) & (df_intervals['Stop'] > bin_start)]
            
            for jj_id in self.jj_coords['JJ_ID']:
                zone_intervals = overlapping[overlapping['Zone #'] == jj_id]
                binned_data.append({
                    "Time": mid_time,
                    "Number of QP": zone_intervals['Particle ID #'].nunique(),
                    "Zone #": jj_id
                })

        df_binned = pd.DataFrame(binned_data)
        df_binned.to_csv(self.binned_csv, index=False)
        return df_binned

    def plot_density(self, df_binned):
        print("Generating Line Plot with Standard Deviation...")
        if df_binned.empty:
            print("No data available to plot.")
            return

        plt.figure(figsize=(14, 7))
        color_map = plt.get_cmap('tab20', max(20, len(self.jj_coords)))
        rolling_window_size = 10 
        
        for idx, jj_id in enumerate(self.jj_coords['JJ_ID']):
            zone_subset = df_binned[df_binned['Zone #'] == jj_id].sort_values(by='Time')
            times = zone_subset['Time']
            counts = zone_subset['Number of QP']
            
            rolling_mean = counts.rolling(window=rolling_window_size, min_periods=1).mean()
            rolling_std = counts.rolling(window=rolling_window_size, min_periods=1).std().fillna(0)
            
            plt.plot(
                times, counts, label=f'JJ {jj_id}', color=color_map(idx),
                marker='o', linestyle='-', markersize=3, linewidth=1, alpha=0.6
            )
            
            plt.fill_between(
                times, rolling_mean - rolling_std, rolling_mean + rolling_std, 
                color=color_map(idx), alpha=0.2, edgecolor='none'
            )

        plt.title('Quasiparticle Density in JJ Zones Over Time')
        plt.xlabel('Time (ns)')
        plt.ylabel('Unique QP Count per Bin')
        plt.grid(True, linestyle='--', alpha=0.5)
        plt.legend(title="Transmon JJs", bbox_to_anchor=(1.01, 1), loc='upper left')
        plt.tight_layout()
        plt.savefig('qp_density_line_plot.png', dpi=300)
        print("Plot saved as 'qp_density_line_plot.png'")

    def run(self, graph_only=False):
        """
        Executes the analysis pipeline. If graph_only is True, skips calculation 
        and directly plots from the existing binned CSV.
        """
        if graph_only:
            print(f"Graph-only mode activated. Loading data directly from '{self.binned_csv}'...")
            if os.path.exists(self.binned_csv):
                df_binned = pd.read_csv(self.binned_csv)
                self.plot_density(df_binned)
            else:
                print(f"Error: '{self.binned_csv}' not found. Please run the full pipeline first.")
        else:
            qp_data_raw = self.parse_simulation_data()
            qp_data_timed = self.calculate_absolute_step_times(qp_data_raw)
            df_intervals = self.calculate_zone_intervals(qp_data_timed)
            df_binned = self.bin_time_intervals(df_intervals)
            self.plot_density(df_binned)


if __name__ == "__main__":
    # Setup Argument Parser
    parser = argparse.ArgumentParser(description="Analyze Quasiparticle SLURM output.")
    parser.add_argument(
        '-g', '--graph-only', 
        action='store_true', 
        help="Skip log parsing/binning and only generate the graph from existing CSVs."
    )
    args = parser.parse_args()

    SLURM_OUTPUT = "simulation_output.txt"
    JJ_COORDINATES = ""
    
    analyzer = QuasiparticleAnalyzer(
        slurm_file=SLURM_OUTPUT, 
        jj_csv_file=JJ_COORDINATES,
        zone_radius_mm=0.1,      
        time_bin_size_ns=100.0   
    )
    
    # Pass the argument to the run method
    analyzer.run(graph_only=args.graph_only)