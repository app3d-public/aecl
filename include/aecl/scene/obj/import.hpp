#pragma once

#include <oneapi/tbb/concurrent_vector.h>
#include "../import.hpp"

namespace aecl::scene::obj
{
    /**
     * @brief Load the scene
     * @return Read state result
     **/
    class APPLIB_API Importer : public ILoader
    {
    public:
        Importer(const acul::string &filename) : ILoader(filename) {};

        ~Importer();

        virtual acul::op_result read_source() override;

        virtual void build_geometry() override;

        virtual acul::op_result load_materials() override;

    private:
        struct ImportCtx *_ctx;
    };
} // namespace aecl::scene::obj