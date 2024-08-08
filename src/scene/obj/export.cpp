#include <algorithm>
#include <core/io/file.hpp>
#include <core/log.hpp>
#include <core/std/string.hpp>
#include <ecl/image/export.hpp>
#include <ecl/scene/obj/export.hpp>
#include <filesystem>
#include <oneapi/tbb/parallel_for.h>

namespace ecl
{
    namespace scene
    {
        namespace obj
        {
            void transformVertex(glm::vec3 &pos, MeshExportFlags flags)
            {
                if (flags & MeshExportFlagBits::transform_reverseX) pos.x = -pos.x;
                if (flags & MeshExportFlagBits::transform_reverseY) pos.y = -pos.y;
                if (flags & MeshExportFlagBits::transform_reverseZ) pos.z = -pos.z;
                if (flags & MeshExportFlagBits::transform_swapXY) std::swap(pos.x, pos.y);
                if (flags & MeshExportFlagBits::transform_swapXZ) std::swap(pos.x, pos.z);
                if (flags & MeshExportFlagBits::transform_swapYZ) std::swap(pos.y, pos.z);
            }

            void Exporter::writeVertices(assets::meta::mesh::Model &model, std::stringstream &ss)
            {
                // v
                for (auto &group : model.vertexGroups)
                {
                    auto &vID = group.vertices.front();
                    glm::vec3 &pos = model.vertices[vID].pos;
                    transformVertex(pos, _meshFlags);
                    ss << "v " << pos.x << " " << pos.y << " " << pos.z << "\n";
                }

                // vt and vn
                for (auto &vertex : model.vertices)
                {
                    if (_meshFlags & MeshExportFlagBits::export_uv)
                    {
                        auto [it, inserted] = _vtMap.emplace(vertex.uv, _vtMap.size());
                        if (inserted) ss << "vt " << vertex.uv.x << " " << vertex.uv.y << "\n";
                    }
                    if (_meshFlags & MeshExportFlagBits::export_normals)
                    {
                        auto &normal = vertex.normal;
                        transformVertex(normal, _meshFlags);
                        auto [it, inserted] = _vnMap.emplace(normal, _vnMap.size());
                        if (inserted) ss << "vn " << normal.x << " " << normal.y << " " << normal.z << "\n";
                    }
                }

                if ((_meshFlags & MeshExportFlagBits::transform_reverseX) ||
                    (_meshFlags & MeshExportFlagBits::transform_reverseY) ||
                    (_meshFlags & MeshExportFlagBits::transform_reverseZ))
                    for (auto &face : model.faces) std::reverse(face.vertices.begin(), face.vertices.end());
            }

            void Exporter::writeTriangles(assets::meta::mesh::MeshBlock *meta, std::ostream &os)
            {
                const auto &indices = meta->model.indices;
                const auto &vertices = meta->model.vertices;
                auto &groups = meta->model.vertexGroups;
                DArray<u32> positions(vertices.size());
                for (int g = 0; g < groups.size(); g++)
                    for (auto id : groups[g].vertices) positions[id] = g;

                size_t threadCount = oneapi::tbb::this_task_arena::max_concurrency();
                DArray<std::stringstream> blocks(threadCount);
                oneapi::tbb::parallel_for(
                    oneapi::tbb::blocked_range<size_t>(0, meta->model.faces.size()),
                    [&](const tbb::blocked_range<size_t> &range) {
                        size_t threadId = oneapi::tbb::this_task_arena::current_thread_index();
                        for (size_t r = range.begin(); r != range.end(); ++r)
                        {
                            auto &face = meta->model.faces[r];
                            for (u32 iter = 0, currentID = face.startID; iter < face.indexCount / 3; ++iter)
                            {
                                blocks[threadId] << "f ";
                                for (size_t vertexIndex = 0; vertexIndex < 3; ++vertexIndex)
                                {
                                    auto id = indices[currentID + vertexIndex];
                                    auto vID = positions[id];
                                    blocks[threadId] << vID + 1 << "/";
                                    if (_meshFlags & MeshExportFlagBits::export_uv)
                                        blocks[threadId] << _vtMap[vertices[id].uv] + 1;
                                    if (_meshFlags & MeshExportFlagBits::export_normals)
                                        blocks[threadId] << "/" << _vnMap[vertices[id].normal] + 1;
                                    blocks[threadId] << " ";
                                }
                                blocks[threadId] << "\n";
                                currentID += 3;
                            }
                        }
                    });
                for (const auto &block : blocks) os << block.str();
            }

            void Exporter::writeFaces(assets::meta::mesh::MeshBlock *meta, std::ostream &os)
            {
                size_t threadCount = oneapi::tbb::this_task_arena::max_concurrency();
                DArray<std::stringstream> blocks(threadCount);
                auto &faces = meta->model.faces;
                oneapi::tbb::parallel_for(oneapi::tbb::blocked_range<size_t>(0, faces.size()),
                                          [&](const tbb::blocked_range<size_t> &range) {
                                              size_t threadId = oneapi::tbb::this_task_arena::current_thread_index();
                                              for (size_t i = range.begin(); i != range.end(); ++i)
                                              {
                                                  blocks[threadId] << "f ";
                                                  for (auto &ref : faces[i].vertices)
                                                  {
                                                      blocks[threadId] << ref.group + 1 << "/";
                                                      auto &vertex = meta->model.vertices[ref.vertex];
                                                      if (_meshFlags & MeshExportFlagBits::export_uv)
                                                          blocks[threadId] << _vtMap[vertex.uv] + 1;
                                                      if (_meshFlags & MeshExportFlagBits::export_normals)
                                                          blocks[threadId] << "/" << _vnMap[vertex.normal] + 1;
                                                      blocks[threadId] << " ";
                                                  }
                                                  blocks[threadId] << "\n";
                                              }
                                          });

                for (const auto &block : blocks) os << block.str();
            }

            inline void writeVec3sRGB(std::ostream &os, const std::string &token, const glm::vec3 &vec)
            {
                os << token << " " << vec.x << " " << vec.y << " " << vec.z << "\n";
            }

            /**
             * Writes a number to the output stream with a given token.
             *
             * @param os The output stream to write to.
             * @param token The token to write before the number.
             * @param value The number to write.
             */
            template <typename T>
            inline void writeNumber(std::ostream &os, const std::string &token, const T &value)
            {
                os << token << " " << value << "\n";
            }

            void Exporter::writeTexture2D(std::ostream &os, const std::string &token, const TextureNode &tex)
            {
                if (tex.flags & assets::ImageTypeFlagBits::tGenerated)
                {
                    std::string filename = f("generated_%d.png", _genID++);
                    std::filesystem::path parent = _path.parent_path();
                    std::filesystem::path texFolder = parent / "tex";
                    std::filesystem::path path = texFolder / filename;
                    if (!std::filesystem::exists(path)) std::filesystem::create_directories(texFolder);
                    logInfo("Converting generated texture to image: %ls", path.c_str());
                    ecl::image::PNGExporter exporter(path, tex.image);
                    if (!exporter.compression(6).dpi(72.0f).save(tex.image.bytesPerChannel))
                        logWarn("Failed to save generated texture");
                    else
                        os << token << " ./tex/" << filename << "\n";
                }
                else if (_materialFlags & MaterialExportFlagBits::texture_origin)
                    os << token << " " << tex.path.string() << "\n";
                else if (_materialFlags & MaterialExportFlagBits::texture_copyToLocal)
                {
                    std::filesystem::path parent = _path.parent_path();
                    std::filesystem::path path = parent / "tex" / tex.path.filename();
                    if (io::file::copyFile(tex.path, path, std::filesystem::copy_options::overwrite_existing))
                        os << token << " ./tex/" << path.filename().string() << "\n";
                }
            }

            void writeDefaultMaterial(std::ostream &os, bool usePBR)
            {
                std::stringstream matBlock;
                matBlock << "newmtl default\n";
                writeVec3sRGB(matBlock, "Ka", {1, 1, 1});
                writeVec3sRGB(matBlock, "Kd", {1, 1, 1});
                writeVec3sRGB(matBlock, "Ks", {1, 1, 1});
                writeNumber(matBlock, "Ns", 80);
                if (usePBR)
                {
                    writeNumber(matBlock, "Pr", 0.33);
                    writeNumber(matBlock, "Pm", 1);
                }
                writeNumber(matBlock, "illum", 7);
                os << "\n" << matBlock.str();
            }

            void Exporter::writeMaterial(const MaterialNode &mat, std::ostream &os)
            {
                std::stringstream matBlock;
                matBlock << "newmtl " << mat.name << "\n";
                writeVec3sRGB(matBlock, "Ka", {1, 1, 1});
                writeVec3sRGB(matBlock, "Kd", mat.info.albedo.rgb);
                if (mat.info.albedo.textured)
                {
                    auto &tex = _textures[mat.info.albedo.textureID];
                    writeTexture2D(matBlock, "map_Kd", tex);
                }
                writeVec3sRGB(matBlock, "Ks", {1, 1, 1});
                writeNumber(matBlock, "Ns", 80);
                if (_objFlags & ObjExportFlagBits::mat_PBR)
                {
                    writeNumber(matBlock, "Pr", 0.33);
                    writeNumber(matBlock, "Pm", 1);
                }
                writeNumber(matBlock, "illum", 7);
                os << "\n" << matBlock.str();
            }

            bool Exporter::save()
            {
                logInfo("Exporting OBJ file: %ls", _path.c_str());
                try
                {
                    std::stringstream ss;
                    ss << "# App3D ECL OBJ Exporter\n";
                    std::ofstream mtllibFileStream;
                    if (_materialFlags != MaterialExportFlagBits::none)
                    {
                        std::filesystem::path mtllibPath = _path;
                        mtllibPath.replace_extension(".mtl");
                        mtllibFileStream.open(mtllibPath.string());
                        if (!mtllibFileStream.is_open())
                            logWarn("Failed to write MTL file: %ls", mtllibPath.c_str());
                        else
                        {
                            mtllibPath = mtllibPath.filename();
                            ss << "mtllib ./" << mtllibPath.string() << "\n";
                            mtllibFileStream << "# App3D ECL MTL Exporter\n";
                        }
                    }

                    bool allMaterialsExists = true;
                    for (const auto &mesh : _meshes)
                    {
                        if (_objFlags & ObjExportFlagBits::mgp_groups)
                            ss << "g " << mesh.name << "\n";
                        else if (_objFlags & ObjExportFlagBits::mgp_objects)
                            ss << "o " << mesh.name << "\n";
                        auto &model = mesh.meta->model;
                        writeVertices(model, ss);
                        std::string mtlName;
                        if (mesh.matID == -1)
                        {
                            mtlName = "default";
                            allMaterialsExists = false;
                        }
                        else
                            mtlName = _materials[mesh.matID].name;
                        ss << "usemtl " << mtlName << "\n";

                        if (_meshFlags & MeshExportFlagBits::export_triangulated)
                            writeTriangles(mesh.meta, ss);
                        else
                            writeFaces(mesh.meta, ss);
                    }
                    std::string error;
                    if (!io::file::writeFileByBlock(_path.string(), ss.str().c_str(), 1024 * 1024, error))
                    {
                        logError("OBJ export failed: %s", error.c_str());
                        return false;
                    }
                    if (mtllibFileStream)
                    {
                        std::filesystem::path texFolder = _path;
                        texFolder.replace_filename("tex");
                        if (!allMaterialsExists)
                            writeDefaultMaterial(mtllibFileStream, _objFlags & ObjExportFlagBits::mat_PBR);
                        for (const auto &mat : _materials) writeMaterial(mat, mtllibFileStream);
                        mtllibFileStream.close();
                    }
                    return true;
                }
                catch (const std::exception &e)
                {
                    logError("Failed to export file: %s", e.what());
                    return false;
                }
                return true;
            }
        } // namespace obj
    } // namespace scene
} // namespace ecl