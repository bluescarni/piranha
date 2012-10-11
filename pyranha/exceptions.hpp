// NOTE: the idea here is to use non-type template parameters to handle generically exceptions.
// The standard Python exceptions are pointers with external linkage and as such they are usable as non-type template
// parameters.
// http://docs.python.org/extending/extending.html#intermezzo-errors-and-exceptions
// http://stackoverflow.com/questions/1655271/explanation-of-pyapi-data-macro
// http://publib.boulder.ibm.com/infocenter/comphelp/v8v101/index.jsp?topic=%2Fcom.ibm.xlcpp8a.doc%2Flanguage%2Fref%2Ftemplate_non-type_arguments.htm

template <PyObject **PyEx,typename CppEx>
inline void generic_translator(const CppEx &cpp_ex)
{
	::PyErr_SetString(*PyEx,cpp_ex.what());
}

template <PyObject **PyEx,typename CppEx>
inline void generic_translate()
{
	bp::register_exception_translator<CppEx>(generic_translator<PyEx,CppEx>);
}

