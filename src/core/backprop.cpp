#include "core/backprop.hpp"

#include <cmath>
#include <sstream>
#include <iomanip>

namespace workbench::core {

ForwardState forward_pass(double x, double y, const NetworkParams& p) {
    ForwardState s;
    s.x = x;
    s.y = y;
    s.z = p.w * x + p.b;
    s.h = std::tanh(s.z);
    s.y_hat = p.v * s.h + p.c;
    s.error = s.y_hat - y;
    s.loss = 0.5 * s.error * s.error;
    return s;
}

double compute_loss(double x, double y, const NetworkParams& p) {
    const double z = p.w * x + p.b;
    const double h = std::tanh(z);
    const double y_hat = p.v * h + p.c;
    const double error = y_hat - y;
    return 0.5 * error * error;
}

NetworkGradients compute_analytic_gradients(const ForwardState& s, const NetworkParams& p) {
    NetworkGradients g;
    // dJ/d(y_hat) = y_hat - y = error
    const double dJ_dyhat = s.error;

    // y_hat = v * h + c
    // dJ/dc = dJ/dyhat * dyhat/dc = error * 1
    g.dc = dJ_dyhat;

    // dJ/dv = dJ/dyhat * dyhat/dv = error * h
    g.dv = dJ_dyhat * s.h;

    // dJ/dh = dJ/dyhat * dyhat/dh = error * v
    const double dJ_dh = dJ_dyhat * p.v;

    // h = tanh(z) => dh/dz = 1 - tanh^2(z) = 1 - h^2
    const double dh_dz = 1.0 - s.h * s.h;

    // dJ/dz = dJ/dh * dh/dz = (error * v) * (1 - h^2)
    const double delta_z = dJ_dh * dh_dz;

    // z = w * x + b
    // dJ/dw = delta_z * dz/dw = delta_z * x
    g.dw = delta_z * s.x;

    // dJ/db = delta_z * dz/db = delta_z * 1
    g.db = delta_z;

    return g;
}

NetworkGradients compute_finite_diff_gradients(double x, double y, const NetworkParams& p, double epsilon) {
    NetworkGradients g;

    // Numerical gradient w.r.t w
    {
        NetworkParams p_plus = p;
        NetworkParams p_minus = p;
        p_plus.w += epsilon;
        p_minus.w -= epsilon;
        g.dw = (compute_loss(x, y, p_plus) - compute_loss(x, y, p_minus)) / (2.0 * epsilon);
    }

    // Numerical gradient w.r.t b
    {
        NetworkParams p_plus = p;
        NetworkParams p_minus = p;
        p_plus.b += epsilon;
        p_minus.b -= epsilon;
        g.db = (compute_loss(x, y, p_plus) - compute_loss(x, y, p_minus)) / (2.0 * epsilon);
    }

    // Numerical gradient w.r.t v
    {
        NetworkParams p_plus = p;
        NetworkParams p_minus = p;
        p_plus.v += epsilon;
        p_minus.v -= epsilon;
        g.dv = (compute_loss(x, y, p_plus) - compute_loss(x, y, p_minus)) / (2.0 * epsilon);
    }

    // Numerical gradient w.r.t c
    {
        NetworkParams p_plus = p;
        NetworkParams p_minus = p;
        p_plus.c += epsilon;
        p_minus.c -= epsilon;
        g.dc = (compute_loss(x, y, p_plus) - compute_loss(x, y, p_minus)) / (2.0 * epsilon);
    }

    return g;
}

NetworkParams step_gradient_descent(const NetworkParams& p, const NetworkGradients& g, double lr) {
    NetworkParams updated;
    updated.w = p.w - lr * g.dw;
    updated.b = p.b - lr * g.db;
    updated.v = p.v - lr * g.dv;
    updated.c = p.c - lr * g.dc;
    return updated;
}

std::vector<StepRecord> train_network(double x, double y, NetworkParams current_params, double lr, int steps) {
    std::vector<StepRecord> history;
    history.reserve(steps + 1);

    for (int step = 0; step <= steps; ++step) {
        ForwardState state = forward_pass(x, y, current_params);
        NetworkGradients grads = compute_analytic_gradients(state, current_params);

        StepRecord rec;
        rec.step = step;
        rec.w = current_params.w;
        rec.b = current_params.b;
        rec.v = current_params.v;
        rec.c = current_params.c;
        rec.y_hat = state.y_hat;
        rec.loss = state.loss;
        rec.grads = grads;
        history.push_back(rec);

        if (step < steps) {
            current_params = step_gradient_descent(current_params, grads, lr);
        }
    }

    return history;
}

std::string export_training_csv(const std::vector<StepRecord>& history) {
    std::ostringstream oss;
    oss << "step,w,b,v,c,y_hat,loss,dw,db,dv,dc\n";
    oss << std::setprecision(10) << std::fixed;

    for (const auto& r : history) {
        oss << r.step << ","
            << r.w << ","
            << r.b << ","
            << r.v << ","
            << r.c << ","
            << r.y_hat << ","
            << r.loss << ","
            << r.grads.dw << ","
            << r.grads.db << ","
            << r.grads.dv << ","
            << r.grads.dc << "\n";
    }

    return oss.str();
}

} // namespace workbench::core
