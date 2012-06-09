inline void bnc_translator(const boost::numeric::bad_numeric_cast &bnc)
{
	PyErr_SetString(PyExc_OverflowError,bnc.what());
}

inline void po_translator(const boost::numeric::positive_overflow &po)
{
	PyErr_SetString(PyExc_OverflowError,po.what());
}

inline void no_translator(const boost::numeric::negative_overflow &no)
{
	PyErr_SetString(PyExc_OverflowError,no.what());
}

inline void oe_translator(const std::overflow_error &oe)
{
	PyErr_SetString(PyExc_OverflowError,oe.what());
}

inline void zde_translator(const zero_division_error &zde)
{
	PyErr_SetString(PyExc_ZeroDivisionError,zde.what());
}
