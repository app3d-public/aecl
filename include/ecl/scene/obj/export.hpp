#pragma once

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
            class Exporter final : public IExporter
            {
            public:
                /**
                 * Constructs an Exporter object with the given parameters.
                 *
                 * @param path The path to the output file.
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
                oneapi::tbb::concurrent_unordered_set<glm::vec3> _vSet;
                oneapi::tbb::concurrent_unordered_set<glm::vec2> _vtSet;
                oneapi::tbb::concurrent_unordered_set<glm::vec3> _vnSet;
                emhash5::HashMap<glm::vec3, int> _vMap;
                emhash5::HashMap<glm::vec2, int> _vtMap;
                emhash5::HashMap<glm::vec3, int> _vnMap;

                void writeVertices(const DArray<assets::mesh::Vertex> &vertices, std::stringstream &ss);
                void writeFaces(assets::mesh::MeshBlock *meta, std::ostream &os);
                void writeTriangles(assets::mesh::MeshBlock *meta, std::ostream &os);

                // void transformVertexPos(glm::vec3 &pos);
                // std::string getGroupByStrategy(const std::string &name) const;
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