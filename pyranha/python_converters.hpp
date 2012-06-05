// NOTE: useful resources for python converters and C API:
// - http://misspent.wordpress.com/2009/09/27/how-to-write-boost-python-converters
// - http://svn.felspar.com/public/fost-py/trunk/fost-py/Cpp/fost-python/pystring.cpp
// - http://svn.felspar.com/public/fost-py/trunk/fost-py/Cpp/fost-python/pyjson.cpp
// - http://stackoverflow.com/questions/937884/how-do-i-import-modules-in-boostpython-embedded-python-code
// - http://docs.python.org/c-api/

template <typename T>
inline void construct_from_str(PyObject *obj_ptr, converter::rvalue_from_python_stage1_data *data, const std::string &name)
{
	PyObject *str_obj = PyObject_Str(obj_ptr);
	if (!str_obj) {
		piranha_throw(std::runtime_error,std::string("unable to extract string representation of ") + name);
	}
	handle<> str_rep(str_obj);
	const char *s = PyString_AsString(str_rep.get());
	void *storage = reinterpret_cast<converter::rvalue_from_python_storage<T> *>(data)->storage.bytes;
	::new (storage) T(s);
	data->convertible = storage;
}

struct integer_from_python_int
{
	integer_from_python_int()
	{
		converter::registry::push_back(&convertible,&construct,type_id<integer>());
	}
	static void *convertible(PyObject *obj_ptr)
	{
		if (!obj_ptr || (!PyInt_CheckExact(obj_ptr) && !PyLong_CheckExact(obj_ptr))) {
			return piranha_nullptr;
		}
		return obj_ptr;
	}
	static void construct(PyObject *obj_ptr, converter::rvalue_from_python_stage1_data *data)
	{
		construct_from_str<integer>(obj_ptr,data,"integer");
	}
};

struct rational_from_python_fraction
{
	rational_from_python_fraction()
	{
		converter::registry::push_back(&convertible,&construct,type_id<rational>());
	}
	static void *convertible(PyObject *obj_ptr)
	{
		if (!obj_ptr) {
			return piranha_nullptr;
		}
		object frac_module = import("fractions");
		object frac_class = frac_module.attr("Fraction");
		if (!PyObject_IsInstance(obj_ptr,frac_class.ptr())) {
			return piranha_nullptr;
		}
		return obj_ptr;
	}
	static void construct(PyObject *obj_ptr, converter::rvalue_from_python_stage1_data *data)
	{
		construct_from_str<rational>(obj_ptr,data,"rational");
	}
};
