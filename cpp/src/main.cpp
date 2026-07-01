#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Triangulation_face_base_with_info_2.h>
#include <CGAL/Triangulation_vertex_base_with_info_2.h>
#include <CGAL/Triangulation_data_structure_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/linear_least_squares_fitting_3.h>
#include <CGAL/Plane_3.h>
#include <json.hpp>

#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <cmath>
#include <string>
#include <queue>

using json = nlohmann::json;
typedef CGAL::Exact_predicates_inexact_constructions_kernel K;

typedef K::Point_3 Point_3;
typedef K::Point_2 Point_2;
typedef K::Plane_3 Plane;
typedef K::Vector_3 Vector_3;

// Face structure to track the odd-even rule traversal
struct FaceInfo {
    bool visited = false;
    bool in_domain = false;
};

// Vertex Info to safely store original global indices without floating-point map bugs
typedef CGAL::Triangulation_vertex_base_with_info_2<int, K> Vb;
typedef CGAL::Constrained_triangulation_face_base_2<K> Constrained_Fb;
typedef CGAL::Triangulation_face_base_with_info_2<FaceInfo, K, Constrained_Fb> Fb;
typedef CGAL::Triangulation_data_structure_2<Vb, Fb> TDS;
typedef CGAL::Constrained_Delaunay_triangulation_2<K, TDS> CDT;
typedef CDT::Face_handle Face_handle;
typedef CDT::Vertex_handle Vertex_handle;

struct Projection {
    Plane plane;
    Point_2 to2D(const Point_3& p) const { return plane.to_2d(p); }
};

Projection fit_plane(const std::vector<Point_3>& pts) {
    Projection proj;
    if (pts.size() >= 3) {
        CGAL::linear_least_squares_fitting_3(
            pts.begin(), pts.end(), proj.plane, CGAL::Dimension_tag<0>()
        );
    }
    return proj;
}

// 1. CUSTOM VOLUME COMPUTATION
double custom_tetrahedron_volume(const Point_3& p1, const Point_3& p2, const Point_3& p3) {

    double determinant = p1.x() * (p2.y() * p3.z() - p2.z() * p3.y())
                       - p1.y() * (p2.x() * p3.z() - p2.z() * p3.x())
                       + p1.z() * (p2.x() * p3.y() - p2.y() * p3.x());

    return determinant / 6.0;
}

// 2. CUSTOM AREA COMPUTATION
double custom_triangle_area(const Point_3& p1, const Point_3& p2, const Point_3& p3) {
    double ux = p2.x() - p1.x();
    double uy = p2.y() - p1.y();
    double uz = p2.z() - p1.z();

    double vx = p3.x() - p1.x();
    double vy = p3.y() - p1.y();
    double vz = p3.z() - p1.z();

    double cross_x = uy * vz - uz * vy;
    double cross_y = uz * vx - ux * vz;
    double cross_z = ux * vy - uy * vx;

    return 0.5 * std::sqrt(cross_x * cross_x + cross_y * cross_y + cross_z * cross_z);
}

Point_3 get_vertex_coords(const json& vertices, const json& scale, const json& translate, int index) {
    return Point_3(
        vertices[index][0].get<int>() * scale[0].get<double>() + translate[0].get<double>(),
        vertices[index][1].get<int>() * scale[1].get<double>() + translate[1].get<double>(),
        vertices[index][2].get<int>() * scale[2].get<double>() + translate[2].get<double>()
    );
}
// ODD-EVEN TRIANGULATION WITH ROBUST NEWELL'S NORMAL ORIENTATION CORRECTION
std::vector<std::array<int, 3>> triangulate_surface(
    const std::vector<std::vector<int>>& rings,
    const json& vertices, const json& scale, const json& translate)
{
    std::vector<std::array<int, 3>> interior_triangles;
    std::vector<Point_3> pts3d;

    for (const auto& ring : rings) {
        for (int idx : ring) {
            pts3d.push_back(get_vertex_coords(vertices, scale, translate, idx));
        }
    }
    double nx = 0.0, ny = 0.0, nz = 0.0;
    const auto& outer_ring = rings[0];
    size_t n = outer_ring.size();
    for (size_t i = 0; i < n; ++i) {
        Point_3 cur = get_vertex_coords(vertices, scale, translate, outer_ring[i]);
        Point_3 next = get_vertex_coords(vertices, scale, translate, outer_ring[(i + 1) % n]);

        nx += (cur.y() - next.y()) * (cur.z() + next.z());
        ny += (cur.z() - next.z()) * (cur.x() + next.x());
        nz += (cur.x() - next.x()) * (cur.y() + next.y());
    }
    Vector_3 orig_normal(nx, ny, nz);
    if (rings.empty() || rings[0].size() < 3) return interior_triangles;

    //NEWELL'S METHOD FOR NORMAL EXTRACTION

    // cross-product normal calculation; if points are collinear the normal will be wrong, skewed or zero
    // newell loops through all vertices of the polygon and project it onto cartesian plane and compute
    // signed area of the 2D projections.
    // if 3 points collinear, we get near-o or wrong normal. if concave polygon -> normal flipped
    // in this case, non-planar and concave polygons work as it accumulates bet signed area across all edges, local anomalies cancelled out

    Projection proj = fit_plane(pts3d);
    CDT cdt;

    for (const auto& ring : rings) {
        if (ring.empty()) continue;
        std::vector<Vertex_handle> current_ring_handles;

        for (int idx : ring) {
            Point_3 p3 = get_vertex_coords(vertices, scale, translate, idx);
            Point_2 p2 = proj.to2D(p3);

            Vertex_handle vh = cdt.insert(p2);
            vh->info() = idx;
            current_ring_handles.push_back(vh);
        }

        for (size_t i = 0; i < current_ring_handles.size(); ++i) {
            cdt.insert_constraint(
                current_ring_handles[i],
                current_ring_handles[(i + 1) % current_ring_handles.size()]
            );
        }
    }

    // ODD-EVEM BREADTH FIRST FILL
    std::queue<Face_handle> queue;
    Face_handle infinite_face = cdt.infinite_face();
    infinite_face->info().visited = true;
    infinite_face->info().in_domain = false;
    queue.push(infinite_face);

    while (!queue.empty()) {
        Face_handle curr = queue.front();
        queue.pop();

        for (int i = 0; i < 3; ++i) {
            Face_handle neighbor = curr->neighbor(i);
            if (neighbor == nullptr || neighbor->info().visited) continue; // remove is_infinite check

            bool is_constraint = cdt.is_constrained(CDT::Edge(curr, i));
            neighbor->info().in_domain = is_constraint
                ? !curr->info().in_domain
                : curr->info().in_domain;
            neighbor->info().visited = true;
            queue.push(neighbor);
        }
    }

    // Collect internal triangles and accurately track normal orientation flips
    for (auto fit = cdt.finite_faces_begin(); fit != cdt.finite_faces_end(); ++fit) {
        if (fit->info().in_domain) {
            int idx0 = fit->vertex(0)->info();
            int idx1 = fit->vertex(1)->info();
            int idx2 = fit->vertex(2)->info();

            Point_3 tri_p0 = get_vertex_coords(vertices, scale, translate, idx0);
            Point_3 tri_p1 = get_vertex_coords(vertices, scale, translate, idx1);
            Point_3 tri_p2 = get_vertex_coords(vertices, scale, translate, idx2);
            Vector_3 tri_normal = CGAL::cross_product(tri_p1 - tri_p0, tri_p2 - tri_p0);

            // Using standard dot product orientation matching
            if (CGAL::angle(orig_normal, tri_normal) == CGAL::OBTUSE) {
                interior_triangles.push_back({idx0, idx2, idx1});
            } else {
                interior_triangles.push_back({idx0, idx1, idx2});
            }
        }
    }

    return interior_triangles;
}


int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_city_json>" << std::endl;
        return 1;
    }

    std::string input_path = argv[1];
    std::ifstream f(input_path);
    if (!f.is_open()) {
        std::cerr << "Failed to open input CityJSON file: " << input_path << std::endl;
        return 1;
    }

    json data = json::parse(f);
    f.close();

    auto& cityObjects = data["CityObjects"];
    auto& vertices = data["vertices"];
    auto& scales = data["transform"]["scale"];
    auto& translate = data["transform"]["translate"];

    std::vector<std::string> keys_to_delete;

    // Phase 1: Filter LoD 2.2 and merge to Parent structures
    for (auto& [id, obj] : cityObjects.items()) {
        if (obj.contains("parents")) {
            json lod22_geometry = json::array();
            for (auto& geom : obj["geometry"]) {
                if (geom["lod"] == "2.2") {
                    lod22_geometry.push_back(geom);
                    break;
                }
            }

            if (!lod22_geometry.empty()) {
                std::string parentId = obj["parents"][0];
                if (cityObjects.contains(parentId)) {
                    cityObjects[parentId]["geometry"] = lod22_geometry;
                    if (cityObjects[parentId].contains("children")) {
                        cityObjects[parentId].erase("children");
                    }
                }
            }
            keys_to_delete.push_back(id);
        }
    }

    for (const std::string& key : keys_to_delete) {
        cityObjects.erase(key);
    }

    // Phase 2: Compute Metrics & Build Output Triangulated Geometry
    for (auto& [id, obj] : cityObjects.items()) {
        if (obj["type"] != "Building" || !obj.contains("geometry") || obj["geometry"].empty()) continue;

        auto& geom = obj["geometry"][0];
        if (!geom.contains("boundaries") || geom["type"] != "Solid") continue;

        double total_building_volume = 0.0;
        double total_roof_area = 0.0;

        json working_boundaries = json::array();
        json working_semantic_values = json::array();
        bool has_semantics = geom.contains("semantics") && geom["semantics"].contains("values");

        for (size_t shell_idx = 0; shell_idx < geom["boundaries"].size(); ++shell_idx) {
            json new_shell = json::array();
            json new_shell_values = json::array();

            for (size_t surface_idx = 0; surface_idx < geom["boundaries"][shell_idx].size(); ++surface_idx) {
                const auto& surface = geom["boundaries"][shell_idx][surface_idx];

                std::vector<std::vector<int>> rings;
                for (const auto& ring : surface) {
                    std::vector<int> r;
                    for (int idx : ring) r.push_back(idx);
                    rings.push_back(r);
                }

                std::vector<std::array<int, 3>> triangles = triangulate_surface(rings, vertices, scales, translate);

                int semantic_idx = -1;
                std::string semantic_type = "";
                if (has_semantics && shell_idx < geom["semantics"]["values"].size() &&
                    surface_idx < geom["semantics"]["values"][shell_idx].size()) {
                    semantic_idx = geom["semantics"]["values"][shell_idx][surface_idx].get<int>();
                    if (semantic_idx >= 0) {
                        semantic_type = geom["semantics"]["surfaces"][semantic_idx]["type"].get<std::string>();
                    }
                }

                // Append newly generated triangles back into structural boundaries array
                for (const auto& tri : triangles) {
                    new_shell.push_back(json::array({json::array({tri[0], tri[1], tri[2]})}));
                    if (has_semantics) {
                        new_shell_values.push_back(semantic_idx);
                    }

                    Point_3 p1 = get_vertex_coords(vertices, scales, translate, tri[0]);
                    Point_3 p2 = get_vertex_coords(vertices, scales, translate, tri[1]);
                    Point_3 p3 = get_vertex_coords(vertices, scales, translate, tri[2]);

                    total_building_volume += custom_tetrahedron_volume(p1, p2, p3);

                    if (semantic_type == "RoofSurface") {
                        total_roof_area += custom_triangle_area(p1, p2, p3);
                    }
                }
            }
            working_boundaries.push_back(new_shell);
            if (has_semantics) {
                working_semantic_values.push_back(new_shell_values);
            }
        }

        geom["boundaries"] = working_boundaries;
        if (has_semantics) {
            geom["semantics"]["values"] = working_semantic_values;
        }

        if (!obj.contains("attributes")) {
            obj["attributes"] = json::object();
        }
        obj["attributes"]["geo1004_volume"] = std::abs(total_building_volume);
        obj["attributes"]["geo1004_total_roof_area"] = total_roof_area;
    }

    // Phase 3: Write Output File
    size_t ext_pos = input_path.find(".city.json");
    std::string output_path = (ext_pos != std::string::npos) ?
                              input_path.substr(0, ext_pos) + "_out.city.json" :
                              input_path + "_out.city.json";

    std::ofstream o(output_path);
    if (o.is_open()) {
        o << data.dump(2);
        std::cout << "Processed tile successfully saved to: " << output_path << std::endl;
    } else {
        std::cerr << "Error writing to destination output file." << std::endl;
    }
    o.close();

    return 0;
}
