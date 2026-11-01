#include "doctest/doctest.h"
#include "core/parallel_batch.hpp"

#include <cmath>

using namespace workbench::core;

TEST_SUITE("Task D: Multicore / Multithreaded Execution") {

    TEST_CASE("Reproducible batch generator: fixed seed produces identical positive workloads") {
        BatchConfig cfg;
        cfg.seed = 12345;
        cfg.num_cases = 50;
        cfg.N_elements = 20;

        auto batch1 = generate_reproducible_batch(cfg);
        auto batch2 = generate_reproducible_batch(cfg);

        REQUIRE(batch1.size() == 50);
        REQUIRE(batch2.size() == 50);

        for (size_t i = 0; i < batch1.size(); ++i) {
            // Must have strictly positive k and q
            CHECK(batch1[i].k > 0.0);
            CHECK(batch1[i].q > 0.0);
            CHECK(batch1[i].k >= cfg.k_min);
            CHECK(batch1[i].k <= cfg.k_max);
            CHECK(batch1[i].q >= cfg.q_min);
            CHECK(batch1[i].q <= cfg.q_max);

            // Must match identically across runs with same seed
            CHECK(batch1[i].k == batch2[i].k);
            CHECK(batch1[i].q == batch2[i].q);
        }
    }

    TEST_CASE("Serial vs Parallel Agreement (diff <= 1e-8 deg C) and Deterministic Ordering") {
        BatchConfig cfg;
        cfg.seed = 42;
        cfg.num_cases = 100;
        cfg.N_elements = 25;

        auto cases = generate_reproducible_batch(cfg);
        auto serial_res = solve_batch_serial(cases);

        REQUIRE(serial_res.size() == 100);

        // Test with 2 and 4 workers
        for (int workers : {2, 4}) {
            auto parallel_res = solve_batch_parallel(cases, workers);
            REQUIRE(parallel_res.size() == 100);

            // Compute max difference across all nodes and all cases
            double max_diff = compute_max_agreement_diff(serial_res, parallel_res);

            // Must satisfy rubric requirement: max abs difference <= 1e-8 C
            CHECK(max_diff <= 1.0e-8);
            CHECK(max_diff >= 0.0);

            // Verify deterministic ordering: individual case endpoints must match
            for (size_t i = 0; i < cases.size(); ++i) {
                CHECK(parallel_res[i].T_nodes.front() == doctest::Approx(cases[i].T0));
                CHECK(parallel_res[i].T_nodes.back() == doctest::Approx(cases[i].TL));
                CHECK(parallel_res[i].params.k == cases[i].k);
                CHECK(parallel_res[i].params.q == cases[i].q);
            }
        }
    }

    TEST_CASE("Scaling Benchmark Harness runs with 1 warmup and 5 repetitions") {
        BatchConfig cfg;
        cfg.seed = 999;
        cfg.num_cases = 100;
        cfg.N_elements = 20;

        auto cases = generate_reproducible_batch(cfg);
        std::vector<int> workers = {1, 2};

        auto records = run_scaling_benchmark(cases, workers);
        REQUIRE(records.size() == 2);

        for (const auto& r : records) {
            CHECK(r.warmup_time_ms > 0.0);
            CHECK(r.repetition_times_ms.size() == 5);
            CHECK(r.median_time_ms > 0.0);
            CHECK(r.max_agreement_diff_C <= 1.0e-8);
        }

        // T1 speedup is 1.0, efficiency is 1.0
        CHECK(records[0].speedup == doctest::Approx(1.0));
        CHECK(records[0].efficiency == doctest::Approx(1.0));
    }
}
