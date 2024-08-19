#pragma once

#include <core/hash.hpp>
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
                /**
                 * Constructs an Exporter object with the given parameters.
                 *
                 * @param path The path to the output file.
                 * @param meshFlags Mesh export flags.
                 * @param materialFlags Material export flags.
                 * @param objFlags Wavefront OBJ specific export flags.
                 * @param texFolder The path to the texture folder. (Used if textures are exported as copies)
                 */
                Exporter(const std::filesystem::path &path, MeshExportFlags meshFlags,
                         MaterialExportFlags materialFlags, ObjExportFlags objFlags)
                    : IExporter(path, meshFlags, materialFlags), _objFlags(objFlags)
                {
                }

                /**
                 * Saves the exported scene to the output file.
                 *
                 * @return `true` if the export was successful, `false` otherwise.
                 */
                bool save() override;

            private:
                ObjExportFlags _objFlags;
                emhash5::HashMap<glm::vec2, u32> _vtMap;
                emhash5::HashMap<glm::vec3, u32> _vnMap;
                int _genID = 0;
                bool _allMaterialsExist = true;

                void writeVertices(assets::meta::mesh::Model &model, std::stringstream &ss);
                void writeFaces(assets::meta::mesh::MeshBlock *meta, std::ostream &os, const DArray<u32> &faces);
                void writeTriangles(assets::meta::mesh::MeshBlock *meta, std::ostream &os, const DArray<u32> &faces);
                void writeTexture2D(std::ostream &os, const std::string &token, const TextureNode &tex);
                void writeMaterial(const std::shared_ptr<assets::Material> &mat, std::ostream &os);
                void writeMtlLibInfo(std::ofstream &mtlStream, std::stringstream &objStream,
                                     DArray<std::shared_ptr<assets::meta::MaterialBlock>> &metaInfo);
                void writeMtl(std::ofstream &stream);
                void writeObject(const std::shared_ptr<assets::Object>& object, const DArray<std::shared_ptr<assets::meta::MaterialBlock>>& matMeta, std::stringstream &objStream);
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