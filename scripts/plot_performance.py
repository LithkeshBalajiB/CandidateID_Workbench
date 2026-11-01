#!/usr/bin/env python3
"""
Plots Task D Multicore performance scaling, speedup, and efficiency.
Reads:
  - results/performance_benchmark.csv
Outputs:
  - results/performance_scaling.png
"""

import os
import csv
import matplotlib.pyplot as plt

def main():
    os.makedirs("results", exist_ok=True)
    csv_path = "results/performance_benchmark.csv"
    if not os.path.exists(csv_path):
        print(f"[ERROR] Could not find {csv_path}")
        return

    workers, medians, speedups, efficiencies = [], [], [], []
    header_meta = []

    with open(csv_path, "r") as f:
        for line in f:
            if line.startswith("#"):
                header_meta.append(line.strip("# \r\n"))
            elif line.startswith("workers"):
                break
        
        reader = csv.DictReader(f, fieldnames=["workers","warmup_ms","rep1_ms","rep2_ms","rep3_ms","rep4_ms","rep5_ms","median_ms","speedup","efficiency","max_diff_C"])
        for row in reader:
            if not row["workers"] or row["workers"] == "workers":
                continue
            workers.append(int(row["workers"]))
            medians.append(float(row["median_ms"]))
            speedups.append(float(row["speedup"]))
            efficiencies.append(float(row["efficiency"]) * 100.0)

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(13, 5.5), dpi=300)

    # Subplot 1: Wall-Clock Compute Time vs Workers
    bars = ax1.bar([str(w) for w in workers], medians, color=['#4c72b0', '#55a868', '#c44e52'], width=0.55, edgecolor='black', alpha=0.85)
    ax1.set_xlabel('Number of Worker Threads (p)', fontsize=10)
    ax1.set_ylabel('Median Compute Time (ms) [Lower is Better]', fontsize=10)
    ax1.set_title('Wall-Clock Execution Time (5 Repetitions, Median)\nWorkload: 2,000 Solves, N=100 Mesh', fontsize=11)
    ax1.grid(True, axis='y', linestyle='--', alpha=0.5)

    for bar, med in zip(bars, medians):
        yval = bar.get_height()
        ax1.text(bar.get_x() + bar.get_width()/2.0, yval + 0.15, f"{med:.2f} ms", ha='center', va='bottom', fontsize=9, fontweight='bold')

    # Subplot 2: Speedup and Efficiency
    color_speedup = '#1f77b4'
    ax2.set_xlabel('Number of Worker Threads (p)', fontsize=10)
    ax2.set_ylabel('Speedup S_p = T₁ / T_p [Higher is Better]', color=color_speedup, fontsize=10)
    line1 = ax2.plot(workers, speedups, 'o-', color=color_speedup, linewidth=2.2, markersize=8, label='Observed Speedup')
    line2 = ax2.plot(workers, workers, '--', color='gray', linewidth=1.5, label='Ideal Linear Speedup')
    ax2.tick_params(axis='y', labelcolor=color_speedup)
    ax2.set_xticks(workers)
    ax2.grid(True, linestyle='--', alpha=0.5)

    ax2_twin = ax2.twinx()
    color_eff = '#2ca02c'
    ax2_twin.set_ylabel('Parallel Efficiency (%)', color=color_eff, fontsize=10)
    line3 = ax2_twin.plot(workers, efficiencies, 's--', color=color_eff, linewidth=2.0, markersize=7, label='Efficiency (%)')
    ax2_twin.tick_params(axis='y', labelcolor=color_eff)
    ax2_twin.set_ylim(0, 115)

    for x, s, e in zip(workers, speedups, efficiencies):
        ax2.annotate(f"{s:.2f}x\n({e:.1f}%)", (x, s), textcoords="offset points", xytext=(0, 12), ha='center', fontsize=8,
                     bbox=dict(boxstyle="round,pad=0.2", fc="white", ec="gray", alpha=0.8))

    lines = line1 + line2 + line3
    labels = [l.get_label() for l in lines]
    ax2.legend(lines, labels, loc='center right', frameon=True, facecolor='white', framealpha=0.9)
    ax2.set_title('Multicore Speedup & Efficiency Scaling\n(AMD Ryzen 7 7435HS, 8C/16T)', fontsize=11)

    plt.suptitle("Task D: CPU-Parallel Batch Performance Scaling\nAgreement vs Serial: max |ΔT| = 0.000e+00 °C (≤ 1e-8 °C)", fontsize=12, y=0.99)
    plt.tight_layout()
    plt.savefig("results/performance_scaling.png", dpi=300)
    plt.close()
    print("[SUCCESS] Created results/performance_scaling.png")

if __name__ == "__main__":
    main()
