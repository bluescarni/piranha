template <template <typename ...> class Series, typename Descriptor>
class exposer
{
		using params = typename Descriptor::params;
		// Detect the presence of interoperable types.
		PIRANHA_DECLARE_HAS_TYPEDEF(interop_types);
		// Detect the presence of pow types.
		PIRANHA_DECLARE_HAS_TYPEDEF(pow_types);
		// Detect the presence of evaluation types.
		PIRANHA_DECLARE_HAS_TYPEDEF(eval_types);
		// Detect the presence of substitution types.
		PIRANHA_DECLARE_HAS_TYPEDEF(subs_types);
		// for_each tuple algorithm.
		template <typename Tuple, typename Op, std::size_t Idx = 0u>
		static void tuple_for_each(const Tuple &t, const Op &op, typename std::enable_if<Idx != std::tuple_size<Tuple>::value>::type * = nullptr)
		{
			op(std::get<Idx>(t));
			static_assert(Idx != boost::integer_traits<std::size_t>::const_max,"Overflow error.");
			tuple_for_each<Tuple,Op,Idx + std::size_t(1)>(t,op);
		}
		template <typename Tuple, typename Op, std::size_t Idx = 0u>
		static void tuple_for_each(const Tuple &, const Op &, typename std::enable_if<Idx == std::tuple_size<Tuple>::value>::type * = nullptr)
		{}
		// Expose constructor conditionally.
		template <typename U, typename T>
		static void expose_ctor(bp::class_<T> &cl,
			typename std::enable_if<std::is_constructible<T,U>::value>::type * = nullptr)
		{
			cl.def(bp::init<U>());
		}
		template <typename U, typename T>
		static void expose_ctor(bp::class_<T> &,
			typename std::enable_if<!std::is_constructible<T,U>::value>::type * = nullptr)
		{}
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
		// Sparsity wrapper.
		template <typename S>
		static bp::tuple table_sparsity_wrapper(const S &s)
		{
			const auto retval = s.table_sparsity();
			return bp::make_tuple(std::get<0u>(retval),std::get<1u>(retval));
		}
		// Wrapper to list.
		template <typename S>
		static bp::list to_list_wrapper(const S &s)
		{
			bp::list retval;
			for (const auto &t: s) {
				retval.append(bp::make_tuple(t.first,t.second));
			}
			return retval;
		}
		// TMP to check if a type is in the tuple.
		template <typename T, typename Tuple, std::size_t I = 0u, typename Enable = void>
		struct type_in_tuple
		{
			static_assert(I < boost::integer_traits<std::size_t>::const_max,"Overflow error.");
			static const bool value = std::is_same<T,typename std::tuple_element<I,Tuple>::type>::value ||
				type_in_tuple<T,Tuple,I + 1u>::value;
		};
		template <typename T, typename Tuple, std::size_t I>
		struct type_in_tuple<T,Tuple,I,typename std::enable_if<I == std::tuple_size<Tuple>::value>::type>
		{
			static const bool value = false;
		};
		// Handle division specially (allowed only with non-series types).
		template <typename S, typename T>
		static void expose_division(bp::class_<S> &series_class, const T &in,
			typename std::enable_if<!is_instance_of<T,series>::value>::type * = nullptr)
		{
			series_class.def(bp::self /= in);
			series_class.def(bp::self / in);
		}
		template <typename S, typename T>
		static void expose_division(bp::class_<S> &, const T &,
			typename std::enable_if<is_instance_of<T,series>::value>::type * = nullptr)
		{}
		// Expose arithmetics operations with another type.
		// NOTE: this will have to be conditional in the future.
		template <typename T, typename S>
		static void expose_arithmetics(bp::class_<S> &series_class)
		{
			namespace sn = boost::python::self_ns;
			T in;
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
		}
		// Exponentiation support.
		template <typename S>
		struct pow_exposer
		{
			pow_exposer(bp::class_<S> &series_class):m_series_class(series_class) {}
			bp::class_<S> &m_series_class;
			template <typename T>
			void operator()(const T &, typename std::enable_if<is_exponentiable<S,T>::value>::type * = nullptr) const
			{
				m_series_class.def("__pow__",pow_wrapper<S,T>);
			}
			template <typename T>
			void operator()(const T &, typename std::enable_if<!is_exponentiable<S,T>::value>::type * = nullptr) const
			{}
		};
		template <typename T, typename U>
		static auto pow_wrapper(const T &s, const U &x) -> decltype(math::pow(s,x))
		{
			return math::pow(s,x);
		}
		template <typename S, typename T = Descriptor>
		static void expose_pow(bp::class_<S> &series_class, typename std::enable_if<has_typedef_pow_types<T>::value>::type * = nullptr)
		{
			using pow_types = typename Descriptor::pow_types;
			pow_types pt;
			tuple_for_each(pt,pow_exposer<S>(series_class));
		}
		template <typename S, typename T = Descriptor>
		static void expose_pow(bp::class_<S> &, typename std::enable_if<!has_typedef_pow_types<T>::value>::type * = nullptr)
		{}
		// Evaluation.
		template <typename S>
		struct eval_exposer
		{
			eval_exposer(bp::class_<S> &series_class):m_series_class(series_class) {}
			bp::class_<S> &m_series_class;
			template <typename T>
			void operator()(const T &, typename std::enable_if<is_evaluable<S,T>::value>::type * = nullptr) const
			{
				m_series_class.def("_evaluate",evaluate_wrapper<S,T>);
			}
			template <typename T>
			void operator()(const T &, typename std::enable_if<!is_evaluable<S,T>::value>::type * = nullptr) const
			{}
		};
		template <typename S, typename T>
		static auto evaluate_wrapper(const S &s, bp::dict dict, const T &) -> decltype(s.evaluate(std::declval<std::unordered_map<std::string,T>>()))
		{
			std::unordered_map<std::string,T> cpp_dict;
			bp::stl_input_iterator<std::string> it(dict), end;
			for (; it != end; ++it) {
				cpp_dict[*it] = bp::extract<T>(dict[*it])();
			}
			return s.evaluate(cpp_dict);
		}
		template <typename S, typename T = Descriptor>
		static void expose_eval(bp::class_<S> &series_class, typename std::enable_if<has_typedef_eval_types<T>::value>::type * = nullptr)
		{
			using eval_types = typename Descriptor::eval_types;
			eval_types et;
			tuple_for_each(et,eval_exposer<S>(series_class));
		}
		template <typename S, typename T = Descriptor>
		static void expose_eval(bp::class_<S> &, typename std::enable_if<!has_typedef_eval_types<T>::value>::type * = nullptr)
		{}
		// Substitution.
		template <typename S>
		struct subs_exposer
		{
			subs_exposer(bp::class_<S> &series_class):m_series_class(series_class) {}
			bp::class_<S> &m_series_class;
			template <typename T>
			void operator()(const T &, typename std::enable_if<has_subs<S,T>::value && has_ipow_subs<S,T>::value &&
				has_t_subs<S,T>::value>::type * = nullptr) const
			{
				// NOTE: this should probably be replaced with a wrapper that calls math::... functions.
				m_series_class.def("_subs",&S::template subs<T>);
				m_series_class.def("_ipow_subs",&S::template ipow_subs<T>);
				m_series_class.def("_t_subs",&S::template t_subs<T,T>);
			}
			template <typename T>
			void operator()(const T &, typename std::enable_if<!has_subs<S,T>::value || !has_ipow_subs<S,T>::value ||
				!has_t_subs<S,T>::value>::type * = nullptr) const
			{}
		};
		template <typename S, typename T = Descriptor>
		static void expose_subs(bp::class_<S> &series_class, typename std::enable_if<has_typedef_subs_types<T>::value>::type * = nullptr)
		{
			using subs_types = typename Descriptor::subs_types;
			subs_types st;
			tuple_for_each(st,subs_exposer<S>(series_class));
		}
		template <typename S, typename T = Descriptor>
		static void expose_subs(bp::class_<S> &, typename std::enable_if<!has_typedef_subs_types<T>::value>::type * = nullptr)
		{}
		// Interaction with interoperable types.
		template <typename S>
		struct interop_exposer
		{
			interop_exposer(bp::class_<S> &series_class):m_series_class(series_class) {}
			bp::class_<S> &m_series_class;
			template <typename T>
			void operator()(const T &) const
			{
				expose_ctor<const T &>(m_series_class);
				expose_arithmetics<T>(m_series_class);
			}
		};
		template <typename S, typename T = Descriptor>
		static void expose_interoperable(bp::class_<S> &series_class, typename std::enable_if<has_typedef_interop_types<T>::value>::type * = nullptr)
		{
			using interop_types = typename Descriptor::interop_types;
			interop_types it;
			tuple_for_each(it,interop_exposer<S>(series_class));
		}
		template <typename S, typename T = Descriptor>
		static void expose_interoperable(bp::class_<S> &, typename std::enable_if<!has_typedef_interop_types<T>::value>::type * = nullptr)
		{}
		// Expose integration conditionally.
		template <typename S>
		static S integrate_wrapper(const S &s, const std::string &name)
		{
			return math::integrate(s,name);
		}
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
		// Differentiation.
		template <typename S>
		static S partial_wrapper(const S &s, const std::string &name)
		{
			return math::partial(s,name);
		}
		template <typename S>
		static void expose_partial(bp::class_<S> &series_class,
			typename std::enable_if<is_differentiable<S>::value>::type * = nullptr)
		{
			series_class.def("partial",&S::partial);
			bp::def("_partial",partial_wrapper<S>);
		}
		template <typename S>
		static void expose_partial(bp::class_<S> &,
			typename std::enable_if<!is_differentiable<S>::value>::type * = nullptr)
		{}
		// Main exposer.
		struct exposer_op
		{
			explicit exposer_op() = default;
			template <typename ... Args>
			void operator()(const std::tuple<Args...> &) const
			{
				using s_type = Series<Args...>;
				// Get the series name.
				const std::string s_name = descriptor<s_type>::name();
				if (series_archive.find(s_name) != series_archive.end()) {
					piranha_throw(std::runtime_error,"series was already registered");
				}
				const std::string exposed_name = std::string("_series_") + boost::lexical_cast<std::string>(series_counter);
				// Start exposing.
				bp::class_<s_type> series_class(exposed_name.c_str(),bp::init<>());
				series_archive[s_name] = series_counter;
				++series_counter;
				// Constructor from string, if available.
				expose_ctor<const std::string &>(series_class);
				// Copy constructor.
				series_class.def(bp::init<const s_type &>());
				// Shallow and deep copy.
				series_class.def("__copy__",copy_wrapper<s_type>);
				series_class.def("__deepcopy__",deepcopy_wrapper<s_type>);
				// NOTE: here repr is found via argument-dependent lookup.
				series_class.def(repr(bp::self));
				// Length.
				series_class.def("__len__",&s_type::size);
				// Table properties.
				series_class.def("table_load_factor",&s_type::table_load_factor);
				series_class.def("table_bucket_count",&s_type::table_bucket_count);
				series_class.def("table_sparsity",table_sparsity_wrapper<s_type>);
				// Conversion to list.
				series_class.add_property("list",to_list_wrapper<s_type>);
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
				// Expose interoperable types.
				expose_interoperable(series_class);
				// Expose pow.
				expose_pow(series_class);
				// Evaluate.
				expose_eval(series_class);
				// Subs.
				expose_subs(series_class);
				// Integration.
				expose_integrate(series_class);
				// Partial differentiation.
				expose_partial(series_class);
			}
		};
	public:
		exposer()
		{
			params p;
			tuple_for_each(p,exposer_op{});
		}
		exposer(const exposer &) = delete;
		exposer(exposer &&) = delete;
		exposer &operator=(const exposer &) = delete;
		exposer &operator=(exposer &&) = delete;
		~exposer() = default;
};
