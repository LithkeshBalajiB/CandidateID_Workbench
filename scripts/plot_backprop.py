#!/usr/bin/env python3
"""
Plots Task F Backpropagation training trajectory and loss curve.
Reads:
  - results/backprop_loss.csv
Outputs:
  - results/backprop_loss.png
"""

import os
import csv
import matplotlib.pyplot as plt

def main():
    os.makedirs("results", exist_ok=True)
    csv_path = "results/backprop_loss.csv"
    if not os.path.exists(csv_path):
        print(f"[ERROR] Could not find {csv_path}")
        return

    steps, w_vals, b_vals, v_vals, c_vals, y_hats, losses = [], [], [], [], [], [], []
    with open(csv_path, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            steps.append(int(row["step"]))
            w_vals.append(float(row["w"]))
            b_vals.append(float(row["b"]))
            v_vals.append(float(row["v"]))
            c_vals.append(float(row["c"]))
            y_hats.append(float(row["y_hat"]))
            losses.append(float(row["loss"]))

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(13, 5.5), dpi=300)

    # Subplot 1: Loss & Prediction vs Step
    color_loss = '#d62728'
    ax1.set_xlabel('Training Step (Iteration)', fontsize=10)
    ax1.set_ylabel('Squared Error Loss, J = 0.5(y_hat - y)²', color=color_loss, fontsize=10)
    line1 = ax1.plot(steps, losses, color=color_loss, linewidth=2.2, label='Loss J')
    ax1.tick_params(axis='y', labelcolor=color_loss)
    ax1.grid(True, linestyle='--', alpha=0.5)

    ax1_twin = ax1.twinx()
    color_yhat = '#1f77b4'
    ax1_twin.set_ylabel('Model Prediction ŷ', color=color_yhat, fontsize=10)
    line2 = ax1_twin.plot(steps, y_hats, color=color_yhat, linewidth=2.0, linestyle='-', label='Prediction ŷ')
    line3 = ax1_twin.axhline(y=2.0, color='green', linestyle='--', linewidth=1.5, label='Target y = 2.0')
    ax1_twin.tick_params(axis='y', labelcolor=color_yhat)

    # Combine legends
    lines = line1 + line2 + [line3]
    labels = [l.get_label() for l in lines]
    ax1.legend(lines, labels, loc='center right', frameon=True, facecolor='white', framealpha=0.9)
    ax1.set_title('Loss Convergence & Output Prediction\n(Single sample x=1, target y=2, η=0.1)', fontsize=11)

    # Subplot 2: Parameter Trajectories
    ax2.plot(steps, w_vals, label='Weight w (initial 0.5)', linewidth=1.8)
    ax2.plot(steps, b_vals, label='Bias b (initial 0.0)', linewidth=1.8)
    ax2.plot(steps, v_vals, label='Weight v (initial 1.0)', linewidth=1.8)
    ax2.plot(steps, c_vals, label='Bias c (initial 0.0)', linewidth=1.8)
    ax2.set_xlabel('Training Step (Iteration)', fontsize=10)
    ax2.set_ylabel('Parameter Value', fontsize=10)
    ax2.set_title('Parameter Trajectories (Gradient Descent Updates)\nAll parameters increase because initial ŷ < y', fontsize=11)
    ax2.grid(True, linestyle='--', alpha=0.5)
    ax2.legend(frameon=True, facecolor='white', framealpha=0.9, loc='lower right')

    plt.tight_layout()
    plt.savefig("results/backprop_loss.png", dpi=300)
    plt.close()
    print("[SUCCESS] Created results/backprop_loss.png")

if __name__ == "__main__":
    main()
