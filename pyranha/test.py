# -*- coding: iso-8859-1 -*-
# Copyright 2009-2016 Francesco Biscani (bluescarni@gmail.com)
#
# This file is part of the Piranha library.
#
# The Piranha library is free software; you can redistribute it and/or modify
# it under the terms of either:
#
#  * the GNU Lesser General Public License as published by the Free
#    Software Foundation; either version 3 of the License, or (at your
#    option) any later version.
#
# or
#
#  * the GNU General Public License as published by the Free Software
#    Foundation; either version 3 of the License, or (at your option) any
#    later version.
#
# or both in parallel, as here.
#
# The Piranha library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received copies of the GNU General Public License and the
# GNU Lesser General Public License along with the Piranha library.  If not,
# see https://www.gnu.org/licenses/.

from __future__ import absolute_import as _ai

import unittest as _ut


# s11n tests for save/load file.
def _s11n_load_save_test(self, p):
    import tempfile
    import os
    import shutil
    from . import load_file, save_file, data_format as df, compression as comp
    for form in [df.boost_portable, df.boost_binary, df.msgpack_portable, df.msgpack_binary]:
        for c in [comp.none, comp.bzip2, comp.gzip, comp.zlib]:
            f = tempfile.NamedTemporaryFile(delete=False)
            f.close()
            try:
                save_file(p, f.name, form, c)
                ret = type(p)()
                load_file(ret, f.name, form, c)
                self.assertEqual(ret, p)
            except NotImplementedError:
                pass
            finally:
                os.remove(f.name)
    # Deduce from filename.
    temp_dir = tempfile.mkdtemp()
    try:
        for suff in ['.boostb', '.boostp', '.mpackb', '.mpackp']:
            for comp in ['', '.bz2', '.zip', '.gz']:
                filename = os.path.join(temp_dir, 'foo' + suff + comp)
                save_file(p, filename)
                ret = type(p)()
                load_file(ret, filename)
                self.assertEqual(ret, p)
    except NotImplementedError:
        pass
    finally:
        shutil.rmtree(temp_dir)
    self.assertRaises(ValueError, lambda: save_file(p, "foo"))
    self.assertRaises(ValueError, lambda: load_file(p, "foo"))
    # Input args checking.
    self.assertRaisesRegexp(
        TypeError, "the file name must be a string", lambda: save_file(p, 123))
    self.assertRaisesRegexp(
        ValueError, "the data format was provided but the compression format was not", lambda: save_file(p, "foo", df=1))
    self.assertRaisesRegexp(
        ValueError, "the compression format was provided but the data format was not", lambda: save_file(p, "foo", cf=1, df=None))

# Helper to check that pickle roundtrips.


def _pickle_test(self, x):
    import pickle
    str_rep = pickle.dumps(x)
    self.assertEqual(x, pickle.loads(str_rep))


class basic_test_case(_ut.TestCase):
    """Basic test case.

    To be used within the :mod:`unittest` framework. Will test features common
    to all series types.

    >>> import unittest as ut
    >>> suite = ut.TestLoader().loadTestsFromTestCase(basic_test_case)

    """

    def runTest(self):
        from fractions import Fraction
        from copy import copy, deepcopy
        # Test version number.
        import pyranha
        self.assertTrue(pyranha.__version__)
        # Use polynomial for testing.
        from .types import polynomial, integer, rational, int16, double, monomial
        # Arithmetic with int and Fraction, len, str and comparisons.
        for s, t in [(integer, int), (rational, Fraction)]:
            tp = polynomial[s, monomial[int16]]()
            self.assertEqual(tp('x') - tp('x'), t(1) - t(1))
            self.assertEqual(tp(tp('x')) - tp('x'), t(1) - t(1))
            self.assertEqual(tp(), t(0))
            self.assertEqual(tp(t(2)), t(2))
            self.assertEqual(tp('x'), +tp('x'))
            foo = tp(t(1))
            foo += t(2)
            self.assertEqual(foo, t(1) + t(2))
            self.assertEqual(tp(1) + tp(2), t(1) + t(2))
            self.assertEqual(tp() + t(2), t(0) + t(2))
            self.assertEqual(t(2) + tp(), t(0) + t(2))
            self.assertEqual(tp('x') * t(-1), -tp('x'))
            foo -= t(4)
            self.assertEqual(foo, t(3) - t(4))
            self.assertEqual(tp(1) - tp(2), t(1) - t(2))
            self.assertEqual(tp() - t(2), t(0) - t(2))
            self.assertEqual(t(2) - tp(), t(2) - t(0))
            self.assertEqual(tp('x'), -(-tp('x')))
            foo *= t(-5)
            self.assertEqual(foo, t(-1) * t(-5))
            self.assertEqual(tp(1) * tp(2), t(1) * t(2))
            self.assertEqual(tp(1) * t(2), t(1) * t(2))
            self.assertEqual(t(2) * tp(1), t(2) * t(1))
            self.assertNotEqual(repr(tp('x')), '')
            self.assertNotEqual(str(tp('x')), '')
            self.assertEqual(len(tp('x') + 1), 2)
            self.assertTrue(tp('x') != tp('y'))
            self.assertTrue(tp('x') != t(1))
            self.assertTrue(t(1) != tp('x'))
            self.assertTrue(tp('x') == tp('x'))
            self.assertTrue(tp(t(1)) == t(1))
            self.assertTrue(t(1) == tp(t(1)))
            self.assertTrue(tp('x') ** 3 == tp('x') * tp('x') * tp('x'))
            # A couple of trimming tests.
            x, y, z = [tp(_) for _ in "xyz"]
            self.assertEqual((x + y + z).symbol_set, ['x', 'y', 'z'])
            self.assertEqual((x + y + z - y - z).symbol_set, ['x', 'y', 'z'])
            self.assertEqual((x + y + z - y - z).trim().symbol_set, ['x'])
        tp_int = polynomial[integer, monomial[int16]]()
        self.assertRaises(ValueError, tp_int, float('inf'))
        self.assertRaises(ZeroDivisionError, lambda: tp_int() ** -1)
        tp_q = polynomial[rational, monomial[int16]]()
        self.assertRaises(ZeroDivisionError,
                          lambda: tp_q(Fraction(0, 1)) ** -1)
        self.assertEqual(tp_q(Fraction(1, 3)) ** -2, 9)
        self.assertRaises(ZeroDivisionError,
                          lambda: tp_int(Fraction(1, 3)) ** -2)
        tp_f = polynomial[double, monomial[int16]]()
        # NOTE: here we are going to assume that Python's float implementation uses C++ doubles and
        # the corresponding pow() function.
        self.assertEqual(tp_f(0.1) ** (0.5), 0.1**0.5)
        q1 = tp_q(Fraction(1, 6))
        self.assertEqual(q1 ** 20, Fraction(2, 12) ** 20)
        self.assertEqual(Fraction(1, 3) ** 100000,
                         tp_q(Fraction(1, 3)) ** 100000)
        # Copy and deepcopy.
        s1 = tp_int(2)
        self.assertNotEqual(id(copy(s1)), s1)
        self.assertEqual(copy(s1), s1)
        self.assertNotEqual(id(deepcopy(s1)), s1)
        self.assertEqual(deepcopy(s1), s1)
        # Latex renderer, if available.
        x = tp_int('x')
        tmp = x._repr_png_()
        self.assertTrue(tmp is None or len(tmp) != 0)
        # Evaluation.
        from .math import evaluate
        x = tp_q('x')
        self.assertEqual(evaluate(x, {'x': 3}), 3)
        self.assertEqual(evaluate(2 * x, {'x': Fraction(3, 2)}), Fraction(3))
        # Test exception translation.
        # NOTE: the msgpack exception translation is tested elsewhere.
        from ._core import _test_safe_cast_failure, _test_zero_division_error, _test_not_implemented_error, _test_overflow_error, \
            _test_bn_poverflow_error, _test_bn_noverflow_error, _test_bn_bnc, _test_inexact_division
        self.assertRaisesRegexp(
            ValueError, "hello world", _test_safe_cast_failure)
        self.assertRaisesRegexp(
            ZeroDivisionError, "hello world", _test_zero_division_error)
        self.assertRaisesRegexp(
            NotImplementedError, "hello world", _test_not_implemented_error)
        self.assertRaisesRegexp(
            OverflowError, "hello world", _test_overflow_error)
        self.assertRaisesRegexp(
            OverflowError, "positive overflow", _test_bn_poverflow_error)
        self.assertRaisesRegexp(
            OverflowError, "negative overflow", _test_bn_noverflow_error)
        self.assertRaisesRegexp(
            OverflowError, "overflow", _test_bn_bnc)
        self.assertRaisesRegexp(
            ArithmeticError, "inexact division", _test_inexact_division)


class series_division_test_case(_ut.TestCase):
    """Series division test case.

    To be used within the :mod:`unittest` framework. Will test series division.

    >>> import unittest as ut
    >>> suite = ut.TestLoader().loadTestsFromTestCase(series_division_test_case)

    """

    def runTest(self):
        from fractions import Fraction as F
        from .types import poisson_series, polynomial, monomial, int16, rational, double
        pt = poisson_series[polynomial[rational, monomial[int16]]]()
        pt2 = poisson_series[polynomial[double, monomial[int16]]]()
        x, y, z = [pt(_) for _ in 'xyz']
        self.assertEqual(pt(4) / pt(3), F(4, 3))
        self.assertEqual(type(pt(4) / pt(3)), pt)
        self.assertEqual(pt(4) / 2, 2)
        self.assertEqual(type(pt(4) / 2), pt)
        self.assertEqual(1 / pt(2), F(1, 2))
        self.assertEqual(type(1 / pt(2)), pt)
        self.assertEqual(1. / pt(2), 1. / 2.)
        # We don't support mixed operations among series types.
        self.assertRaises(TypeError, lambda: type(pt(2) / pt2(1)))
        self.assertEqual(type(1. / pt(2)), pt2)
        self.assertEqual(type(pt(2) / 1.), pt2)
        self.assertEqual((x**2 - 1) / (x + 1), x - 1)
        self.assertEqual(type((x**2 - 1) / (x + 1)), pt)
        self.assertRaises(ZeroDivisionError, lambda: x / 0)
        self.assertRaises(ArithmeticError, lambda: x / y)
        tmp = pt(4)
        tmp /= 4
        self.assertEqual(tmp, 1)


class custom_derivatives_test_case(_ut.TestCase):
    """Test case for custom derivatives in series.

    To be used within the :mod:`unittest` framework. Will check that the custom
    derivatives machinery works properly.

    >>> import unittest as ut
    >>> suite = ut.TestLoader().loadTestsFromTestCase(custom_derivatives_test_case)

    """

    def runTest(self):
        from .types import polynomial, monomial, int16, rational
        from .math import partial
        pt = polynomial[rational, monomial[int16]]()
        x = pt('x')
        # A custom derivative functor with a state,
        # used to check we actually deepcopy it.

        class cd(object):

            def __init__(self):
                self.value = 1

            def __call__(self, p):
                return pt(self.value)

            def set_value(self, value):
                self.value = value
        c = cd()
        pt.register_custom_derivative('x', c)
        pt.register_custom_derivative('y', c)
        self.assertEqual(partial(x, 'x'), 1)
        c.set_value(42)
        self.assertEqual(partial(x, 'x'), 1)
        pt.unregister_custom_derivative('x')
        pt.unregister_all_custom_derivatives()


class series_in_place_ops_test_case(_ut.TestCase):
    """Series in-place operations test case.

    To be used within the :mod:`unittest` framework. Will test in-place series arithmetics.

    >>> import unittest as ut
    >>> suite = ut.TestLoader().loadTestsFromTestCase(series_in_place_ops_test_case)

    """

    def runTest(self):
        # The idea of the test is to make sure that in-place operations
        # really mutate the object (and do not end up creating a new object
        # instead).
        from .types import polynomial, monomial, int16, rational
        pt = polynomial[rational, monomial[int16]]()
        x = pt('x')
        x0 = pt('x')
        # Test with scalar.
        x_id = id(x)
        x += 1
        self.assertEqual(x, x0 + 1)
        self.assertEqual(id(x), x_id)
        x -= 2
        self.assertEqual(x, x0 - 1)
        self.assertEqual(id(x), x_id)
        x *= 2
        self.assertEqual(x, 2 * x0 - 2)
        self.assertEqual(id(x), x_id)
        x /= 2
        self.assertEqual(x, x0 - 1)
        self.assertEqual(id(x), x_id)
        # Test with series.
        x, y = pt('x'), pt('y')
        x_id = id(x)
        x += y
        self.assertEqual(x, x0 + y)
        self.assertEqual(id(x), x_id)
        x -= y
        self.assertEqual(x, x0)
        self.assertEqual(id(x), x_id)
        x *= y
        self.assertEqual(x, x0 * y)
        self.assertEqual(id(x), x_id)
        x /= pt(2)
        self.assertEqual(x, x0 * y / 2)
        self.assertEqual(id(x), x_id)


class mpmath_test_case(_ut.TestCase):
    """:mod:`mpmath` test case.

    To be used within the :mod:`unittest` framework. Will test interoperability between
    the :mod:`mpmath` library and the C++ *real* class. If the :mod:`mpmath` library is not available, the
    test will return immediately.

    >>> import unittest as ut
    >>> suite = ut.TestLoader().loadTestsFromTestCase(mpmath_test_case)

    """

    def runTest(self):
        try:
            from mpmath import workdps, mpf, pi
        except ImportError:
            return
        from .types import polynomial, integer, int16, monomial
        from .math import evaluate
        pt = polynomial[integer, monomial[int16]]()
        x = pt('x')
        self.assertEqual(evaluate(x,{'x':mpf('4.5667')}), mpf('4.5667'))
        self.assert_(type(evaluate(x,{'x':mpf('4.5667')})) == mpf)
        for n in [11, 21, 51, 101, 501]:
            with workdps(n):
                self.assertEqual(evaluate(x,{'x':mpf('4.5667')}), mpf('4.5667'))
                self.assert_(type(evaluate(x,{'x':mpf('4.5667')})) == mpf)


class math_test_case(_ut.TestCase):
    """:mod:`math` module test case.

    To be used within the :mod:`unittest` framework. Will test the functions implemented in the
    :mod:`math` module.

    >>> import unittest as ut
    >>> suite = ut.TestLoader().loadTestsFromTestCase(math_test_case)

    """

    def runTest(self):
        import math
        from .math import cos as pcos, sin as psin
        from .types import polynomial, k_monomial, double, real
        # NOTE: according to
        # https://docs.python.org/3/library/math.html
        # the CPython math functions are wrappers around the corresponding
        # C functions. The results should thus be roughly the same.
        self.assertAlmostEqual(math.cos(3.), pcos(3.))
        self.assertAlmostEqual(math.cos(3.1234), pcos(3.1234))
        self.assertAlmostEqual(math.sin(3.), psin(3.))
        self.assertAlmostEqual(math.sin(3.1234), psin(3.1234))
        pt = polynomial[double, k_monomial]()
        self.assertAlmostEqual(math.cos(3), pcos(pt(3)).list[0][0])
        self.assertAlmostEqual(math.cos(2.456), pcos(pt(2.456)).list[0][0])
        self.assertAlmostEqual(math.sin(3), psin(pt(3)).list[0][0])
        self.assertAlmostEqual(math.sin(-2.456), psin(pt(-2.456)).list[0][0])
        self.assertRaises(TypeError, lambda: pcos(""))
        self.assertRaises(TypeError, lambda: psin(""))
        self.binomialTest()
        self.sincosTest()
        self.evaluateTest()
        self.subsTest()
        self.invertTest()
        self.lambdifyTest()

    def binomialTest(self):
        from fractions import Fraction as F
        from .math import binomial
        # Check the return types of binomial.
        self.assertEqual(type(binomial(5, 4)), int)
        self.assertEqual(type(binomial(5, 4.)), float)
        self.assertEqual(type(binomial(5, F(4))), float)
        self.assertEqual(type(binomial(5., 4)), float)
        self.assertEqual(type(binomial(5., 4.)), float)
        self.assertEqual(type(binomial(5., F(4))), float)
        self.assertEqual(type(binomial(F(5), 4)), F)
        self.assertEqual(type(binomial(F(5), 4.)), float)
        self.assertEqual(type(binomial(F(5), F(4))), float)
        try:
            from mpmath import mpf
        except ImportError:
            return
        self.assertEqual(type(binomial(mpf(5), mpf(4))), mpf)
        self.assertEqual(type(binomial(5, mpf(4))), mpf)
        self.assertEqual(type(binomial(5., mpf(4))), mpf)
        self.assertEqual(type(binomial(F(5), mpf(4))), mpf)
        self.assertEqual(type(binomial(mpf(5), 4)), mpf)
        self.assertEqual(type(binomial(mpf(5), 4.)), mpf)
        self.assertEqual(type(binomial(mpf(5), F(4))), mpf)

    def sincosTest(self):
        from fractions import Fraction as F
        from .math import sin, cos
        # Check the return types.
        self.assertEqual(type(cos(0)), int)
        self.assertEqual(type(sin(0)), int)
        self.assertEqual(type(cos(F(0))), F)
        self.assertEqual(type(sin(F(0))), F)
        self.assertEqual(type(cos(1.)), float)
        self.assertEqual(type(sin(1.)), float)
        try:
            from mpmath import mpf
        except ImportError:
            return
        self.assertEqual(type(cos(mpf(1))), mpf)
        self.assertEqual(type(sin(mpf(1))), mpf)

    def evaluateTest(self):
        from fractions import Fraction as F
        from .math import evaluate
        from .types import polynomial, k_monomial, double, integer, real, rational
        pt = polynomial[double, k_monomial]()
        x, y, z = pt('x'), pt('y'), pt('z')
        self.assertEqual(evaluate(3 * x * y, {'x': 4, 'y': 5}), 3. * (4. * 5.))
        self.assertEqual(type(evaluate(3 * x * y, {'x': 4, 'y': 5})), float)
        self.assertEqual(
            evaluate(x**2 * y**3 / 5, {'x': 4, 'y': 5}), (4.**2 * 5.**3) / 5)
        pt = polynomial[rational, k_monomial]()
        x, y, z = pt('x'), pt('y'), pt('z')
        self.assertEqual(
            evaluate(3 * x * y, {'x': F(4), 'y': F(5)}), 3 * (F(4) * F(5)))
        self.assertEqual(type(evaluate(3 * x * y, {'x': F(4), 'y': F(5)})), F)
        self.assertEqual(
            evaluate(x**2 * y**3 / 5, {'x': F(4), 'y': F(5)}), (F(4)**2 * F(5)**3) / 5)
        pt = polynomial[integer, k_monomial]()
        x, y, z = pt('x'), pt('y'), pt('z')
        self.assertEqual(evaluate(3 * x * y, {'x': 4, 'y': 5}), 3 * (4 * 5))
        self.assertEqual(type(evaluate(3 * x * y, {'x': 4, 'y': 5})), int)
        # Integer division truncates.
        self.assertEqual(evaluate(x**2 * y**3 / 5, {'x': 2, 'y': 1}), 0)
        # Check errors.
        self.assertRaises(ValueError, lambda: evaluate(pt(0), {}))
        self.assertRaises(TypeError, lambda: evaluate(
            x**2 * y**3 / 5, {'x': 2., 'y': 1}))
        self.assertRaises(TypeError, lambda: evaluate(
            x**2 * y**3 / 5, {2: 2, 'y': 1}))
        self.assertRaises(TypeError, lambda: evaluate(x**2 * y**3 / 5, [1, 2]))
        # A missing symbol, thrown from C++.
        self.assertRaises(ValueError, lambda: evaluate(
            3 * x * y, {'x': 4, 'z': 5}))
        try:
            from mpmath import mpf
        except ImportError:
            return
        pt = polynomial[double, k_monomial]()
        x, y, z = pt('x'), pt('y'), pt('z')
        self.assertEqual(
            evaluate(3 * x * y, {'x': mpf(4), 'y': mpf(5)}), mpf(3.) * (4. * 5.))
        self.assertEqual(
            type(evaluate(3 * x * y, {'x': mpf(4), 'y': mpf(5)})), mpf)
        self.assertEqual(
            evaluate(x**2 * y**3 / 5, {'x': mpf(4), 'y': mpf(5)}), (mpf(4)**2 * 5.**3) / 5)

    def subsTest(self):
        from fractions import Fraction as F
        from .math import subs, ipow_subs, t_subs, cos, sin
        from .types import poisson_series, polynomial, k_monomial, rational, double, real
        pt = poisson_series[polynomial[rational, k_monomial]]()
        x, y, z = pt('x'), pt('y'), pt('z')
        # Normal subs().
        self.assertEqual(subs(z * cos(x + y), 'x', 0), z * cos(y))
        # Make sure that substitution with int does not trigger any conversion
        # to floating point.
        self.assertEqual(type(subs(z * cos(x + y), 'x', 0)), pt)
        # Trigger a floating-point conversion.
        self.assertEqual(type(subs(z * cos(x + y), 'x', 0.)),
                         poisson_series[polynomial[double, k_monomial]]())
        # Trig subs.
        self.assertEqual(t_subs(z * sin(x + y), 'y', 0, 1), z * cos(x))
        self.assertEqual(type(t_subs(z * sin(x + y), 'y', 0, 1)), pt)
        n, m = 2, 3
        c, s = F(n**2 - m**2, n**2 + m**2), F(n * m, n**2 + m**2)
        self.assertEqual(t_subs(z * sin(x + y), 'y', c, s),
                         z * sin(x) * c + z * cos(x) * s)
        self.assertEqual(type(t_subs(z * sin(x + y), 'y', c, s)), pt)
        self.assertEqual(type(t_subs(z * sin(x + y), 'y', 0., 1.)),
                         poisson_series[polynomial[double, k_monomial]]())
        # Ipow subs.
        self.assertEqual(ipow_subs(x**5 * y**2 * z / 5,
                                   'x', 2, 3), 9 * x * y**2 * z / 5)
        self.assertEqual(type(ipow_subs(x**5 * y**2 * z / 5, 'x', 2, 3)), pt)
        self.assertEqual(ipow_subs(x**6 * y**2 * z / 5,
                                   'x', 2, 3), 27 * y**2 * z / 5)
        self.assertEqual(type(ipow_subs(x**5 * y**2 * z / 5, 'x', 2, 3.)),
                         poisson_series[polynomial[double, k_monomial]]())

    def invertTest(self):
        from fractions import Fraction as F
        from .math import invert
        self.assertEqual(invert(F(3, 4)), F(4, 3))
        self.assertEqual(invert(F(3, -4)), -F(4, 3))
        self.assertAlmostEqual(invert(1.234), 1.234**-1.)
        self.assertAlmostEqual(invert(-3.456), -3.456**-1.)
        try:
            from mpmath import mpf
            self.assertEqual(type(invert(mpf(1.23))), mpf)
        except ImportError:
            pass
        from .types import polynomial, monomial, rational, int16, divisor_series, divisor, poisson_series
        t = polynomial[rational, monomial[rational]]()
        self.assertEqual(invert(t(F(3, 4))), F(4, 3))
        self.assertEqual(type(invert(t(F(3, 4)))), t)
        self.assertEqual(invert(t('x')), t('x')**-1)
        t = divisor_series[polynomial[
            rational, monomial[rational]], divisor[int16]]()
        self.assertEqual(invert(t(F(3, 4))), F(4, 3))
        self.assertEqual(type(invert(t(F(3, 4)))), t)
        self.assertEqual(str(invert(t('x'))), '1/[(x)]')
        self.assertEqual(str(t('x')**-1), 'x**-1')
        t = poisson_series[divisor_series[polynomial[
            rational, monomial[rational]], divisor[int16]]]()
        self.assertEqual(invert(t(F(3, 4))), F(4, 3))
        self.assertEqual(type(invert(t(F(3, 4)))), t)
        self.assertEqual(str(invert(t('x'))), '1/[(x)]')
        self.assertEqual(str(t('x')**-1), 'x**-1')

    def lambdifyTest(self):
        from .math import lambdify
        from fractions import Fraction as F
        from .types import polynomial, k_monomial, rational
        pt = polynomial[rational, k_monomial]()
        x, y, z = [pt(_) for _ in 'xyz']
        l = lambdify(F, 3 * x**4 / 2 - y / 3 + z**2, ['y', 'z', 'x'])
        self.assertEqual(l([F(1, 2), F(3, 4), F(5, 6)]),
                         3 * F(5, 6)**4 / 2 - F(1, 2) / 3 + F(3, 4)**2)
        self.assertEqual(type(l([F(1, 2), F(3, 4), F(5, 6)])), F)
        l = lambdify(float, 3 * x**4 / 2 - y / 3 + z**2, ['y', 'z', 'x'])
        self.assertAlmostEqual(
            l([1.2, 3.4, 5.6]), 3 * 5.6**4 / 2 - 1.2 / 3 + 3.4**2)
        self.assertEqual(type(l([1.2, 3.4, 5.6])), float)
        self.assertRaises(TypeError, lambda: lambdify(
            3, 3 * x**4 / 2 - y / 3 + z**2, ['y', 'z', 'x']))
        self.assertRaises(ValueError, lambda: lambdify(
            int, 3 * x**4 / 2 - y / 3 + z**2, ['y', 'z', 'x', 'y']))
        self.assertRaises(TypeError, lambda: lambdify(
            int, 3 * x**4 / 2 - y / 3 + z**2, [1, 2, 3]))
        self.assertRaises(ValueError, lambda: l([1.2, 3.4]))
        self.assertRaises(ValueError, lambda: l([1.2, 3.4, 5.6, 6.7]))
        # Try with extra maps.
        l = lambdify(float, 3 * x**4 / 2 - y / 3 + z **
                     2, ['y', 'z'], {'x': lambda _: 5.})
        self.assertAlmostEqual(l([1.2, 3.4]), 3 * 5.**4 / 2 - 1.2 / 3 + 3.4**2)
        # Check that a deep copy of the functor is made.

        class functor(object):

            def __init__(self):
                self.n = 42.

            def __call__(self, a):
                return self.n
        f = functor()
        l = lambdify(float, 3 * x**4 / 2 - y / 3 + z**2, ['y', 'z'], {'x': f})
        f.n = 0.
        self.assertEqual(f(1), 0.)
        self.assertAlmostEqual(l([1.2, 3.4]), 3 * 42. **
                               4 / 2 - 1.2 / 3 + 3.4**2)
        # Try various errors.
        self.assertRaises(TypeError, lambda: lambdify(
            float, 3 * x**4 / 2 - y / 3 + z**2, ['y', 'z'], {'x': 1}))
        self.assertRaises(TypeError, lambda: lambdify(
            float, 3 * x**4 / 2 - y / 3 + z**2, ['y', 'z'], {3: lambda _: 3}))
        l = lambdify(float, 3 * x**4 / 2 - y / 3 + z **
                     2, ['y', 'z'], {'x': lambda _: ""})
        self.assertRaises(TypeError, lambda: l([1.2, 3.4]))
        self.assertRaises(ValueError, lambda: lambdify(
            float, 3 * x**4 / 2 - y / 3 + z**2, ['y', 'z'], {'y': lambda _: ""}))
        # Check the str repr.
        eobj = 3 * x**4 / 2 - y / 3 + z**2
        l = lambdify(float, eobj, ['y', 'z'])
        self.assertEqual(str(l), "Lambdified object: " + str(eobj) +
                         "\nEvaluation variables: [\"y\",\"z\"]\nSymbols in the extra map: []")
        l = lambdify(float, eobj, ['y', 'z'], {'x': f})
        self.assertEqual(str(l), "Lambdified object: " + str(eobj) +
                         "\nEvaluation variables: [\"y\",\"z\"]\nSymbols in the extra map: [\"x\"]")
        # Try with optional modules:
        try:
            from mpmath import mpf
            l = lambdify(mpf, x - y, ['x', 'y'])
            self.assertEqual(l([mpf(1), mpf(2)]), mpf(1) - mpf(2))
            self.assertEqual(type(l([mpf(1), mpf(2)])), mpf)
        except ImportError:
            pass
        try:
            from numpy import array
            l = lambdify(float, 3 * x**4 / 2 - y / 3 + z**2, ['y', 'z', 'x'])
            self.assertAlmostEqual(
                l(array([1.2, 3.4, 5.6])), 3 * 5.6**4 / 2 - 1.2 / 3 + 3.4**2)
            self.assertEqual(type(l(array([1.2, 3.4, 5.6]))), float)
        except ImportError:
            pass


class polynomial_test_case(_ut.TestCase):
    """:mod:`polynomial` module test case.

    To be used within the :mod:`unittest` framework. Will test the functions implemented in the
    :mod:`polynomial` module.

    >>> import unittest as ut
    >>> suite = ut.TestLoader().loadTestsFromTestCase(polynomial_test_case)

    """

    def runTest(self):
        from . import polynomial_gcd_algorithm as pga
        from .types import polynomial, rational, int16, integer, double, real, monomial
        from fractions import Fraction
        from .math import integrate, gcd
        self.assertEqual(
            type(polynomial[rational, monomial[int16]]()(1).list[0][0]), Fraction)
        self.assertEqual(
            type(polynomial[integer, monomial[int16]]()(1).list[0][0]), int)
        self.assertEqual(
            type(polynomial[double, monomial[int16]]()(1).list[0][0]), float)
        # A couple of tests for integration.
        pt = polynomial[rational, monomial[int16]]()
        x, y, z = [pt(_) for _ in ['x', 'y', 'z']]
        self.assertEqual(integrate(x, 'x'), x * x / 2)
        self.assertEqual(integrate(x, 'y'), x * y)
        self.assertEqual(integrate(x, 'z'), x * z)
        self.assertEqual(integrate(x + y * z, 'x'), x * x / 2 + x * y * z)
        # Some tests for find_cf().
        self.assertEqual(x.find_cf([0]), 0)
        self.assertEqual(x.find_cf([1]), 1)
        self.assertEqual((3 * x).find_cf([1]), 3)
        self.assertEqual(
            (3 * x + (4 * y**2) / 3).find_cf([0, 2]), Fraction(4, 3))
        self.assertRaises(ValueError, lambda: x.find_cf([1, 2]))
        self.assertRaises(TypeError, lambda: x.find_cf([1.]))
        self.assertRaises(TypeError, lambda: x.find_cf([1, 'a']))
        self.assertRaises(TypeError, lambda: x.find_cf(1))
        # Split and join.
        pt = polynomial[integer, monomial[int16]]()
        pt2 = polynomial[double, monomial[int16]]()
        x, y, z = [pt(_) for _ in ['x', 'y', 'z']]
        self.assertEqual((x + 3 * y - 2 * z).split().join(), x + 3 * y - 2 * z)
        self.assertEqual(type((x + 3 * y - 2 * z).split()),
                         polynomial[polynomial[integer, monomial[int16]], monomial[int16]]())
        # Division.
        # NOTE: should also add some uprem testing as well.
        self.assertEqual(x / x, 1)
        self.assertEqual(((x + y) * (x + 1)) / (x + 1), x + y)
        x /= x
        self.assertEqual(x, 1)
        x = pt('x')
        self.assertRaises(ArithmeticError, lambda: pt(1) / x)
        self.assertEqual(pt.udivrem(x, x)[0], 1)
        self.assertEqual(pt.udivrem(x, x)[1], 0)
        self.assertEqual(pt.udivrem(x, x + 1)[0], 1)
        self.assertEqual(pt.udivrem(x, x + 1)[1], -1)
        self.assertRaises(ValueError, lambda: pt.udivrem(x + y, x + y))
        # GCD.
        self.assertTrue(gcd((x**2 - y**2) * (x + 3), (x - y) * (x**3 + y)) == x -
                        y or gcd((x**2 - y**2) * (x + 3), (x - y) * (x**3 + y)) == -x + y)
        self.assertRaises(TypeError, lambda: gcd(x, 1))
        self.assertRaises(ValueError, lambda: gcd(x**-1, y))
        # Test the methods to get/set the default GCD algo.
        self.assertEqual(pt.get_default_gcd_algorithm(), pga.automatic)
        pt.set_default_gcd_algorithm(pga.prs_sr)
        self.assertEqual(pt.get_default_gcd_algorithm(), pga.prs_sr)
        pt.reset_default_gcd_algorithm()
        self.assertEqual(pt.get_default_gcd_algorithm(), pga.automatic)

        def gcd_check(a, b, cmp):
            for algo in [pga.automatic, pga.prs_sr, pga.heuristic]:
                res = pt.gcd(a, b, algo)
                self.assertTrue(res[0] == cmp or res[0] == -cmp)
                res = pt.gcd(a, b, True, algo)
                self.assertTrue(res[0] == cmp or res[0] == -cmp)
                self.assertEqual(res[1], a / res[0])
                self.assertEqual(res[2], b / res[0])
        gcd_check((x**2 - y**2) * (x + 3), (x - y) * (x**3 + y), x - y)
        # Content.
        self.assertEqual((12 * x + 20 * y).content(), 4)
        self.assertEqual((x - x).content(), 0)
        self.assertRaises(AttributeError, lambda: pt2().content())
        # Primitive part.
        self.assertEqual((12 * x + 20 * y).primitive_part(), 3 * x + 5 * y)
        self.assertRaises(ZeroDivisionError, lambda: (x - x).primitive_part())
        self.assertRaises(AttributeError, lambda: pt2().primitive_part())
        # s11n.
        pt = polynomial[integer, monomial[int16]]()
        x, y, z = pt('x'), pt('y'), pt('z')
        p = (x + 3 * y - 4 * z)**4
        _s11n_load_save_test(self, p)
        _pickle_test(self, p)


class divisor_series_test_case(_ut.TestCase):
    """:mod:`divisor_series` module test case.

    To be used within the :mod:`unittest` framework. Will test the functions implemented in the
    :mod:`divisor_series` module.

    >>> import unittest as ut
    >>> suite = ut.TestLoader().loadTestsFromTestCase(divisor_series_test_case)

    """

    def runTest(self):
        from .types import divisor_series, rational, int16, double, real, divisor, monomial, polynomial
        from fractions import Fraction
        from .math import partial, integrate, invert
        self.assertEqual(type(divisor_series[polynomial[rational, monomial[int16]], divisor[
            int16]]()(1).list[0][0]), polynomial[rational, monomial[int16]]())
        self.assertEqual(type(divisor_series[polynomial[double, monomial[int16]], divisor[
            int16]]()(1).list[0][0]), polynomial[double, monomial[int16]]())
        # A couple of tests for the invert() functionality.
        dt = divisor_series[polynomial[
            rational, monomial[int16]], divisor[int16]]()
        self.assertEqual(
            str(invert(2 * dt('x') + 4 * dt('y'))), "1/2*1/[(x+2*y)]")
        self.assertEqual(str(invert(dt('x') + 2 * dt('y'))), "1/[(x+2*y)]")
        self.assertRaises(ValueError, lambda: invert(
            dt('x') + 2 * dt('y') / 3))
        self.assertRaises(ValueError, lambda: invert(dt('x') + 1))
        # Check about partial().
        x, y, z = [dt(_) for _ in ['x', 'y', 'z']]
        self.assertEqual(partial(z, 'x'), 0)
        self.assertEqual(partial(z * x, 'x'), z)
        self.assertEqual(partial(x**2, 'x'), 2 * x)
        # Variables both in the coefficient and in the divisors.
        self.assertEqual(partial(y * x**-1, 'x'), -y * x**-2)
        self.assertEqual(partial(x * invert(x - y), 'x'),
                         invert(x - y) - x * invert(x - y)**2)
        # Some integrate() testing.
        self.assertEqual(integrate(x, 'x'), x * x / 2)
        self.assertEqual(integrate(x * invert(y), 'x'), x * x / 2 * invert(y))
        self.assertEqual(integrate(x * invert(y) + z, 'x'),
                         z * x + x * x / 2 * invert(y))
        self.assertRaises(ValueError, lambda: integrate(invert(x), 'x'))
        # s11n.
        p = (x + 3 * y - 4 * invert(z))**4
        _s11n_load_save_test(self, p)
        _pickle_test(self, p)


class poisson_series_test_case(_ut.TestCase):
    """:mod:`poisson_series` module test case.

    To be used within the :mod:`unittest` framework. Will test the functions implemented in the
    :mod:`poisson_series` module.

    >>> import unittest as ut
    >>> suite = ut.TestLoader().loadTestsFromTestCase(poisson_series_test_case)

    """

    def runTest(self):
        from .types import poisson_series, rational, monomial, int16, divisor_series, divisor, polynomial, \
            k_monomial
        from .math import partial, integrate, sin, cos, invert
        # A couple of tests with eps.
        eps = poisson_series[divisor_series[polynomial[
            rational, monomial[int16]], divisor[int16]]]()
        x, y, z = [eps(_) for _ in ['x', 'y', 'z']]
        self.assertEqual(str(cos(x)), 'cos(x)')
        self.assertEqual(str(sin(x)), 'sin(x)')
        self.assertRaises(ValueError, lambda: sin(x**2))
        self.assertEqual(str(invert(x - y)), '1/[(x-y)]')
        self.assertEqual(str(invert(-2 * x + 4 * y)**2), '1/4*1/[(x-2*y)**2]')
        self.assertEqual(str(invert(-2 * x + 4 * y)**3), '-1/8*1/[(x-2*y)**3]')
        # Partial.
        self.assertEqual(partial(invert(x - y) * cos(z),
                                 'z'), -invert(x - y) * sin(z))
        self.assertEqual(partial(x * invert(x - y) * cos(x), 'x'), invert(x - y)
                         * cos(x) + x * (-invert(x - y)**2 * cos(x) - sin(x) * invert(x - y)))
        # Integrate.
        self.assertEqual(integrate(invert(x - y) * cos(z),
                                   'z'), invert(x - y) * sin(z))
        self.assertEqual(integrate(invert(x - y) * cos(2 * z),
                                   'z'), invert(x - y) * sin(2 * z) / 2)
        self.assertEqual(integrate(y * invert(x) * cos(2 * z),
                                   'y'), y * y / 2 * invert(x) * cos(2 * z))
        self.assertRaises(ValueError, lambda: integrate(
            y * invert(x) * cos(2 * z), 'x'))
        self.assertRaises(ValueError, lambda: integrate(
            z * invert(x) * cos(2 * z), 'z'))
        # s11n.
        p = (x + 3 * invert(y) - 4 * cos(z))**4
        _s11n_load_save_test(self, p)
        _pickle_test(self, p)


class converters_test_case(_ut.TestCase):
    """Test case for the automatic conversion to/from Python from/to C++.

    To be used within the :mod:`unittest` framework.

    >>> import unittest as ut
    >>> suite = ut.TestLoader().loadTestsFromTestCase(converters_test_case)

    """

    def runTest(self):
        import sys
        from .types import polynomial, int16, integer, rational, real, monomial
        from fractions import Fraction as F
        from .math import evaluate
        # Context for the temporary monkey-patching of type t to return bogus
        # string bad_str for representation via str.

        class patch_str(object):

            def __init__(self, t, bad_str):
                self.t = t
                self.bad_str = bad_str

            def __enter__(self):
                self.old_str = self.t.__str__
                self.t.__str__ = lambda s: self.bad_str

            def __exit__(self, type, value, traceback):
                self.t.__str__ = self.old_str
        # Same for repr.

        class patch_repr(object):

            def __init__(self, t, bad_repr):
                self.t = t
                self.bad_repr = bad_repr

            def __enter__(self):
                self.old_repr = self.t.__repr__
                self.t.__repr__ = lambda s: self.bad_repr

            def __exit__(self, type, value, traceback):
                self.t.__repr__ = self.old_repr
        # Start with plain integers.
        pt = polynomial[integer, monomial[int16]]()
        # Use small integers to make it work both on Python 2 and Python 3.
        self.assertEqual(int, type(pt(4).list[0][0]))
        self.assertEqual(pt(4).list[0][0], 4)
        self.assertEqual(int, type(evaluate(pt("x"), {"x": 5})))
        self.assertEqual(evaluate(pt("x"), {"x": 5}), 5)
        # Specific for Python 2.
        if sys.version_info[0] == 2:
            self.assertEqual(int, type(pt(sys.maxint).list[0][0]))
            self.assertEqual(pt(sys.maxint).list[0][0], sys.maxint)
            self.assertEqual(long, type((pt(sys.maxint) + 1).list[0][0]))
            self.assertEqual((pt(sys.maxint) + 1).list[0][0], sys.maxint + 1)
            self.assertEqual(int, type(evaluate(pt("x"), {"x": sys.maxint})))
            self.assertEqual(evaluate(pt("x"), {"x": sys.maxint}), sys.maxint)
            self.assertEqual(long, type(
                evaluate(pt("x"), {"x": sys.maxint + 1})))
            self.assertEqual(
                evaluate(pt("x"), {"x": sys.maxint + 1}), sys.maxint + 1)
        # Now rationals.
        pt = polynomial[rational, monomial[int16]]()
        self.assertEqual(F, type((pt(4) / 3).list[0][0]))
        self.assertEqual((pt(4) / 3).list[0][0], F(4, 3))
        self.assertEqual(F, type(evaluate(pt("x"), {"x": F(5, 6)})))
        self.assertEqual(evaluate(pt("x"), {"x": F(5, 6)}), F(5, 6))
        # NOTE: if we don't go any more through str for the conversion, these types
        # of tests can go away.
        with patch_str(F, "boo"):
            self.assertRaises(ValueError, lambda: pt(F(5, 6)))
        with patch_str(F, "5/0"):
            self.assertRaises(ZeroDivisionError, lambda: pt(F(5, 6)))
        # Reals, if possible.
        try:
            from mpmath import mpf, mp, workdps, binomial as bin, fabs
        except ImportError:
            return
        pt = polynomial[integer, monomial[int16]]()
        self.assertEqual(mpf, type(evaluate(pt("x"), {"x": mpf(5)})))
        self.assertEqual(evaluate(pt("x"), {"x": mpf(5)}), mpf(5))
        # Check the handling of the precision.
        from .math import binomial
        # Original number of decimal digits.
        orig_dps = mp.dps
        tmp = binomial(mpf(5.1), mpf(3.2))
        self.assertEqual(tmp.context.dps, orig_dps)
        with workdps(100):
            # Check that the precision is maintained on output
            # and that the values computed with our implementation and
            # mpmath are close.
            tmp = binomial(mpf(5.1), mpf(3.2))
            self.assertEqual(tmp.context.dps, 100)
            self.assertTrue(fabs(tmp - bin(mpf(5.1), mpf(3.2)))
                            < mpf('5e-100'))

class serialization_test_case(_ut.TestCase):
    """Test case for the serialization of series.

    To be used within the :mod:`unittest` framework.

    >>> import unittest as ut
    >>> suite = ut.TestLoader().loadTestsFromTestCase(serialization_test_case)

    """

    def runTest(self):
        # NOTE: this used to contain more, but now it contains
        # only this exception translation check.
        # Check msgpack exception translation.
        import os
        import shutil
        import tempfile
        from . import load_file, save_file
        from .types import polynomial, integer, rational, int16, monomial
        pt1 = polynomial[integer, monomial[int16]]()
        pt2 = polynomial[rational, monomial[int16]]()
        temp_dir = tempfile.mkdtemp()
        try:
            x = pt1('x')
            save_file(x, os.path.join(temp_dir, 'foo.mpackp'))
            ret = pt2()
            self.assertRaises(TypeError, lambda: load_file(
                ret, os.path.join(temp_dir, 'foo.mpackp')))
        except NotImplementedError:
            pass
        finally:
            shutil.rmtree(temp_dir)


class truncate_degree_test_case(_ut.TestCase):
    """Test case for the degree-based truncation of series.

    To be used within the :mod:`unittest` framework.

    >>> import unittest as ut
    >>> suite = ut.TestLoader().loadTestsFromTestCase(truncate_degree_test_case)

    """

    def runTest(self):
        from fractions import Fraction as F
        from .math import truncate_degree, cos, sin, degree
        from .types import polynomial, int16, rational, poisson_series, monomial
        pt = polynomial[rational, monomial[int16]]()
        x, y, z = pt('x'), pt('y'), pt('z')
        s = x**5 * y + F(1, 2) * z**-5 * x * y + x * y * z / 4
        self.assertEqual(s.truncate_degree(3), z**-
                         5 / 2 * x * y + x * y * z / 4)
        self.assertEqual(truncate_degree(s, 3), z**-
                         5 / 2 * x * y + x * y * z / 4)
        self.assertEqual(s.truncate_degree(
            2, ["x"]), z**-5 / 2 * x * y + x * y * z / 4)
        self.assertEqual(truncate_degree(
            s, 2, ["x"]), z**-5 / 2 * x * y + x * y * z / 4)
        self.assertRaises(TypeError, lambda: truncate_degree(s, 2, ["x", 1]))
        self.assertRaises(TypeError, lambda: truncate_degree(s, 2, "x"))
        pt = poisson_series[polynomial[rational, monomial[int16]]]()
        x, y, z, a, b = pt("x"), pt("y"), pt("z"), pt("a"), pt("b")
        s = (x + y**2 / 4 + 3 * x * y * z / 7) * cos(a) + \
            (x * y + y * z / 3 + 3 * z**2 * x / 8) * sin(a + b)
        self.assertEqual(s.truncate_degree(2), (x + y * y / 4)
                         * cos(a) + (x * y + z * y / 3) * sin(a + b))
        self.assertEqual(truncate_degree(s, 2), (x + y * y / 4)
                         * cos(a) + (x * y + z * y / 3) * sin(a + b))
        self.assertEqual(s.truncate_degree(
            1, ["y", "x"]), x * cos(a) + (z * y / 3 + 3 * z * z * x / 8) * sin(a + b))
        self.assertEqual(truncate_degree(
            s, 1, ["y", "x"]), x * cos(a) + (z * y / 3 + 3 * z * z * x / 8) * sin(a + b))
        self.assertRaises(TypeError, lambda: truncate_degree(s, 2, ["x", 1]))
        self.assertRaises(TypeError, lambda: truncate_degree(s, 2, "x"))
        # Automatic truncation.
        # Just make sure we do not re-use any previous result.
        pt = polynomial[rational, monomial[int16]]()
        x, y = pt('x'), pt('y')
        pt.clear_pow_cache()
        self.assertEqual(pt.get_auto_truncate_degree(), (0, 0, []))
        pt.set_auto_truncate_degree(4)
        self.assertEqual(pt.get_auto_truncate_degree(), (1, 4, []))
        self.assertEqual(degree(x * x * x), 3)
        self.assertEqual(x * x * x * x * x, 0)
        pt.set_auto_truncate_degree(3, ['x', 'y'])
        self.assertEqual(pt.get_auto_truncate_degree(), (2, 3, ['x', 'y']))
        self.assertEqual(1 + 12 * x * y + 4 * x + 12 * x**2 * y + 6 * x**2 +
                         4 * x**3 + 4 * y**3 + 6 * y**2 + 4 * y + 12 * x * y**2, (x + y + 1)**4)
        # Check throwing conversion.
        self.assertRaises(
            ValueError, lambda: pt.set_auto_truncate_degree(F(4, 3), ['x', 'y']))
        self.assertEqual(pt.get_auto_truncate_degree(), (2, 3, ['x', 'y']))
        # Check multiplication.
        self.assertEqual(degree(x * x * y), 3)
        self.assertEqual((x * x * y * y), 0)
        # Check we cannot use float for auto truncation.
        self.assertRaises(TypeError, lambda: pt.set_auto_truncate_degree(1.23))
        # Reset before finishing.
        pt.unset_auto_truncate_degree()
        pt.clear_pow_cache()
        self.assertEqual(pt.get_auto_truncate_degree(), (0, 0, []))
        # Explicit truncated/untruncated multiplication methods.
        pt = polynomial[rational, monomial[int16]]()
        x, y = pt('x'), pt('y')
        pt.clear_pow_cache()
        res = x**2 - y**2
        res2 = -y * y
        res3 = x * x
        pt.set_auto_truncate_degree(1)
        self.assertEqual(pt.untruncated_multiplication(x + y, x - y), res)
        pt.set_auto_truncate_degree(5)
        self.assertEqual(pt.truncated_multiplication(x + y, x - y, 1), 0)
        self.assertEqual(pt.truncated_multiplication(x + y, x - y, F(1)), 0)
        self.assertRaises(
            ValueError, lambda: pt.truncated_multiplication(x + y, x - y, F(1, 2)))
        self.assertEqual(pt.truncated_multiplication(
            x + y, x - y, 1, ["x"]), res2)
        self.assertEqual(pt.truncated_multiplication(
            x + 1, x, 1, ["x", "y"]), x)
        self.assertRaises(ValueError, lambda: pt.truncated_multiplication(
            x + y, x - y, F(3, 2), ["x"]))
        self.assertRaises(TypeError, lambda: pt.truncated_multiplication(
            x + y, x - y, 3, ["x", 3]))
        # Reset before finishing.
        pt.unset_auto_truncate_degree()
        pt.clear_pow_cache()
        # Test the new behaviour of pow cache clearing.
        pt = polynomial[rational, monomial[int16]]()
        x, y = pt('x'), pt('y')
        self.assertEqual((x + y + 1)**2, 2 * x + 1 + 2 *
                         y + x * x + y * y + 2 * x * y)
        pt.set_auto_truncate_degree(1)
        self.assertEqual((x + y + 1)**2, 2 * x + 1 + 2 * y)
        pt.set_auto_truncate_degree(1, ['x'])
        self.assertEqual((x + y + 1)**2, 2 * x + 1 + 2 * y + y * y + 2 * x * y)
        pt.set_auto_truncate_degree(1, ['y'])
        self.assertEqual((x + y + 1)**2, 2 * x + 1 + 2 * y + x * x + 2 * x * y)
        # Reset before finishing.
        pt.unset_auto_truncate_degree()
        pt.clear_pow_cache()


class integrate_test_case(_ut.TestCase):
    """Test case for the integration functionality.

    To be used within the :mod:`unittest` framework.

    >>> import unittest as ut
    >>> suite = ut.TestLoader().loadTestsFromTestCase(integrate_test_case)

    """

    def runTest(self):
        from fractions import Fraction as F
        from .math import sin, cos
        from .types import polynomial, int16, rational, poisson_series, monomial, integer
        pt = poisson_series[polynomial[rational, monomial[int16]]]()
        x, y, z = pt('x'), pt('y'), pt('z')
        s = F(1, 3) * z * sin(4 * x - 2 * y)
        self.assertEqual(s.integrate('y'), F(1, 6) * z * cos(4 * x - 2 * y))
        self.assertEqual(s.integrate('z'), z**2 / 6 * sin(4 * x - 2 * y))
        pt = polynomial[integer, monomial[rational]]()
        z = pt('z')
        s = z**F(3, 4)
        self.assertEqual(type(s), pt)
        self.assertEqual(s.integrate('z'), F(4, 7) * z**F(7, 4))
        self.assertEqual(type(s.integrate('z')), polynomial[
            rational, monomial[rational]]())


class t_integrate_test_case(_ut.TestCase):
    """Test case for the time integration functionality.

    To be used within the :mod:`unittest` framework.

    >>> import unittest as ut
    >>> suite = ut.TestLoader().loadTestsFromTestCase(t_integrate_test_case)

    """

    def runTest(self):
        from fractions import Fraction as F
        from .math import sin, cos
        from .types import polynomial, int16, rational, poisson_series, monomial, divisor, divisor_series, \
            k_monomial
        pt = poisson_series[divisor_series[polynomial[
            rational, monomial[int16]], divisor[int16]]]()
        x, y, z = pt('x'), pt('y'), pt('z')
        s = F(1, 3) * z * sin(4 * x - 2 * y)
        st = s.t_integrate()
        self.assertEqual(type(st.list[0][0]), divisor_series[
            polynomial[rational, monomial[int16]], divisor[int16]]())
        self.assertEqual(
            str(st), '-1/6*z*1/[(2*\\nu_{x}-\\nu_{y})]*cos(4*x-2*y)')
        st = s.t_integrate(['a', 'b'])
        self.assertEqual(type(st.list[0][0]), divisor_series[
            polynomial[rational, monomial[int16]], divisor[int16]]())
        self.assertEqual(str(st), '-1/6*z*1/[(2*a-b)]*cos(4*x-2*y)')
        self.assertRaises(ValueError, lambda: s.t_integrate([]))
        self.assertRaises(ValueError, lambda: s.t_integrate(['a', 'b', 'c']))
        self.assertRaises(ValueError, lambda: s.t_integrate(['b', 'a']))
        st = s.t_integrate(['a', 'b', 'b'])
        self.assertEqual(str(st), '-1/6*z*1/[(2*a-b)]*cos(4*x-2*y)')
        st = s.t_integrate(['a', 'a', 'b', 'b'])
        self.assertEqual(str(st), '-1/6*z*1/[(2*a-b)]*cos(4*x-2*y)')
        st = s.t_integrate(['a', 'a', 'b'])
        self.assertEqual(str(st), '-1/6*z*1/[(2*a-b)]*cos(4*x-2*y)')


class doctests_test_case(_ut.TestCase):
    """Test case that will run all the doctests.

    To be used within the :mod:`unittest` framework.

    >>> import unittest as ut
    >>> suite = ut.TestLoader().loadTestsFromTestCase(doctests_test_case)

    """

    def runTest(self):
        import doctest
        import pyranha
        from . import celmec, math, test
        try:
            doctest.testmod(pyranha, raise_on_error=True)
            doctest.testmod(celmec, raise_on_error=True)
            doctest.testmod(math, raise_on_error=True)
            doctest.testmod(test, raise_on_error=True)
        except Exception as e:
            self.fail(str(e))


class tutorial_test_case(_ut.TestCase):
    """Test case that will check the tutorial files.

    To be used within the :mod:`unittest` framework.

    >>> import unittest as ut
    >>> suite = ut.TestLoader().loadTestsFromTestCase(tutorial_test_case)

    """

    def runTest(self):
        from . import _tutorial


class degree_test_case(_ut.TestCase):
    """Test case for the degree/ldegree functionality.

    To be used within the :mod:`unittest` framework.

    >>> import unittest as ut
    >>> suite = ut.TestLoader().loadTestsFromTestCase(degree_test_case)

    """

    def runTest(self):
        from .types import polynomial, int16, rational, poisson_series, monomial
        from .math import degree, ldegree
        from fractions import Fraction as F
        pt = polynomial[rational, monomial[int16]]()
        x, y, z = [pt(_) for _ in 'xyz']
        self.assertEqual(degree(x), 1)
        self.assertEqual(degree(x**2 * y**-3 * z**-4), -5)
        self.assertEqual(degree(x**2 * y**-3 * z**-4 + 1), 0)
        self.assertEqual(degree(x**2 * y**-3 * z**-4, ['x']), 2)
        self.assertEqual(degree(x**2 * y**-3 * z**-4, ['x', 'y']), -1)
        self.assertEqual(degree(x**2 * y**-3 * z**-4 + 1, ['x', 'y']), 0)
        self.assertRaises(TypeError, lambda: degree(
            x**2 * y**-3 * z**-4 + 1, [11]))
        self.assertEqual(ldegree(x**2 * y**-3 * z**-4 + 1), -5)
        self.assertEqual(ldegree(x**2 * y**-3 * z**-4 + 1, ['y']), -3)
        self.assertEqual(ldegree(x**2 * y**-3 * z**-4 + 1, ['y', 'z']), -7)
        self.assertRaises(TypeError, lambda: ldegree(
            x**2 * y**-3 * z**-4 + 1, [11]))
        pt = poisson_series[polynomial[rational, monomial[rational]]]()
        x, y, z = [pt(_) for _ in 'xyz']
        self.assertEqual(degree(x**F(4, 5) * y**-3 * z**-4), F(4, 5) - 3 - 4)
        self.assertEqual(degree(x**F(4, 5) * y**-3 * z**-4 + 1), 0)
        self.assertEqual(degree(x**F(4, 5) * y**-3 * z**-4, ['x']), F(4, 5))
        self.assertEqual(degree(x**F(4, 5) * y**-3 *
                                z**-4, ['x', 'z']), F(4, 5) - 4)
        self.assertRaises(TypeError, lambda: degree(
            x**2 * y**-3 * z**-4 + 1, [11]))
        self.assertEqual(ldegree(x**F(4, 5) * y**-3 *
                                 z**-4 + 1), F(4, 5) - 3 - 4)
        self.assertEqual(ldegree(x**F(4, 5) * y**-3 * z**-4 + 1, ['y']), -3)
        self.assertEqual(
            ldegree(x**F(4, 5) * y**-3 * z**-4 + 1, ['y', 'z']), -7)
        self.assertRaises(TypeError, lambda: ldegree(
            x**2 * y**-3 * z**-4 + 1, [11]))


class t_degree_order_test_case(_ut.TestCase):
    """Test case for the t_degree/t_ldegree/t_order/t_lorder functionality.

    To be used within the :mod:`unittest` framework.

    >>> import unittest as ut
    >>> suite = ut.TestLoader().loadTestsFromTestCase(t_degree_order_test_case)

    """

    def runTest(self):
        from .types import polynomial, int16, rational, poisson_series, monomial
        from .math import t_degree, t_ldegree, t_order, t_lorder, cos, sin
        pt = poisson_series[polynomial[rational, monomial[int16]]]()
        x, y, z = [pt(_) for _ in 'xyz']
        self.assertEqual(t_degree(cos(x)), 1)
        self.assertEqual(t_degree(cos(3 * x - y)), 2)
        self.assertEqual(t_degree(cos(3 * x - y), ['x']), 3)
        self.assertRaises(TypeError, lambda: t_degree(cos(3 * x - y), [11]))
        self.assertEqual(t_ldegree(cos(x)), 1)
        self.assertEqual(t_ldegree(cos(3 * x - y) + cos(x)), 1)
        self.assertEqual(t_ldegree(cos(3 * x - y) + sin(y), ['x']), 0)
        self.assertRaises(TypeError, lambda: t_ldegree(cos(3 * x - y), [11]))
        self.assertEqual(t_order(cos(x)), 1)
        self.assertEqual(t_order(cos(3 * x - y)), 4)
        self.assertEqual(t_order(cos(3 * x - y), ['x']), 3)
        self.assertRaises(TypeError, lambda: t_order(cos(3 * x - y), [11]))
        self.assertEqual(t_lorder(cos(x)), 1)
        self.assertEqual(t_lorder(cos(3 * x - y) + cos(x - y)), 2)
        self.assertEqual(t_lorder(cos(3 * x - y) + sin(y), ['x']), 0)
        self.assertRaises(TypeError, lambda: t_lorder(cos(3 * x - y), [11]))


def run_test_suite():
    """Run the full test suite.

    This function will raise an exception if at least one test fails.

    """
    retval = 0
    suite = _ut.TestLoader().loadTestsFromTestCase(basic_test_case)
    suite.addTest(series_in_place_ops_test_case())
    suite.addTest(custom_derivatives_test_case())
    suite.addTest(series_division_test_case())
    suite.addTest(mpmath_test_case())
    suite.addTest(math_test_case())
    suite.addTest(polynomial_test_case())
    suite.addTest(divisor_series_test_case())
    suite.addTest(poisson_series_test_case())
    suite.addTest(converters_test_case())
    suite.addTest(serialization_test_case())
    suite.addTest(integrate_test_case())
    suite.addTest(t_integrate_test_case())
    suite.addTest(truncate_degree_test_case())
    suite.addTest(degree_test_case())
    suite.addTest(t_degree_order_test_case())
    suite.addTest(doctests_test_case())
    test_result = _ut.TextTestRunner(verbosity=2).run(suite)
    if len(test_result.failures) > 0 or len(test_result.errors) > 0:
        retval = 1
    suite = _ut.TestLoader().loadTestsFromTestCase(tutorial_test_case)
    # Context for the suppression of output while running the tutorials. Inspired by:
    # http://stackoverflow.com/questions/8522689/how-to-temporary-hide-stdout-or-stderr-while-running-a-unittest-in-python
    # This will temporarily replace sys.stdout with the null device.

    class suppress_stdout(object):

        def __init__(self):
            pass

        def __enter__(self):
            import sys
            import os
            self._stdout = sys.stdout
            # NOTE: originally here it was 'wb', but apparently this will create problems
            # in Python 3 due to the string changes. With 'wt' it seems to work ok in both Python 2
            # and 3.
            null = open(os.devnull, 'wt')
            sys.stdout = null

        def __exit__(self, type, value, traceback):
            import sys
            sys.stdout = self._stdout
    with suppress_stdout():
        test_result = _ut.TextTestRunner(verbosity=2).run(suite)
        if len(test_result.failures) > 0:
            retval = 1
    if retval != 0:
        raise RuntimeError('One or more tests failed.')
