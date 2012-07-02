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
		// Sin and cos.
		bp::def("_sin",sin_cos_wrapper<false,series_type>);
		bp::def("_cos",sin_cos_wrapper<true,series_type>);
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
