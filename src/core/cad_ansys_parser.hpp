#pragma once

#include <vector>
#include <string>
#include <array>
#include <cstdint>

namespace workbench::core {

/**
 * @brief 3D Bounding Box in Cartesian space (meters).
 */
struct BoundingBox3D {
    double min_x = 0.0, max_x = 0.0;
    double min_y = 0.0, max_y = 0.0;
    double min_z = 0.0, max_z = 0.0;

    double size_x() const { return max_x - min_x; }
    double size_y() const { return max_y - min_y; }
    double size_z() const { return max_z - min_z; }
    std::array<double, 3> center() const {
        return { 0.5 * (min_x + max_x), 0.5 * (min_y + max_y), 0.5 * (min_z + max_z) };
    }
};

/**
 * @brief Represents a single facet triangle from an ASCII STL.
 */
struct STLTriangle {
    std::array<double, 3> normal{0.0, 0.0, 0.0};
    std::array<double, 3> v1{0.0, 0.0, 0.0};
    std::array<double, 3> v2{0.0, 0.0, 0.0};
    std::array<double, 3> v3{0.0, 0.0, 0.0};
};

/**
 * @brief Parsed and validated ASCII STL Geometry data.
 */
struct STLGeometry {
    std::string solid_name;
    std::vector<STLTriangle> triangles;
    BoundingBox3D bbox;
    size_t triangle_count = 0;
    std::vector<float> flat_positions; ///< [x1,y1,z1, x2,y2,z2, ...] for WebGL
    std::vector<float> flat_normals;   ///< [nx,ny,nz, ...] per vertex for WebGL
};

/**
 * @brief Represents a single ANSYS temperature point node.
 */
struct ANSYSNode {
    int64_t node_id = 0;
    double x_m = 0.0;
    double y_m = 0.0;
    double z_m = 0.0;
    double temperature_C = 0.0;
};

/**
 * @brief Parsed and validated ANSYS nodal temperature point cloud.
 */
struct ANSYSPointCloud {
    std::vector<ANSYSNode> nodes;
    BoundingBox3D bbox;
    double min_temp_C = 0.0;
    double max_temp_C = 0.0;
    double mean_temp_C = 0.0;
    size_t node_count = 0;
    std::vector<float> flat_positions;    ///< [x, y, z] for WebGL
    std::vector<float> flat_temperatures; ///< [temp] for WebGL color mapping
};

/**
 * @brief Detailed validation result structure with line numbers and actionable guidance.
 */
struct ParseValidationResult {
    bool is_valid = false;
    std::string error_message;
    size_t line_number = 0;
    size_t column_number = 0;
};

/**
 * @brief Detects if a file or buffer is binary STL instead of ASCII.
 */
bool is_binary_stl(const std::string& content);

/**
 * @brief Parses and validates an ASCII STL file content.
 */
ParseValidationResult parse_ascii_stl_string(const std::string& content, STLGeometry& out_geometry);

/**
 * @brief Parses and validates an ASCII STL file from path.
 */
ParseValidationResult parse_ascii_stl_file(const std::string& filepath, STLGeometry& out_geometry);

/**
 * @brief Parses and validates an ANSYS nodal temperature CSV content.
 * Expected header: node_id,x_m,y_m,z_m,temperature_C
 */
ParseValidationResult parse_ansys_csv_string(const std::string& content, ANSYSPointCloud& out_cloud);

/**
 * @brief Parses and validates an ANSYS nodal temperature CSV from path.
 */
ParseValidationResult parse_ansys_csv_file(const std::string& filepath, ANSYSPointCloud& out_cloud);

} // namespace workbench::core
