// TODO: still remains:
// - interop with cf type.
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
				m_series_class.def("subs",&S::template subs<T>);
				m_series_class.def("ipow_subs",&S::template ipow_subs<T>);
				m_series_class.def("t_subs",&S::template t_subs<T,T>);
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
			// Implement subs with self.
			S tmp;
			subs_exposer<S> se(series_class);
			se(tmp);
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
		static S partial_member_wrapper(const S &s, const std::string &name)
		{
			return s.partial(name);
		}
		// Custom partial derivatives registration wrapper.
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
		template <typename S>
		static void expose_partial(bp::class_<S> &series_class,
			typename std::enable_if<is_differentiable<S>::value>::type * = nullptr)
		{
			series_class.def("partial",partial_member_wrapper<S>);
			bp::def("_partial",partial_wrapper<S>);
			// Custom derivatives support.
			series_class.def("register_custom_derivative",register_custom_derivative<S>).staticmethod("register_custom_derivative");
			series_class.def("unregister_custom_derivative",
				S::unregister_custom_derivative).staticmethod("unregister_custom_derivative");
			series_class.def("unregister_all_custom_derivatives",
				S::unregister_all_custom_derivatives).staticmethod("unregister_all_custom_derivatives");
		}
		template <typename S>
		static void expose_partial(bp::class_<S> &,
			typename std::enable_if<!is_differentiable<S>::value>::type * = nullptr)
		{}
		// Poisson bracket.
		template <typename S>
		static S pbracket_wrapper(const S &s1, const S &s2, bp::list p_list, bp::list q_list)
		{
			bp::stl_input_iterator<std::string> begin_p(p_list), end_p;
			bp::stl_input_iterator<std::string> begin_q(q_list), end_q;
			return math::pbracket(s1,s2,std::vector<std::string>(begin_p,end_p),
				std::vector<std::string>(begin_q,end_q));
		}
		template <typename S>
		static void expose_pbracket(bp::class_<S> &,
			typename std::enable_if<has_pbracket<S>::value>::type * = nullptr)
		{
			bp::def("_pbracket",pbracket_wrapper<S>);
		}
		template <typename S>
		static void expose_pbracket(bp::class_<S> &,
			typename std::enable_if<!has_pbracket<S>::value>::type * = nullptr)
		{}
		// Canonical transformation.
		// NOTE: last param is dummy to let the Boost.Python type system to pick the correct type.
		template <typename S>
		static bool canonical_wrapper(bp::list new_p, bp::list new_q, bp::list p_list, bp::list q_list, const S &)
		{
			bp::stl_input_iterator<S> begin_new_p(new_p), end_new_p;
			bp::stl_input_iterator<S> begin_new_q(new_q), end_new_q;
			bp::stl_input_iterator<std::string> begin_p(p_list), end_p;
			bp::stl_input_iterator<std::string> begin_q(q_list), end_q;
			return math::transformation_is_canonical(std::vector<S>(begin_new_p,end_new_p),std::vector<S>(begin_new_q,end_new_q),
				std::vector<std::string>(begin_p,end_p),std::vector<std::string>(begin_q,end_q));
		}
		template <typename S>
		static void expose_canonical(bp::class_<S> &,
			typename std::enable_if<has_transformation_is_canonical<S>::value>::type * = nullptr)
		{
			bp::def("_transformation_is_canonical",canonical_wrapper<S>);
		}
		template <typename S>
		static void expose_canonical(bp::class_<S> &,
			typename std::enable_if<!has_transformation_is_canonical<S>::value>::type * = nullptr)
		{}
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
			bp::object builtin_module = bp::import(
#if PY_MAJOR_VERSION < 3
			"__builtin__"
#else
			"builtins"
#endif
			);
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
			typedef typename S::term_type::cf_type cf_type;
			check_callable(func);
			auto cpp_func = [func](const std::pair<cf_type,S> &p) -> std::pair<cf_type,S> {
				bp::object tmp = func(bp::make_tuple(p.first,p.second));
				check_tuple_2(tmp);
				cf_type tmp_cf = bp::extract<cf_type>(tmp[0]);
				S tmp_key = bp::extract<S>(tmp[1]);
				return std::make_pair(std::move(tmp_cf),std::move(tmp_key));
			};
			return s.transform(cpp_func);
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
		template <typename S>
		static void expose_sin_cos(typename std::enable_if<has_sine<S>::value && has_cosine<S>::value>::type * = nullptr)
		{
			bp::def("_sin",sin_cos_wrapper<false,S>);
			bp::def("_cos",sin_cos_wrapper<true,S>);
		}
		template <typename S>
		static void expose_sin_cos(typename std::enable_if<!has_sine<S>::value || !has_cosine<S>::value>::type * = nullptr)
		{}
		// Power series exposer.
		template <typename T>
		static void expose_power_series(bp::class_<T> &series_class,
			typename std::enable_if<has_degree<T>::value && has_ldegree<T>::value>::type * = nullptr)
		{
			// NOTE: probably we should make these math:: wrappers. Same for the trig ones.
			series_class.def("degree",wrap_degree<T>);
			series_class.def("degree",wrap_partial_degree_set<T>);
			series_class.def("ldegree",wrap_ldegree<T>);
			series_class.def("ldegree",wrap_partial_ldegree_set<T>);
		}
		template <typename T>
		static void expose_power_series(bp::class_<T> &,
			typename std::enable_if<!has_degree<T>::value || !has_ldegree<T>::value>::type * = nullptr)
		{}
		// degree() wrappers.
		template <typename S>
		static auto wrap_degree(const S &s) -> decltype(s.degree())
		{
			return s.degree();
		}
		template <typename S>
		static auto wrap_partial_degree_set(const S &s, bp::list l) -> decltype(s.degree(std::set<std::string>{}))
		{
			bp::stl_input_iterator<std::string> begin(l), end;
			return s.degree(std::set<std::string>(begin,end));
		}
		template <typename S>
		static auto wrap_ldegree(const S &s) -> decltype(s.ldegree())
		{
			return s.ldegree();
		}
		template <typename S>
		static auto wrap_partial_ldegree_set(const S &s, bp::list l) -> decltype(s.ldegree(std::set<std::string>{}))
		{
			bp::stl_input_iterator<std::string> begin(l), end;
			return s.ldegree(std::set<std::string>(begin,end));
		}
		// Trigonometric exposer.
		template <typename S>
		static void expose_trigonometric_series(bp::class_<S> &series_class, typename std::enable_if<
			has_t_degree<S>::value && has_t_ldegree<S>::value && has_t_order<S>::value && has_t_lorder<S>::value>::type * = nullptr)
		{
			series_class.def("t_degree",wrap_t_degree<S>);
			series_class.def("t_degree",wrap_partial_t_degree<S>);
			series_class.def("t_ldegree",wrap_t_ldegree<S>);
			series_class.def("t_ldegree",wrap_partial_t_ldegree<S>);
			series_class.def("t_order",wrap_t_order<S>);
			series_class.def("t_order",wrap_partial_t_order<S>);
			series_class.def("t_lorder",wrap_t_lorder<S>);
			series_class.def("t_lorder",wrap_partial_t_lorder<S>);
		}
		template <typename S>
		static void expose_trigonometric_series(bp::class_<S> &, typename std::enable_if<
			!has_t_degree<S>::value || !has_t_ldegree<S>::value || !has_t_order<S>::value || !has_t_lorder<S>::value>::type * = nullptr)
		{}
		template <typename S>
		static auto wrap_t_degree(const S &s) -> decltype(s.t_degree())
		{
			return s.t_degree();
		}
		template <typename S>
		static auto wrap_partial_t_degree(const S &s, bp::list l) -> decltype(s.t_degree(std::set<std::string>{}))
		{
			bp::stl_input_iterator<std::string> begin(l), end;
			return s.t_degree(std::set<std::string>(begin,end));
		}
		template <typename S>
		static auto wrap_t_ldegree(const S &s) -> decltype(s.t_ldegree())
		{
			return s.t_ldegree();
		}
		template <typename S>
		static auto wrap_partial_t_ldegree(const S &s, bp::list l) -> decltype(s.t_ldegree(std::set<std::string>{}))
		{
			bp::stl_input_iterator<std::string> begin(l), end;
			return s.t_ldegree(std::set<std::string>(begin,end));
		}
		template <typename S>
		static auto wrap_t_order(const S &s) -> decltype(s.t_order())
		{
			return s.t_order();
		}
		template <typename S>
		static auto wrap_partial_t_order(const S &s, bp::list l) -> decltype(s.t_order(std::set<std::string>{}))
		{
			bp::stl_input_iterator<std::string> begin(l), end;
			return s.t_order(std::set<std::string>(begin,end));
		}
		template <typename S>
		static auto wrap_t_lorder(const S &s) -> decltype(s.t_lorder())
		{
			return s.t_lorder();
		}
		template <typename S>
		static auto wrap_partial_t_lorder(const S &s, bp::list l) -> decltype(s.t_lorder(std::set<std::string>{}))
		{
			bp::stl_input_iterator<std::string> begin(l), end;
			return s.t_lorder(std::set<std::string>(begin,end));
		}
		// Latex representation.
		template <typename S>
		static std::string wrap_latex(const S &s)
		{
			std::ostringstream oss;
			s.print_tex(oss);
			return oss.str();
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
				// Poisson bracket.
				expose_pbracket(series_class);
				// Canonical test.
				expose_canonical(series_class);
				// Filter and transform.
				series_class.def("filter",wrap_filter<s_type>);
				series_class.def("transform",wrap_transform<s_type>);
				// Trimming.
				series_class.def("trim",&s_type::trim);
				// Sin and cos.
				expose_sin_cos<s_type>();
				// Power series.
				expose_power_series(series_class);
				// Trigonometric series.
				expose_trigonometric_series(series_class);
				// Latex.
				series_class.def("_latex_",wrap_latex<s_type>);
				// Arguments set.
				series_class.add_property("symbol_set",symbol_set_wrapper<s_type>);
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
