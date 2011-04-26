#include "src/piranha.hpp"

using namespace piranha;

template <typename Cf, typename Expo>
class polynomial:
	public top_level_series<polynomial_term<Cf,Expo>,polynomial<Cf,Expo>>
{
		typedef top_level_series<polynomial_term<Cf,Expo>,polynomial<Cf,Expo>> base;
	public:
		polynomial() = default;
		polynomial(const polynomial &) = default;
		polynomial(polynomial &&) = default;
		explicit polynomial(const std::string &name):base()
		{
			typedef typename base::term_type term_type;
			// Insert the symbol.
			this->m_ed.template add_symbol<term_type>(symbol(name));
			// Construct and insert the term.
			this->insert(term_type(Cf(1,this->m_ed),typename term_type::key_type{Expo(1)}),this->m_ed);
		}
		~polynomial() = default;
		polynomial &operator=(const polynomial &) = default;
		polynomial &operator=(polynomial &&other) piranha_noexcept_spec(true)
		{
			base::operator=(std::move(other));
			return *this;
		}
		template <typename T>
		polynomial &operator=(T &&x)
		{
			base::operator=(std::forward<T>(x));
			return *this;
		}
};

template <typename Base>
class base_complex: public Base
{
	public:
		base_complex() = default;
		base_complex(const base_complex &) = default;
		base_complex(base_complex &&) = default;
		~base_complex() = default;
		base_complex &operator=(const base_complex &) = default;
		base_complex &operator=(base_complex &&) = default;
		template <typename T>
		base_complex &operator=(T &&x)
		{
std::cout << "complex assign!!!\n";
			Base::operator=(std::forward<T>(x));
			return *this;
		}
	protected:
		template <typename Series1, typename Series2>
		explicit base_complex(Series1 &&s1, Series2 &&s2, const echelon_descriptor<typename Base::term_type> &ed,
			typename std::enable_if<std::is_base_of<detail::base_series_tag,typename strip_cv_ref<Series1>::type>::value &&
			std::is_base_of<detail::base_series_tag,typename strip_cv_ref<Series2>::type>::value>::type * = piranha_nullptr)
		{
std::cout << "LOLLER called!!!\n";
		}
};

template <typename Cf, typename Expo>
class polynomial2:
	public base_complex<top_level_series<polynomial_term<Cf,Expo>,polynomial2<Cf,Expo>>>
{
		typedef base_complex<top_level_series<polynomial_term<Cf,Expo>,polynomial2<Cf,Expo>>> base;
	public:
		polynomial2() = default;
		polynomial2(const polynomial2 &) = default;
		polynomial2(polynomial2 &&) = default;
		explicit polynomial2(const std::string &name):base()
		{
			typedef typename base::term_type term_type;
			// Insert the symbol.
			this->m_ed.template add_symbol<term_type>(symbol(name));
			// Construct and insert the term.
			this->insert(term_type(Cf(1,this->m_ed),typename term_type::key_type{Expo(1)}),this->m_ed);
		}
		explicit polynomial2(const char *name):base()
		{
			typedef typename base::term_type term_type;
			// Insert the symbol.
			this->m_ed.template add_symbol<term_type>(symbol(name));
			// Construct and insert the term.
			this->insert(term_type(Cf(1,this->m_ed),typename term_type::key_type{Expo(1)}),this->m_ed);
		}
		template <typename... Args>
		explicit polynomial2(Args && ... params):base(std::forward<Args>(params)...) {}
		~polynomial2() = default;
		polynomial2 &operator=(const polynomial2 &) = default;
		polynomial2 &operator=(polynomial2 &&other) piranha_noexcept_spec(true)
		{
			base::operator=(std::move(other));
			return *this;
		}
		template <typename T>
		polynomial2 &operator=(T &&x)
		{
std::cout << "poly2 assign\n";
			base::operator=(std::forward<T>(x));
			return *this;
		}
};

struct foo
{
	foo() = default;
	foo(foo &&) = delete;
	foo(const foo &) = default;
};

int main()
{
// 	std::cout << is_nothrow_move_assignable<int &>::value << '\n';
	typedef polynomial2<numerical_coefficient<integer>,int> p2_type;

	p2_type p, q;
	p = 5;
	p = q;

	std::cout << (p == 0) << '\n';
	std::cout << (0 == p) << '\n';
	
	p = 5;

	std::cout << (p == 5) << '\n';
	std::cout << (5 == p) << '\n';
	std::cout << (q == p) << '\n';
// 	foo f = foo{};
	return 0;
	
	
	
	polynomial<numerical_coefficient<double>,int> x("x");
	std::cout << x << '\n';
	x += 4;
	std::cout << x << '\n';
	x += integer(5);
	std::cout << x << '\n';
	x += x;
	polynomial2<numerical_coefficient<integer>,int> x2("x");
	x += std::move(x2);
	polynomial2<numerical_coefficient<integer>,int> x3("x");
	x += std::move(x3);

	polynomial2<numerical_coefficient<integer>,int> y("y");
	x += y;
 	x + y;
	std::cout << x << "\n-------------\n";
	std::cout << x + y << "\n-------------\n";
// 	2 + x;
// 	std::cout << x << "\n-------------\n";
	3 + polynomial2<numerical_coefficient<std::complex<double>>,int>{};
// 	p2_type foo, bar(foo);
// 	p2_type frob{foo,bar,echelon_descriptor<p2_type::term_type>{}};
}
