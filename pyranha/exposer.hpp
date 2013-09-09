template <template <typename ...> class Series, typename Descriptor>
class exposer
{
		using params = typename Descriptor::params;
		struct exposer_op
		{
			template <typename ... Args>
			void operator()(const std::tuple<Args...> &) const
			{
				Series<typename Args::type ...> s;
			}
		};
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
		/*template <typename Tuple, std::size_t Idx = 0u>
		static auto extract_types(const Tuple &, typename std::enable_if<Idx != std::tuple_size<Tuple>::value>::type * = nullptr)
		{
			return std::tuple_cat(std::declval<typename std::tuple_type<Idx,Tuple>::type::type>())
		}*/
	public:
		explicit exposer(const std::string &name):m_name(name)
		{
			if (unlikely(series_archive.find(name) != series_archive.end())) {
				piranha_throw(std::runtime_error,"trying to expose the same series twice");
			}
			series_archive[name] = std::set<std::vector<std::string>>{};
			params p;
			tuple_for_each(p,exposer_op{});
			//series_archive.insert(std::make_pair(name),tuple_reduce<params>());
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
