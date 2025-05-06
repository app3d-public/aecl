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
            void transform_vertex(glm::vec3 &pos, MeshExportFlags flags)
            {
                if (flags & MeshExportFlagBits::TransformReverseX) pos.x = -pos.x;
                if (flags & MeshExportFlagBits::TransformReverseY) pos.y = -pos.y;
                if (flags & MeshExportFlagBits::TransformReverseZ) pos.z = -pos.z;
                if (flags & MeshExportFlagBits::TransformSwapXY) std::swap(pos.x, pos.y);
                if (flags & MeshExportFlagBits::TransformSwapXZ) std::swap(pos.x, pos.z);
                if (flags & MeshExportFlagBits::TransformSwapYZ) std::swap(pos.y, pos.z);
            }

            void Exporter::write_vertices(umbf::mesh::Model &model, const acul::vector<umbf::mesh::VertexGroup> &groups,
                                          acul::stringstream &ss)
            {
                // v
                for (auto &group : groups)
                {
                    auto &vertex_id = group.vertices.front();
                    glm::vec3 &pos = model.vertices[vertex_id].pos;
                    transform_vertex(pos, mesh_flags);
                    ss << "v " << pos.x << " " << pos.y << " " << pos.z << "\n";
                }

                // vt and vn
                for (auto &vertex : model.vertices)
                {
                    if (mesh_flags & MeshExportFlagBits::ExportUV)
                    {
                        auto [it, inserted] = _vt_map.emplace(vertex.uv, _vt_map.size());
                        if (inserted) ss << "vt " << vertex.uv.x << " " << vertex.uv.y << "\n";
                    }
                    if (mesh_flags & MeshExportFlagBits::ExportNormals)
                    {
                        auto &normal = vertex.normal;
                        transform_vertex(normal, mesh_flags);
                        auto [it, inserted] = _vn_map.emplace(normal, _vn_map.size());
                        if (inserted) ss << "vn " << normal.x << " " << normal.y << " " << normal.z << "\n";
                    }
                }

                if ((mesh_flags & MeshExportFlagBits::TransformReverseX) ||
                    (mesh_flags & MeshExportFlagBits::TransformReverseY) ||
                    (mesh_flags & MeshExportFlagBits::TransformReverseZ))
                    for (auto &face : model.faces) std::reverse(face.vertices.begin(), face.vertices.end());
            }

            void Exporter::write_triangles(umbf::mesh::MeshBlock *meta, acul::stringstream &os,
                                           const acul::vector<u32> &faces,
                                           const acul::vector<umbf::mesh::VertexGroup> &groups)
            {
                const auto &m = meta->model;
                acul::vector<u32> positions(m.vertices.size());
                for (int g = 0; g < groups.size(); g++)
                    for (auto id : groups[g].vertices) positions[id] = g;

                size_t thread_count = oneapi::tbb::this_task_arena::max_concurrency();
                acul::vector<acul::stringstream> blocks(thread_count);
                oneapi::tbb::parallel_for(
                    oneapi::tbb::blocked_range<size_t>(0, faces.size()), [&](const tbb::blocked_range<size_t> &range) {
                        size_t thread_id = oneapi::tbb::this_task_arena::current_thread_index();
                        for (size_t r = range.begin(); r != range.end(); ++r)
                        {
                            auto &face = m.faces[faces[r]];
                            for (u32 iter = 0, currentID = face.first_vertex; iter < face.count / 3; ++iter)
                            {
                                blocks[thread_id] << "f ";
                                for (size_t vertexIndex = 0; vertexIndex < 3; ++vertexIndex)
                                {
                                    auto id = m.indices[currentID + vertexIndex];
                                    auto vID = positions[id];
                                    blocks[thread_id] << vID + 1 << "/";
                                    if (mesh_flags & MeshExportFlagBits::ExportUV)
                                        blocks[thread_id] << _vt_map[m.vertices[id].uv] + 1;
                                    if (mesh_flags & MeshExportFlagBits::ExportNormals)
                                        blocks[thread_id] << "/" << _vn_map[m.vertices[id].normal] + 1;
                                    blocks[thread_id] << " ";
                                }
                                blocks[thread_id] << "\n";
                                currentID += 3;
                            }
                        }
                    });
                for (const auto &block : blocks) os << block.str();
            }

            void Exporter::write_faces(umbf::mesh::MeshBlock *meta, acul::stringstream &os,
                                       const acul::vector<u32> &faces)
            {
                size_t thread_count = oneapi::tbb::this_task_arena::max_concurrency();
                acul::vector<acul::stringstream> blocks(thread_count);
                auto &origin_faces = meta->model.faces;
                oneapi::tbb::parallel_for(oneapi::tbb::blocked_range<size_t>(0, faces.size()),
                                          [&](const tbb::blocked_range<size_t> &range) {
                                              size_t thread_id = oneapi::tbb::this_task_arena::current_thread_index();
                                              for (size_t i = range.begin(); i != range.end(); ++i)
                                              {
                                                  blocks[thread_id] << "f ";
                                                  for (auto &ref : origin_faces[faces[i]].vertices)
                                                  {
                                                      blocks[thread_id] << ref.group + 1 << "/";
                                                      auto &vertex = meta->model.vertices[ref.vertex];
                                                      if (mesh_flags & MeshExportFlagBits::ExportUV)
                                                          blocks[thread_id] << _vt_map[vertex.uv] + 1;
                                                      if (mesh_flags & MeshExportFlagBits::ExportNormals)
                                                          blocks[thread_id] << "/" << _vn_map[vertex.normal] + 1;
                                                      blocks[thread_id] << " ";
                                                  }
                                                  blocks[thread_id] << "\n";
                                              }
                                          });

                for (const auto &block : blocks) os << block.str();
            }

            inline void write_vec3_as_rgb(acul::stringstream &os, const acul::string &token, const glm::vec3 &vec)
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
            inline void write_number(acul::stringstream &os, const acul::string &token, const T &value)
            {
                os << token << " " << value << "\n";
            }

            void Exporter::write_texture(acul::stringstream &os, const acul::string &token, const acul::string &tex)
            {
                if (material_flags == MaterialExportFlags::TextureOrigin)
                    os << token << " " << tex << "\n";
                else if (material_flags == MaterialExportFlags::TextureCopyToLocal)
                {
                    acul::io::path parent = acul::io::path(path).parent_path();
                    acul::io::path tex_path = parent / "tex" / tex;
                    if (acul::io::file::copy(tex.c_str(), tex_path.str().c_str(), true))
                        os << token << " ./tex/" << tex_path.filename() << "\n";
                }
            }

            void write_default_material(std::ofstream &os, bool use_pbr)
            {
                acul::stringstream mat_block;
                mat_block << "newmtl default\n";
                write_vec3_as_rgb(mat_block, "Ka", {1, 1, 1});
                write_vec3_as_rgb(mat_block, "Kd", {1, 1, 1});
                write_vec3_as_rgb(mat_block, "Ks", {1, 1, 1});
                write_number(mat_block, "Ns", 80);
                if (use_pbr)
                {
                    write_number(mat_block, "Pr", 0.33);
                    write_number(mat_block, "Pm", 1);
                }
                write_number(mat_block, "illum", 7);
                os << "\n" << mat_block.str().c_str();
            }

            void Exporter::write_material(const acul::shared_ptr<umbf::MaterialInfo> &material_info,
                                          const acul::shared_ptr<umbf::Material> &material, std::ostream &os)
            {
                acul::stringstream mat_block;
                mat_block << "newmtl " << material_info->name << "\n";
                write_vec3_as_rgb(mat_block, "Ka", {1, 1, 1});
                write_vec3_as_rgb(mat_block, "Kd", material->albedo.rgb);
                if (material->albedo.textured)
                {
                    auto &tex = textures[material->albedo.texture_id];
                    write_texture(mat_block, "map_Kd", tex);
                }
                write_vec3_as_rgb(mat_block, "Ks", {1, 1, 1});
                write_number(mat_block, "Ns", 80);
                if (obj_flags & ObjExportFlagBits::MaterialsPBR)
                {
                    write_number(mat_block, "Pr", 0.33);
                    write_number(mat_block, "Pm", 1);
                }
                write_number(mat_block, "illum", 7);
                os << "\n" << mat_block.str().c_str();
            }

            void Exporter::write_mtllib_info(std::ofstream &mtl_stream, acul::stringstream &obj_stream)
            {
                if (material_flags != MaterialExportFlags::None)
                {
                    acul::string mtlPath = acul::io::replace_extension(path, ".mtl");
                    mtl_stream.open(mtlPath.c_str());
                    if (!mtl_stream.is_open())
                        LOG_WARN("Failed to write MTL file: %s", mtlPath.c_str());
                    else
                    {
                        obj_stream << "mtllib ./" << acul::io::get_filename(mtlPath) << "\n";
                        mtl_stream << "# App3D ECL MTL Exporter\n";
                    }
                }

                for (auto &material : Exporter::materials)
                {
                    acul::shared_ptr<umbf::Material> ptr;
                    for (auto &block : material.blocks)
                    {
                        switch (block->signature())
                        {
                            case umbf::sign_block::meta::Material:
                                ptr = acul::static_pointer_cast<umbf::Material>(block);
                                break;
                            case umbf::sign_block::meta::MaterialInfo:
                            {
                                auto info = acul::static_pointer_cast<umbf::MaterialInfo>(block);
                                _material_map[info->id] = {info, ptr};
                            }
                            break;
                            default:
                                break;
                        }
                    }
                }
            }

            void Exporter::write_object(const umbf::Object &object, acul::stringstream &stream)
            {
                acul::shared_ptr<umbf::mesh::MeshBlock> mesh;
                acul::vector<acul::shared_ptr<umbf::MatRangeAssignAttr>> assignes;
                for (auto &block : object.meta)
                {
                    switch (block->signature())
                    {
                        case umbf::sign_block::meta::Mesh:
                            mesh = acul::static_pointer_cast<umbf::mesh::MeshBlock>(block);
                            break;
                        case umbf::sign_block::meta::MaterialRangeAssign:
                            assignes.push_back(acul::static_pointer_cast<umbf::MatRangeAssignAttr>(block));
                            break;
                        default:
                            LOG_WARN("Unsupported signature: 0x%08x", block->signature());
                            break;
                    }
                }
                if (!mesh) return;
                acul::vector<umbf::mesh::VertexGroup> vertex_groups;
                umbf::utils::mesh::fill_vertex_groups(mesh->model, vertex_groups);
                if (obj_flags & ObjExportFlagBits::ObjectPolicyGroups)
                    stream << "g " << object.name << "\n";
                else if (obj_flags & ObjExportFlagBits::ObjectPolicyObjects)
                    stream << "o " << object.name << "\n";
                auto &model = mesh->model;
                write_vertices(model, vertex_groups, stream);
                acul::vector<acul::shared_ptr<umbf::MatRangeAssignAttr>> assignesAttr;
                auto default_mat_id_it = std::find_if(
                    assignes.begin(), assignes.end(),
                    [](const acul::shared_ptr<umbf::MatRangeAssignAttr> &range) { return range->faces.empty(); });
                u64 default_mat_id = default_mat_id_it == assignes.end() ? 0 : (*default_mat_id_it)->mat_id;
                umbf::utils::filter_mat_assignments(assignes, model.faces.size(), default_mat_id, assignesAttr);
                for (auto &assign : assignesAttr)
                {
                    if (assign->faces.empty()) continue;
                    if (material_flags != MaterialExportFlags::None)
                    {
                        if (assign->mat_id == 0)
                        {
                            stream << "usemtl default\n";
                            _all_materials_exist = false;
                        }
                        else
                        {
                            auto it = _material_map.find(assign->mat_id);
                            if (it == _material_map.end())
                                LOG_ERROR("Failed to find material: %llx", assign->mat_id);
                            else
                                stream << "usemtl " << it->second.info->name << "\n";
                        }
                    }
                    if (mesh_flags & MeshExportFlagBits::ExportTriangulated)
                        write_triangles(mesh.get(), stream, assign->faces, vertex_groups);
                    else
                        write_faces(mesh.get(), stream, assign->faces);
                }
            }

            void Exporter::write_mtl(std::ofstream &stream)
            {
                if (!stream) return;
                if (!_all_materials_exist) write_default_material(stream, obj_flags & ObjExportFlagBits::MaterialsPBR);
                for (auto it = _material_map.begin(); it != _material_map.end(); it++)
                {
                    auto &ref = it->second;
                    write_material(ref.info, ref.mat, stream);
                }
                stream.close();
            }

            bool Exporter::save()
            {
                LOG_INFO("Exporting OBJ file: %s", path.c_str());
                try
                {
                    acul::stringstream ss;
                    ss << "# App3D ECL OBJ Exporter\n";
                    std::ofstream mtlStream;
                    write_mtllib_info(mtlStream, ss);
                    for (auto &object : objects) write_object(object, ss);

                    acul::string error;
                    if (!acul::io::file::write_by_block(path, ss.str().c_str(), 1024 * 1024, error))
                    {
                        LOG_ERROR("OBJ export failed: %s", error.c_str());
                        return false;
                    }
                    write_mtl(mtlStream);
                    return true;
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR("Failed to export file: %s", e.what());
                    return false;
                }
                return true;
            }
        } // namespace obj
    } // namespace scene
} // namespace ecl