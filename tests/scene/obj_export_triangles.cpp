#include <ecl/scene/obj/export.hpp>

using namespace ecl;
using namespace scene;

void createCubeVerticles(DArray<assets::mesh::Vertex> &vertices)
{
    vertices.resize(24);
    vertices[0] = {{100, -100, 100}, {0, 0}, {0, 0, 1}};
    vertices[1] = {{100, 100, 100}, {1, 1}, {0, 0, 1}};
    vertices[2] = {{-100, 100, 100}, {0, 1}, {0, 0, 1}};
    vertices[3] = {{-100, -100, 100}, {0, 0}, {0, 0, 1}};
    vertices[4] = {{100, -100, -100}, {0, 0}, {1, 0, 0}};
    vertices[5] = {{100, 100, -100}, {1, 1}, {1, 0, 0}};
    vertices[6] = {{100, 100, 100}, {0, 1}, {1, 0, 0}};
    vertices[7] = {{100, -100, 100}, {0, 0}, {1, 0, 0}};
    vertices[8] = {{-100, -100, -100}, {0, 0}, {0, 0, -1}};
    vertices[9] = {{-100, 100, -100}, {1, 1}, {0, 0, -1}};
    vertices[10] = {{100, 100, -100}, {0, 1}, {0, 0, -1}};
    vertices[11] = {{100, -100, -100}, {0, 0}, {0, 0, -1}};
    vertices[12] = {{-100, -100, 100}, {0, 0}, {-1, 0, 0}};
    vertices[13] = {{-100, 100, 100}, {1, 1}, {-1, 0, 0}};
    vertices[14] = {{-100, 100, -100}, {0, 1}, {-1, 0, 0}};
    vertices[15] = {{-100, -100, -100}, {0, 0}, {-1, 0, 0}};
    vertices[16] = {{100, 100, 100}, {0, 0}, {0, 1, 0}};
    vertices[17] = {{100, 100, -100}, {1, 1}, {0, 1, 0}};
    vertices[18] = {{-100, 100, -100}, {0, 1}, {0, 1, 0}};
    vertices[19] = {{-100, 100, 100}, {0, 0}, {0, 1, 0}};
    vertices[20] = {{100, -100, -100}, {0, 0}, {0, -1, 0}};
    vertices[21] = {{100, -100, 100}, {1, 1}, {0, -1, 0}};
    vertices[22] = {{-100, -100, 100}, {0, 1}, {0, -1, 0}};
    vertices[23] = {{-100, -100, -100}, {0, 0}, {0, -1, 0}};
}

void createCubeVGroups(DArray<assets::mesh::VertexGroup> &vertexGroups)
{
    vertexGroups.resize(8);
    vertexGroups[0] = {{3, 12, 22}, {0, 3, 5}};
    vertexGroups[1] = {{2, 13, 19}, {0, 3, 4}};
    vertexGroups[2] = {{0, 7, 21}, {0, 1, 5}};
    vertexGroups[3] = {{1, 6, 16}, {0, 1, 4}};
    vertexGroups[4] = {{4, 11, 20}, {1, 2, 5}};
    vertexGroups[5] = {{5, 10, 17}, {1, 2, 4}};
    vertexGroups[6] = {{8, 15, 23}, {2, 3, 5}};
    vertexGroups[7] = {{9, 14, 18}, {2, 3, 4}};
}

void createCubeFaces(DArray<assets::mesh::Face> &faces)
{
    faces.resize(6);
    faces[0] = {{
                    {2, 0},
                    {3, 1},
                    {1, 2},
                    {0, 3},
                },
                {0, 0, 1},
                0,
                6};
    faces[1] = {{
                    {4, 4},
                    {5, 5},
                    {3, 6},
                    {2, 7},
                },
                {1, 0, 0},
                6,
                6};
    faces[2] = {{
                    {6, 8},
                    {7, 9},
                    {5, 10},
                    {4, 11},
                },
                {0, 0, -1},
                12,
                6};
    faces[3] = {{
                    {0, 12},
                    {1, 13},
                    {7, 14},
                    {6, 15},
                },
                {-1, 0, 0},
                18,
                6};
    faces[4] = {{
                    {3, 16},
                    {5, 17},
                    {7, 18},
                    {1, 19},
                },
                {0, 1, 0},
                24,
                6};
    faces[5] = {{
                    {4, 20},
                    {2, 21},
                    {0, 22},
                    {6, 23},
                },
                {0, -1, 0},
                30,
                6};
}

void createMeshes(DArray<MeshNode> &meshes)
{
    MeshNode node;
    node.meta = new assets::mesh::MeshBlock();
    node.name = "cube";
    node.matID = -1;
    auto &model = node.meta->model;
    createCubeVerticles(model.vertices);
    model.indices = {2,  3,  0,  0,  1,  2,  6,  7,  4,  4,  5,  6,  10, 11, 8,  8,  9,  10,
                     14, 15, 12, 12, 13, 14, 18, 19, 16, 16, 17, 18, 22, 23, 20, 20, 21, 22};
    createCubeVGroups(model.vertexGroups);
    createCubeFaces(model.faces);
    model.aabb = {{-100, -100, -100}, {100, 100, 100}};
    meshes.push_back(node);
}

bool runTest(const std::filesystem::path &dataDir, const std::filesystem::path &outputDir)
{
    MeshExportFlags meshFlags = MeshExportFlagBits::export_normals | MeshExportFlagBits::export_triangulated;
    MaterialExportFlags materialFlags = MaterialExportFlagBits::texture_none;
    obj::ObjExportFlags objFlags = obj::ObjExportFlagBits::mgp_groups | obj::ObjExportFlagBits::mat_PBR;
    obj::Exporter exporter(outputDir / "export_origin.obj", meshFlags, materialFlags, objFlags, outputDir / "tex");
    DArray<MeshNode> meshes;
    createMeshes(meshes);
    exporter.meshes(meshes);
    auto state = exporter.save();
    exporter.clear();
    return state;
}