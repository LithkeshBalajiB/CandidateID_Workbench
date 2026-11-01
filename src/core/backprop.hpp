#pragma once

#include <vector>
#include <string>

namespace workbench::core {

/**
 * @brief Network parameters for single-hidden-unit regression network:
 * h = tanh(w * x + b)
 * y_hat = v * h + c
 */
struct NetworkParams {
    double w = 0.5;
    double b = 0.0;
    double v = 1.0;
    double c = 0.0;
};

/**
 * @brief Analytical gradients of loss J = 0.5 * (y_hat - y)^2 w.r.t parameters.
 */
struct NetworkGradients {
    double dw = 0.0;
    double db = 0.0;
    double dv = 0.0;
    double dc = 0.0;
};

/**
 * @brief Forward pass intermediate activations and loss.
 */
struct ForwardState {
    double x = 1.0;
    double y = 2.0;
    double z = 0.0;       ///< Pre-activation: z = w * x + b
    double h = 0.0;       ///< Hidden unit activation: h = tanh(z)
    double y_hat = 0.0;   ///< Prediction: y_hat = v * h + c
    double error = 0.0;   ///< Residual: y_hat - y
    double loss = 0.0;    ///< Loss: J = 0.5 * (y_hat - y)^2
};

/**
 * @brief Recorded training trajectory at each step.
 */
struct StepRecord {
    int step = 0;
    double w = 0.0;
    double b = 0.0;
    double v = 0.0;
    double c = 0.0;
    double y_hat = 0.0;
    double loss = 0.0;
    NetworkGradients grads;
};

/**
 * @brief Performs forward pass computation.
 */
ForwardState forward_pass(double x, double y, const NetworkParams& p);

/**
 * @brief Evaluates loss directly given parameters.
 */
double compute_loss(double x, double y, const NetworkParams& p);

/**
 * @brief Computes exact analytical gradients via chain rule without autodiff.
 */
NetworkGradients compute_analytic_gradients(const ForwardState& state, const NetworkParams& p);

/**
 * @brief Computes numerical gradients using central finite differences:
 * dJ/dtheta ≈ [J(theta + eps) - J(theta - eps)] / (2 * eps)
 */
NetworkGradients compute_finite_diff_gradients(double x, double y, const NetworkParams& p, double epsilon = 1.0e-6);

/**
 * @brief Executes one simultaneous gradient descent update:
 * theta_new = theta_old - lr * dJ/dtheta
 */
NetworkParams step_gradient_descent(const NetworkParams& p, const NetworkGradients& g, double lr);

/**
 * @brief Executes training for N steps, returning the recorded optimization trajectory.
 */
std::vector<StepRecord> train_network(double x, double y, NetworkParams initial_params, double lr, int steps);

/**
 * @brief Exports step-by-step training trajectory to CSV format.
 */
std::string export_training_csv(const std::vector<StepRecord>& history);

} // namespace workbench::core
