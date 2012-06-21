struct polynomial_exposer
{
	typedef boost::mpl::vector<double,integer,rational,real> cf_types;
	typedef cf_types interop_types;
	// Functor for interoperability with other types.
	template <typename Cf>
	struct interop_exposer
	{
		typedef bp::class_<polynomial<Cf,kronecker_monomial<>>> c_type;
		interop_exposer(c_type &cl):m_cl(cl) {}
		template <typename Interop>
		void operator()(const Interop &in) const
		{
			// Constrcutor from interoperable.
			m_cl.def(bp::init<const Interop &>());
			// NOTE: to resolve ambiguities when we interop with other series types
			// we can try using the namespace-qualified operators from Boost.Python.
			// Arithmetic and comparison with interoperable type.
			m_cl.def(bp::self += in);
			m_cl.def(bp::self + in);
			m_cl.def(in + bp::self);
			m_cl.def(bp::self -= in);
			m_cl.def(bp::self - in);
			m_cl.def(in - bp::self);
			m_cl.def(bp::self *= in);
			m_cl.def(bp::self * in);
			m_cl.def(in * bp::self);
			m_cl.def(bp::self == in);
			m_cl.def(in == bp::self);
			m_cl.def(bp::self != in);
			m_cl.def(in != bp::self);
			m_cl.def(bp::self /= in);
			m_cl.def(bp::self / in);
		}
		c_type &m_cl;
	};
	// TODO: use is_exponentiable type trait or something along those lines here instead of hard-coding?
	template <typename Series>
	static void pow_float(bp::class_<Series> &cl, typename std::enable_if<
		std::is_floating_point<typename Series::term_type::cf_type>::value>::type * = piranha_nullptr)
	{
		cl.def("__pow__",&Series::template pow<double>);
	}
	template <typename Series>
	static void pow_float(bp::class_<Series> &, typename std::enable_if<
		!std::is_floating_point<typename Series::term_type::cf_type>::value>::type * = piranha_nullptr)
	{}
	template <typename Cf>
	void operator()(const Cf &) const
	{
		typedef polynomial<Cf,kronecker_monomial<>> p_type;
		// Index of the type in the MPL vector, used to identify uniquely the resulting Python type.
		const auto index = boost::mpl::distance<typename boost::mpl::begin<cf_types>::type,
			typename boost::mpl::find<cf_types,Cf>::type>::value;
		bp::class_<p_type> p_class((std::string("_polynomial_") + boost::lexical_cast<std::string>(index)).c_str(),bp::init<>());
		p_class.def(bp::init<const std::string &>());
		p_class.def(bp::init<const p_type &>());
		// NOTE: here repr is found via argument-dependent lookup.
		p_class.def(repr(bp::self));
		p_class.def("__len__",&p_type::size);
		// Interaction with self.
		p_class.def(bp::self += bp::self);
		p_class.def(bp::self + bp::self);
		p_class.def(bp::self -= bp::self);
		p_class.def(bp::self - bp::self);
		p_class.def(bp::self *= bp::self);
		p_class.def(bp::self * bp::self);
		p_class.def(bp::self == bp::self);
		p_class.def(bp::self != bp::self);
		p_class.def(+bp::self);
		p_class.def(-bp::self);
		// Interaction with interoperable types.
		boost::mpl::for_each<interop_types>(interop_exposer<Cf>(p_class));
		// Exponentiation.
		pow_float(p_class);
		p_class.def("__pow__",&p_type::template pow<integer>);
	}
	struct cf_types_exposer
	{
		explicit cf_types_exposer(bp::list &l):m_l(l) {}
		template <typename Cf>
		void operator()(const Cf &) const
		{
			// NOTE: here we init via 0 in order to avoid undefined initialisation of variable cf when
			// building the result list.
			Cf cf(0);
			const auto index = boost::mpl::distance<typename boost::mpl::begin<cf_types>::type,
				typename boost::mpl::find<cf_types,Cf>::type>::value;
			try {
				m_l.append(bp::make_tuple(cf,boost::lexical_cast<std::string>(index)));
			} catch (const std::runtime_error &) {
				// NOTE: here a runtime error is thrown if cf could not be converted to a Python type - this
				// means that the coefficient type is not interoperable with Python types.
			}
		}
		bp::list &m_l;
	};
	static bp::list get_cf_types()
	{
		bp::list retval;
		boost::mpl::for_each<cf_types>(cf_types_exposer(retval));
		return retval;
	}
};

inline void expose_polynomials()
{
	boost::mpl::for_each<polynomial_exposer::cf_types>(polynomial_exposer());
	def("_polynomial_get_cf_types",&polynomial_exposer::get_cf_types);
}
