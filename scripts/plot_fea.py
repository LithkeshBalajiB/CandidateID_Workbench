#!/usr/bin/env python3
"""
Generates publication-quality plots for Task C Thermal FEA verification.
Reads:
  - results/fea_profiles.csv
  - results/fea_convergence.csv
Outputs:
  - results/fea_profiles.png
  - results/fea_convergence.png
"""

import os
import csv
import matplotlib.pyplot as plt

def main():
    os.makedirs("results", exist_ok=True)

    # 1. Plot Temperature Profiles
    profiles_path = "results/fea_profiles.csv"
    if os.path.exists(profiles_path):
        data_by_N = {}
        with open(profiles_path, "r") as f:
            reader = csv.DictReader(f)
            for row in reader:
                N = int(row["N"])
                if N not in data_by_N:
                    data_by_N[N] = {"x": [], "T_fe": [], "T_exact": []}
                data_by_N[N]["x"].append(float(row["x_m"]))
                data_by_N[N]["T_fe"].append(float(row["T_fe_C"]))
                data_by_N[N]["T_exact"].append(float(row["T_exact_C"]))

        plt.figure(figsize=(9, 5.5), dpi=300)
        # Plot exact curve once from largest N
        largest_N = max(data_by_N.keys())
        plt.plot(data_by_N[largest_N]["x"], data_by_N[largest_N]["T_exact"], 
                 'k-', linewidth=2.5, label='Exact Analytical', zorder=1)

        markers = {10: 'o', 20: 's', 40: '^', 80: 'x'}
        styles = {10: '--', 20: '-.', 40: ':', 80: '-'}
        colors = {10: '#1f77b4', 20: '#ff7f0e', 40: '#2ca02c', 80: '#d62728'}

        for N in sorted(data_by_N.keys()):
            plt.plot(data_by_N[N]["x"], data_by_N[N]["T_fe"],
                     linestyle=styles.get(N, '-'),
                     marker=markers.get(N, '.'),
                     markersize=5 if N <= 20 else 3,
                     color=colors.get(N, None),
                     alpha=0.85,
                     label=f'FE Solution (N = {N})')

        plt.title('1D Steady-State Heat Conduction in Uniform Rod\nGoverning Eq: -d/dx(kA dT/dx) = qA, L=1.0 m, q=10 kW/m³, k=10 W/(m·K)', fontsize=11)
        plt.xlabel('Position along Rod, x (m)', fontsize=10)
        plt.ylabel('Temperature, T (°C)', fontsize=10)
        plt.grid(True, linestyle='--', alpha=0.6)
        plt.legend(frameon=True, facecolor='white', framealpha=0.9)
        plt.tight_layout()
        plt.savefig("results/fea_profiles.png", dpi=300)
        plt.close()
        print("[SUCCESS] Created results/fea_profiles.png")

    # 2. Plot Mesh Convergence (Midpoint Error vs h)
    conv_path = "results/fea_convergence.csv"
    if os.path.exists(conv_path):
        N_list, h_list, node_errs, mid_errs, orders = [], [], [], [], []
        with open(conv_path, "r") as f:
            reader = csv.DictReader(f)
            for row in reader:
                N_list.append(int(row["N"]))
                h_list.append(float(row["h_m"]))
                node_errs.append(float(row["max_node_error_C"]))
                mid_errs.append(float(row["max_midpoint_error_C"]))
                orders.append(float(row["observed_order"]))

        fig, ax = plt.subplots(figsize=(8.5, 5.5), dpi=300)
        ax.loglog(h_list, mid_errs, 'ro-', linewidth=2, markersize=8, label='FE Midpoint Error (Observed)')

        # Plot theoretical O(h^2) line: (q / 8k) * h^2 = 125 * h^2
        h_ref = [h_list[0], h_list[-1]]
        err_ref = [125.0 * (h**2) for h in h_ref]
        ax.loglog(h_ref, err_ref, 'k--', linewidth=1.5, label='Theoretical Midpoint Error: (q / 8k) · h² (Slope = 2.0)')

        # Add nodal error for contrast
        ax.loglog(h_list, [max(e, 1e-15) for e in node_errs], 'bs--', linewidth=1.5, markersize=6, 
                  label='FE Nodal Error (< 10⁻¹² °C, machine precision)')

        for i, N in enumerate(N_list):
            order_str = f"p = {orders[i]:.2f}" if orders[i] > 0 else "initial"
            ax.annotate(f"N={N}\n({mid_errs[i]:.4f} °C)\n{order_str}",
                        (h_list[i], mid_errs[i]),
                        textcoords="offset points",
                        xytext=(10, -5 if i%2==0 else 10),
                        ha='left', fontsize=8,
                        bbox=dict(boxstyle="round,pad=0.2", fc="yellow", alpha=0.3))

        ax.set_title('Mesh Convergence Analysis (Midpoint Error vs Element Size h)\nObserved Order p ≈ 2.00 (Second-Order Asymptotic Convergence)', fontsize=11)
        ax.set_xlabel('Element Size, h (m)', fontsize=10)
        ax.set_ylabel('Maximum Absolute Error (°C)', fontsize=10)
        ax.grid(True, which="both", linestyle='--', alpha=0.6)
        ax.legend(frameon=True, loc='upper left')
        plt.tight_layout()
        plt.savefig("results/fea_convergence.png", dpi=300)
        plt.close()
        print("[SUCCESS] Created results/fea_convergence.png")

if __name__ == "__main__":
    main()
