// NOTE: useful resources for python converters and C API:
// - http://misspent.wordpress.com/2009/09/27/how-to-write-boost-python-converters
// - http://svn.felspar.com/public/fost-py/trunk/fost-py/Cpp/fost-python/pystring.cpp
// - http://svn.felspar.com/public/fost-py/trunk/fost-py/Cpp/fost-python/pyjson.cpp
// - http://stackoverflow.com/questions/937884/how-do-i-import-modules-in-boostpython-embedded-python-code
// - http://docs.python.org/c-api/

template <typename T>
inline void construct_from_str(PyObject *obj_ptr, bp::converter::rvalue_from_python_stage1_data *data, const std::string &name)
{
	PyObject *str_obj = PyObject_Str(obj_ptr);
	if (!str_obj) {
		piranha_throw(std::runtime_error,std::string("unable to extract string representation of ") + name);
	}
	bp::handle<> str_rep(str_obj);
	const char *s = PyString_AsString(str_rep.get());
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
		static PyObject *convert(const integer &n)
		{
			// TODO: use PyLong_FromString here instead?
			const std::string str = boost::lexical_cast<std::string>(n);
			bp::object bi_module = bp::import("__builtin__");
			bp::object int_class = bi_module.attr("int");
			return bp::incref(int_class(str).ptr());
		}
	};
	static void *convertible(PyObject *obj_ptr)
	{
		// TODO: do not convert if input object fits a long, let Boost.Python handle that case.
		// Use the function PyLong_AsLongAndOverflow for the job.
		if (!obj_ptr || (!PyInt_CheckExact(obj_ptr) && !PyLong_CheckExact(obj_ptr))) {
			return piranha_nullptr;
		}
		return obj_ptr;
	}
	static void construct(PyObject *obj_ptr, bp::converter::rvalue_from_python_stage1_data *data)
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
		static PyObject *convert(const rational &q)
		{
			const std::string str = boost::lexical_cast<std::string>(q);
			bp::object frac_module = bp::import("fractions");
			bp::object frac_class = frac_module.attr("Fraction");
			return bp::incref(frac_class(str).ptr());
		}
	};
	static void *convertible(PyObject *obj_ptr)
	{
		if (!obj_ptr) {
			return piranha_nullptr;
		}
		bp::object frac_module = bp::import("fractions");
		bp::object frac_class = frac_module.attr("Fraction");
		if (!PyObject_IsInstance(obj_ptr,frac_class.ptr())) {
			return piranha_nullptr;
		}
		return obj_ptr;
	}
	static void construct(PyObject *obj_ptr, bp::converter::rvalue_from_python_stage1_data *data)
	{
		construct_from_str<rational>(obj_ptr,data,"rational");
	}
};
