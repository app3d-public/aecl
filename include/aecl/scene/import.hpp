#pragma once

#include <acul/io/file.hpp>
#include <umbf/umbf.hpp>

namespace aecl
{
    namespace scene
    {
        class ParseException : public acul::exception
        {
        public:
            ParseException(const acul::string_view &line, size_t line_index) : _line(line), _line_index(line_index)
            {
                _msg = "Failed to parse line: \"" + acul::string(line.begin(), line.end()) + "\" at line " +
                       acul::to_string(_line_index);
            }

            const char *what() const noexcept override { return _msg.c_str(); }

        private:
            acul::string _line;
            size_t _line_index;
            acul::string _msg; // Here we store the error message
        };

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
            virtual acul::io::file::op_state read_source() = 0;

            // Load the scene includes all intermediate calls
            bool load()
            {
                if (read_source() != acul::io::file::op_state::success) return false;
                build_geometry();
                load_materials();
                return true;
            }
            // Indexing geometry to UMBF format
            virtual void build_geometry() = 0;

            // Loadd materials & textures
            virtual void load_materials() = 0;

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

        protected:
            acul::string _path;
            acul::vector<umbf::Object> _objects;
            acul::vector<acul::shared_ptr<umbf::File>> _materials;
            acul::vector<acul::shared_ptr<umbf::Target>> _textures;
        };
    } // namespace scene
} // namespace aecl