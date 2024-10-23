#pragma once

#include <assets/asset.hpp>
#include <core/event.hpp>
#include <core/io/file.hpp>
#include <exception>
#include <string>

namespace ecl
{
    namespace scene
    {
        class ParseException : public std::exception
        {
        public:
            ParseException(const std::string_view &line, size_t lineIndex) : _line(line), _lineIndex(lineIndex)
            {
                _msg = "Failed to parse line: \"" + std::string(line.begin(), line.end()) + "\" at line " +
                       std::to_string(_lineIndex);
            }

            const char *what() const noexcept override { return _msg.c_str(); }

        private:
            std::string _line;
            size_t _lineIndex;
            std::string _msg; // Here we store the error message
        };

        class ILoader
        {
        public:
            /**
             * @brief Create a scene importer
             * @param filename Name of the file
             */
            ILoader(const std::filesystem::path &filename) : _path(filename) {}
            virtual ~ILoader() = default;

            /**
             * @brief Load the scene
             * @return Read state result
             **/
            virtual io::file::ReadState load(events::Manager &e) = 0;

            // Get the filename path
            const std::filesystem::path path() const { return _path.string(); }

            // Get the list of imported meshes
            astl::vector<assets::Object> &objects() { return _objects; }

            // Get the list of imported materials
            astl::vector<astl::shared_ptr<assets::Asset>> &materials() { return _materials; }

            // Get the list of imported textures
            astl::vector<astl::shared_ptr<assets::Target>> &textures() { return _textures; }

            inline void clear()
            {
                _objects.clear();
                _materials.clear();
                _textures.clear();
            }

        protected:
            std::filesystem::path _path;
            astl::vector<assets::Object> _objects;
            astl::vector<astl::shared_ptr<assets::Asset>> _materials;
            astl::vector<astl::shared_ptr<assets::Target>> _textures;
        };
    } // namespace scene
} // namespace ecl