// NOTE: useful resources for python converters and C API:
// - http://misspent.wordpress.com/2009/09/27/how-to-write-boost-python-converters
// - http://svn.felspar.com/public/fost-py/trunk/fost-py/Cpp/fost-python/pystring.cpp
// - http://svn.felspar.com/public/fost-py/trunk/fost-py/Cpp/fost-python/pyjson.cpp
// - http://stackoverflow.com/questions/937884/how-do-i-import-modules-in-boostpython-embedded-python-code
// - http://docs.python.org/c-api/

template <typename T>
inline void construct_from_str(::PyObject *obj_ptr, bp::converter::rvalue_from_python_stage1_data *data, const std::string &name)
{
	piranha_assert(obj_ptr);
	::PyObject *str_obj = ::PyObject_Str(obj_ptr);
	if (!str_obj) {
		piranha_throw(std::runtime_error,std::string("unable to extract string representation of ") + name);
	}
	bp::handle<> str_rep(str_obj);
#if PY_MAJOR_VERSION < 3
	const char *s = ::PyString_AsString(str_rep.get());
#else
	::PyObject *unicode_str_obj = ::PyUnicode_AsEncodedString(str_rep.get(),"ascii","strict");
	if (!unicode_str_obj) {
		piranha_throw(std::runtime_error,std::string("unable to extract string representation of ") + name);
	}
	bp::handle<> unicode_str(unicode_str_obj);
	const char *s = ::PyBytes_AsString(unicode_str.get());
	if (!s) {
		::PyErr_Clear();
		piranha_throw(std::runtime_error,std::string("unable to extract string representation of ") + name);
	}
#endif
	void *storage = reinterpret_cast<bp::converter::rvalue_from_python_storage<T> *>(data)->storage.bytes;
	::new (storage) T(s);
	data->convertible = storage;
}

struct integer_converter
{
	integer_converter()
	{
		bp::to_python_converter<integer,to_python>();
		bp::converter::registry::push_back(&convertible,&construct,bp::type_id<integer>());
	}
	struct to_python
	{
		static ::PyObject *convert(const integer &n)
		{
			// NOTE: use PyLong_FromString here instead?
			const std::string str = boost::lexical_cast<std::string>(n);
#if PY_MAJOR_VERSION < 3
			bp::object bi_module = bp::import("__builtin__");
#else
			bp::object bi_module = bp::import("builtins");
#endif
			bp::object int_class = bi_module.attr("int");
			return bp::incref(int_class(str).ptr());
		}
	};
	static void *convertible(::PyObject *obj_ptr)
	{
		if (!obj_ptr || (
#if PY_MAJOR_VERSION < 3
			!PyInt_CheckExact(obj_ptr) &&
#endif
			!PyLong_CheckExact(obj_ptr))) {
			return nullptr;
		}
		return obj_ptr;
	}
	static void construct(::PyObject *obj_ptr, bp::converter::rvalue_from_python_stage1_data *data)
	{
		construct_from_str<integer>(obj_ptr,data,"integer");
	}
};

struct rational_converter
{
	rational_converter()
	{
		bp::to_python_converter<rational,to_python>();
		bp::converter::registry::push_back(&convertible,&construct,bp::type_id<rational>());
	}
	struct to_python
	{
		static ::PyObject *convert(const rational &q)
		{
			const std::string str = boost::lexical_cast<std::string>(q);
			bp::object frac_module = bp::import("fractions");
			bp::object frac_class = frac_module.attr("Fraction");
			return bp::incref(frac_class(str).ptr());
		}
	};
	static void *convertible(::PyObject *obj_ptr)
	{
		if (!obj_ptr) {
			return nullptr;
		}
		bp::object frac_module = bp::import("fractions");
		bp::object frac_class = frac_module.attr("Fraction");
		if (!::PyObject_IsInstance(obj_ptr,frac_class.ptr())) {
			return nullptr;
		}
		return obj_ptr;
	}
	static void construct(::PyObject *obj_ptr, bp::converter::rvalue_from_python_stage1_data *data)
	{
		construct_from_str<rational>(obj_ptr,data,"rational");
	}
};

struct real_converter
{
	real_converter()
	{
		bp::to_python_converter<real,to_python>();
		bp::converter::registry::push_back(&convertible,&construct,bp::type_id<real>());
	}
	struct to_python
	{
		static PyObject *convert(const real &r)
		{
			bp::object str(boost::lexical_cast<std::string>(r));
			try {
				bp::object mpmath = bp::import("mpmath");
				bp::object mpf = mpmath.attr("mpf");
				return bp::incref(mpf(str).ptr());
			} catch (...) {
				// NOTE: here it seems like in case of import errors, Boost.Python both throws and
				// sets the Python exception. Clear it and just throw pure C++.
				::PyErr_Clear();
				piranha_throw(std::runtime_error,"could not convert real number to mpf object - please check the installation of mpmath");
			}
		}
	};
	static void *convertible(::PyObject *obj_ptr)
	{
		if (!obj_ptr) {
			return nullptr;
		}
		try {
			// NOTE: here we want to ignore errors when mpmath is missing, and just report that
			// the object is not convertible.
			bp::object mpmath = bp::import("mpmath");
			bp::object mpf = mpmath.attr("mpf");
			if (!::PyObject_IsInstance(obj_ptr,mpf.ptr())) {
				return nullptr;
			}
		} catch (...) {
			::PyErr_Clear();
			return nullptr;
		}
		return obj_ptr;
	}
	static void construct(::PyObject *obj_ptr, bp::converter::rvalue_from_python_stage1_data *data)
	{
		// NOTE: here we cannot construct directly from string, as we need to query the precision.
		piranha_assert(obj_ptr);
		// NOTE: here the handle is from borrowed because we are not responsible for the generation of obj_ptr.
		bp::handle<> obj_handle(bp::borrowed(obj_ptr));
		bp::object obj(obj_handle);
		const ::mpfr_prec_t prec = boost::numeric_cast< ::mpfr_prec_t>(
			static_cast<long>(bp::extract<long>(obj.attr("context").attr("prec"))));
		::PyObject *str_obj = ::PyObject_Repr(obj_ptr);
		if (!str_obj) {
			piranha_throw(std::runtime_error,std::string("unable to extract string representation of real"));
		}
		bp::handle<> str_rep(str_obj);
#if PY_MAJOR_VERSION < 3
		const char *s = ::PyString_AsString(str_rep.get());
#else
		::PyObject *unicode_str_obj = ::PyUnicode_AsEncodedString(str_rep.get(),"ascii","strict");
		if (!unicode_str_obj) {
			piranha_throw(std::runtime_error,std::string("unable to extract string representation of real"));
		}
		bp::handle<> unicode_str(unicode_str_obj);
		const char *s = ::PyBytes_AsString(unicode_str.get());
		if (!s) {
			::PyErr_Clear();
			piranha_throw(std::runtime_error,std::string("unable to extract string representation of real"));
		}
#endif
		// NOTE: the search for "'" is due to the string format of mpmath.mpf objects.
		while (*s != '\0' && *s != '\'') {
			++s;
		}
		if (s == '\0') {
			piranha_throw(std::runtime_error,std::string("invalid string input converting to real"));
		}
		++s;
		auto start = s;
		while (*s != '\0' && *s != '\'') {
			++s;
		}
		if (s == '\0') {
			piranha_throw(std::runtime_error,std::string("invalid string input converting to real"));
		}
		void *storage = reinterpret_cast<bp::converter::rvalue_from_python_storage<real> *>(data)->storage.bytes;
		::new (storage) real(std::string(start,s),prec);
		data->convertible = storage;
	}
};
