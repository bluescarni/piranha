#include <cstdlib>

int main()
{
    typedef decltype(::posix_memalign) f_type;
    return 0;
}
