#include "core/backprop.hpp"
#include "core/thermal_fea.hpp" // For save_string_to_file

#include <iostream>
#include <iomanip>
#include <filesystem>
#include <vector>

int main() {
    using namespace workbench::core;

    std::cout << "=========================================================\n";
    std::cout << "Task F: Handwritten Backpropagation Verification Tool\n";
    std::cout << "=========================================================\n";

    const double x = 1.0;
    const double y = 2.0;
    const double lr = 0.1;
    const NetworkParams p0{0.5, 0.0, 1.0, 0.0};
    const double eps = 1.0e-6;

    // Initial forward pass
    ForwardState s0 = forward_pass(x, y, p0);
    NetworkGradients g_analytic = compute_analytic_gradients(s0, p0);
    NetworkGradients g_numeric = compute_finite_diff_gradients(x, y, p0, eps);
    NetworkParams p1 = step_gradient_descent(p0, g_analytic, lr);
    ForwardState s1 = forward_pass(x, y, p1);

    std::cout << "\n1. INITIAL FORWARD PASS (Step 0):\n";
    std::cout << "   x            = " << s0.x << "\n";
    std::cout << "   Target y     = " << s0.y << "\n";
    std::cout << "   Initial w    = " << p0.w << "\n";
    std::cout << "   Initial b    = " << p0.b << "\n";
    std::cout << "   Initial v    = " << p0.v << "\n";
    std::cout << "   Initial c    = " << p0.c << "\n";
    std::cout << "   z = w*x + b  = " << std::fixed << std::setprecision(8) << s0.z << "\n";
    std::cout << "   h = tanh(z)  = " << s0.h << "\n";
    std::cout << "   y_hat=v*h+c  = " << s0.y_hat << "\n";
    std::cout << "   error=y_hat-y= " << s0.error << "\n";
    std::cout << "   Loss J=0.5*e^2 = " << s0.loss << "\n";

    std::cout << "\n2. GRADIENTS & FINITE-DIFFERENCE CHECK (eps = 1e-6):\n";
    std::cout << "------------------------------------------------------------------------------------\n";
    std::cout << std::setw(12) << "Parameter"
              << std::setw(20) << "Analytic dJ/dTheta"
              << std::setw(22) << "Numerical (Central)"
              << std::setw(18) << "Absolute Diff"
              << std::setw(12) << "Status\n";
    std::cout << "------------------------------------------------------------------------------------\n";

    auto print_grad = [](const char* name, double a, double n) {
        double diff = std::abs(a - n);
        std::cout << std::setw(12) << name
                  << std::setw(20) << std::fixed << std::setprecision(8) << a
                  << std::setw(22) << n
                  << std::setw(18) << std::scientific << std::setprecision(3) << diff
                  << std::setw(12) << (diff <= 1e-5 ? "PASS (<=1e-5)" : "FAIL")
                  << "\n";
    };

    print_grad("w", g_analytic.dw, g_numeric.dw);
    print_grad("b", g_analytic.db, g_numeric.db);
    print_grad("v", g_analytic.dv, g_numeric.dv);
    print_grad("c", g_analytic.dc, g_numeric.dc);
    std::cout << "------------------------------------------------------------------------------------\n";

    std::cout << "\n3. FIRST SIMULTANEOUS PARAMETER UPDATE (lr = 0.1):\n";
    std::cout << "   Formula: theta_new = theta_old - lr * (dJ/dtheta)\n";
    std::cout << "   w_1 = " << p0.w << " - 0.1 * (" << g_analytic.dw << ") = " << std::fixed << std::setprecision(8) << p1.w << "\n";
    std::cout << "   b_1 = " << p0.b << " - 0.1 * (" << g_analytic.db << ") = " << p1.b << "\n";
    std::cout << "   v_1 = " << p0.v << " - 0.1 * (" << g_analytic.dv << ") = " << p1.v << "\n";
    std::cout << "   c_1 = " << p0.c << " - 0.1 * (" << g_analytic.dc << ") = " << p1.c << "\n";
    std::cout << "   Updated Prediction y_hat_1 = " << s1.y_hat << " (closer to target 2.0)\n";
    std::cout << "   Updated Loss J_1           = " << s1.loss << " (decreased from " << s0.loss << ")\n";

    // Run 100 training steps
    std::cout << "\n4. TRAINING FOR 100 SUBSEQUENT STEPS (Total 101 steps):\n";
    auto history = train_network(x, y, p0, lr, 100);

    std::cout << "   Initial Loss (Step 0)   : " << history.front().loss << " (y_hat = " << history.front().y_hat << ")\n";
    std::cout << "   Midway Loss (Step 50)   : " << history[50].loss << " (y_hat = " << history[50].y_hat << ")\n";
    std::cout << "   Final Loss (Step 100)   : " << history.back().loss << " (y_hat = " << history.back().y_hat << ")\n";

    std::filesystem::create_directories("results");
    std::string csv_content = export_training_csv(history);
    if (save_string_to_file("results/backprop_loss.csv", csv_content)) {
        std::cout << "[SUCCESS] Exported training loss history to results/backprop_loss.csv\n";
    } else {
        std::cerr << "[ERROR] Failed to save results/backprop_loss.csv\n";
    }

    return 0;
}
