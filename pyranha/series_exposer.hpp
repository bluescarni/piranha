template <template <typename...> class Series, typename CfTypes, typename InteropTypes>
struct series_exposer
{
	// String constructor.
	template <typename T>
	static void expose_string_ctor(bp::class_<T> &cl,
		typename std::enable_if<std::is_constructible<T,std::string>::value>::type * = piranha_nullptr)
	{
		cl.def(bp::init<const std::string &>());
	}
	template <typename T>
	static void expose_string_ctor(bp::class_<T> &,
		typename std::enable_if<!std::is_constructible<T,std::string>::value>::type * = piranha_nullptr)
	{}
	// Exponentiation support.
	template <typename T, typename U>
	static void pow_exposer(bp::class_<T> &series_class, const U &,
		typename std::enable_if<is_exponentiable<T,U>::value>::type * = piranha_nullptr)
	{
		series_class.def("__pow__",&T::template pow<U>);
	}
	template <typename T, typename U>
	static void pow_exposer(bp::class_<T> &, const U &,
		typename std::enable_if<!is_exponentiable<T,U>::value>::type * = piranha_nullptr)
	{}
	// Evaluation wrapper.
	template <typename S, typename T>
	static decltype(std::declval<S>().evaluate(std::declval<std::unordered_map<std::string,T>>()))
		wrap_evaluate(const S &s, bp::dict dict, const T &)
	{
		std::unordered_map<std::string,T> cpp_dict;
		bp::stl_input_iterator<std::string> it(dict), end;
		for (; it != end; ++it) {
			cpp_dict[*it] = bp::extract<T>(dict[*it])();
		}
		return s.evaluate(cpp_dict);
	}
	// Interaction with interoperable types.
	template <typename S, std::size_t I = 0u, typename... T>
	void interop_exposer(bp::class_<S> &, const std::tuple<T...> &,
		typename std::enable_if<I == sizeof...(T)>::type * = piranha_nullptr) const
	{}
	template <typename S, std::size_t I = 0u, typename... T>
	void interop_exposer(bp::class_<S> &series_class, const std::tuple<T...> &t,
		typename std::enable_if<(I < sizeof...(T))>::type * = piranha_nullptr) const
	{
		typedef typename std::tuple_element<I,std::tuple<T...>>::type interop_type;
		interop_type in;
		// Constrcutor from interoperable.
		series_class.def(bp::init<const interop_type &>());
		// NOTE: to resolve ambiguities when we interop with other series types
		// we can try using the namespace-qualified operators from Boost.Python.
		// Arithmetic and comparison with interoperable type.
		// NOTE: if we fix is_addable type traits for series the above is not needed any more,
		// as series + bp::self is not available any more.
		series_class.def(bp::self += in);
		series_class.def(bp::self + in);
		series_class.def(in + bp::self);
		series_class.def(bp::self -= in);
		series_class.def(bp::self - in);
		series_class.def(in - bp::self);
		series_class.def(bp::self *= in);
		series_class.def(bp::self * in);
		series_class.def(in * bp::self);
		series_class.def(bp::self == in);
		series_class.def(in == bp::self);
		series_class.def(bp::self != in);
		series_class.def(in != bp::self);
		series_class.def(bp::self /= in);
		series_class.def(bp::self / in);
		// Exponentiation.
		pow_exposer(series_class,in);
		// Evaluation.
		series_class.def("_evaluate",wrap_evaluate<S,interop_type>);
		// Substitution.
		subs_exposer<interop_type>(series_class);
		interop_exposer<S,I + 1u,T...>(series_class,t);
	}
	// Differentiation.
	template <typename S>
	static S partial_wrapper(const S &s, const std::string &name)
	{
		return math::partial(s,name);
	}
	// Sin and cos.
	template <bool IsCos, typename S>
	static S sin_cos_wrapper(const S &s)
	{
		if (IsCos) {
			return math::cos(s);
		} else {
			return math::sin(s);
		}
	}
	// Helpers to get coefficient list.
	template <std::size_t I = 0u, typename... T>
	static void build_coefficient_list(const std::tuple<T...> &,
		typename std::enable_if<I == sizeof...(T)>::type * = piranha_nullptr)
	{}
	template <std::size_t I = 0u, typename... T>
	static void build_coefficient_list(const std::tuple<T...> &t,
		typename std::enable_if<(I < sizeof...(T))>::type * = piranha_nullptr)
	{
		try {
			cf_list.append(bp::make_tuple(bp::object(std::get<0u>(std::get<I>(t))),
				std::get<1u>(std::get<I>(t)),I));
		} catch (...) {
			::PyErr_Clear();
			cf_list.append(bp::make_tuple(bp::object(),std::get<1u>(std::get<I>(t)),I));
		}
		build_coefficient_list<I + 1u,T...>(t);
	}
	static bp::list get_coefficient_list()
	{
		return cf_list;
	}
	// Copy operations.
	template <typename S>
	static S copy_wrapper(const S &s)
	{
		return s;
	}
	template <typename S>
	static S deepcopy_wrapper(const S &s, bp::dict)
	{
		return copy_wrapper(s);
	}
	// Utility function to check if object is callable. Will throw TypeError if not.
	static void check_callable(bp::object func)
	{
#if PY_MAJOR_VERSION < 3
		bp::object builtin_module = bp::import("__builtin__");
		if (!builtin_module.attr("callable")(func)) {
			::PyErr_SetString(PyExc_TypeError,"object is not callable");
			bp::throw_error_already_set();
		}
#else
		// This will throw on failure.
		try {
			bp::object call_method = func.attr("__call__");
			(void)call_method;
		} catch (...) {
			::PyErr_Clear();
			::PyErr_SetString(PyExc_TypeError,"object is not callable");
			bp::throw_error_already_set();
		}
#endif
	}
	// Custom partial derivatives.
	// NOTE: here we need to take care of multithreading in the future, most likely by adding
	// the Python threading bits inside the lambda and also outside when checking func.
	template <typename S>
	static void register_custom_derivative(const std::string &name, bp::object func)
	{
		check_callable(func);
		S::register_custom_derivative(name,[func](const S &s) -> S {
			return bp::extract<S>(func(s));
		});
	}
	// filter() wrap.
	template <typename S>
	static S wrap_filter(const S &s, bp::object func)
	{
		typedef typename S::term_type::cf_type cf_type;
		check_callable(func);
		auto cpp_func = [func](const std::pair<cf_type,S> &p) -> bool {
			return bp::extract<bool>(func(bp::make_tuple(p.first,p.second)));
		};
		return s.filter(cpp_func);
	}
	// Check if type is tuple with two elements (for use in wrap_transform).
	static void check_tuple_2(bp::object obj)
	{
		bp::object builtin_module = bp::import("__builtin__");
		bp::object isinstance = builtin_module.attr("isinstance");
		bp::object tuple_type = builtin_module.attr("tuple");
		if (!isinstance(obj,tuple_type)) {
			::PyErr_SetString(PyExc_TypeError,"object is not a tuple");
			bp::throw_error_already_set();
		}
		const std::size_t len = bp::extract<std::size_t>(obj.attr("__len__")());
		if (len != 2u) {
			::PyErr_SetString(PyExc_ValueError,"the tuple to be returned in series transformation must have 2 elements");
			bp::throw_error_already_set();
		}
	}
	// transform() wrap.
	template <typename S>
	static S wrap_transform(const S &s, bp::object func)
	{
		typedef series_exposer<Series,CfTypes,InteropTypes> se_type;
		typedef typename S::term_type::cf_type cf_type;
		check_callable(func);
		auto cpp_func = [func](const std::pair<cf_type,S> &p) -> std::pair<cf_type,S> {
			bp::object tmp = func(bp::make_tuple(p.first,p.second));
			se_type::check_tuple_2(tmp);
			cf_type tmp_cf = bp::extract<cf_type>(tmp[0]);
			S tmp_key = bp::extract<S>(tmp[1]);
			return std::make_pair(std::move(tmp_cf),std::move(tmp_key));
		};
		return s.transform(cpp_func);
	}
	// degree wrappers.
	template <typename S>
	static decltype(std::declval<S>().degree()) wrap_degree(const S &s)
	{
		return s.degree();
	}
	template <typename S>
	static decltype(std::declval<S>().degree(std::declval<std::set<std::string>>())) wrap_partial_degree(const S &s, bp::object obj)
	{
		bp::stl_input_iterator<std::string> begin(obj), end;
		return s.degree(std::set<std::string>(begin,end));
	}
	template <typename S>
	static decltype(std::declval<S>().ldegree()) wrap_ldegree(const S &s)
	{
		return s.ldegree();
	}
	template <typename S>
	static decltype(std::declval<S>().ldegree(std::declval<std::set<std::string>>())) wrap_partial_ldegree(const S &s, bp::object obj)
	{
		bp::stl_input_iterator<std::string> begin(obj), end;
		return s.ldegree(std::set<std::string>(begin,end));
	}
	// Power series exposer.
	template <typename T>
	static void power_series_exposer(bp::class_<T> &series_class,
		typename std::enable_if<is_power_series<T>::value>::type * = piranha_nullptr)
	{
		series_class.def("degree",wrap_degree<T>);
		series_class.def("degree",wrap_partial_degree<T>);
		series_class.def("ldegree",wrap_ldegree<T>);
		series_class.def("ldegree",wrap_partial_ldegree<T>);
	}
	template <typename T>
	static void power_series_exposer(bp::class_<T> &,
		typename std::enable_if<!is_power_series<T>::value>::type * = piranha_nullptr)
	{}
	// Substitution exposer.
	template <typename U, typename T>
	static void subs_exposer(bp::class_<T> &series_class)
	{
		series_class.def("subs",&T::template subs<U>);
	}
	// Latex representation.
	template <typename S>
	static std::string wrap_repr_latex(const S &s)
	{
		std::ostringstream oss;
		oss << "$";
		s.print_tex(oss);
		oss << "$";
		return oss.str();
	}
	// Main exposer function.
	template <std::size_t I = 0u, typename... T>
	void main_exposer(const std::tuple<T...> &,
		typename std::enable_if<I == sizeof...(T)>::type * = piranha_nullptr) const
	{}
	template <std::size_t I = 0u, typename... T>
	void main_exposer(const std::tuple<T...> &t,
		typename std::enable_if<(I < sizeof...(T))>::type * = piranha_nullptr) const
	{
		typedef typename std::tuple_element<0u,
			typename std::tuple_element<I,std::tuple<T...>>::type>::type cf_type;
		typedef Series<cf_type> series_type;
		// Main class object and default constructor.
		bp::class_<series_type> series_class((std::string("_") + m_series_name + std::string("_") +
			boost::lexical_cast<std::string>(I)).c_str(),bp::init<>());
		// Constructor from string, if available.
		expose_string_ctor(series_class);
		// Copy constructor.
		series_class.def(bp::init<const series_type &>());
		// Shallow and deep copy.
		series_class.def("__copy__",copy_wrapper<series_type>);
		series_class.def("__deepcopy__",deepcopy_wrapper<series_type>);
		// NOTE: here repr is found via argument-dependent lookup.
		series_class.def(repr(bp::self));
		// Length.
		series_class.def("__len__",&series_type::size);
		// Interaction with self.
		series_class.def(bp::self += bp::self);
		series_class.def(bp::self + bp::self);
		series_class.def(bp::self -= bp::self);
		series_class.def(bp::self - bp::self);
		series_class.def(bp::self *= bp::self);
		series_class.def(bp::self * bp::self);
		series_class.def(bp::self == bp::self);
		series_class.def(bp::self != bp::self);
		series_class.def(+bp::self);
		series_class.def(-bp::self);
		// Interaction with interop types.
		interop_exposer(series_class,m_interop_types);
		// Partial derivative.
		bp::def("_partial",partial_wrapper<series_type>);
		series_class.def("partial",&series_type::partial);
		series_class.def("register_custom_derivative",register_custom_derivative<series_type>).staticmethod("register_custom_derivative");
		series_class.def("unregister_custom_derivative",
			series_type::unregister_custom_derivative).staticmethod("unregister_custom_derivative");
		series_class.def("unregister_all_custom_derivatives",
			series_type::unregister_all_custom_derivatives).staticmethod("unregister_all_custom_derivatives");
		// Filter and transform.
		series_class.def("filter",wrap_filter<series_type>);
		series_class.def("transform",wrap_transform<series_type>);
		// Sin and cos.
		bp::def("_sin",sin_cos_wrapper<false,series_type>);
		bp::def("_cos",sin_cos_wrapper<true,series_type>);
		// Power series methods.
		power_series_exposer(series_class);
		// Substitution with self.
		subs_exposer<series_type>(series_class);
		// Latex representation.
		series_class.def("_repr_latex_",wrap_repr_latex<series_type>);
		// Next iteration step.
		main_exposer<I + 1u,T...>(t);
	}
	explicit series_exposer(const std::string &series_name, const CfTypes &cf_types,
		const InteropTypes &interop_types):m_series_name(series_name),m_cf_types(cf_types),
		m_interop_types(interop_types)
	{
		main_exposer(m_cf_types);
		// Build and expose the coefficient list.
		// NOTE: not entirely sure here that this code will not be run multiple times on multiple imports.
		// Hence, clear the list for safety.
		cf_list = bp::list();
		build_coefficient_list(m_cf_types);
		bp::def((std::string("_") + m_series_name + std::string("_get_coefficient_list")).c_str(),
			get_coefficient_list);
	}
	const std::string	m_series_name;
	const CfTypes		m_cf_types;
	const InteropTypes	m_interop_types;
	static bp::list		cf_list;
};

template <template <typename...> class Series, typename CfTypes, typename InteropTypes>
bp::list series_exposer<Series,CfTypes,InteropTypes>::cf_list;
