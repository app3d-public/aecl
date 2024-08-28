#include <ecl/scene/utils.hpp>
#include <mapbox/earcut.hpp>

#define isNearlyZero(x) (fabs(x) < 1e-6f)

namespace ecl
{
    namespace utils
    {
        bool isPolygonCCW(const DArray<Vertex2D> &polygon)
        {
            f64 sum = 0.0;
            for (size_t i = 0; i < polygon.size(); i++)
            {
                size_t j = (i + 1) % polygon.size();
                sum += (polygon[j][0] - polygon[i][0]) * (polygon[j][1] + polygon[i][1]);
            }
            return sum > 0;
        }

        using namespace assets::mesh;

        DArray<Vertex2D> projectToVertex2D(const Face &face, const DArray<Vertex> &vertices, DArray<u32> &vertexIndices)
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

            DArray<Vertex2D> projectedPolygon;
            vertexIndices.clear();
            HashSet<u32> localIndices;
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

        DArray<u32> triangulate(const Face &face, const DArray<Vertex> &vertices)
        {
            DArray<u32> resultIndices;
            DArray<u32> vertexIndices;

            if (face.vertices.size() == 3)
            {
                resultIndices.push_back(face.vertices[0].vertex);
                resultIndices.push_back(face.vertices[1].vertex);
                resultIndices.push_back(face.vertices[2].vertex);
                return resultIndices;
            }

            DArray<Vertex2D> projectedPolygon = projectToVertex2D(face, vertices, vertexIndices);
            if (projectedPolygon.empty()) return {};

            if (!isPolygonCCW(projectedPolygon))
            {
                std::reverse(projectedPolygon.begin(), projectedPolygon.end());
                std::reverse(vertexIndices.begin(), vertexIndices.end());
            }

            DArray<DArray<Vertex2D>> polygons = {projectedPolygon};
            for (auto index : mapbox::earcut<u32>(polygons)) resultIndices.push_back(vertexIndices[index]);
            return resultIndices;
        }

        inline glm::vec3 getInnerEdge(size_t pos, f32 v1, f32 v2)
        {
            switch (pos)
            {
                case 0:
                    return glm::vec3(v1, v2, 1.0f);
                case 1:
                    return glm::vec3(v1, 1.0f, v2);
                default:
                    return glm::vec3(1.0f, v1, v2);
            }
        };

        void buildBarycentric(DArray<bary::Vertex> &barycentric, const Face &face, const DArray<Vertex> &vertices,
                              const DArray<u32> &indices)
        {
            static constexpr glm::vec3 barycentric_defaults[3] = {
                {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};

            size_t fv = 0;
            for (size_t i = 0; i < face.vertices.size(); i++)
                if (face.vertices[i].vertex == indices[0]) fv = i;

            for (size_t i = 0; i < indices.size(); i += 3)
            {
                u32 anchorPoint = 0;
                // Assume that the first edge is always the outer  edge,
                // as it is in clockwise-order polygons.
                if (indices[i] == indices[i + 2] || indices[i + 1] == indices[i + 2])
                    anchorPoint = indices[(i + 2) % 3];
                else
                    anchorPoint = indices[i + 2];

                for (size_t j = 0; j < 3; j++)
                {
                    const size_t co = i + j;
                    barycentric.emplace_back();
                    auto &bv = barycentric.back();
                    bv.pos = vertices[indices[co]].pos;
                    u32 start = indices[co];
                    u32 end = indices[(co + 1) % indices.size()];
                    u32 fvNext = fv == face.vertices.size() - 1 ? 0 : fv + 1;
                    if (start == face.vertices[fv].vertex && end == face.vertices[fvNext].vertex)
                    {
                        fv += fv == face.vertices.size() - 1 ? -fv : 1;
                        bv.barycentric = barycentric_defaults[j % 3];
                    }
                    else
                        bv.barycentric =
                            start == anchorPoint ? getInnerEdge(j, 1.0f, 0.0f) : getInnerEdge(j, 1.0f, 1.0f);
                }
            }
        }
    } // namespace utils
} // namespace ecl