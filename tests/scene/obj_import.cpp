#include <aecl/scene/obj/import.hpp>
#include "../env.hpp"

void test_obj_import()
{
    test_environment env;
    create_test_environment(env);
    aecl::scene::obj::Importer importer(acul::io::path(env.data_dir) / "cube.obj");
    assert(!importer.path().empty());
    auto state = importer.load();
    importer.clear();
    assert(state == acul::io::file::op_state::success);
}
