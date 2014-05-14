.. _root:

:mod:`pyranha`
==================

Root pyranha module.

Reference
---------

.. py:module:: pyranha

.. data:: pyranha.settings

   Settings class.
.. attribute:: pyranha.settings.max_term_output

   Maximum number of terms that will be printed in the representation of series.

   >>> settings.max_term_output = 10
   >>> x = polynomial.get_type(int)('x')
   >>> print((x + 1)**10)
   1+10*x+45*x**2+120*x**3+210*x**4+252*x**5+210*x**6+120*x**7+45*x**8+10*x**9+...
   >>> print(settings.max_term_output)
   10
