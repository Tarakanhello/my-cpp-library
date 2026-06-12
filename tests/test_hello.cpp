#include <catch2/catch_all.hpp>

#include "mylib/mylib.h"


TEST_CASE("helloFromMyLib prints nothing unexpected", "hello")
{
    REQUIRE_NOTHROW(helloFromMyLib());
}
