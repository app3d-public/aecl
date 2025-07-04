#include <aecl/scene/import.hpp>

void test_parse_exception()
{
    using namespace aecl::scene;
    acul::string_view inputLine = "some invalid line";
    size_t lineIndex = 42;

    ParseException ex(inputLine, lineIndex);

    acul::string expectedMessage = "Failed to parse line: \"some invalid line\" at line 42";
    assert(acul::string(ex.what()) == expectedMessage);
}
