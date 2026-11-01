#include "doctest/doctest.h"
#include "core/cad_ansys_parser.hpp"

using namespace workbench::core;

TEST_SUITE("Task B: CAD ASCII STL & ANSYS CSV Parsers") {

    TEST_CASE("Positive: Parse bundled sample_rod.stl") {
        STLGeometry geom;
        auto res = parse_ascii_stl_file("data/sample_rod.stl", geom);

        REQUIRE(res.is_valid);
        CHECK(res.error_message.empty());
        CHECK(geom.triangle_count == 12);
        CHECK(geom.triangles.size() == 12);
        CHECK(geom.flat_positions.size() == 12 * 3 * 3); // 12 triangles * 3 vertices * 3 coords
        CHECK(geom.flat_normals.size() == 12 * 3 * 3);

        // Check bounding box
        CHECK(geom.bbox.min_x == doctest::Approx(0.0));
        CHECK(geom.bbox.max_x == doctest::Approx(1.0));
        CHECK(geom.bbox.min_y == doctest::Approx(0.0));
        CHECK(geom.bbox.max_y == doctest::Approx(0.05));
        CHECK(geom.bbox.min_z == doctest::Approx(0.0));
        CHECK(geom.bbox.max_z == doctest::Approx(0.05));
    }

    TEST_CASE("Positive: Parse bundled ansys_thermal_synthetic.csv") {
        ANSYSPointCloud cloud;
        auto res = parse_ansys_csv_file("data/ansys_thermal_synthetic.csv", cloud);

        REQUIRE(res.is_valid);
        CHECK(res.error_message.empty());
        CHECK(cloud.node_count == 99);
        CHECK(cloud.nodes.size() == 99);
        CHECK(cloud.flat_positions.size() == 99 * 3);
        CHECK(cloud.flat_temperatures.size() == 99);

        // Check bounding box and temperature range
        CHECK(cloud.bbox.min_x == doctest::Approx(0.0));
        CHECK(cloud.bbox.max_x == doctest::Approx(1.0));
        CHECK(cloud.min_temp_C > 15.0);
        CHECK(cloud.max_temp_C <= 190.0);
        CHECK(cloud.mean_temp_C > 50.0);
    }

    TEST_CASE("Negative & Validation: ASCII STL Parser Error Handling") {
        STLGeometry geom;

        SUBCASE("Empty file rejected with line 1 diagnostic") {
            auto res = parse_ascii_stl_string("", geom);
            CHECK_FALSE(res.is_valid);
            CHECK(res.line_number == 1);
            CHECK(res.error_message.find("empty") != std::string::npos);
        }

        SUBCASE("Binary STL detected with actionable guidance") {
            // Construct pseudo binary STL with null byte header
            std::string binary_data(100, '\0');
            binary_data[0] = 'B'; binary_data[1] = 'I'; binary_data[2] = 'N';
            auto res = parse_ascii_stl_string(binary_data, geom);
            CHECK_FALSE(res.is_valid);
            CHECK(res.error_message.find("Binary STL") != std::string::npos);
        }

        SUBCASE("Non-finite NaN vertex rejected with line number") {
            std::string bad_stl = 
                "solid BadSolid\n"
                "  facet normal 0 0 1\n"
                "    outer loop\n"
                "      vertex 0.0 0.0 0.0\n"
                "      vertex NaN 1.0 0.0\n"
                "      vertex 0.0 1.0 0.0\n"
                "    endloop\n"
                "  endfacet\n"
                "endsolid BadSolid\n";
            auto res = parse_ascii_stl_string(bad_stl, geom);
            CHECK_FALSE(res.is_valid);
            CHECK(res.line_number == 5);
            CHECK((res.error_message.find("non-finite") != std::string::npos || 
                   res.error_message.find("Malformed") != std::string::npos));
        }

        SUBCASE("Incomplete facet without 3 vertices rejected") {
            std::string incomplete_stl = 
                "solid Incomplete\n"
                "  facet normal 0 0 1\n"
                "    outer loop\n"
                "      vertex 0.0 0.0 0.0\n"
                "      vertex 1.0 0.0 0.0\n"
                "    endloop\n"
                "  endfacet\n"
                "endsolid Incomplete\n";
            auto res = parse_ascii_stl_string(incomplete_stl, geom);
            CHECK_FALSE(res.is_valid);
            CHECK(res.error_message.find("3 vertices") != std::string::npos);
        }
    }

    TEST_CASE("Negative & Validation: ANSYS CSV Parser Error Handling") {
        ANSYSPointCloud cloud;

        SUBCASE("Empty CSV rejected with line 1 message") {
            auto res = parse_ansys_csv_string("", cloud);
            CHECK_FALSE(res.is_valid);
            CHECK(res.line_number == 1);
            CHECK(res.error_message.find("empty") != std::string::npos);
        }

        SUBCASE("Invalid CSV header rejected with schema guidance") {
            std::string bad_header = "id,x,y,z,temp\n1,0,0,0,25.0\n";
            auto res = parse_ansys_csv_string(bad_header, cloud);
            CHECK_FALSE(res.is_valid);
            CHECK(res.line_number == 1);
            CHECK(res.error_message.find("Invalid CSV header") != std::string::npos);
            CHECK(res.error_message.find("node_id,x_m,y_m,z_m,temperature_C") != std::string::npos);
        }

        SUBCASE("Wrong column count rejected with line number") {
            std::string wrong_cols = 
                "node_id,x_m,y_m,z_m,temperature_C\n"
                "1,0.0,0.0,0.0,25.0\n"
                "2,0.1,0.0,30.0\n"; // Missing z column
            auto res = parse_ansys_csv_string(wrong_cols, cloud);
            CHECK_FALSE(res.is_valid);
            CHECK(res.line_number == 3);
            CHECK(res.error_message.find("Expected 5 columns") != std::string::npos);
        }

        SUBCASE("Non-finite NaN temperature rejected with line and column number") {
            std::string nan_temp = 
                "node_id,x_m,y_m,z_m,temperature_C\n"
                "1,0.0,0.0,0.0,25.0\n"
                "2,0.1,0.0,0.0,NaN\n";
            auto res = parse_ansys_csv_string(nan_temp, cloud);
            CHECK_FALSE(res.is_valid);
            CHECK(res.line_number == 3);
            CHECK(res.column_number == 5);
            CHECK(res.error_message.find("Non-finite") != std::string::npos);
        }

        SUBCASE("Duplicate node_id rejected with line number of previous declaration") {
            std::string dup_nodes = 
                "node_id,x_m,y_m,z_m,temperature_C\n"
                "101,0.0,0.0,0.0,25.0\n"
                "102,0.1,0.0,0.0,30.0\n"
                "101,0.2,0.0,0.0,35.0\n"; // Duplicate 101 at line 4
            auto res = parse_ansys_csv_string(dup_nodes, cloud);
            CHECK_FALSE(res.is_valid);
            CHECK(res.line_number == 4);
            CHECK(res.error_message.find("Duplicate node_id 101") != std::string::npos);
            CHECK(res.error_message.find("line 2") != std::string::npos);
        }
    }
}
