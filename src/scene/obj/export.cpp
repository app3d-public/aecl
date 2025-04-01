#include <acul/io/file.hpp>
#include <acul/log.hpp>
#include <acul/string/string.hpp>
#include <algorithm>
#include <ecl/scene/obj/export.hpp>
#include <oneapi/tbb/parallel_for.h>
#include <umbf/utils.hpp>

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

            void Exporter::writeVertices(umbf::mesh::Model &model, const acul::vector<umbf::mesh::VertexGroup> &groups,
                                         acul::stringstream &ss)
            {
                // v
                for (auto &group : groups)
                {
                    auto &vID = group.vertices.front();
                    glm::vec3 &pos = model.vertices.at(vID).pos;
                    transformVertex(pos, meshFlags);
                    ss << "v " << pos.x << " " << pos.y << " " << pos.z << "\n";
                }

                // vt and vn
                for (auto &vertex : model.vertices)
                {
                    if (meshFlags & MeshExportFlagBits::export_uv)
                    {
                        auto [it, inserted] = _vtMap.emplace(vertex.uv, _vtMap.size());
                        if (inserted) ss << "vt " << vertex.uv.x << " " << vertex.uv.y << "\n";
                    }
                    if (meshFlags & MeshExportFlagBits::export_normals)
                    {
                        auto &normal = vertex.normal;
                        transformVertex(normal, meshFlags);
                        auto [it, inserted] = _vnMap.emplace(normal, _vnMap.size());
                        if (inserted) ss << "vn " << normal.x << " " << normal.y << " " << normal.z << "\n";
                    }
                }

                if ((meshFlags & MeshExportFlagBits::transform_reverseX) ||
                    (meshFlags & MeshExportFlagBits::transform_reverseY) ||
                    (meshFlags & MeshExportFlagBits::transform_reverseZ))
                    for (auto &face : model.faces) std::reverse(face.vertices.begin(), face.vertices.end());
            }

            void Exporter::writeTriangles(umbf::mesh::MeshBlock *meta, acul::stringstream &os,
                                          const acul::vector<u32> &faces,
                                          const acul::vector<umbf::mesh::VertexGroup> &groups)
            {
                const auto &m = meta->model;
                acul::vector<u32> positions(m.vertices.size());
                for (int g = 0; g < groups.size(); g++)
                    for (auto id : groups[g].vertices) positions[id] = g;

                size_t threadCount = oneapi::tbb::this_task_arena::max_concurrency();
                acul::vector<acul::stringstream> blocks(threadCount);
                oneapi::tbb::parallel_for(
                    oneapi::tbb::blocked_range<size_t>(0, faces.size()), [&](const tbb::blocked_range<size_t> &range) {
                        size_t threadId = oneapi::tbb::this_task_arena::current_thread_index();
                        for (size_t r = range.begin(); r != range.end(); ++r)
                        {
                            auto &face = m.faces[faces[r]];
                            for (u32 iter = 0, currentID = face.startID; iter < face.indexCount / 3; ++iter)
                            {
                                blocks[threadId] << "f ";
                                for (size_t vertexIndex = 0; vertexIndex < 3; ++vertexIndex)
                                {
                                    auto id = m.indices[currentID + vertexIndex];
                                    auto vID = positions[id];
                                    blocks[threadId] << vID + 1 << "/";
                                    if (meshFlags & MeshExportFlagBits::export_uv)
                                        blocks[threadId] << _vtMap[m.vertices[id].uv] + 1;
                                    if (meshFlags & MeshExportFlagBits::export_normals)
                                        blocks[threadId] << "/" << _vnMap[m.vertices[id].normal] + 1;
                                    blocks[threadId] << " ";
                                }
                                blocks[threadId] << "\n";
                                currentID += 3;
                            }
                        }
                    });
                for (const auto &block : blocks) os << block.str();
            }

            void Exporter::writeFaces(umbf::mesh::MeshBlock *meta, acul::stringstream &os,
                                      const acul::vector<u32> &faces)
            {
                size_t threadCount = oneapi::tbb::this_task_arena::max_concurrency();
                acul::vector<acul::stringstream> blocks(threadCount);
                auto &originFaces = meta->model.faces;
                oneapi::tbb::parallel_for(oneapi::tbb::blocked_range<size_t>(0, faces.size()),
                                          [&](const tbb::blocked_range<size_t> &range) {
                                              size_t threadId = oneapi::tbb::this_task_arena::current_thread_index();
                                              for (size_t i = range.begin(); i != range.end(); ++i)
                                              {
                                                  blocks[threadId] << "f ";
                                                  for (auto &ref : originFaces[faces[i]].vertices)
                                                  {
                                                      blocks[threadId] << ref.group + 1 << "/";
                                                      auto &vertex = meta->model.vertices[ref.vertex];
                                                      if (meshFlags & MeshExportFlagBits::export_uv)
                                                          blocks[threadId] << _vtMap[vertex.uv] + 1;
                                                      if (meshFlags & MeshExportFlagBits::export_normals)
                                                          blocks[threadId] << "/" << _vnMap[vertex.normal] + 1;
                                                      blocks[threadId] << " ";
                                                  }
                                                  blocks[threadId] << "\n";
                                              }
                                          });

                for (const auto &block : blocks) os << block.str();
            }

            inline void writeVec3sRGB(acul::stringstream &os, const acul::string &token, const glm::vec3 &vec)
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
            inline void writeNumber(acul::stringstream &os, const acul::string &token, const T &value)
            {
                os << token << " " << value << "\n";
            }

            void Exporter::writeTexture2D(acul::stringstream &os, const acul::string &token, const acul::string &tex)
            {
                if (materialFlags == MaterialExportFlags::texture_origin)
                    os << token << " " << tex << "\n";
                else if (materialFlags == MaterialExportFlags::texture_copyToLocal)
                {
                    acul::io::path parent = acul::io::path(path).parent_path();
                    acul::io::path tex_path = parent / "tex" / tex;
                    if (acul::io::file::copy(tex.c_str(), tex_path.str().c_str(), true))
                        os << token << " ./tex/" << tex_path.filename() << "\n";
                }
            }

            void writeDefaultMaterial(std::ofstream &os, bool usePBR)
            {
                acul::stringstream matBlock;
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
                os << "\n" << matBlock.str().c_str();
            }

            void Exporter::writeMaterial(const acul::shared_ptr<umbf::MaterialInfo> &matInfo,
                                         const acul::shared_ptr<umbf::Material> &mat, std::ostream &os)
            {
                acul::stringstream matBlock;
                matBlock << "newmtl " << matInfo->name << "\n";
                writeVec3sRGB(matBlock, "Ka", {1, 1, 1});
                writeVec3sRGB(matBlock, "Kd", mat->albedo.rgb);
                if (mat->albedo.textured)
                {
                    auto &tex = textures[mat->albedo.texture_id];
                    writeTexture2D(matBlock, "map_Kd", tex);
                }
                writeVec3sRGB(matBlock, "Ks", {1, 1, 1});
                writeNumber(matBlock, "Ns", 80);
                if (objFlags & ObjExportFlagBits::mat_PBR)
                {
                    writeNumber(matBlock, "Pr", 0.33);
                    writeNumber(matBlock, "Pm", 1);
                }
                writeNumber(matBlock, "illum", 7);
                os << "\n" << matBlock.str().c_str();
            }

            void Exporter::writeMtlLibInfo(std::ofstream &mtlStream, acul::stringstream &objStream)
            {
                if (materialFlags != MaterialExportFlags::none)
                {
                    acul::string mtlPath = acul::io::replace_extension(path, ".mtl");
                    mtlStream.open(mtlPath.c_str());
                    if (!mtlStream.is_open())
                        logWarn("Failed to write MTL file: %s", mtlPath.c_str());
                    else
                    {
                        objStream << "mtllib ./" << acul::io::get_filename(mtlPath) << "\n";
                        mtlStream << "# App3D ECL MTL Exporter\n";
                    }
                }

                for (auto &mat : Exporter::materials)
                {
                    acul::shared_ptr<umbf::Material> ptr;
                    for (auto &block : mat.blocks)
                    {
                        switch (block->signature())
                        {
                            case umbf::sign_block::meta::material:
                                ptr = acul::static_pointer_cast<umbf::Material>(block);
                                break;
                            case umbf::sign_block::meta::material_info:
                            {
                                auto info = acul::static_pointer_cast<umbf::MaterialInfo>(block);
                                _matMap[info->id] = {info, ptr};
                            }
                            break;
                            default:
                                break;
                        }
                    }
                }
            }

            void Exporter::writeObject(const umbf::Object &object, acul::stringstream &objStream)
            {
                acul::shared_ptr<umbf::mesh::MeshBlock> mesh;
                acul::vector<acul::shared_ptr<umbf::MatRangeAssignAtrr>> assignes;
                for (auto &block : object.meta)
                {
                    switch (block->signature())
                    {
                        case umbf::sign_block::meta::mesh:
                            mesh = acul::static_pointer_cast<umbf::mesh::MeshBlock>(block);
                            break;
                        case umbf::sign_block::meta::material_range_assign:
                            assignes.push_back(acul::static_pointer_cast<umbf::MatRangeAssignAtrr>(block));
                            break;
                        default:
                            logWarn("Unsupported signature: 0x%08x", block->signature());
                            break;
                    }
                }
                if (!mesh) return;
                acul::vector<umbf::mesh::VertexGroup> vgroups;
                umbf::utils::mesh::fillVertexGroups(mesh->model, vgroups);
                if (objFlags & ObjExportFlagBits::mgp_groups)
                    objStream << "g " << object.name << "\n";
                else if (objFlags & ObjExportFlagBits::mgp_objects)
                    objStream << "o " << object.name << "\n";
                auto &model = mesh->model;
                writeVertices(model, vgroups, objStream);
                acul::vector<acul::shared_ptr<umbf::MatRangeAssignAtrr>> assignesAttr;
                auto default_matID_it = std::find_if(
                    assignes.begin(), assignes.end(),
                    [](const acul::shared_ptr<umbf::MatRangeAssignAtrr> &range) { return range->faces.empty(); });
                u64 default_matID = default_matID_it == assignes.end() ? 0 : (*default_matID_it)->matID;
                umbf::utils::filterMatAssignments(assignes, model.faces.size(), default_matID, assignesAttr);
                for (auto &assign : assignesAttr)
                {
                    if (assign->faces.empty()) continue;
                    if (materialFlags != MaterialExportFlags::none)
                    {
                        if (assign->matID == 0)
                        {
                            objStream << "usemtl default\n";
                            _allMaterialsExist = false;
                        }
                        else
                        {
                            auto it = _matMap.find(assign->matID);
                            if (it == _matMap.end())
                                logError("Failed to find material: %llx", assign->matID);
                            else
                                objStream << "usemtl " << it->second.info->name << "\n";
                        }
                    }
                    if (meshFlags & MeshExportFlagBits::export_triangulated)
                        writeTriangles(mesh.get(), objStream, assign->faces, vgroups);
                    else
                        writeFaces(mesh.get(), objStream, assign->faces);
                }
            }

            void Exporter::writeMtl(std::ofstream &stream)
            {
                if (!stream) return;
                if (!_allMaterialsExist) writeDefaultMaterial(stream, objFlags & ObjExportFlagBits::mat_PBR);
                for (auto it = _matMap.begin(); it != _matMap.end(); it++)
                {
                    auto &ref = it->second;
                    writeMaterial(ref.info, ref.mat, stream);
                }
                stream.close();
            }

            bool Exporter::save()
            {
                logInfo("Exporting OBJ file: %s", path.c_str());
                try
                {
                    acul::stringstream ss;
                    ss << "# App3D ECL OBJ Exporter\n";
                    std::ofstream mtlStream;
                    writeMtlLibInfo(mtlStream, ss);
                    for (auto &object : objects) writeObject(object, ss);

                    acul::string error;
                    if (!acul::io::file::write_by_block(path, ss.str().c_str(), 1024 * 1024, error))
                    {
                        logError("OBJ export failed: %s", error.c_str());
                        return false;
                    }
                    writeMtl(mtlStream);
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