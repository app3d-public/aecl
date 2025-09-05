#include <acul/hash/hashset.hpp>
#include <aecl/scene/utils.hpp>
#include <mapbox/earcut.hpp>

#define is_nearly_zero(x) (fabs(x) < 1e-6f)

namespace aecl
{
    namespace utils
    {
        bool is_polygon_ccw(const acul::vector<Vertex2D> &polygon)
        {
            f64 sum = 0.0;
            for (size_t i = 0; i < polygon.size(); i++)
            {
                size_t j = (i + 1) % polygon.size();
                sum += (polygon[j][0] - polygon[i][0]) * (polygon[j][1] + polygon[i][1]);
            }
            return sum > 0;
        }

        using namespace umbf::mesh;

        acul::vector<Vertex2D> project_2d_polygon_to_vertex(const Face &face, const acul::vector<Vertex> &vertices,
                                                            acul::vector<u32> &indices)
        {
            amal::vec3 ref_point = vertices[face.vertices[0].vertex].pos;
            amal::vec3 x_axis, y_axis;
            amal::vec3 normal = average_vertex_normal(face, vertices);
            if (is_nearly_zero(amal::dot(normal, amal::vec3(0, 0, 1))))
            {
                x_axis = amal::cross(amal::vec3(1, 0, 0), normal);
                if (is_nearly_zero(amal::length(x_axis))) x_axis = amal::cross(amal::vec3(0, 1, 0), normal);
            }
            else
                x_axis = amal::cross(amal::vec3(0, 0, 1), normal);
            y_axis = amal::cross(normal, x_axis);
            x_axis = amal::normalize(x_axis);
            y_axis = amal::normalize(y_axis);

            acul::vector<Vertex2D> projected;
            indices.clear();
            acul::hashset<u32> local_indices;
            for (const auto &vref : face.vertices)
            {
                auto [it, inserted] = local_indices.insert(vref.vertex);
                if (inserted)
                {
                    const auto &vertex = vertices[vref.vertex];
                    amal::vec3 vec_to_vertex = vertex.pos - ref_point;
                    f32 x = amal::dot(vec_to_vertex, x_axis);
                    f32 y = amal::dot(vec_to_vertex, y_axis);

                    projected.push_back({x, y});
                    indices.push_back(vref.vertex);
                }
            }
            return projected;
        }

        acul::vector<u32> triangulate(const Face &face, const acul::vector<Vertex> &vertices)
        {
            acul::vector<u32> result_indices;
            acul::vector<u32> vertex_indices;

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
    } // namespace utils
} // namespace aecl