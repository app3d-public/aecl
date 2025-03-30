#pragma once

#include <acul/event.hpp>
#include <acul/io/file.hpp>
#include <assets/asset.hpp>

namespace ecl
{
    namespace scene
    {
        class ParseException : public acul::exception
        {
        public:
            ParseException(const acul::string_view &line, size_t lineIndex) : _line(line), _lineIndex(lineIndex)
            {
                _msg = "Failed to parse line: \"" + acul::string(line.begin(), line.end()) + "\" at line " +
                       acul::to_string(_lineIndex);
            }

            const char *what() const noexcept override { return _msg.c_str(); }

        private:
            acul::string _line;
            size_t _lineIndex;
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

            /**
             * @brief Load the scene
             * @return Read state result
             **/
            virtual acul::io::file::op_state load(events::Manager &e) = 0;

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
} // namespace ecl