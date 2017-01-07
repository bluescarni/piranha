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

from ._version import __version__

#: List of Pyranha submodules.
__all__ = ['celmec', 'math', 'test', 'types']

import threading as _thr
from ._common import _cpp_type_catcher, _monkey_patching
from ._core import polynomial_gcd_algorithm, data_format as _df, compression as _cf

# Run the monkey patching.
_monkey_patching()


class settings(object):
    """Settings class.

    This class is used to configure global Pyranha settings via static methods.
    The methods are thread-safe.

    """
    # Main lock for protecting reads/writes from multiple threads.
    __lock = _thr.RLock()

    @staticmethod
    def get_max_term_output():
        """Get the maximum number of series terms to print.

        :returns: the maximum number of series terms to print
        :raises: any exception raised by the invoked low-level function

        >>> settings.get_max_term_output()
        20

        """
        from ._core import _settings as _s
        return _s._get_max_term_output()

    @staticmethod
    def set_max_term_output(n):
        """Set the maximum number of series terms to print.

        :param n: number of series terms to print
        :type n: ``int``
        :raises: any exception raised by the invoked low-level function

        >>> settings.set_max_term_output(10)
        >>> settings.get_max_term_output()
        10
        >>> settings.set_max_term_output(-1) # doctest: +IGNORE_EXCEPTION_DETAIL
        Traceback (most recent call last):
          ...
        OverflowError: invalid value
        >>> settings.set_max_term_output("hello") # doctest: +IGNORE_EXCEPTION_DETAIL
        Traceback (most recent call last):
          ...
        TypeError: invalid type
        >>> settings.reset_max_term_output()

        """
        from ._core import _settings as _s
        return _cpp_type_catcher(_s._set_max_term_output, n)

    @staticmethod
    def reset_max_term_output():
        """Reset the maximum number of series terms to print to the default value.

        :raises: any exception raised by the invoked low-level function

        >>> settings.set_max_term_output(10)
        >>> settings.get_max_term_output()
        10
        >>> settings.reset_max_term_output()
        >>> settings.get_max_term_output()
        20

        """
        from ._core import _settings as _s
        return _s._reset_max_term_output()

    @staticmethod
    def get_n_threads():
        """Get the number of threads that can be used by Piranha.

        The initial value is auto-detected on program startup.

        :raises: any exception raised by the invoked low-level function

        >>> settings.get_n_threads() # doctest: +SKIP
        16 # This will be a platform-dependent value.

        """
        from ._core import _settings as _s
        return _s._get_n_threads()

    @staticmethod
    def set_n_threads(n):
        """Set the number of threads that can be used by Piranha.

        :param n: desired number of threads
        :type n: ``int``
        :raises: any exception raised by the invoked low-level function

        >>> settings.set_n_threads(2)
        >>> settings.get_n_threads()
        2
        >>> settings.set_n_threads(0) # doctest: +IGNORE_EXCEPTION_DETAIL
        Traceback (most recent call last):
          ...
        ValueError: invalid value
        >>> settings.set_n_threads(-1) # doctest: +IGNORE_EXCEPTION_DETAIL
        Traceback (most recent call last):
          ...
        OverflowError: invalid value
        >>> settings.reset_n_threads()

        """
        from ._core import _settings as _s
        return _cpp_type_catcher(_s._set_n_threads, n)

    @staticmethod
    def reset_n_threads():
        """Reset the number of threads that can be used by Piranha to the default value.

        :raises: any exception raised by the invoked low-level function

        >>> n = settings.get_n_threads()
        >>> settings.set_n_threads(10)
        >>> settings.get_n_threads()
        10
        >>> settings.reset_n_threads()
        >>> settings.get_n_threads() == n
        True

        """
        from ._core import _settings as _s
        return _s._reset_n_threads()

    @staticmethod
    def get_latex_repr():
        r"""Check if the TeX representation of exposed types is enabled.

        The ``_repr_latex_()`` method, if present, is used by the Jupyter notebook to display the TeX
        representation of an exposed type via a Javascript library. If an object is very large, it might be preferrable
        to disable the TeX representation in order to improve the performance of the notebook UI. When the TeX
        representation is disabled, Jupyter will fall back to the PNG-based representation of the object, which
        leverages - if available - a local installation of TeX for increased performance via a static rendering
        of the TeX representation of the object to a PNG bitmap.

        By default, the TeX representation is enabled. See also :py:meth:`pyranha.settings.set_latex_repr`.

        >>> settings.get_latex_repr()
        True
        >>> from .types import polynomial, rational, k_monomial
        >>> pt = polynomial[rational,k_monomial]()
        >>> x = pt('x')
        >>> (x**2/2)._repr_latex_()
        '\\[ \\frac{1}{2}{x}^{2} \\]'

        """
        from ._core import _get_exposed_types_list as getl
        s_type = getl()[0]
        with settings.__lock:
            return hasattr(s_type, '_repr_latex_')

    @staticmethod
    def set_latex_repr(flag):
        r"""Set the availability of the ``_repr_latex_()`` method for the exposed types.

        If *flag* is ``True``, the ``_repr_latex_()`` method of exposed types will be enabled. Otherwise,
        the method will be disabled. See the documentation for :py:meth:`pyranha.settings.get_latex_repr` for a
        description of how the method is used.

        :param flag: availability flag for the ``_repr_latex_()`` method
        :type flag: ``bool``
        :raises: :exc:`TypeError` if *flag* is not a ``bool``

        >>> settings.set_latex_repr(False)
        >>> settings.get_latex_repr()
        False
        >>> from .types import polynomial, rational, k_monomial
        >>> pt = polynomial[rational,k_monomial]()
        >>> x = pt('x')
        >>> (x**2/2)._repr_latex_() # doctest: +IGNORE_EXCEPTION_DETAIL
        Traceback (most recent call last):
          ...
        AttributeError: object has no attribute '_latex_repr_'
        >>> settings.set_latex_repr(True)
        >>> (x**2/2)._repr_latex_()
        '\\[ \\frac{1}{2}{x}^{2} \\]'
        >>> settings.set_latex_repr("hello") # doctest: +IGNORE_EXCEPTION_DETAIL
        Traceback (most recent call last):
          ...
        TypeError: the 'flag' parameter must be a bool

        """
        from . import _core
        from ._core import _get_exposed_types_list as getl
        from ._common import _register_repr_latex
        if not isinstance(flag, bool):
            raise TypeError("the 'flag' parameter must be a bool")
        with settings.__lock:
            # NOTE: reentrant lock in action.
            if flag == settings.get_latex_repr():
                return
            if flag:
                _register_repr_latex()
            else:
                for s_type in getl():
                    assert(hasattr(s_type, '_repr_latex_'))
                    delattr(s_type, '_repr_latex_')

    @staticmethod
    def get_min_work_per_thread():
        """Get the minimum work per thread.

        >>> settings.get_min_work_per_thread() # doctest: +SKIP
        500000 # This will be an implementation-defined value.

        """
        from ._core import _settings as _s
        return _s._get_min_work_per_thread()

    @staticmethod
    def set_min_work_per_thread(n):
        """Set the minimum work per thread.

        :param n: desired work per thread
        :type n: ``int``
        :raises: any exception raised by the invoked low-level function

        >>> settings.set_min_work_per_thread(2)
        >>> settings.get_min_work_per_thread() # doctest: +SKIP
        2
        >>> settings.set_min_work_per_thread(0) # doctest: +IGNORE_EXCEPTION_DETAIL
        Traceback (most recent call last):
          ...
        ValueError: invalid value
        >>> settings.set_min_work_per_thread(-1) # doctest: +IGNORE_EXCEPTION_DETAIL
        Traceback (most recent call last):
          ...
        OverflowError: invalid value
        >>> settings.reset_min_work_per_thread()

        """
        from ._core import _settings as _s
        return _cpp_type_catcher(_s._set_min_work_per_thread, n)

    @staticmethod
    def reset_min_work_per_thread():
        """Reset the minimum work per thread.

        >>> n = settings.get_min_work_per_thread()
        >>> settings.set_min_work_per_thread(10)
        >>> settings.get_min_work_per_thread() # doctest: +SKIP
        10
        >>> settings.reset_min_work_per_thread()
        >>> settings.get_min_work_per_thread() == n
        True

        """
        from ._core import _settings as _s
        return _s._reset_min_work_per_thread()

    @staticmethod
    def set_thread_binding(flag):
        """Set the thread binding policy.

        By default, the threads created by Piranha are not bound to specific processors/cores.
        Calling this method with a ``True`` *flag* will instruct Piranha to attempt to bind each
        thread to a different processor/core (which can result in increased performance
        in certain circumstances). If *flag* is ``False``, the threads created by Piranha will be
        free to migrate across processors/cores.

        :param flag: the desired thread binding policy
        :type flag: ``bool``
        :raises: any exception raised by the invoked low-level function

        >>> settings.get_thread_binding()
        False
        >>> settings.set_thread_binding(True)
        >>> settings.get_thread_binding()
        True
        >>> settings.set_thread_binding(False)
        >>> settings.get_thread_binding()
        False
        >>> settings.set_thread_binding(4.56) # doctest: +IGNORE_EXCEPTION_DETAIL
        Traceback (most recent call last):
          ...
        TypeError: invalid argument type(s)

        """
        from ._core import _settings as _s
        return _cpp_type_catcher(_s._set_thread_binding, flag)

    @staticmethod
    def get_thread_binding():
        """Get the thread binding policy.

        This method will return the flag set by :py:meth:`pyranha.settings.set_thread_binding`.
        On program startup, the value returned by this function will be ``False``.

        :returns: the thread binding policy
        :rtype: ``bool``
        :raises: any exception raised by the invoked low-level function

        >>> settings.get_thread_binding()
        False
        >>> settings.set_thread_binding(True)
        >>> settings.get_thread_binding()
        True
        >>> settings.set_thread_binding(False)
        >>> settings.get_thread_binding()
        False

        """
        from ._core import _settings as _s
        return _s._get_thread_binding()


class data_format(object):
    """Data format.

    The members of this class identify the data formats that can be used when saving/loading
    to/from disk symbolic objects via :py:func:`pyranha.save_file` and :py:func:`pyranha.load_file`.
    The Boost formats are based on the Boost serialization library and they are always available.
    The msgpack formats rely on the msgpack-c library (which is an optional dependency).

    The portable variants are slower but suitable for use across architectures and Piranha versions, the binary
    variants are faster but they are not portable across architectures and Piranha versions.

    """

    #: Boost portable format.
    boost_portable = _df.boost_portable
    #: Boost binary format.
    boost_binary = _df.boost_binary
    #: msgpack portable format.
    msgpack_portable = _df.msgpack_portable
    #: msgpack binary format.
    msgpack_binary = _df.msgpack_binary


class compression(object):
    """Compression format.

    The members of this class identify the compression formats that can be used when saving/loading
    to/from disk symbolic objects via :py:func:`pyranha.save_file` and :py:func:`pyranha.load_file`.
    The compression formats are available only if Piranha was compiled with the corresponding optional
    compression options enabled.

    """

    #: No compression.
    none = _cf.none
    #: zlib compression.
    zlib = _cf.zlib
    #: gzip compression.
    gzip = _cf.gzip
    #: bzip2 compression.
    bzip2 = _cf.bzip2


def _save_load_check_params(name, df, cf):
    if not isinstance(name, str):
        raise TypeError("the file name must be a string")
    if df is None and not cf is None:
        raise ValueError(
            "the compression format was provided but the data format was not: please specify both or none")
    if cf is None and not df is None:
        raise ValueError(
            "the data format was provided but the compression format was not: please specify both or none")


def save_file(obj, name, df=None, cf=None):
    """Save to file.

    This function will save the symbolic object *obj* to the file called *name* using *df* as data format
    and *cf* as compression format. The possible values for *df* and *cf* are listed in the
    :py:class:`pyranha.data_format` and :py:class:`pyranha.compression` classes respectively.

    If *df* and *cf* are both ``None``, then the data and compression formats are inferred from the filename:

    * if *name* ends in one of the suffixes ``.bz2``, ``.gz`` or ``.zip`` then the suffix is removed
      for further considerations from *name*, and the corresponding :py:class:`pyranha.compression` format is assumed
      (respectively, :py:attr:`pyranha.compression.bzip2`, :py:attr:`pyranha.compression.gzip` and
      :py:attr:`pyranha.compression.zlib`). Otherwise, :py:attr:`pyranha.compression.none` is assumed;
    * after the removal of any compression suffix, the extension of *name* is examined again: if the extension is
      one of ``.boostp``, ``.boostb``, ``.mpackp`` and ``.mpackb``, then the corresponding data format is selected
      (respectively, :py:attr:`pyranha.data_format.boost_portable`, :py:attr:`pyranha.data_format.boost_binary`,
      :py:attr:`pyranha.data_format.msgpack_portable`, :py:attr:`pyranha.data_format.msgpack_binary`). Othwewise, an
      error will be produced.

    Examples of file names:

    * ``foo.boostb.bz2`` deduces :py:attr:`pyranha.data_format.boost_binary` and :py:attr:`pyranha.compression.bzip2`;
    * ``foo.mpackp`` deduces :py:attr:`pyranha.data_format.msgpack_portable` and :py:attr:`pyranha.compression.none`;
    * ``foo.txt`` produces an error;
    * ``foo.bz2`` produces an error.

    :param obj: the symbolic object that will be saved to file
    :type obj: a supported symbolic type
    :param name: the file name
    :type name: ``str``
    :param df: the desired data format (see :py:class:`pyranha.data_format`)
    :param cf: the desired compression format (see :py:class:`pyranha.compression`)

    :raises: :exc:`TypeError` if the file name is not a string
    :raises: :exc:`ValueError` if only one of *df* and *cf* is ``None``
    :raises: any exception raised by the invoked low-level C++ function

    >>> from pyranha.types import polynomial, rational, k_monomial
    >>> import tempfile, os
    >>> x = polynomial[rational,k_monomial]()('x')
    >>> p = (x + 1)**10
    >>> f = tempfile.NamedTemporaryFile(delete=False) # Generate a temporary file name
    >>> f.close()
    >>> save_file(p, f.name, data_format.boost_portable, compression.none)
    >>> p_load = type(p)()
    >>> load_file(p_load, f.name, data_format.boost_portable, compression.none)
    >>> p_load == p
    True
    >>> save_file(p, 123) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
      ...
    TypeError: the file name must be a string
    >>> save_file(p, "foo", df = data_format.boost_portable) # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
      ...
    ValueError: the data format was provided but the compression format was not
    >>> os.remove(f.name) # Cleanup

    """
    from ._core import _save_file
    _save_load_check_params(name, df, cf)
    if not df is None:
        _cpp_type_catcher(_save_file, obj, name, df, cf)
    else:
        _cpp_type_catcher(_save_file, obj, name)


def load_file(obj, name, df=None, cf=None):
    """Load from file.

    This function will load into the symbolic object *obj* the data stored in the file called *name* using *df*
    as data format and *cf* as compression format. The possible values for *df* and *cf* are listed in the
    :py:class:`pyranha.data_format` and :py:class:`pyranha.compression` classes respectively.

    If *df* and *cf* are both ``None``, then the data and compression formats are inferred from the filename (see
    :py:func:`pyranha.save_file` for a detailed explanation and examples).

    :param obj: the symbolic object into which the file's data will be loaded
    :type obj: a supported symbolic type
    :param name: the source file name
    :type name: ``str``
    :param df: the desired data format (see :py:class:`pyranha.data_format`)
    :param cf: the desired compression format (see :py:class:`pyranha.compression`)

    :raises: :exc:`TypeError` if the file name is not a string
    :raises: :exc:`ValueError` if only one of *df* and *cf* is ``None``
    :raises: any exception raised by the invoked low-level C++ function

    """
    from ._core import _load_file
    _save_load_check_params(name, df, cf)
    if not df is None:
        _cpp_type_catcher(_load_file, obj, name, df, cf)
    else:
        _cpp_type_catcher(_load_file, obj, name)
