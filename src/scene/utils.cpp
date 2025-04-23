#include <acul/hash/hashset.hpp>
#include <ecl/scene/utils.hpp>
#include <mapbox/earcut.hpp>

#define isNearlyZero(x) (fabs(x) < 1e-6f)

namespace ecl
{
    namespace utils
    {
        bool isPolygonCCW(const acul::vector<Vertex2D> &polygon)
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

        acul::vector<Vertex2D> projectToVertex2D(const Face &face, const acul::vector<Vertex> &vertices,
                                                 acul::vector<u32> &vertexIndices)
        {
            glm::vec3 refPoint = vertices[face.vertices[0].vertex].pos;
            glm::vec3 xAxis, yAxis;
            glm::vec3 normal = averageVertexNormal(face, vertices);
            if (isNearlyZero(glm::dot(normal, glm::vec3(0, 0, 1))))
            {
                xAxis = glm::cross(glm::vec3(1, 0, 0), normal);
                if (isNearlyZero(glm::length(xAxis))) xAxis = glm::cross(glm::vec3(0, 1, 0), normal);
            }
            else
                xAxis = glm::cross(glm::vec3(0, 0, 1), normal);
            yAxis = glm::cross(normal, xAxis);
            xAxis = glm::normalize(xAxis);
            yAxis = glm::normalize(yAxis);

            acul::vector<Vertex2D> projectedPolygon;
            vertexIndices.clear();
            acul::hashset<u32> localIndices;
            for (const auto &vref : face.vertices)
            {
                auto [it, inserted] = localIndices.insert(vref.vertex);
                if (inserted)
                {
                    const auto &vertex = vertices[vref.vertex];
                    glm::vec3 vecToVertex = vertex.pos - refPoint;
                    f32 x = glm::dot(vecToVertex, xAxis);
                    f32 y = glm::dot(vecToVertex, yAxis);

                    projectedPolygon.push_back({x, y});
                    vertexIndices.push_back(vref.vertex);
                }
            }
            return projectedPolygon;
        }

        acul::vector<u32> triangulate(const Face &face, const acul::vector<Vertex> &vertices)
        {
            acul::vector<u32> resultIndices;
            acul::vector<u32> vertexIndices;

            if (face.vertices.size() == 3)
            {
                resultIndices.push_back(face.vertices[0].vertex);
                resultIndices.push_back(face.vertices[1].vertex);
                resultIndices.push_back(face.vertices[2].vertex);
                return resultIndices;
            }

            acul::vector<Vertex2D> projectedPolygon = projectToVertex2D(face, vertices, vertexIndices);
            if (projectedPolygon.empty()) return {};

            if (!isPolygonCCW(projectedPolygon))
            {
                std::reverse(projectedPolygon.begin(), projectedPolygon.end());
                std::reverse(vertexIndices.begin(), vertexIndices.end());
            }

            acul::vector<acul::vector<Vertex2D>> polygons = {projectedPolygon};
            for (auto index : mapbox::earcut<u32>(polygons)) resultIndices.push_back(vertexIndices[index]);
            return resultIndices;
        }
    } // namespace utils
} // namespace ecl