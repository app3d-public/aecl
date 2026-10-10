#include <acul/hash/hashset.hpp>
#include <aecl/scene/utils.hpp>
#include <amal/common.hpp>
#include <mapbox/earcut.hpp>

namespace aecl::utils
{
    bool is_polygon_ccw(const acul::vector<Vertex2D> &polygon)
    {
        if (polygon.empty()) return false;
        f64 sum = 0.0;
        const Vertex2D *previous = &polygon.back();
        for (const auto &current : polygon)
        {
            sum += (current[0] - (*previous)[0]) * (current[1] + (*previous)[1]);
            previous = &current;
        }
        return sum > 0;
    }

    using namespace umbf::mesh;

    static amal::vec3 polygon_normal(const Face &face, const acul::vector<Vertex> &vertices)
    {
        if (face.vertices.empty()) return {};
        amal::vec3 normal{0.0f};
        const amal::vec3 *previous = &vertices[face.vertices.back().vertex].pos;
        for (const auto &ref : face.vertices)
        {
            const auto &current = vertices[ref.vertex].pos;
            normal.x = amal::fma(previous->y - current.y, previous->z + current.z, normal.x);
            normal.y = amal::fma(previous->z - current.z, previous->x + current.x, normal.y);
            normal.z = amal::fma(previous->x - current.x, previous->y + current.y, normal.z);
            previous = &current;
        }
        return normal;
    }

    acul::vector<Vertex2D> project_2d_polygon_to_vertex(const Face &face, const acul::vector<Vertex> &vertices,
                                                        acul::vector<u32> &indices)
    {
        indices.clear();
        if (face.vertices.empty()) return {};

        const amal::vec3 magnitude = amal::abs(polygon_normal(face, vertices));
        u32 dominant_axis = 0;
        if (magnitude.y > magnitude[dominant_axis]) dominant_axis = 1;
        if (magnitude.z > magnitude[dominant_axis]) dominant_axis = 2;
        if (magnitude[dominant_axis] == 0.0f) return {};

        const u32 projection_x = dominant_axis == 0 ? 1 : 0;
        const u32 projection_y = dominant_axis == 2 ? 1 : 2;
        const amal::vec3 origin = vertices[face.vertices[0].vertex].pos;

        acul::vector<Vertex2D> projected;
        acul::hashset<u32> local_indices;
        for (const auto &vref : face.vertices)
        {
            auto [it, inserted] = local_indices.insert(vref.vertex);
            if (inserted)
            {
                const amal::vec3 point = vertices[vref.vertex].pos - origin;
                projected.push_back({point[projection_x], point[projection_y]});
                indices.push_back(vref.vertex);
            }
        }
        return projected;
    }

    acul::vector<u32> triangulate(const Face &face, const acul::vector<Vertex> &vertices)
    {
        acul::vector<u32> result_indices;
        acul::vector<u32> vertex_indices;

        if (face.vertices.size() < 3) return result_indices;
        if (face.vertices.size() == 3)
        {
            result_indices.push_back(face.vertices[0].vertex);
            result_indices.push_back(face.vertices[1].vertex);
            result_indices.push_back(face.vertices[2].vertex);
            return result_indices;
        }

        acul::vector<Vertex2D> projected = project_2d_polygon_to_vertex(face, vertices, vertex_indices);
        if (projected.empty()) return {};

        if (!is_polygon_ccw(projected))
        {
            std::reverse(projected.begin(), projected.end());
            std::reverse(vertex_indices.begin(), vertex_indices.end());
        }
        using Polygon = acul::vector<acul::vector<Vertex2D>>;
        auto mapped = mapbox::earcut<u32, Polygon>({projected});
        result_indices.resize(mapped.size());
        for (size_t i = 0; i < mapped.size(); i++) result_indices[i] = vertex_indices[mapped[i]];
        return result_indices;
    }
} // namespace aecl::utils