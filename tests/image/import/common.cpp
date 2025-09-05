

#include <acul/io/path.hpp>
#include <acul/log.hpp>
#include <aecl/image/import.hpp>

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