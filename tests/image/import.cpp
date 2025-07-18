#include <aecl/image/import.hpp>
#include "../env.hpp"

using namespace aecl;
using namespace aecl::image;

void import_test_image(const acul::io::path &p)
{
    LOG_INFO("Loading %s", p.str().c_str());
    auto loader = aecl::image::get_importer_by_path(p);
    assert(loader);
    acul::vector<umbf::Image2D> images;
    auto state = loader->load(p, images);
    assert(state == acul::io::file::op_state::success);
    assert(!images.empty());
    acul::release(loader);
}

void test_image_import()
{
    test_environment env;
    create_test_environment(env);
    umbf::streams::HashResolver meta_resolver;
    meta_resolver.streams = {
        {umbf::sign_block::image, &umbf::streams::image},
    };
    umbf::streams::resolver = &meta_resolver;

    acul::io::path p = env.data_dir;
    import_test_image(p / "image.bmp");
    import_test_image(p / "image.gif");
    import_test_image(p / "image.hdr");
    import_test_image(p / "image.heif");
    import_test_image(p / "image.jpg");
    import_test_image(p / "image.exr");
    import_test_image(p / "image.png");
    import_test_image(p / "image.ppm");
    import_test_image(p / "image.tga");
    import_test_image(p / "image.tiff");
    import_test_image(p / "image.webp");
    import_test_image(p / "image.umia");
}
