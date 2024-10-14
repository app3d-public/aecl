#pragma once

#include <core/std/hash.hpp>
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
            enum class ObjExportFlagBits
            {
                // Mesh Group Policy
                mgp_default = 0x0, // Exporting all object in 'default' group (without grouping)
                mgp_groups = 0x1,  // Exporting objects with 'g' flag
                mgp_objects = 0x2, // Exporting objects with 'o' flag
                mat_PBR = 0x4      // Using PBR materials workflow
            };

            using ObjExportFlags = Flags<ObjExportFlagBits>;

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
                bool _allMaterialsExist = true;

                void writeVertices(assets::mesh::Model &model, std::stringstream &ss);
                void writeFaces(assets::mesh::MeshBlock *meta, std::ostream &os, const astl::vector<u32> &faces);
                void writeTriangles(assets::mesh::MeshBlock *meta, std::ostream &os, const astl::vector<u32> &faces);
                void writeTexture2D(std::ostream &os, const std::string &token, const assets::Target::Addr &tex);
                void writeMaterial(const std::shared_ptr<assets::MaterialInfo> &matInfo,
                                   const std::shared_ptr<assets::Material> &mat, std::ostream &os);
                void writeMtlLibInfo(std::ofstream &mtlStream, std::stringstream &objStream,
                                     astl::vector<std::shared_ptr<assets::MaterialInfo>> &metaInfo,
                                     astl::vector<std::shared_ptr<assets::Material>> &materials);
                void writeMtl(std::ofstream &stream, const astl::vector<std::shared_ptr<assets::MaterialInfo>> &matInfo,
                              const astl::vector<std::shared_ptr<assets::Material>> &materials);
                void writeObject(const assets::Object &object,
                                 const astl::vector<std::shared_ptr<assets::MaterialInfo>> &matInfo,
                                 std::stringstream &objStream);
            };
        } // namespace obj
    } // namespace scene
} // namespace ecl

template <>
struct FlagTraits<ecl::scene::obj::ObjExportFlagBits>
{
    static constexpr bool isBitmask = true;
    static constexpr ecl::scene::obj::ObjExportFlags allFlags =
        ecl::scene::obj::ObjExportFlagBits::mgp_default | ecl::scene::obj::ObjExportFlagBits::mgp_groups |
        ecl::scene::obj::ObjExportFlagBits::mgp_objects | ecl::scene::obj::ObjExportFlagBits::mat_PBR;
};