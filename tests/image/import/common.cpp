

#include <aecl/image/import.hpp>

void import_test_image(const acul::path &p)
{
    auto loader = aecl::image::get_importer_by_path(p);
    assert(loader);
    acul::vector<umbf::Image2D> images;
    assert(loader->load(p, images));
    assert(!images.empty());
    acul::release(loader);
}