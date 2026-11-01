#include "doctest/doctest.h"
#include "core/backprop.hpp"

#include <cmath>

using namespace workbench::core;

TEST_SUITE("Task F: Handwritten Backpropagation & Gradient Verification") {

    TEST_CASE("Forward pass at initial parameters w=0.5, b=0, v=1, c=0") {
        const double x = 1.0;
        const double y = 2.0;
        NetworkParams p{0.5, 0.0, 1.0, 0.0};

        ForwardState state = forward_pass(x, y, p);

        const double expected_z = 0.5;
        const double expected_h = std::tanh(0.5); // ~0.462117157
        const double expected_y_hat = expected_h;
        const double expected_error = expected_y_hat - 2.0; // ~ -1.53788284
        const double expected_loss = 0.5 * expected_error * expected_error; // ~1.18254182

        CHECK(state.z == doctest::Approx(expected_z).epsilon(1e-12));
        CHECK(state.h == doctest::Approx(expected_h).epsilon(1e-12));
        CHECK(state.y_hat == doctest::Approx(expected_y_hat).epsilon(1e-12));
        CHECK(state.error == doctest::Approx(expected_error).epsilon(1e-12));
        CHECK(state.loss == doctest::Approx(expected_loss).epsilon(1e-12));
    }

    TEST_CASE("Central finite-difference gradient check (epsilon=1e-6, error <= 1e-5)") {
        const double x = 1.0;
        const double y = 2.0;
        const NetworkParams p{0.5, 0.0, 1.0, 0.0};
        const double eps = 1.0e-6;

        ForwardState state = forward_pass(x, y, p);
        NetworkGradients analytic = compute_analytic_gradients(state, p);
        NetworkGradients numerical = compute_finite_diff_gradients(x, y, p, eps);

        // Required rubric assertion: absolute error <= 1e-5
        const double err_w = std::abs(analytic.dw - numerical.dw);
        const double err_b = std::abs(analytic.db - numerical.db);
        const double err_v = std::abs(analytic.dv - numerical.dv);
        const double err_c = std::abs(analytic.dc - numerical.dc);

        CHECK(err_w <= 1.0e-5);
        CHECK(err_b <= 1.0e-5);
        CHECK(err_v <= 1.0e-5);
        CHECK(err_c <= 1.0e-5);

        // Even tighter check: central diff accuracy O(eps^2) matches within 1e-9
        CHECK(err_w < 1.0e-9);
        CHECK(err_b < 1.0e-9);
        CHECK(err_v < 1.0e-9);
        CHECK(err_c < 1.0e-9);
    }

    TEST_CASE("First simultaneous gradient descent update") {
        const double x = 1.0;
        const double y = 2.0;
        const double lr = 0.1;
        const NetworkParams p0{0.5, 0.0, 1.0, 0.0};

        ForwardState state0 = forward_pass(x, y, p0);
        NetworkGradients g0 = compute_analytic_gradients(state0, p0);
        NetworkParams p1 = step_gradient_descent(p0, g0, lr);

        // Because error is negative (y_hat < y), all gradients are negative,
        // so all parameters must INCREASE to raise y_hat towards y.
        CHECK(g0.dw < 0.0);
        CHECK(g0.db < 0.0);
        CHECK(g0.dv < 0.0);
        CHECK(g0.dc < 0.0);

        CHECK(p1.w > p0.w);
        CHECK(p1.b > p0.b);
        CHECK(p1.v > p0.v);
        CHECK(p1.c > p0.c);

        // Specific numerical check of update values:
        // dw = -1.20946441, db = -1.20946441, dv = -0.71068188, dc = -1.53788284
        // w1 = 0.5 - 0.1 * (-1.20946441) = 0.62094644
        // b1 = 0.0 - 0.1 * (-1.20946441) = 0.12094644
        // v1 = 1.0 - 0.1 * (-0.71068188) = 1.07106819
        // c1 = 0.0 - 0.1 * (-1.53788284) = 0.15378828
        CHECK(p1.w == doctest::Approx(0.62094644).epsilon(1e-6));
        CHECK(p1.b == doctest::Approx(0.12094644).epsilon(1e-6));
        CHECK(p1.v == doctest::Approx(1.07106819).epsilon(1e-6));
        CHECK(p1.c == doctest::Approx(0.15378828).epsilon(1e-6));

        ForwardState state1 = forward_pass(x, y, p1);
        CHECK(state1.loss < state0.loss);
    }

    TEST_CASE("100 subsequent training steps: monotone decrease and convergence towards target y=2") {
        const double x = 1.0;
        const double y = 2.0;
        const double lr = 0.1;
        const NetworkParams p0{0.5, 0.0, 1.0, 0.0};
        const int steps = 100;

        auto history = train_network(x, y, p0, lr, steps);
        REQUIRE(history.size() == 101); // step 0 to step 100

        // Check strictly monotone decrease of loss at every step
        for (size_t t = 1; t < history.size(); ++t) {
            CHECK(history[t].loss < history[t - 1].loss);
        }

        // Check convergence towards target y = 2
        const double initial_loss = history.front().loss;
        const double final_loss = history.back().loss;
        const double final_y_hat = history.back().y_hat;

        CHECK(initial_loss == doctest::Approx(1.1825418).epsilon(1e-5));
        // Loss should reduce by over 99% after 100 steps
        CHECK(final_loss < 0.005);
        CHECK(final_y_hat > 1.90);
        CHECK(final_y_hat <= 2.01);
    }
}
