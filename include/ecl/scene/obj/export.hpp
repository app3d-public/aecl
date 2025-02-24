#pragma once

#include <astl/hash.hpp>
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

            using ObjExportFlags = astl::flags<ObjExportFlagBits>;

            struct MaterialRef
            {
                astl::shared_ptr<assets::MaterialInfo> info;
                astl::shared_ptr<assets::Material> mat;
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
                Exporter(const std::filesystem::path &path) : IExporter(path) {}

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

                void writeVertices(assets::mesh::Model &model, const astl::vector<assets::mesh::VertexGroup> &groups,
                                   std::stringstream &ss);
                void writeFaces(assets::mesh::MeshBlock *meta, std::ostream &os, const astl::vector<u32> &faces);
                void writeTriangles(assets::mesh::MeshBlock *meta, std::ostream &os, const astl::vector<u32> &faces,
                                    const astl::vector<assets::mesh::VertexGroup> &groups);
                void writeTexture2D(std::ostream &os, const std::string &token, const assets::Target::Addr &tex);

                void writeMaterial(const astl::shared_ptr<assets::MaterialInfo> &matInfo,
                                   const astl::shared_ptr<assets::Material> &mat, std::ostream &os);
                void writeMtlLibInfo(std::ofstream &mtlStream, std::stringstream &objStream);
                void writeMtl(std::ofstream &stream);
                void writeObject(const assets::Object &object, std::stringstream &objStream);
            };
        } // namespace obj
    } // namespace scene
} // namespace ecl