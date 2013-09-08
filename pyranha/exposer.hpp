template <template <typename ...> class Series, typename Descriptor>
class exposer
{
		using params = typename Descriptor::params;
	public:
		explicit exposer(const std::string &name)
		{

		}
		exposer() = delete;
		exposer(const exposer &) = delete;
		exposer(exposer &&) = delete;
		exposer &operator=(const exposer &) = delete;
		exposer &operator=(exposer &&) = delete;
		~exposer() = default;
};
