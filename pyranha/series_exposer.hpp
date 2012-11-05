template <template <typename...> class Series, typename CfTypes, typename InteropTypes>
struct series_exposer
{
	// String constructor.
	template <typename T>
	static void expose_string_ctor(bp::class_<T> &cl,
		typename std::enable_if<std::is_constructible<T,std::string>::value>::type * = nullptr)
	{
		cl.def(bp::init<const std::string &>());
	}
	template <typename T>
	static void expose_string_ctor(bp::class_<T> &,
		typename std::enable_if<!std::is_constructible<T,std::string>::value>::type * = nullptr)
	{}
	// Exponentiation support.
	template <typename T, typename U>
	static void pow_exposer(bp::class_<T> &series_class, const U &,
		typename std::enable_if<is_exponentiable<T,U>::value>::type * = nullptr)
	{
		series_class.def("__pow__",&T::template pow<U>);
	}
	template <typename T, typename U>
	static void pow_exposer(bp::class_<T> &, const U &,
		typename std::enable_if<!is_exponentiable<T,U>::value>::type * = nullptr)
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
	// Handle division specially (allowed only with non-series types).
	template <typename S, typename T>
	static void expose_division(bp::class_<S> &series_class, const T &in,
		typename std::enable_if<!std::is_base_of<detail::series_tag,T>::value>::type * = nullptr)
	{
		series_class.def(bp::self /= in);
		series_class.def(bp::self / in);
	}
	template <typename S, typename T>
	static void expose_division(bp::class_<S> &, const T &,
		typename std::enable_if<std::is_base_of<detail::series_tag,T>::value>::type * = nullptr)
	{}
	// TMP to check if a type is in the tuple.
	template <typename T, typename Tuple, std::size_t I = 0u, typename Enable = void>
	struct type_in_tuple
	{
		static const bool value = std::is_same<T,typename std::tuple_element<I,Tuple>::type>::value ||
			type_in_tuple<T,Tuple,I + 1u>::value;
	};
	template <typename T, typename Tuple, std::size_t I>
	struct type_in_tuple<T,Tuple,I,typename std::enable_if<I == std::tuple_size<Tuple>::value>::type>
	{
		static const bool value = false;
	};
	// Interaction with interoperable types.
	template <typename S, std::size_t I = 0u, typename... T>
	void interop_exposer(bp::class_<S> &series_class, const std::tuple<T...> &,
		typename std::enable_if<I == sizeof...(T)>::type * = nullptr) const
	{
		// Add interoperability with coefficient type if it is not already included in
		// the interoperable types.
		if (!type_in_tuple<typename S::term_type::cf_type,std::tuple<T...>>::value) {
			typename S::term_type::cf_type cf;
			interop_exposer(series_class,std::make_tuple(cf));
		}
	}
	template <typename S, std::size_t I = 0u, typename... T>
	void interop_exposer(bp::class_<S> &series_class, const std::tuple<T...> &t,
		typename std::enable_if<(I < sizeof...(T))>::type * = nullptr) const
	{
		namespace sn = boost::python::self_ns;
		typedef typename std::tuple_element<I,std::tuple<T...>>::type interop_type;
		interop_type in;
		// Constructor from interoperable.
		series_class.def(bp::init<const interop_type &>());
		// Arithmetic and comparison with interoperable type.
		// NOTE: in order to resolve ambiguities when we interop with other series types,
		// we use the namespace-qualified operators from Boost.Python.
		// NOTE: if we fix is_addable type traits for series the above is not needed any more,
		// as series + bp::self is not available any more.
		series_class.def(sn::operator+=(bp::self,in));
		series_class.def(sn::operator+(bp::self,in));
		series_class.def(sn::operator+(in,bp::self));
		series_class.def(sn::operator-=(bp::self,in));
		series_class.def(sn::operator-(bp::self,in));
		series_class.def(sn::operator-(in,bp::self));
		series_class.def(sn::operator*=(bp::self,in));
		series_class.def(sn::operator*(bp::self,in));
		series_class.def(sn::operator*(in,bp::self));
		series_class.def(sn::operator==(bp::self,in));
		series_class.def(sn::operator==(in,bp::self));
		series_class.def(sn::operator!=(bp::self,in));
		series_class.def(sn::operator!=(in,bp::self));
		expose_division(series_class,in);
		// Exponentiation.
		pow_exposer(series_class,in);
		// Evaluation.
		series_class.def("_evaluate",wrap_evaluate<S,interop_type>);
		// Substitutions.
		series_class.def("subs",&S::template subs<interop_type>);
		series_class.def("ipow_subs",&S::template ipow_subs<interop_type>);
		t_subs_exposer<interop_type>(series_class);
		interop_exposer<S,I + 1u,T...>(series_class,t);
	}
	// Differentiation.
	template <typename S>
	static S partial_wrapper(const S &s, const std::string &name)
	{
		return math::partial(s,name);
	}
	// Integration.
	template <typename S>
	static S integrate_wrapper(const S &s, const std::string &name)
	{
		return math::integrate(s,name);
	}
	// Poisson bracket.
	template <typename S>
	static S pbracket_wrapper(const S &s1, const S &s2, bp::list p_list, bp::list q_list)
	{
		bp::stl_input_iterator<std::string> begin_p(p_list), end_p;
		bp::stl_input_iterator<std::string> begin_q(q_list), end_q;
		return math::pbracket(s1,s2,std::vector<std::string>(begin_p,end_p),
			std::vector<std::string>(begin_q,end_q));
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
	static void build_coefficient_list(bp::list &, const std::tuple<T...> &,
		typename std::enable_if<I == sizeof...(T)>::type * = nullptr)
	{}
	template <std::size_t I = 0u, typename... T>
	static void build_coefficient_list(bp::list &retval, const std::tuple<T...> &t,
		typename std::enable_if<(I < sizeof...(T))>::type * = nullptr)
	{
		try {
			retval.append(bp::make_tuple(bp::object(std::get<0u>(std::get<I>(t))),
				std::get<1u>(std::get<I>(t)),I));
		} catch (...) {
			::PyErr_Clear();
			retval.append(bp::make_tuple(bp::object(),std::get<1u>(std::get<I>(t)),I));
		}
		build_coefficient_list<I + 1u,T...>(retval,t);
	}
	// Return the list of coefficient types available for the series. The list elements
	// are tuples (instance,descriptor,index).
	static bp::list get_coefficient_list()
	{
		bp::list retval;
		build_coefficient_list(retval,m_cf_types);
		return retval;
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
	// degree() wrappers.
	template <typename S>
	static decltype(std::declval<S>().degree()) wrap_degree(const S &s)
	{
		return s.degree();
	}
	template <typename S>
	static decltype(std::declval<S>().degree(std::declval<std::set<std::string>>())) wrap_partial_degree_set(const S &s, bp::list l)
	{
		bp::stl_input_iterator<std::string> begin(l), end;
		return s.degree(std::set<std::string>(begin,end));
	}
	template <typename S>
	static decltype(std::declval<S>().degree(std::declval<std::string>())) wrap_partial_degree_string(const S &s, const std::string &str)
	{
		return s.degree(str);
	}
	template <typename S>
	static decltype(std::declval<S>().ldegree()) wrap_ldegree(const S &s)
	{
		return s.ldegree();
	}
	template <typename S>
	static decltype(std::declval<S>().ldegree(std::declval<std::set<std::string>>())) wrap_partial_ldegree_set(const S &s, bp::list l)
	{
		bp::stl_input_iterator<std::string> begin(l), end;
		return s.ldegree(std::set<std::string>(begin,end));
	}
	template <typename S>
	static decltype(std::declval<S>().ldegree(std::declval<std::string>())) wrap_partial_ldegree_string(const S &s, const std::string &str)
	{
		return s.ldegree(str);
	}
	// Power series exposer.
	template <typename T>
	static void power_series_exposer(bp::class_<T> &series_class,
		typename std::enable_if<is_power_series<T>::value>::type * = nullptr)
	{
		series_class.def("degree",wrap_degree<T>);
		series_class.def("degree",wrap_partial_degree_set<T>);
		series_class.def("degree",wrap_partial_degree_string<T>);
		series_class.def("ldegree",wrap_ldegree<T>);
		series_class.def("ldegree",wrap_partial_ldegree_set<T>);
		series_class.def("ldegree",wrap_partial_ldegree_string<T>);
	}
	template <typename T>
	static void power_series_exposer(bp::class_<T> &,
		typename std::enable_if<!is_power_series<T>::value>::type * = nullptr)
	{}
	// Trigonometric wrappers.
	template <typename S>
	static decltype(std::declval<S>().t_degree()) wrap_t_degree(const S &s)
	{
		return s.t_degree();
	}
	template <typename S>
	static decltype(std::declval<S>().t_degree(std::declval<std::set<std::string>>())) wrap_partial_t_degree(const S &s, bp::list l)
	{
		bp::stl_input_iterator<std::string> begin(l), end;
		return s.t_degree(std::set<std::string>(begin,end));
	}
	template <typename S>
	static decltype(std::declval<S>().t_ldegree()) wrap_t_ldegree(const S &s)
	{
		return s.t_ldegree();
	}
	template <typename S>
	static decltype(std::declval<S>().t_ldegree(std::declval<std::set<std::string>>())) wrap_partial_t_ldegree(const S &s, bp::list l)
	{
		bp::stl_input_iterator<std::string> begin(l), end;
		return s.t_ldegree(std::set<std::string>(begin,end));
	}
	template <typename S>
	static decltype(std::declval<S>().t_order()) wrap_t_order(const S &s)
	{
		return s.t_order();
	}
	template <typename S>
	static decltype(std::declval<S>().t_order(std::declval<std::set<std::string>>())) wrap_partial_t_order(const S &s, bp::list l)
	{
		bp::stl_input_iterator<std::string> begin(l), end;
		return s.t_order(std::set<std::string>(begin,end));
	}
	template <typename S>
	static decltype(std::declval<S>().t_lorder()) wrap_t_lorder(const S &s)
	{
		return s.t_lorder();
	}
	template <typename S>
	static decltype(std::declval<S>().t_lorder(std::declval<std::set<std::string>>())) wrap_partial_t_lorder(const S &s, bp::list l)
	{
		bp::stl_input_iterator<std::string> begin(l), end;
		return s.t_lorder(std::set<std::string>(begin,end));
	}
	// Trigonometric exposer.
	template <typename T>
	static void trigonometric_series_exposer(bp::class_<T> &series_class,
		typename std::enable_if<std::is_base_of<detail::trigonometric_series_tag,T>::value>::type * = nullptr)
	{
		series_class.def("t_degree",wrap_t_degree<T>);
		series_class.def("t_degree",wrap_partial_t_degree<T>);
		series_class.def("t_ldegree",wrap_t_ldegree<T>);
		series_class.def("t_ldegree",wrap_partial_t_ldegree<T>);
		series_class.def("t_order",wrap_t_order<T>);
		series_class.def("t_order",wrap_partial_t_order<T>);
		series_class.def("t_lorder",wrap_t_lorder<T>);
		series_class.def("t_lorder",wrap_partial_t_lorder<T>);
	}
	template <typename T>
	static void trigonometric_series_exposer(bp::class_<T> &,
		typename std::enable_if<!std::is_base_of<detail::trigonometric_series_tag,T>::value>::type * = nullptr)
	{}
	// t_subs exposer.
	template <typename S, typename T>
	static auto wrap_t_subs(const S &series, const std::string &name, const T &c, const T &s) -> decltype(series.t_subs(name,c,s))
	{
		return series.t_subs(name,c,s);
	}
	template <typename T, typename S>
	static void t_subs_exposer(bp::class_<S> &series_class,
		typename std::enable_if<has_t_subs<S,T>::value>::type * = nullptr)
	{
		series_class.def("t_subs",wrap_t_subs<S,T>);
	}
	template <typename T, typename S>
	static void t_subs_exposer(bp::class_<S> &,
		typename std::enable_if<!has_t_subs<S,T>::value>::type * = nullptr)
	{}
	// Latex representation.
	template <typename S>
	static std::string wrap_latex(const S &s)
	{
		std::ostringstream oss;
		s.print_tex(oss);
		return oss.str();
	}
	template <typename S>
	static bp::list to_list_wrapper(const S &s)
	{
		bp::list retval;
		for (auto it = s.begin(); it != s.end(); ++it) {
			retval.append(bp::make_tuple(it->first,it->second));
		}
		return retval;
	}
	// Expose integration conditionally.
	template <typename S>
	static void expose_integrate(bp::class_<S> &series_class,
		typename std::enable_if<is_integrable<S>::value>::type * = nullptr)
	{
		series_class.def("integrate",&S::integrate);
		bp::def("_integrate",integrate_wrapper<S>);
	}
	template <typename S>
	static void expose_integrate(bp::class_<S> &,
		typename std::enable_if<!is_integrable<S>::value>::type * = nullptr)
	{}
	// Sparsity wrapper.
	template <typename S>
	static bp::tuple table_sparsity_wrapper(const S &s)
	{
		const auto retval = s.table_sparsity();
		return bp::make_tuple(std::get<0u>(retval),std::get<1u>(retval));
	}
	// Symbol set wrapper.
	template <typename S>
	static bp::list symbol_set_wrapper(const S &s)
	{
		bp::list retval;
		for (auto it = s.get_symbol_set().begin(); it != s.get_symbol_set().end(); ++it) {
			retval.append(it->get_name());
		}
		return retval;
	}
	// Main exposer function.
	template <std::size_t I = 0u, typename... T>
	void main_exposer(const std::tuple<T...> &,
		typename std::enable_if<I == sizeof...(T)>::type * = nullptr) const
	{}
	template <std::size_t I = 0u, typename... T>
	void main_exposer(const std::tuple<T...> &t,
		typename std::enable_if<(I < sizeof...(T))>::type * = nullptr) const
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
		// Table properties.
		series_class.def("table_load_factor",&series_type::table_load_factor);
		series_class.def("table_bucket_count",&series_type::table_bucket_count);
		series_class.def("table_sparsity",table_sparsity_wrapper<series_type>);
		// Conversion to list.
		series_class.add_property("list",to_list_wrapper<series_type>);
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
		// Integration.
		expose_integrate(series_class);
		// Poisson bracket.
		bp::def("_pbracket",pbracket_wrapper<series_type>);
		// Filter and transform.
		series_class.def("filter",wrap_filter<series_type>);
		series_class.def("transform",wrap_transform<series_type>);
		// Trimming.
		series_class.def("trim",&series_type::trim);
		// Sin and cos.
		bp::def("_sin",sin_cos_wrapper<false,series_type>);
		bp::def("_cos",sin_cos_wrapper<true,series_type>);
		// Power series methods.
		power_series_exposer(series_class);
		// Trigonometric series methods.
		trigonometric_series_exposer(series_class);
		// Substitution with self.
		series_class.def("subs",&series_type::template subs<series_type>);
		series_class.def("ipow_subs",&series_type::template ipow_subs<series_type>);
		// Trigonometric subs.
		t_subs_exposer<series_type>(series_class);
		// Latex.
		series_class.def("_latex_",wrap_latex<series_type>);
		// Arguments set.
		series_class.add_property("symbol_set",symbol_set_wrapper<series_type>);
		// Next iteration step.
		main_exposer<I + 1u,T...>(t);
	}
	explicit series_exposer(const std::string &series_name, const CfTypes &cf_types,
		const InteropTypes &interop_types):m_series_name(series_name),m_interop_types(interop_types)
	{
		m_cf_types = cf_types;
		main_exposer(m_cf_types);
		bp::def((std::string("_") + m_series_name + std::string("_get_coefficient_list")).c_str(),
			get_coefficient_list);
	}
	const std::string	m_series_name;
	const InteropTypes	m_interop_types;
	// This needs to exist statically as it is used when building the coefficients list.
	static CfTypes		m_cf_types;
};

template <template <typename...> class Series, typename CfTypes, typename InteropTypes>
CfTypes series_exposer<Series,CfTypes,InteropTypes>::m_cf_types;
