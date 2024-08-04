#include <core/event/event.hpp>
#include <core/hash.hpp>
#include <core/locales.hpp>
#include <core/log.hpp>
#include <core/std/string.hpp>
#include <core/task.hpp>
#include <ecl/image/import.hpp>
#include <ecl/scene/obj/import.hpp>
#include <ecl/scene/utils.hpp>
#include <emhash/hash_table5.hpp>
#include <oneapi/tbb/parallel_sort.h>

namespace ecl
{
    namespace scene
    {
        namespace obj
        {
            void parseLine(ParseIndexed &buffer, const char *line, int lineIndex)
            {
                if (line[0] == '\0' || line[0] == '#') return;
                const char *token = line;
                if (token[0] == 'v')
                {
                    if (isspace(token[1]))
                    {
                        token += 2;
                        glm::vec3 v;
                        if (!strToV3(token, v))
                            throw std::runtime_error("Failed to parse line: " + std::string(line));
                        else
                            buffer.v.emplace_back(lineIndex, v);
                    }
                    else if (token[1] == 't')
                    {
                        token += 3;
                        glm::vec2 vt;
                        if (!strToV2(token, vt))
                            throw std::runtime_error("Failed to parse line: " + std::string(line));
                        else
                            buffer.vt.emplace_back(lineIndex, vt);
                    }
                    else if (token[1] == 'n')
                    {
                        token += 3;
                        glm::vec3 vn;
                        if (!strToV3(token, vn))
                            throw std::runtime_error("Failed to parse line: " + std::string(line));
                        else
                            buffer.vn.emplace_back(lineIndex, vn);
                    }
                }
                else if (token[0] == 'g' || token[0] == 'o')
                {
                    token += 2;
                    std::string str = trimEnd({token});
                    if (str != "off" && !str.empty()) buffer.g.emplace_back(lineIndex, str);
                }
                else if (token[0] == 'f')
                {
                    token += 2;
                    DArray<glm::ivec3> *vtn = new DArray<glm::ivec3>();
                    while (true)
                    {
                        int vId{0}, vtId{0}, vnId{0};
                        if (!(strToI(token, vId))) break;
                        // Handle negative indices
                        if (vId < 0) vId += buffer.v.size() + 1;

                        if (vId > 0)
                        {
                            // Changed this section
                            if (*token == '/')
                            {
                                ++token;
                                if (strToI(token, vtId))
                                    if (vtId < 0) vtId += buffer.vt.size() + 1;
                                if (*token == '/')
                                {
                                    ++token;
                                    if (strToI(token, vnId))
                                        if (vnId < 0) vnId += buffer.vn.size() + 1;
                                }
                            }
                            vtn->emplace_back(vId, vtId, vnId);

                            // Skip spaces
                            while (isspace(*token)) ++token;
                        }
                        else
                            break;
                    }

                    buffer.f.emplace_back(lineIndex, vtn);
                }
                else if (strncmp(token, "mtllib", 6) == 0)
                {
                    token += 7;
                    buffer.mtllib = getStrRange(token);
                }
                else if (strncmp(token, "usemtl", 6) == 0)
                {
                    token += 7;
                    buffer.useMtl.emplace_back(lineIndex, getStrRange(token));
                }
            }

            int getGroupRangeEnd(int startIndex, int rangeEnd, ParseIndexed &parsed)
            {
                for (int i = startIndex; i < parsed.f.size(); ++i)
                    if (parsed.f[i].index >= rangeEnd) return i;
                return parsed.f.size();
            }

            struct GroupRange
            {
                int startIndex;
                int rangeEnd;
                std::string name;
                assets::mesh::MeshBlock *mesh;
            };

            glm::vec3 getInnerEdge(size_t pos, f32 v1, f32 v2)
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

            struct ParseSingleThread
            {
                Line<glm::vec3> *__restrict v;
                size_t vsize;
                Line<glm::vec2> *__restrict vt;
                size_t vtsize;
                Line<glm::vec3> *__restrict vn;
                size_t vnsize;
                Line<DArray<glm::ivec3> *> *__restrict f;
                size_t fsize;
                Line<std::string> *g;
                size_t gsize;
                Line<std::string> *useMtl;
                size_t useMtlsize;
            };

            glm::vec3 calculateNormal(const ParseSingleThread &ps, const DArray<glm::ivec3> &__restrict inFace)
            {
                glm::vec3 normal{0.0f};
                for (int v = 0; v < inFace.size(); ++v)
                {
                    glm::vec3 current = ps.v[inFace[v].x - 1].value;
                    glm::vec3 next = ps.v[inFace[(v + 1) % inFace.size()].x - 1].value;
                    normal.x += (current.y - next.y) * (current.z + next.z);
                    normal.y += (current.z - next.z) * (current.x + next.x);
                    normal.z += (current.x - next.x) * (current.y + next.y);
                }
                return glm::normalize(normal);
            }

            void addVertexToFace(const ParseSingleThread &__restrict ps, DArray<u32> &vgv, u32 current,
                                 emhash5::HashMap<glm::ivec3, u32> &vtnMap, const glm::ivec3 &vtn,
                                 assets::mesh::Model &m, assets::mesh::Face &face)
            {
                auto [vIt, vInserted] = vtnMap.emplace(vtn, vtnMap.size());
                if (vInserted)
                {
                    vgv.emplace_back(m.vertices.size());
                    face.vertices.emplace_back(current, m.vertices.size());
                    m.vertices.emplace_back(ps.v[current].value);
                    auto &vertex = m.vertices.back();
                    if (vtn.y != 0 && ps.vtsize > vtn.y) vertex.uv = ps.vt[vtn.y - 1].value;
                    vertex.normal = vtn.z != 0 && ps.vnsize > vtn.z ? ps.vn[vtn.z - 1].value : face.normal;
                    m.aabb.min = glm::min(m.aabb.min, vertex.pos);
                    m.aabb.max = glm::max(m.aabb.max, vertex.pos);
                }
                else
                    face.vertices.emplace_back(current, vIt->second);
            }

            void addVertexToFace(const ParseSingleThread &__restrict ps, DArray<u32> &vgv, u32 current,
                                 const glm::ivec3 &vtn, assets::mesh::Model &m, assets::mesh::Face &face)
            {
                assets::mesh::Vertex vertex{ps.v[current].value};
                if (vtn.y != 0 && ps.vtsize > vtn.y) vertex.uv = ps.vt[vtn.y - 1].value;
                if (vtn.z != 0 && ps.vnsize > vtn.z)
                    vertex.normal = ps.vn[vtn.z - 1].value;
                else
                    vertex.normal = face.normal;
                auto it = std::find_if(vgv.begin(), vgv.end(),
                                       [&m, &vertex](u32 index) { return m.vertices[index] == vertex; });
                if (it == vgv.end())
                {
                    vgv.emplace_back(m.vertices.size());
                    face.vertices.emplace_back(current, m.vertices.size());
                    m.vertices.emplace_back(vertex);
                    m.aabb.min = glm::min(m.aabb.min, vertex.pos);
                    m.aabb.max = glm::max(m.aabb.max, vertex.pos);
                }
                else
                    face.vertices.emplace_back(current, *it);
            }

            void indexMesh(size_t faceCount, const ParseSingleThread &ps, GroupRange &group)
            {
                emhash5::HashMap<glm::ivec3, u32> vtnMap;
                if (ps.vnsize > 0) vtnMap.reserve(ps.vsize);
                group.mesh->model.faces.resize(faceCount);
                for (size_t f = 0; f < faceCount; ++f)
                {
                    auto &inFace = ps.f[group.startIndex + f].value;
                    auto &m = group.mesh->model;
                    auto &face = m.faces[f];
                    face.normal = calculateNormal(ps, *inFace);
                    for (int v = 0; v < inFace->size(); ++v)
                    {
                        auto &vtn = (*inFace)[v];
                        const int current = vtn.x - 1;
                        if (ps.vsize < vtn.x) continue;
                        auto &vertexGroup = m.vertexGroups[current];
                        if (ps.vnsize == 0)
                            addVertexToFace(ps, vertexGroup.vertices, current, vtn, m, face);
                        else
                            addVertexToFace(ps, vertexGroup.vertices, current, vtnMap, vtn, m, face);
                        vertexGroup.faces.push_back(f);
                    }
                }
            }

            DArray<assets::mesh::MeshBlock *> serializeToGroups(ParseIndexed &parsed)
            {
                oneapi::tbb::parallel_sort(parsed.v.begin(), parsed.v.end());
                oneapi::tbb::parallel_sort(parsed.vn.begin(), parsed.vn.end());
                oneapi::tbb::parallel_sort(parsed.vt.begin(), parsed.vt.end());
                oneapi::tbb::parallel_sort(parsed.g.begin(), parsed.g.end());
                oneapi::tbb::parallel_sort(parsed.f.begin(), parsed.f.end());

                ParseSingleThread ps;
                ps.vsize = parsed.v.size();
                ps.vtsize = parsed.vt.size();
                ps.vnsize = parsed.vn.size();
                ps.fsize = parsed.f.size();
                ps.gsize = parsed.g.size();
                ps.useMtlsize = parsed.useMtl.size();

                ps.v = (Line<glm::vec3> *)scalable_malloc(ps.vsize * sizeof(Line<glm::vec3>));
                ps.vt = (Line<glm::vec2> *)scalable_malloc(ps.vtsize * sizeof(Line<glm::vec2>));
                ps.vn = (Line<glm::vec3> *)scalable_malloc(ps.vnsize * sizeof(Line<glm::vec3>));
                ps.f = (Line<DArray<glm::ivec3> *> *)scalable_malloc(ps.fsize * sizeof(Line<DArray<glm::ivec3> *>));
                ps.g = (Line<std::string> *)scalable_malloc(ps.gsize * sizeof(Line<std::string>));
                ps.useMtl = (Line<std::string> *)scalable_malloc(ps.useMtlsize * sizeof(Line<std::string>));

                // Construct
                for (size_t i = 0; i < parsed.v.size(); ++i) new (&ps.v[i]) Line<glm::vec3>(parsed.v[i]);
                for (size_t i = 0; i < parsed.vt.size(); ++i) new (&ps.vt[i]) Line<glm::vec2>(parsed.vt[i]);
                for (size_t i = 0; i < parsed.vn.size(); ++i) new (&ps.vn[i]) Line<glm::vec3>(parsed.vn[i]);
                for (size_t i = 0; i < parsed.f.size(); ++i) new (&ps.f[i]) Line<DArray<glm::ivec3> *>(parsed.f[i]);
                for (size_t i = 0; i < parsed.g.size(); ++i) new (&ps.g[i]) Line<std::string>(parsed.g[i]);
                for (size_t i = 0; i < parsed.useMtl.size(); ++i)
                    new (&ps.useMtl[i]) Line<std::string>(parsed.useMtl[i]);

                DArray<assets::mesh::MeshBlock *> meshes;
                DArray<GroupRange> groups;
                groups.reserve(ps.gsize + 1);

                if (ps.fsize == 0) return meshes;

                int lfi = 0;
                if (ps.gsize == 0 || ps.f->index < ps.g->index)
                {
                    int rangeEnd = parsed.g.empty() ? ps.f[ps.fsize - 1].index + 1 : ps.g->index;
                    lfi = getGroupRangeEnd(0, rangeEnd, parsed);
                    groups.emplace_back(0, lfi, "default");
                }

                for (int g = 0; g < ps.gsize; ++g)
                {
                    int rangeEnd = (g < parsed.g.size() - 1) ? ps.g[g + 1].index : ps.f[ps.fsize - 1].index + 1;
                    int temp = getGroupRangeEnd(lfi, rangeEnd, parsed);
                    groups.emplace_back(lfi, temp, ps.g[g].value);
                    lfi = temp;
                }

                for (auto &group : groups)
                {
                    logInfo("Indexing group data: '%s'", group.name.c_str());
                    const size_t faceCount = group.rangeEnd - group.startIndex;
                    group.mesh = new assets::mesh::MeshBlock();
                    group.mesh->model.vertexGroups.resize(ps.vsize);
                    indexMesh(faceCount, ps, group);
                    auto &m = group.mesh->model;
                    logInfo("Imported vertices: %zu", m.vertices.size());
                    logInfo("Imported faces: %zu", m.faces.size());
                    logInfo("Triangulating mesh group");
                    DArray<DArray<u32>> ires(faceCount);
                    DArray<DArray<assets::mesh::bary::Vertex>> bres(faceCount);
                    oneapi::tbb::parallel_for(oneapi::tbb::blocked_range<size_t>(0, faceCount),
                                              [&](const oneapi::tbb::blocked_range<size_t> &range) {
                                                  for (size_t i = range.begin(); i < range.end(); ++i)
                                                  {
                                                      ires[i] = utils::triangulate(m.faces[i], m.vertices);
                                                      m.faces[i].indexCount = ires[i].size();
                                                      utils::buildBarycentric(bres[i], m.faces[i], m.vertices, ires[i]);
                                                  }
                                              });
                    auto &barycentrics = group.mesh->barycentricVertices;
                    u32 currentID = 0;
                    for (int i = 0; i < faceCount; ++i)
                    {
                        m.faces[i].startID = currentID;
                        m.indices.insert(m.indices.end(), ires[i].begin(), ires[i].end());
                        barycentrics.insert(barycentrics.end(), bres[i].begin(), bres[i].end());
                        currentID += ires[i].size();
                    }
                    logDebug("Indices: %zu", m.indices.size());
                    meshes.push_back(group.mesh);
                }
                // Free temporary resources
                scalable_free(ps.v);
                scalable_free(ps.vt);
                scalable_free(ps.vn);
                for (size_t i = 0; i < ps.fsize; ++i)
                {
                    delete ps.f[i].value;
                    ps.f[i].~Line();
                }
                scalable_free(ps.f);
                for (size_t i = 0; i < ps.gsize; ++i) ps.g[i].~Line();
                scalable_free(ps.g);
                for (size_t i = 0; i < ps.useMtlsize; ++i) ps.useMtl[i].~Line();
                scalable_free(ps.useMtl);
                return meshes;
            }

            void parseMTL(const std::filesystem::path &basePath, const ParseIndexed &parsed,
                          DArray<Material> &materials)
            {
                std::filesystem::path parsedPath = parsed.mtllib;
                if (parsedPath.is_relative()) parsedPath = parsedPath.lexically_normal();

                std::filesystem::path filename = basePath;
                filename.replace_filename(parsedPath);
                std::ifstream fileStream(filename, std::ios::binary | std::ios::ate);
                if (!fileStream.is_open())
                {
                    logError("Could not open file: %s", filename.string().c_str());
                    return;
                }

                logInfo("Loading MTL file: %s", filename.string().c_str());
                std::ostringstream oss;
                oss << fileStream.rdbuf();
                std::string fileContent = oss.str();
                fileStream.close();
                StringPool<char> stringPool(fileContent.size());
                io::file::fillLineBuffer(oss.str().c_str(), fileContent.size(), stringPool);

                // Process each line from the buffer
                int materialIndex = -1;
                int lineIndex = 1;
                for (const auto &line : stringPool) parseMTLline(line, materials, materialIndex, lineIndex++);
            }

            bool processMTLColorOption(const char *&token, ColorOption &dst)
            {
                ColorOption colorOption{};
                while (isspace(*token)) ++token;
                if (!isdigit(*token))
                {
                    if (strncmp(token, "xyz", 3) == 0)
                    {
                        colorOption.type = ColorOption::Type::XYZ;
                        token += 3;
                    }
                    else
                        return false;
                }
                glm::vec3 color;
                if (strToV3(token, color))
                    colorOption.value = color;
                else
                    return false;
                dst = colorOption;
                return true;
            }

            bool processOnOffFlag(const char *&token, bool &dst)
            {
                while (isspace(*token)) ++token;
                if (strncmp(token, "on", 2) == 0)
                {
                    dst = true;
                    token += 2;
                }
                else if (strncmp(token, "off", 3) == 0)
                {
                    dst = false;
                    token += 3;
                }
                else
                    return false;
                return true;
            }

            bool processMTLTextureOption(const char *&token, TextureOption &dst)
            {
                bool pathProcessed = false;
                do {
                    while (isspace(*token)) ++token;

                    if (token[0] == '-')
                    {
                        ++token;

                        if (strncmp(token, "blendu", 6) == 0)
                        {
                            token += 7;
                            if (!processOnOffFlag(token, dst.blendu)) return false;
                        }
                        else if (strncmp(token, "blendv", 6) == 0)
                        {
                            token += 7;
                            if (!processOnOffFlag(token, dst.blendv)) return false;
                        }
                        else if (strncmp(token, "boost", 5) == 0)
                        {
                            token += 6;
                            if (!strToF(token, dst.boost)) return false;
                        }
                        else if (strncmp(token, "mm", 2) == 0)
                        {
                            token += 3;
                            if (!strToV2(token, dst.mm)) return false;
                        }
                        else if (token[0] == 'o')
                        {
                            token += 2;
                            strToV3Optional(token, dst.offset);
                        }
                        else if (token[0] == 's')
                        {
                            token += 2;
                            strToV3Optional(token, dst.scale);
                        }
                        else if (token[0] == 't')
                        {
                            if (isspace(token[1]))
                            {
                                token += 2;
                                strToV3Optional(token, dst.turbulence);
                            }
                            else if (strncmp(token, "texres", 6) == 0)
                            {
                                token += 7;
                                if (!strToI(token, dst.resolution)) return false;
                            }
                            else if (strncmp(token, "type", 4) == 0)
                            {
                                token += 5;
                                dst.type = getStrRange(token);
                            }
                            else
                                return false;
                        }
                        else if (strncmp(token, "clamp", 5) == 0)
                        {
                            token += 6;
                            if (!processOnOffFlag(token, dst.clamp)) return false;
                        }
                        else if (strncmp(token, "bm", 2) == 0)
                        {
                            token += 3;
                            if (!strToF(token, dst.bumpIntensity)) return false;
                        }
                        else if (strncmp(token, "imfchan", 7) == 0)
                        {
                            token += 8;
                            while (isspace(*token)) ++token;
                            dst.imfchan = token[0];
                            ++token;
                        }
                        else
                        {
                            logWarn("Unknown option: %s", getStrRange(token).c_str());
                            return false;
                        }
                    }
                    else
                    {
                        if (pathProcessed) break;
                        dst.path = getStrRange(token);
                        pathProcessed = true;
                    }
                } while (true);
                return true;
            }

            void parseMTLline(const std::string_view &line, DArray<Material> &materials, int &matIndex, int lineIndex)
            {
                try
                {
                    if (line.empty() || line[0] == '\0' || line[0] == '#') return;
                    const char *token = line.data();
                    assert(token);
                    if (strncmp(token, "newmtl", 6) == 0)
                    {
                        token += 7;
                        Material material(getStrRange(token));
                        materials.push_back(std::move(material));
                        ++matIndex;
                    }
                    else if (token[0] == 'K')
                    {
                        if (token[1] == 'a')
                        {
                            token += 3;
                            if (!processMTLColorOption(token, materials[matIndex].Ka))
                                throw ParseException(line, lineIndex);
                        }
                        else if (token[1] == 'd')
                        {
                            token += 3;
                            if (!processMTLColorOption(token, materials[matIndex].Kd))
                                throw ParseException(line, lineIndex);
                        }
                    }
                    else if (token[0] == 'N')
                    {
                        if (token[1] == 's')
                        {
                            token += 3;
                            float ns;
                            if (!strToF(token, ns)) throw ParseException(line, lineIndex);
                            materials[matIndex].Ns = ns;
                        }
                        else if (token[1] == 'i')
                        {
                            token += 3;
                            if (!strToF(token, materials[matIndex].Ni)) throw ParseException(line, lineIndex);
                        }
                    }
                    else if (strncmp(token, "illum", 5) == 0)
                    {
                        token += 6;
                        if (!strToI(token, materials[matIndex].illum)) throw ParseException(line, lineIndex);
                    }
                    else if (token[0] == 'd')
                    {
                        token += 2;
                        if (!strToF(token, materials[matIndex].d)) throw ParseException(line, lineIndex);
                    }
                    else if (token[0] == 'T')
                    {
                        if (token[1] == 'r')
                        {
                            token += 3;
                            if (!strToF(token, materials[matIndex].Tr)) throw ParseException(line, lineIndex);
                        }
                        else if (token[1] == 'f')
                        {
                            token += 3;
                            if (!processMTLColorOption(token, materials[matIndex].Tf))
                                throw ParseException(line, lineIndex);
                        }
                    }
                    else if (strncmp(token, "map_", 4) == 0)
                    {
                        if (token[4] == 'K')
                        {
                            if (token[5] == 'a')
                            {
                                token += 6;
                                TextureOption map_Ka;
                                if (processMTLTextureOption(token, map_Ka))
                                    materials[matIndex].map_Ka = std::move(map_Ka);
                                else
                                    throw ParseException(line, lineIndex);
                            }
                            else if (token[5] == 'd')
                            {
                                token += 6;
                                TextureOption map_Kd;
                                if (processMTLTextureOption(token, map_Kd))
                                    materials[matIndex].map_Kd = std::move(map_Kd);
                                else
                                    throw ParseException(line, lineIndex);
                            }
                            else if (token[5] == 's')
                            {
                                token += 6;
                                TextureOption map_Ks;
                                if (processMTLTextureOption(token, map_Ks))
                                    materials[matIndex].map_Ks = std::move(map_Ks);
                                else
                                    throw ParseException(line, lineIndex);
                            }
                            else if (token[5] == 'e')
                            {
                                token += 6;
                                TextureOption map_Ke;
                                if (processMTLTextureOption(token, map_Ke))
                                    materials[matIndex].map_Ke = std::move(map_Ke);
                                else
                                    throw ParseException(line, lineIndex);
                            }
                        }
                        else if (token[5] == 'N' && token[6] == 's')
                        {
                            token += 6;
                            TextureOption map_Ns;
                            if (processMTLTextureOption(token, map_Ns))
                                materials[matIndex].map_Ns = std::move(map_Ns);
                            else
                                throw ParseException(line, lineIndex);
                        }
                        else if (token[5] == 'd')
                        {
                            token += 6;
                            TextureOption map_d;
                            if (processMTLTextureOption(token, map_d))
                                materials[matIndex].map_d = std::move(map_d);
                            else
                                throw ParseException(line, lineIndex);
                        }
                        else if (token[5] == 'T' && token[6] == 'r')
                        {
                            token += 6;
                            TextureOption map_Tr;
                            if (processMTLTextureOption(token, map_Tr))
                                materials[matIndex].map_Tr = std::move(map_Tr);
                            else
                                throw ParseException(line, lineIndex);
                        }
                        else if (strncmp(token, "map_bump", 8) == 0)
                        {
                            token += 9;
                            TextureOption map_bump;
                            if (processMTLTextureOption(token, map_bump))
                                materials[matIndex].map_bump = std::move(map_bump);
                            else
                                throw ParseException(line, lineIndex);
                        }
                        /// PBR Workflow
                        else if (token[4] == 'P')
                        {
                            if (token[5] == 'r')
                            {
                                token += 6;
                                TextureOption map_Pr;
                                if (processMTLTextureOption(token, map_Pr))
                                    materials[matIndex].map_Pr = std::move(map_Pr);
                                else
                                    throw ParseException(line, lineIndex);
                            }
                            else if (token[5] == 'm')
                            {
                                token += 6;
                                TextureOption map_Pm;
                                if (processMTLTextureOption(token, map_Pm))
                                    materials[matIndex].map_Pm = std::move(map_Pm);
                                else
                                    throw ParseException(line, lineIndex);
                            }
                            else if (token[5] == 's')
                            {
                                token += 6;
                                TextureOption map_Ps;
                                if (processMTLTextureOption(token, map_Ps))
                                    materials[matIndex].map_Ps = std::move(map_Ps);
                                else
                                    throw ParseException(line, lineIndex);
                            }
                        }
                    }
                    else if (strncmp(token, "bump", 4) == 0)
                    {
                        token += 5;
                        TextureOption bump;
                        if (processMTLTextureOption(token, materials[matIndex].map_bump))
                            materials[matIndex].map_bump = std::move(bump);
                        else
                            throw ParseException(line, lineIndex);
                    }
                    else if (strncmp(token, "disp", 4) == 0)
                    {
                        token += 5;
                        TextureOption disp;
                        if (processMTLTextureOption(token, disp))
                            materials[matIndex].disp = std::move(disp);
                        else
                            throw ParseException(line, lineIndex);
                    }
                    else if (strncmp(token, "decal", 5) == 0)
                    {
                        token += 6;
                        TextureOption decal;
                        if (processMTLTextureOption(token, decal))
                            materials[matIndex].decal = std::move(decal);
                        else
                            throw ParseException(line, lineIndex);
                    }
                    /// PBR Workflow
                    else if (token[0] == 'P')
                    {
                        if (token[1] == 'r')
                        {
                            token += 3;
                            if (!strToF(token, materials[matIndex].Pr)) throw ParseException(line, lineIndex);
                        }
                        else if (token[1] == 'm')
                        {
                            token += 3;
                            if (!strToF(token, materials[matIndex].Pm)) throw ParseException(line, lineIndex);
                        }
                        else if (token[1] == 's')
                        {
                            token += 3;
                            if (!strToF(token, materials[matIndex].Ps)) throw ParseException(line, lineIndex);
                        }
                        else if (token[1] == 'c')
                        {
                            if (isspace(token[2]))
                            {
                                token += 3;
                                if (!strToF(token, materials[matIndex].Pc)) throw ParseException(line, lineIndex);
                            }
                            else if (token[2] == 'r')
                            {
                                token += 4;
                                if (!strToF(token, materials[matIndex].Pcr)) throw ParseException(line, lineIndex);
                            }
                        }
                    }
                    else if (strncmp(token, "aniso", 5) == 0)
                    {
                        if (isspace(token[5]))
                        {
                            token += 6;
                            if (!strToF(token, materials[matIndex].aniso)) throw ParseException(line, lineIndex);
                        }
                        else if (token[5] == 'r')
                        {
                            token += 7;
                            if (!strToF(token, materials[matIndex].anisor)) throw ParseException(line, lineIndex);
                        }
                    }
                    else if (strncmp(token, "norm", 4) == 0)
                    {
                        token += 5;
                        TextureOption norm;
                        if (processMTLTextureOption(token, norm))
                            materials[matIndex].norm = std::move(norm);
                        else
                            throw ParseException(line, lineIndex);
                    }
                }
                catch (const ParseException &e)
                {
                    logWarn("%s", e.what());
                }
                catch (const std::exception &e)
                {
                    logWarn("Failed to process line with error: %s", e.what());
                }
            }

            void convertToMaterials(std::filesystem::path basePath, const DArray<Material> &mtlMatList,
                                    DArray<assets::MaterialInfo> &materials, DArray<assets::Image2D> &textures)
            {
                emhash5::HashMap<std::string, size_t> texMap;
                for (auto &mtl : mtlMatList)
                {
                    materials.emplace_back();
                    auto &mat = materials.back();
                    if (mtl.map_Kd.path.empty())
                        mat.albedo.rgb = mtl.Kd.value;
                    else
                    {
                        std::filesystem::path parsedPath = mtl.map_Kd.path;
                        if (parsedPath.is_relative()) parsedPath = parsedPath.lexically_normal();
                        basePath.replace_filename(parsedPath);
                        auto [it, inserted] = texMap.insert({parsedPath.string(), textures.size()});
                        if (inserted)
                        {
                            auto importer = ecl::image::getImporterByPath(basePath);
                            DArray<assets::ImageInfo> images;
                            if (importer && importer->load(basePath, images) != io::file::ReadState::Success)
                            {
                                logError("Failed to load texture '%ls'", basePath.c_str());
                                continue;
                            }
                            textures.emplace_back(std::move(images.front()));
                        }
                        mat.albedo.textured = true;
                        mat.albedo.textureID = it->second;
                    }
                }
            }
            io::file::ReadState Importer::load()
            {
                auto start = std::chrono::high_resolution_clock::now();
                logInfo("Loading OBJ file: %s", _path.string().c_str());
                std::string header = f("%s %ls", _("Task:File:Load"), _path.filename().c_str());
                events::mng.dispatch<TaskUpdateEvent>("task:update", (void *)this, header, _("Task:File:Read"));
                ParseIndexed parsed;
                io::file::ReadState result = io::file::readByBlock(_path.string(), parsed, parseLine);
                if (result != io::file::ReadState::Success) return result;

                events::mng.dispatch<TaskUpdateEvent>("task:update", (void *)this, header, _("Task:File:Serialize"),
                                                      0.2f);
                logInfo("Serializing parse result");
                _meshes = serializeToGroups(parsed);
                events::mng.dispatch<TaskUpdateEvent>("task:update", (void *)this, header, _("Task:File:Load:Mat"),
                                                      0.8f);
                if (!parsed.mtllib.empty())
                {
                    DArray<Material> mtlMaterials;
                    parseMTL(_path, parsed, mtlMaterials);
                    convertToMaterials(_path, mtlMaterials, _materials, _textures);
                }
                auto end = std::chrono::high_resolution_clock::now();
                logInfo("Loaded in %.5f ms", std::chrono::duration<double, std::milli>(end - start).count());

                return io::file::ReadState::Success;
            }
        } // namespace obj
    } // namespace scene
} // namespace ecl