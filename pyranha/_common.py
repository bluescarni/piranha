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

from . import _core

# Trick to infer the type of the Boost ArgumentError exception.
# https://mail.python.org/pipermail/cplusplus-sig/2010-April/015471.html
try:
    _core._generate_argument_error()
except Exception as e:
    _BAE_type = type(e)


def _cpp_type_catcher(func, *args):
    # Decorator to prettify the type errors resulting when calling a C++ exposed function
    # with an invalid signature.
    try:
        return func(*args)
    except _BAE_type:
        raise TypeError('invalid argument type(s) for the C++ function \'{0}\': {1}'
                        .format(func.__name__, [type(_).__name__ for _ in args]))


def _repr_png_(self):
    # Render a series in png format using latex + dvipng.
    # Code adapted from and inspired by:
    # http://xyne.archlinux.ca/projects/tex2png
    from tempfile import mkdtemp, NamedTemporaryFile
    from subprocess import Popen, PIPE, STDOUT
    from shlex import split
    from shutil import rmtree
    from os.path import join
    import sys
    # Get the latex representation of the series.
    str_latex = r'\[ ' + self._latex_() + r' \]'
    tex_text = r"""
		\documentclass{article}
		\usepackage[paperwidth=\maxdimen,paperheight=\maxdimen]{geometry}
		\pagestyle{empty}
		\usepackage{amsmath}
		\usepackage{amssymb}
		\begin{document}
		\begin{samepage}"""
    # Need to cut out the math environment specifiers in str_latex.
    tex_text += str_latex + r"""
		\end{samepage}
		\end{document}"""
    # Create the temporary directory in which we are going to operate.
    tempd_name = mkdtemp(prefix='pyranha')
    try:
        # Create a temp filename in which we write the tex.
        tex_file = NamedTemporaryFile(
            dir=tempd_name, suffix=r'.tex', delete=False)
        if sys.version_info < (3, 0, 0):
            tex_file.write(tex_text)
        else:
            # NOTE: write in ascii, we know nothing about utf-8 in piranha.
            tex_file.write(bytes(tex_text, 'ascii'))
        tex_file.close()
        tex_filename = tex_file.name
        # Run latex.
        raw_command = r'latex -interaction=nonstopmode "' + tex_filename + r'"'
        proc = Popen(split(raw_command), cwd=tempd_name,
                     stdout=PIPE, stderr=STDOUT)
        output = proc.communicate()[0]
        if proc.returncode:
            raise RuntimeError(output)
        # Convert dvi to png.
        raw_command = r'dvipng -q -D 120 -T tight -bg "Transparent" -png -o "' + \
            tex_filename[0:-4] + r'.png" "' + tex_filename[0:-4] + r'.dvi"'
        proc = Popen(split(raw_command), cwd=tempd_name,
                     stdout=PIPE, stderr=STDOUT)
        output = proc.communicate()[0]
        if proc.returncode:
            raise RuntimeError(output)
        # Read png and return.
        png_file = open(join(tempd_name, tex_filename[0:-4] + r'.png'), 'rb')
        retval = png_file.read()
        png_file.close()
        return retval
    except Exception as e:
        # Let's just return None in case of errors. These include latex/dvipng
        # not being available, some problem in their execution (e.g., wrong tex syntax),
        # filesystem errors, etc.
        return None
    finally:
        # No matter what happens, always remove the temp directory with all the
        # content.
        rmtree(tempd_name)


def _register_repr_png():
    # Register the png representation method.
    from ._core import _get_exposed_types_list as getl
    for s_type in getl():
        setattr(s_type, '_repr_png_', _repr_png_)


def _register_repr_latex():
    # Register the latex representation method.
    from ._core import _get_exposed_types_list as getl
    for s_type in getl():
        setattr(s_type, '_repr_latex_',
                lambda self: r'\[ ' + self._latex_() + r' \]')


def _register_wrappers():
    # Register common wrappers.
    _register_repr_png()
    _register_repr_latex()


def _remove_hash():
    # Remove hashing from exposed types.
    from ._core import _get_exposed_types_list as getl
    for s_type in getl():
        setattr(s_type, '__hash__', None)


def _monkey_patching():
    # NOTE: here it is not clear to me if we should protect this with a global flag against multiple reloads.
    # Keep this in mind in case problem arises.
    # NOTE: it seems like concurrent import is not an issue:
    # http://stackoverflow.com/questions/12389526/import-inside-of-a-python-thread
    _register_wrappers()
    _remove_hash()
