#pragma once

#include <acul/hash/hashmap.hpp>
#include <emhash/hash_table5.hpp>
#include <oneapi/tbb/concurrent_unordered_set.h>
#include "../export.hpp"

namespace ecl
{
    namespace scene
    {
        namespace obj
        {
            // Policy for exporting objects
            struct ObjExportFlagBits
            {
                enum enum_type
                {
                    // Mesh Group Policy
                    mgp_default = 0x0, // Exporting all object in 'default' group (without grouping)
                    mgp_groups = 0x1,  // Exporting objects with 'g' flag
                    mgp_objects = 0x2, // Exporting objects with 'o' flag
                    mat_PBR = 0x4      // Using PBR materials workflow
                };
                using flag_bitmask = std::true_type;
            };

            using ObjExportFlags = acul::flags<ObjExportFlagBits>;

            struct MaterialRef
            {
                acul::shared_ptr<umbf::MaterialInfo> info;
                acul::shared_ptr<umbf::Material> mat;
            };

            // OBJ file exporter
            class APPLIB_API Exporter final : public IExporter
            {
            public:
                ObjExportFlags objFlags;
                /**
                 * Constructs an Exporter object with the given parameters.
                 *
                 * @param path The path to the output file.
                 */
                Exporter(const acul::string &path) : IExporter(path) {}

                /**
                 * Saves the exported scene to the output file.
                 *
                 * @return `true` if the export was successful, `false` otherwise.
                 */
                bool save() override;

            private:
                emhash5::HashMap<glm::vec2, u32> _vtMap;
                emhash5::HashMap<glm::vec3, u32> _vnMap;
                emhash5::HashMap<u64, MaterialRef> _matMap;
                bool _allMaterialsExist = true;

                void writeVertices(umbf::mesh::Model &model, const acul::vector<umbf::mesh::VertexGroup> &groups,
                                   std::stringstream &ss);
                void writeFaces(umbf::mesh::MeshBlock *meta, std::ostream &os, const acul::vector<u32> &faces);
                void writeTriangles(umbf::mesh::MeshBlock *meta, std::ostream &os, const acul::vector<u32> &faces,
                                    const acul::vector<umbf::mesh::VertexGroup> &groups);
                void writeTexture2D(std::ostream &os, const std::string &token, const acul::string &tex);

                void writeMaterial(const acul::shared_ptr<umbf::MaterialInfo> &matInfo,
                                   const acul::shared_ptr<umbf::Material> &mat, std::ostream &os);
                void writeMtlLibInfo(std::ofstream &mtlStream, std::stringstream &objStream);
                void writeMtl(std::ofstream &stream);
                void writeObject(const umbf::Object &object, std::stringstream &objStream);
            };
        } // namespace obj
    } // namespace scene
} // namespace ecl