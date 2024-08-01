#include <core/hash.hpp>
#include <core/io/file.hpp>
#include <core/log.hpp>
#include <core/std/string.hpp>
#include <ecl/scene/obj/export.hpp>
#include <oneapi/tbb/parallel_for.h>
#include <string>


namespace ecl
{
    namespace scene
    {
        namespace obj
        {
            void transformVertexPos(glm::vec3 &pos, MeshExportFlags flags)
            {
                if (flags & MeshExportFlagBits::transform_reverseX) pos.x = -pos.x;
                if (flags & MeshExportFlagBits::transform_reverseY) pos.y = -pos.y;
                if (flags & MeshExportFlagBits::transform_reverseZ) pos.z = -pos.z;
                if (flags & MeshExportFlagBits::transform_swapXY) std::swap(pos.x, pos.y);
                if (flags & MeshExportFlagBits::transform_swapXZ) std::swap(pos.x, pos.z);
                if (flags & MeshExportFlagBits::transform_swapYZ) std::swap(pos.y, pos.z);
            }

            void Exporter::writeVertices(const DArray<assets::mesh::Vertex> &vertices, std::stringstream &ss)
            {
                oneapi::tbb::parallel_for(oneapi::tbb::blocked_range<size_t>(0, vertices.size()),
                                          [&](const tbb::blocked_range<size_t> &range) {
                                              for (size_t i = range.begin(); i != range.end(); ++i)
                                              {
                                                  auto vertex = vertices[i];
                                                  auto [vIt, vInserted] = _vSet.insert(vertex.pos);
                                                  if (vInserted)
                                                  {
                                                      transformVertexPos(vertex.pos, _meshFlags);
                                                      _vSet.emplace(vertex.pos);
                                                  }
                                                  if (_meshFlags & MeshExportFlagBits::export_uv)
                                                      _vtSet.emplace(vertex.uv);
                                                  if (_meshFlags & MeshExportFlagBits::export_normals)
                                                      _vnSet.emplace(vertex.normal);
                                              }
                                          });
                {
                    const size_t bufSize = _vSet.size() * 50;
                    char *buf = new char[bufSize];
                    int currentBufPos = 0;
                    for (const auto &item : _vSet)
                    {
                        const glm::vec3 &pos = item;
                        buf[currentBufPos + 0] = 'v';
                        buf[currentBufPos + 1] = ' ';
                        int vec3Written = vec3ToStr(pos, buf + 2 + currentBufPos, 50, 0);
                        if (vec3Written == 0) continue;
                        buf[2 + vec3Written + currentBufPos] = '\n';
                        currentBufPos += 3 + vec3Written;
                        _vMap.insert({pos, _vMap.size() + 1});
                    }
                    ss.write(buf, currentBufPos);
                    delete[] buf;
                }
                if (_meshFlags & MeshExportFlagBits::export_uv)
                {
                    const size_t bufSize = _vtSet.size() * 35;
                    char *buf = new char[bufSize];
                    int currentBufPos = 0;
                    for (const auto &item : _vtSet)
                    {
                        const glm::vec2 &uv = item;
                        buf[currentBufPos + 0] = 'v';
                        buf[currentBufPos + 1] = 't';
                        buf[currentBufPos + 2] = ' ';
                        int vec2Written = vec2ToStr(uv, buf + 3 + currentBufPos, 35, 0);
                        if (vec2Written == 0) continue;
                        buf[3 + vec2Written + currentBufPos] = '\n';
                        currentBufPos += 4 + vec2Written;
                        _vtMap.insert({uv, _vtMap.size() + 1});
                    }
                    ss.write(buf, currentBufPos);
                    delete[] buf;
                }
                if (_meshFlags & MeshExportFlagBits::export_normals)
                {
                    const size_t bufSize = _vnSet.size() * 50;
                    char *buf = new char[bufSize];
                    int currentBufPos = 0;
                    for (const auto &item : _vnSet)
                    {
                        const glm::vec3 &normal = item;
                        buf[currentBufPos + 0] = 'v';
                        buf[currentBufPos + 1] = 'n';
                        buf[currentBufPos + 2] = ' ';
                        int vec3Written = vec3ToStr(normal, buf + 3 + currentBufPos, 35, 0);
                        if (vec3Written == 0) continue;
                        buf[3 + vec3Written + currentBufPos] = '\n';
                        currentBufPos += 4 + vec3Written;
                        _vnMap.insert({normal, _vnMap.size() + 1});
                    }
                    ss.write(buf, currentBufPos);
                    delete[] buf;
                }
            }

            void Exporter::writeTriangles(assets::mesh::MeshBlock *meta, std::ostream &os)
            {
                const auto &indices = meta->model.indices;
                const auto &vertices = meta->model.vertices;
                size_t threadCount = oneapi::tbb::this_task_arena::max_concurrency();
                DArray<std::stringstream> blocks(threadCount);
                oneapi::tbb::parallel_for(oneapi::tbb::blocked_range<size_t>(0, indices.size() / 3),
                                          [&](const tbb::blocked_range<size_t> &range) {
                                              size_t threadId = oneapi::tbb::this_task_arena::current_thread_index();
                                              for (size_t i = range.begin(); i != range.end(); ++i)
                                              {
                                                  size_t triangleIndex = i * 3;
                                                  blocks[threadId] << "f ";

                                                  for (size_t vertexIndex = 0; vertexIndex < 3; ++vertexIndex)
                                                  {
                                                      const size_t index = indices[triangleIndex + vertexIndex];
                                                      auto &vertex = vertices[index];
                                                      auto vIt = _vMap.find(vertex.pos);
                                                      if (vIt != _vMap.end()) blocks[threadId] << vIt->second << "/";
                                                      if (_meshFlags & MeshExportFlagBits::export_uv)
                                                      {
                                                          auto vtIt = _vtMap.find(vertex.uv);
                                                          if (vtIt != _vtMap.end())
                                                              blocks[threadId] << vtIt->second << "/";
                                                      }
                                                      if (_meshFlags & MeshExportFlagBits::export_normals)
                                                      {
                                                          auto vnIt = _vnMap.find(vertex.normal);
                                                          if (vnIt != _vnMap.end()) blocks[threadId] << vnIt->second;
                                                      }
                                                      blocks[threadId] << " ";
                                                  }
                                                  blocks[threadId] << "\n";
                                              }
                                          });
                for (const auto &block : blocks) os << block.str();
            }

            void Exporter::writeFaces(assets::mesh::MeshBlock *meta, std::ostream &os)
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
                                                  auto &face = faces[i];
                                                  //   for (const auto &edge : face.edges)
                                                  //   {
                                                  //       auto orderedEdge = face.getOrderedEdge(model->edges(), edge);
                                                  //       auto &vertex = model->vertices()[orderedEdge.first];
                                                  //       auto vIt = _vMap.find(vertex.pos);
                                                  //       if (vIt != _vMap.end())
                                                  //           _faceTokenCallback.v(blocks[threadId], vIt->second);
                                                  //       if (_builder.isUVEnabled())
                                                  //       {
                                                  //           auto vtIt = _vtMap.find(vertex.uv);
                                                  //           if (vtIt != _vtMap.end())
                                                  //               _faceTokenCallback.vt(blocks[threadId],
                                                  //               vtIt->second);
                                                  //       }
                                                  //       if (_builder.isNormalsEnable())
                                                  //       {
                                                  //           auto vnIt = _vnMap.find(vertex.normal);
                                                  //           if (vnIt != _vnMap.end())
                                                  //               _faceTokenCallback.vn(blocks[threadId],
                                                  //               vnIt->second);
                                                  //       }
                                                  //       blocks[threadId] << " ";
                                                  //   }

                                                  blocks[threadId] << "\n";
                                              }
                                          });

                for (const auto &block : blocks) os << block.str();
            }

            inline void writeVec3sRGB(std::ostream &os, const std::string &token, const glm::vec3 &vec)
            {
                os << token << " " << vec.x << " " << vec.y << " " << vec.z << "\n";
            }

            void writeDefaultMaterial(std::ostream &os, bool usePBR)
            {
                std::stringstream matBlock;
                matBlock << "newmtl default\n";
                writeVec3sRGB(matBlock, "Ka", {1, 1, 1});
                //                     writeVec3sRGB(matBlock, "Kd", {1, 1, 1});
                //                     writeVec3sRGB(matBlock, "Ks", {1, 1, 1});
                //                     writeNumber(matBlock, "Ns", 80);
                //                     if (usePBR)
                //                     {
                //                         writeNumber(matBlock, "Pr", 0.33);
                //                         writeNumber(matBlock, "Pm", 1);
                //                     }
                //                     writeNumber(matBlock, "illum", 7);
                //                     os << "\n" << matBlock.str();
            }

            bool Exporter::save()
            {
                logInfo("Exporting OBJ file: %ls", _path.c_str());
                try
                {
                    std::stringstream ss;
                    ss << "# App3D OBJ Exporter\n";
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
                            mtllibFileStream << "# App3D MTL Exporter\n";
                        }
                    }
                    _vSet.clear();
                    _vtSet.clear();
                    _vMap.clear();
                    _vtMap.clear();

                    bool allMaterialsExists = true;
                    for (const auto &mesh : _meshes)
                    {
                        if (_objFlags & ObjExportFlagBits::mgp_groups)
                            ss << "g " << mesh.name << "\n";
                        else if (_objFlags & ObjExportFlagBits::mgp_objects)
                            ss << "o " << mesh.name << "\n";
                        writeVertices(mesh.meta->model.vertices, ss);
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
                        logError("Failed to write OBJ file: %s", error.c_str());
                        return false;
                    }
                    if (mtllibFileStream)
                    {
                        std::filesystem::path texFolder = _path;
                        texFolder.replace_filename("tex");
                        // if (!allMaterialsExists)
                        //     writeDefaultMaterial(mtllibFileStream, _objFlags & ObjExportFlagBits::mat_PBR);
                        // for (const auto &mat : _materials)
                        //     writeMaterial(mtllibFileStream, mat, _objFlags & ObjExportFlagBits::mat_PBR);

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