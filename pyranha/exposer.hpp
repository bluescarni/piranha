template <template <typename ...> class Series, typename Descriptor>
class exposer
{
		using params = typename Descriptor::params;
		// Detect the presence of interoperable types.
		PIRANHA_DECLARE_HAS_TYPEDEF(interop_types);
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
		// Turn template parameters into a list of strings, via their p_descriptor<>::name members.
		template <typename Arg0, typename ... Args>
		static void stringify_params(std::vector<std::string> &params_list, typename std::enable_if<sizeof...(Args) != 0u>::type * = nullptr)
		{
			params_list.push_back(p_descriptor<Arg0>::name);
			stringify_params<Args...>(params_list);
		}
		template <typename Arg0, typename ... Args>
		static void stringify_params(std::vector<std::string> &params_list, typename std::enable_if<sizeof...(Args) == 0u>::type * = nullptr)
		{
			params_list.push_back(p_descriptor<Arg0>::name);
		}
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
		// Interaction with interoperable types.
		template <typename S, std::size_t I = 0u, typename... T>
		void interop_exposer_(bp::class_<S> &series_class, const std::tuple<T...> &,
			typename std::enable_if<I == sizeof...(T)>::type * = nullptr) const
		{
			// Add interoperability with coefficient type if it is not already included in
			// the interoperable types.
			if (!type_in_tuple<typename S::term_type::cf_type,std::tuple<T...>>::value) {
				typename S::term_type::cf_type cf;
				interop_exposer_(series_class,std::make_tuple(cf));
			}
		}
		template <typename S, std::size_t I = 0u, typename... T>
		void interop_exposer_(bp::class_<S> &series_class, const std::tuple<T...> &t,
			typename std::enable_if<(I < sizeof...(T))>::type * = nullptr) const
		{
			namespace sn = boost::python::self_ns;
			using interop_type = typename std::tuple_element<I,std::tuple<T...>>::type;
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
			/*pow_exposer(series_class,in);*/
			// Evaluation.
			/*series_class.def("_evaluate",wrap_evaluate<S,interop_type>);*/
			// Substitutions.
			/*series_class.def("subs",&S::template subs<interop_type>);
			series_class.def("ipow_subs",&S::template ipow_subs<interop_type>);
			t_subs_exposer<interop_type>(series_class);
			interop_exposer<S,I + 1u,T...>(series_class,t);*/
		}
		template <typename S>
		struct interop_exposer
		{
			interop_exposer(bp::class_<S> &series_class):m_series_class(series_class) {}
			template <typename T>
			void operator()(const T &) const
			{
				expose_ctor<const T &>(m_series_class);
				//expose_arithmetics<T>(m_series_class);
			}
			bp::class_<S> &m_series_class;
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
		// Main exposer.
		struct exposer_op
		{
			explicit exposer_op(const std::string &name):m_name(name) {}
			std::string create_class_name(const std::vector<std::string> &params_list) const
			{
				return std::string("_") + m_name + std::accumulate(params_list.begin(),params_list.end(),std::string(""),
					[](const std::string &a, const std::string &b) {return a + "___" + b;});
			}
			template <typename ... Args>
			void operator()(const std::tuple<Args...> &) const
			{
				using s_type = Series<Args...>;
				piranha_assert(series_archive.find(m_name) != series_archive.end());
				// Transform the pack of template parameters into a list of strings representing them.
				std::vector<std::string> params_list;
				stringify_params<Args...>(params_list);
				// Check that the params list is not there already, and insert it.
				if (unlikely(series_archive[m_name].find(params_list) != series_archive[m_name].end())) {
					piranha_throw(std::runtime_error,"trying to expose the same set of template parameters for the same series type twice");
				}
				auto i_retval = series_archive[m_name].insert(params_list);
				piranha_assert(i_retval.second);
				(void)i_retval;
				// Start exposing.
				bp::class_<s_type> series_class(create_class_name(params_list).c_str(),bp::init<>());
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
			}
			const std::string &m_name;
		};
	public:
		explicit exposer(const std::string &name):m_name(name)
		{
			// Check that series name is a valid identifier.
			check_name(name);
			// We don't want to split the exposition of a series in multiple instances of this class. Just assume
			// we are exposing all series of the same root type at a time.
			if (unlikely(series_archive.find(m_name) != series_archive.end())) {
				piranha_throw(std::runtime_error,"trying to expose the same series twice");
			}
			series_archive[m_name] = std::set<std::vector<std::string>>{};
			params p;
			tuple_for_each(p,exposer_op{m_name});
		}
		// Delete all other ctors and assignment operators, keep only the destructor.
		exposer() = delete;
		exposer(const exposer &) = delete;
		exposer(exposer &&) = delete;
		exposer &operator=(const exposer &) = delete;
		exposer &operator=(exposer &&) = delete;
		~exposer() = default;
	private:
		const std::string m_name;
};
