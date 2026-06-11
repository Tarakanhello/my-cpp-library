#include <iostream>

#include "mylib/mylib.h"

class Foo
{
    friend std::ostream& operator<<(std::ostream& out,
                                    [[maybe_unused]] const Foo& foo)
    {
        out << "print class Foo";
        return out;
    }
};

int main()
{
    std::cout << "Testing my library:\n";
    helloFromMyLib();

    int a{ 5 };
    double b{ 6.3 };

    SHOW_DEBUG_LOG_INFO(a + b);
    SHOW_DEBUG_LOG_INFO_MSG(a + b, "use sum of a and b");

    Foo foo{};

    SHOW_DEBUG_LOG_INFO(foo);

    std::string str1{ "First" };
    std::string str2{ " Second" };

    SHOW_DEBUG_LOG_INFO(str1 + str2);

    return 0;
}
