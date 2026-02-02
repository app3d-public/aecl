#pragma once

#include <acul/op_result.hpp>
#include <umbf/umbf.hpp>

namespace aecl
{
    namespace scene
    {
        class ILoader
        {
        public:
            /**
             * @brief Create a scene importer
             * @param filename Name of the file
             */
            ILoader(const acul::string &filename) : _path(filename) {}
            virtual ~ILoader() = default;

            // Load raw source file
            virtual acul::op_result read_source() = 0;

            // Load the scene includes all intermediate calls
            acul::op_result load()
            {
                auto state = read_source();
                if (!state.success()) return state;
                build_geometry();
                load_materials();
                return acul::make_op_success();
            }
            // Indexing geometry to UMBF format
            virtual void build_geometry() = 0;

            // Loadd materials & textures
            virtual acul::op_result load_materials() = 0;

            // Get the filename path
            const acul::string path() const { return _path; }

            // Get the list of imported meshes
            acul::vector<umbf::Object> &objects() { return _objects; }

            // Get the list of imported materials
            acul::vector<acul::shared_ptr<umbf::File>> &materials() { return _materials; }

            // Get the list of imported textures
            acul::vector<acul::shared_ptr<umbf::Target>> &textures() { return _textures; }

            inline void clear()
            {
                _objects.clear();
                _materials.clear();
                _textures.clear();
            }

            const acul::string &error() const { return _error; }

        protected:
            acul::string _path, _error;
            acul::vector<umbf::Object> _objects;
            acul::vector<acul::shared_ptr<umbf::File>> _materials;
            acul::vector<acul::shared_ptr<umbf::Target>> _textures;
        };
    } // namespace scene
} // namespace aecl