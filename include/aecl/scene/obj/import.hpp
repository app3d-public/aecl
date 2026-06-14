#pragma once

#include <aecl/symbol_export.h>
#include <oneapi/tbb/concurrent_vector.h>
#include "../import.hpp"


namespace aecl::scene::obj
{
    /**
     * @brief Load the scene
     * @return Read state result
     **/
    class Importer : public ILoader
    {
    public:
        Importer(const acul::string &filename) : ILoader(filename) {};

        AECL_EXPORT ~Importer();
        AECL_EXPORT virtual acul::op_result read_source() override;
        AECL_EXPORT virtual void build_geometry() override;
        AECL_EXPORT virtual acul::op_result load_materials() override;

    private:
        struct ImportCtx *_ctx;
    };
} // namespace aecl::scene::obj