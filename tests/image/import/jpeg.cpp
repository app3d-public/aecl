#include <aecl/image/import.hpp>
#include "../../env.hpp"

using namespace aecl;
using namespace aecl::image;

void import_test_image(const acul::io::path &p);

void test_image_import_jpeg()
{
    test_environment env;
    create_test_environment(env);
    umbf::streams::HashResolver meta_resolver;
    meta_resolver.streams = {
        {umbf::sign_block::image, &umbf::streams::image},
    };
    umbf::streams::resolver = &meta_resolver;
    acul::io::path p = env.data_dir;
    import_test_image(p / "image.jpg");
}
