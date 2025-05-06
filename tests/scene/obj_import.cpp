#include <ecl/scene/obj/import.hpp>
#include "../env.hpp"

void test_obj_import()
{
    test_environment env;
    create_test_environment(env);
    ecl::scene::obj::Importer importer(acul::io::path(env.data_dir) / "cube.obj");
    assert(!importer.path().empty());
    auto state = importer.load();
    importer.clear();
    assert(state == acul::io::file::op_state::Success);
    printf("o: %zu, m: %zu, t: %zu", importer.objects().size(), importer.materials().size(),
           importer.textures().size());
}
