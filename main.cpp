#include <boost/timer/timer.hpp>

#include "src/piranha.hpp"

#include "new_integer.hpp"

namespace piranha
{

template <typename Cf,typename Key>
inline polynomial<Cf,Key> fateman1()
{
	typedef polynomial<Cf,Key> p_type;
	p_type x("x"), y("y"), z("z"), t("t");
	auto f = x + y + z + t + 1;
	auto tmp(f);
	for (auto i = 1; i < 20; ++i) {
		f *= tmp;
	}
	{
	boost::timer::auto_cpu_timer t;
	return f * (f + 1);
	}
}

template <typename Cf,typename Key>
inline polynomial<Cf,Key> fateman2()
{
        typedef polynomial<Cf,Key> p_type;
        p_type x("x"), y("y"), z("z"), t("t");
        auto f = x + y + z + t + 1;
        auto tmp(f);
        for (auto i = 1; i < 30; ++i) {
                f *= tmp;
        }
        {
        boost::timer::auto_cpu_timer t;
        return f * (f + 1);
        }
}

template <typename Cf,typename Key>
inline polynomial<Cf,Key> pearce1()
{
	typedef polynomial<Cf,Key> p_type;
	p_type x("x"), y("y"), z("z"), t("t"), u("u");

	auto f = (x + y + z*z*2 + t*t*t*3 + u*u*u*u*u*5 + 1);
	auto tmp_f(f);
	auto g = (u + t + z*z*2 + y*y*y*3 + x*x*x*x*x*5 + 1);
	auto tmp_g(g);
	for (int i = 1; i < 12; ++i) {
		f *= tmp_f;
		g *= tmp_g;
	}
	{
	boost::timer::auto_cpu_timer t;
	return f * g;
	}
}

template <typename Cf,typename Key>
inline polynomial<Cf,Key> pearce2()
{
	typedef polynomial<Cf,Key> p_type;
	p_type x("x"), y("y"), z("z"), t("t"), u("u");

	auto f = (x + y + z*z*2 + t*t*t*3 + u*u*u*u*u*5 + 1);
	auto tmp_f(f);
	auto g = (u + t + z*z*2 + y*y*y*3 + x*x*x*x*x*5 + 1);
	auto tmp_g(g);
	for (int i = 1; i < 16; ++i) {
		f *= tmp_f;
		g *= tmp_g;
	}
	{
	boost::timer::auto_cpu_timer t;
	return f * g;
	}
}

namespace math
{
template <typename T>
struct is_zero_impl<T,typename std::enable_if<std::is_same<T,new_integer>::value>::type>
{
	bool operator()(const T &n) const
	{
		return n.is_zero();
	}
};

template <typename T>
struct negate_impl<T,typename std::enable_if<std::is_same<T,new_integer>::value>::type>
{
	void operator()(new_integer &n) const
	{
		n.negate();
	}
};

template <typename T>
struct multiply_accumulate_impl<T,T,T,typename std::enable_if<std::is_same<T,new_integer>::value>::type>
{
	auto operator()(T &x, const T &y, const T &z) const -> decltype(x.multiply_accumulate(y,z))
	{
		return x.multiply_accumulate(y,z);
	}
};

}}

using namespace piranha;

int main()
{
	environment env;
	settings::set_n_threads(4);
	std::cout << pearce2<new_integer,kronecker_monomial<>>().size() << '\n';

	/*polynomial<double,unsigned> p{"x"}, q{1}, mq{-1}, l{"y"};
	std::cout << p << '\n';
	std::cout << q << '\n';
	std::cout << mq << '\n';
	std::cout << (p + q) * (p + q) * (p + q) * (p + q) * (p + q) * (p + q) << '\n';
	polynomial<polynomial<integer,int>,int> pp{"y"};
	std::cout << (p + pp + 1) << '\n';
	std::cout << ((-p + 1) * pp - pp) << '\n';
	std::cout << (p * pp) << '\n';
	std::cout << (-polynomial<double,unsigned>{"x"} + 1) << '\n';*/
}
