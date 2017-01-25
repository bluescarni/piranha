#include <piranha/piranha.hpp>

using namespace piranha;

int main()
{
    using p_type = polynomial<double, monomial<int>>;
    p_type x{"x"};
    save_file(x, "tmp.mpackp.bz2");
}
