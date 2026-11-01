#include "core/cad_ansys_parser.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <unordered_map>
#include <limits>

namespace workbench::core {

namespace {

// Trim leading and trailing whitespace
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

// Case-insensitive compare
bool iequals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != 
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

// Split string by delimiter
std::vector<std::string> split_csv_line(const std::string& line) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(line);
    while (std::getline(tokenStream, token, ',')) {
        tokens.push_back(trim(token));
    }
    // If the line ends with a comma, add an empty token
    if (!line.empty() && line.back() == ',') {
        tokens.push_back("");
    }
    return tokens;
}

} // anonymous namespace

bool is_binary_stl(const std::string& content) {
    if (content.size() < 84) {
        return false;
    }

    // Binary STL has 80 byte header followed by 4 byte unsigned int triangle count
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(content.data());
    uint32_t expected_triangles = bytes[80] | (bytes[81] << 8) | (bytes[82] << 16) | (bytes[83] << 24);
    
    // Each triangle in binary STL is 50 bytes (12 floats = 48 bytes + 2 bytes attribute byte count)
    size_t expected_size = 84 + static_cast<size_t>(expected_triangles) * 50;
    if (content.size() == expected_size && expected_triangles > 0) {
        return true;
    }

    // Check for null bytes in the first 256 bytes
    size_t check_len = std::min(content.size(), static_cast<size_t>(256));
    for (size_t i = 0; i < check_len; ++i) {
        if (content[i] == '\0') {
            return true;
        }
    }

    // Check if it doesn't start with "solid"
    std::string first_word;
    std::istringstream iss(content);
    iss >> first_word;
    if (!iequals(first_word, "solid")) {
        return true;
    }

    return false;
}

ParseValidationResult parse_ascii_stl_string(const std::string& content, STLGeometry& out_geometry) {
    ParseValidationResult res;
    out_geometry = STLGeometry();

    if (trim(content).empty()) {
        res.is_valid = false;
        res.line_number = 1;
        res.error_message = "Error at line 1: File is empty. Actionable fix: Provide a valid ASCII STL file starting with 'solid <name>'.";
        return res;
    }

    if (is_binary_stl(content)) {
        res.is_valid = false;
        res.line_number = 1;
        res.error_message = "Error at line 1: Binary STL file detected. The workbench requires ASCII STL format. Actionable fix: Re-export the CAD file as ASCII STL or convert via MeshLab/Blender.";
        return res;
    }

    std::istringstream stream(content);
    std::string line;
    size_t line_num = 0;
    bool in_solid = false;
    bool in_facet = false;
    bool in_loop = false;
    std::vector<std::array<double, 3>> current_vertices;
    std::array<double, 3> current_normal{0.0, 0.0, 0.0};

    double min_x = std::numeric_limits<double>::infinity();
    double min_y = std::numeric_limits<double>::infinity();
    double min_z = std::numeric_limits<double>::infinity();
    double max_x = -std::numeric_limits<double>::infinity();
    double max_y = -std::numeric_limits<double>::infinity();
    double max_z = -std::numeric_limits<double>::infinity();

    while (std::getline(stream, line)) {
        ++line_num;
        std::string trimmed = trim(line);
        if (trimmed.empty()) {
            continue; // Skip blank lines
        }

        std::istringstream line_iss(trimmed);
        std::string token;
        line_iss >> token;

        if (iequals(token, "solid")) {
            if (in_solid) {
                res.is_valid = false;
                res.line_number = line_num;
                res.error_message = "Error at line " + std::to_string(line_num) + ": Nested 'solid' declaration found. Actionable fix: File must define exactly one solid.";
                return res;
            }
            in_solid = true;
            std::string name;
            std::getline(line_iss, name);
            out_geometry.solid_name = trim(name);
        } else if (iequals(token, "facet")) {
            if (!in_solid) {
                res.is_valid = false;
                res.line_number = line_num;
                res.error_message = "Error at line " + std::to_string(line_num) + ": 'facet' defined outside 'solid' block. Actionable fix: File must begin with 'solid <name>'.";
                return res;
            }
            std::string normal_keyword;
            line_iss >> normal_keyword;
            if (!iequals(normal_keyword, "normal")) {
                res.is_valid = false;
                res.line_number = line_num;
                res.error_message = "Error at line " + std::to_string(line_num) + ": Expected 'facet normal nx ny nz', found 'facet " + normal_keyword + "'.";
                return res;
            }
            double nx, ny, nz;
            if (!(line_iss >> nx >> ny >> nz)) {
                res.is_valid = false;
                res.line_number = line_num;
                res.error_message = "Error at line " + std::to_string(line_num) + ": Malformed facet normal components. Actionable fix: Provide three real numeric normal values.";
                return res;
            }
            if (!std::isfinite(nx) || !std::isfinite(ny) || !std::isfinite(nz)) {
                res.is_valid = false;
                res.line_number = line_num;
                res.error_message = "Error at line " + std::to_string(line_num) + ": Non-finite (NaN/Inf) facet normal detected. Actionable fix: Replace non-finite coordinates.";
                return res;
            }
            current_normal = {nx, ny, nz};
            in_facet = true;
            current_vertices.clear();
        } else if (iequals(token, "outer")) {
            std::string loop_kw;
            line_iss >> loop_kw;
            if (!iequals(loop_kw, "loop") || !in_facet) {
                res.is_valid = false;
                res.line_number = line_num;
                res.error_message = "Error at line " + std::to_string(line_num) + ": Expected 'outer loop' inside facet block.";
                return res;
            }
            in_loop = true;
        } else if (iequals(token, "vertex")) {
            if (!in_loop) {
                res.is_valid = false;
                res.line_number = line_num;
                res.error_message = "Error at line " + std::to_string(line_num) + ": 'vertex' found outside 'outer loop'.";
                return res;
            }
            double vx, vy, vz;
            if (!(line_iss >> vx >> vy >> vz) || !std::isfinite(vx) || !std::isfinite(vy) || !std::isfinite(vz)) {
                res.is_valid = false;
                res.line_number = line_num;
                res.error_message = "Error at line " + std::to_string(line_num) + ": Malformed or non-finite (NaN/Inf) vertex coordinate detected. Actionable fix: Provide three valid finite real numbers: 'vertex x y z'.";
                return res;
            }
            current_vertices.push_back({vx, vy, vz});

            min_x = std::min(min_x, vx);
            min_y = std::min(min_y, vy);
            min_z = std::min(min_z, vz);
            max_x = std::max(max_x, vx);
            max_y = std::max(max_y, vy);
            max_z = std::max(max_z, vz);
        } else if (iequals(token, "endloop")) {
            if (!in_loop) {
                res.is_valid = false;
                res.line_number = line_num;
                res.error_message = "Error at line " + std::to_string(line_num) + ": Unexpected 'endloop' without matching 'outer loop'.";
                return res;
            }
            in_loop = false;
        } else if (iequals(token, "endfacet")) {
            if (!in_facet || in_loop) {
                res.is_valid = false;
                res.line_number = line_num;
                res.error_message = "Error at line " + std::to_string(line_num) + ": Unexpected 'endfacet'. Check loop closure.";
                return res;
            }
            if (current_vertices.size() != 3) {
                res.is_valid = false;
                res.line_number = line_num;
                res.error_message = "Error at line " + std::to_string(line_num) + ": Facet has " + std::to_string(current_vertices.size()) + " vertices. Standard STL requires exactly 3 vertices per facet.";
                return res;
            }

            STLTriangle tri;
            tri.normal = current_normal;
            tri.v1 = current_vertices[0];
            tri.v2 = current_vertices[1];
            tri.v3 = current_vertices[2];
            out_geometry.triangles.push_back(tri);

            // Populate flat arrays for WebGL
            for (int v = 0; v < 3; ++v) {
                out_geometry.flat_positions.push_back(static_cast<float>(current_vertices[v][0]));
                out_geometry.flat_positions.push_back(static_cast<float>(current_vertices[v][1]));
                out_geometry.flat_positions.push_back(static_cast<float>(current_vertices[v][2]));

                out_geometry.flat_normals.push_back(static_cast<float>(current_normal[0]));
                out_geometry.flat_normals.push_back(static_cast<float>(current_normal[1]));
                out_geometry.flat_normals.push_back(static_cast<float>(current_normal[2]));
            }

            in_facet = false;
        } else if (iequals(token, "endsolid")) {
            in_solid = false;
        }
    }

    if (in_solid || in_facet || in_loop) {
        res.is_valid = false;
        res.line_number = line_num;
        res.error_message = "Error: Incomplete ASCII STL file. File ended without proper 'endsolid' closure.";
        return res;
    }

    if (out_geometry.triangles.empty()) {
        res.is_valid = false;
        res.line_number = line_num;
        res.error_message = "Error: STL file contains zero valid facets. Actionable fix: Provide an ASCII STL with geometric facets.";
        return res;
    }

    out_geometry.triangle_count = out_geometry.triangles.size();
    out_geometry.bbox.min_x = min_x;
    out_geometry.bbox.min_y = min_y;
    out_geometry.bbox.min_z = min_z;
    out_geometry.bbox.max_x = max_x;
    out_geometry.bbox.max_y = max_y;
    out_geometry.bbox.max_z = max_z;

    res.is_valid = true;
    return res;
}

ParseValidationResult parse_ascii_stl_file(const std::string& filepath, STLGeometry& out_geometry) {
    std::ifstream ifs(filepath, std::ios::binary);
    if (!ifs.is_open()) {
        ParseValidationResult res;
        res.is_valid = false;
        res.error_message = "Error: Could not open file '" + filepath + "'. Check file existence and read permissions.";
        return res;
    }
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    return parse_ascii_stl_string(content, out_geometry);
}

ParseValidationResult parse_ansys_csv_string(const std::string& content, ANSYSPointCloud& out_cloud) {
    ParseValidationResult res;
    out_cloud = ANSYSPointCloud();

    if (trim(content).empty()) {
        res.is_valid = false;
        res.line_number = 1;
        res.error_message = "Error at line 1: CSV file is empty. Actionable fix: File must begin with header 'node_id,x_m,y_m,z_m,temperature_C'.";
        return res;
    }

    std::istringstream stream(content);
    std::string line;
    size_t line_num = 0;

    // Line 1: Header verification
    if (!std::getline(stream, line)) {
        res.is_valid = false;
        res.line_number = 1;
        res.error_message = "Error at line 1: Empty file. Expected header 'node_id,x_m,y_m,z_m,temperature_C'.";
        return res;
    }
    ++line_num;

    std::string header = trim(line);
    const std::string expected_header = "node_id,x_m,y_m,z_m,temperature_C";
    if (header != expected_header) {
        res.is_valid = false;
        res.line_number = 1;
        res.error_message = "Error at line 1: Invalid CSV header. Expected exactly '" + expected_header + "', but received '" + header + "'. Actionable fix: Rename CSV columns to match expected schema.";
        return res;
    }

    std::unordered_map<int64_t, size_t> seen_node_ids;

    double min_x = std::numeric_limits<double>::infinity();
    double min_y = std::numeric_limits<double>::infinity();
    double min_z = std::numeric_limits<double>::infinity();
    double max_x = -std::numeric_limits<double>::infinity();
    double max_y = -std::numeric_limits<double>::infinity();
    double max_z = -std::numeric_limits<double>::infinity();

    double min_t = std::numeric_limits<double>::infinity();
    double max_t = -std::numeric_limits<double>::infinity();
    double sum_t = 0.0;

    while (std::getline(stream, line)) {
        ++line_num;
        std::string trimmed = trim(line);
        if (trimmed.empty()) {
            continue; // Skip trailing blank lines
        }

        std::vector<std::string> tokens = split_csv_line(trimmed);
        if (tokens.size() != 5) {
            res.is_valid = false;
            res.line_number = line_num;
            res.error_message = "Error at line " + std::to_string(line_num) + ": Expected 5 columns, found " + std::to_string(tokens.size()) + ". Actionable fix: Ensure each line has format 'node_id,x_m,y_m,z_m,temperature_C'.";
            return res;
        }

        // 1. Parse node_id
        int64_t node_id = 0;
        try {
            size_t idx = 0;
            node_id = std::stoll(tokens[0], &idx);
            if (idx != tokens[0].size()) throw std::invalid_argument("trailing chars");
        } catch (...) {
            res.is_valid = false;
            res.line_number = line_num;
            res.column_number = 1;
            res.error_message = "Error at line " + std::to_string(line_num) + ", column 1: Invalid node_id '" + tokens[0] + "'. Actionable fix: node_id must be an integer.";
            return res;
        }

        // Duplicate node_id check
        auto it = seen_node_ids.find(node_id);
        if (it != seen_node_ids.end()) {
            res.is_valid = false;
            res.line_number = line_num;
            res.column_number = 1;
            res.error_message = "Error at line " + std::to_string(line_num) + ": Duplicate node_id " + std::to_string(node_id) + " detected (previously defined at line " + std::to_string(it->second) + "). Actionable fix: All node IDs must be strictly unique.";
            return res;
        }
        seen_node_ids[node_id] = line_num;

        // 2. Parse x_m, y_m, z_m, temperature_C
        double coords[4];
        for (int c = 0; c < 4; ++c) {
            try {
                size_t idx = 0;
                coords[c] = std::stod(tokens[c + 1], &idx);
                if (idx != tokens[c + 1].size()) throw std::invalid_argument("trailing chars");
            } catch (...) {
                res.is_valid = false;
                res.line_number = line_num;
                res.column_number = c + 2;
                res.error_message = "Error at line " + std::to_string(line_num) + ", column " + std::to_string(c + 2) + ": Non-numeric or invalid value '" + tokens[c + 1] + "'. Actionable fix: Must be a valid floating-point number.";
                return res;
            }

            if (!std::isfinite(coords[c])) {
                res.is_valid = false;
                res.line_number = line_num;
                res.column_number = c + 2;
                res.error_message = "Error at line " + std::to_string(line_num) + ", column " + std::to_string(c + 2) + ": Non-finite (NaN/Inf) value '" + tokens[c + 1] + "' detected. Actionable fix: Provide finite numeric data.";
                return res;
            }
        }

        ANSYSNode node;
        node.node_id = node_id;
        node.x_m = coords[0];
        node.y_m = coords[1];
        node.z_m = coords[2];
        node.temperature_C = coords[3];
        out_cloud.nodes.push_back(node);

        // Update bounds
        min_x = std::min(min_x, node.x_m);
        min_y = std::min(min_y, node.y_m);
        min_z = std::min(min_z, node.z_m);
        max_x = std::max(max_x, node.x_m);
        max_y = std::max(max_y, node.y_m);
        max_z = std::max(max_z, node.z_m);

        min_t = std::min(min_t, node.temperature_C);
        max_t = std::max(max_t, node.temperature_C);
        sum_t += node.temperature_C;

        // Flat buffers for WebGL rendering
        out_cloud.flat_positions.push_back(static_cast<float>(node.x_m));
        out_cloud.flat_positions.push_back(static_cast<float>(node.y_m));
        out_cloud.flat_positions.push_back(static_cast<float>(node.z_m));
        out_cloud.flat_temperatures.push_back(static_cast<float>(node.temperature_C));
    }

    if (out_cloud.nodes.empty()) {
        res.is_valid = false;
        res.line_number = line_num;
        res.error_message = "Error: CSV contains header but zero data rows. Actionable fix: Provide CSV with data records.";
        return res;
    }

    out_cloud.node_count = out_cloud.nodes.size();
    out_cloud.bbox.min_x = min_x;
    out_cloud.bbox.min_y = min_y;
    out_cloud.bbox.min_z = min_z;
    out_cloud.bbox.max_x = max_x;
    out_cloud.bbox.max_y = max_y;
    out_cloud.bbox.max_z = max_z;
    out_cloud.min_temp_C = min_t;
    out_cloud.max_temp_C = max_t;
    out_cloud.mean_temp_C = sum_t / static_cast<double>(out_cloud.node_count);

    res.is_valid = true;
    return res;
}

ParseValidationResult parse_ansys_csv_file(const std::string& filepath, ANSYSPointCloud& out_cloud) {
    std::ifstream ifs(filepath, std::ios::binary);
    if (!ifs.is_open()) {
        ParseValidationResult res;
        res.is_valid = false;
        res.error_message = "Error: Could not open file '" + filepath + "'. Check file existence and read permissions.";
        return res;
    }
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    return parse_ansys_csv_string(content, out_cloud);
}

} // namespace workbench::core
