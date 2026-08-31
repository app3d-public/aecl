#pragma once

#include <acul/op_result.hpp>
#include "../asset.hpp"

namespace aecl::scene
{

    class ILoader
    {
    public:
        ILoader(const acul::string &filename) : _path(filename) {}
        virtual ~ILoader() = default;

        virtual acul::op_result read_source() = 0;

        acul::op_result load()
        {
            auto state = read_source();
            if (!state.success()) return state;
            build_geometry();
            return load_materials();
        }

        virtual void build_geometry() = 0;
        virtual acul::op_result load_materials() = 0;

        const acul::string path() const { return _path; }

        // Get the list of imported objects
        acul::vector<Asset> &objects() { return _objects; }

        // Get the list of imported materials
        acul::vector<Asset> &materials() { return _materials; }

        // Get the list of imported textures
        acul::vector<Asset> &images() { return _images; }

        inline void clear()
        {
            _objects.clear();
            _materials.clear();
            _images.clear();
        }
        const acul::string &error() const { return _error; }

    protected:
        acul::string _path, _error;
        acul::vector<Asset> _objects;
        acul::vector<Asset> _images;
        acul::vector<Asset> _materials;
    };
} // namespace aecl::scene
