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

# Decorator to prettify the type errors resulting when calling a C++ exposed function
# with an invalid signature.
def _cpp_type_catcher(func,*args):
	try:
		return func(*args)
	except TypeError:
		raise TypeError('invalid argument type(s) for the C++ function \'{0}\': {1}'\
			.format(func.__name__,[type(_).__name__ for _ in args]))

# Function to be run at module unload to clear registered custom derivatives.
# The rationale is that custom derivatives will contain Python objects, and we
# want to remove them before the C++ library exits.
def _cleanup_custom_derivatives():
	from ._core import _get_exposed_types_list as getl
	for s_type in getl():
		if hasattr(s_type,'unregister_all_custom_derivatives'):
			s_type.unregister_all_custom_derivatives()
	print('Custom derivatives cleanup completed.')

# NOTE: this is probably not needed at the moment as there is no way Python objects
# end up in the cache, but it might happen in the future.
def _cleanup_pow_caches():
	from ._core import _get_exposed_types_list as getl
	for s_type in getl():
		if hasattr(s_type,'clear_pow_cache'):
			s_type.clear_pow_cache()
	print('Pow caches cleanup completed.')

# Render a series in png format using latex + dvipng.
# Code adapted from and inspired by:
# http://xyne.archlinux.ca/projects/tex2png
def _repr_png_(self):
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
	tempd_name = mkdtemp(prefix = 'pyranha')
	try:
		# Create a temp filename in which we write the tex.
		tex_file = NamedTemporaryFile(dir = tempd_name, suffix = r'.tex', delete = False)
		if sys.version_info < (3,0,0):
			tex_file.write(tex_text)
		else:
			# NOTE: write in ascii, we know nothing about utf-8 in piranha.
			tex_file.write(bytes(tex_text,'ascii'))
		tex_file.close()
		tex_filename = tex_file.name
		# Run latex.
		raw_command = r'latex -interaction=nonstopmode "' + tex_filename + r'"'
		proc = Popen(split(raw_command), cwd = tempd_name, stdout = PIPE, stderr = STDOUT)
		output = proc.communicate()[0]
		if proc.returncode:
			raise RuntimeError(output)
		# Convert dvi to png.
		raw_command = r'dvipng -q -D 120 -T tight -bg "Transparent" -png -o "' + tex_filename[0:-4] + r'.png" "' + tex_filename[0:-4] + r'.dvi"'
		proc = Popen(split(raw_command), cwd = tempd_name, stdout = PIPE, stderr = STDOUT)
		output = proc.communicate()[0]
		if proc.returncode:
			raise RuntimeError(output)
		# Read png and return.
		png_file = open(join(tempd_name,tex_filename[0:-4] + r'.png'),'rb')
		retval = png_file.read()
		png_file.close()
		return retval
	finally:
		# No matter what happens, always remove the temp directory with all the content.
		rmtree(tempd_name)

# Register the png representation method.
def _register_repr_png():
	from ._core import _get_exposed_types_list as getl
	for s_type in getl():
		setattr(s_type,'_repr_png_',_repr_png_)

# Register the latex representation method.
def _register_repr_latex():
	from ._core import _get_exposed_types_list as getl
	for s_type in getl():
		setattr(s_type,'_repr_latex_',lambda self: r'\[ ' + self._latex_() + r' \]')

# Register common wrappers.
def _register_wrappers():
	_register_repr_png()
	_register_repr_latex()

# Monkey patch the generic type generator class to accept normal args instead of a list.
def _replace_gtg_call():
	_orig_gtg_call = _core._generic_type_generator.__call__
	def _gtg_call_wrapper(self,*args):
		l_args = list(args)
		if not all([isinstance(_,_core._type_generator) for _ in l_args]):
			raise TypeError('all the arguments must be type generators')
		return _orig_gtg_call(self,l_args)
	_core._generic_type_generator.__call__ = _gtg_call_wrapper

# Remove hashing from exposed types.
def _remove_hash():
	from ._core import _get_exposed_types_list as getl
	for s_type in getl():
		setattr(s_type,'__hash__',None)

def _monkey_patching():
	# NOTE: here it is not clear to me if we should protect this with a global flag against multiple reloads.
	# Keep this in mind in case problem arises.
	# NOTE: it seems like concurrent import is not an issue:
	# http://stackoverflow.com/questions/12389526/import-inside-of-a-python-thread
	_register_wrappers()
	_replace_gtg_call()
	_remove_hash()

# Cleanup function.
def _cleanup():
	_cleanup_custom_derivatives()
	_cleanup_pow_caches()
	_core._cleanup_type_system()
	print("Pyranha type system cleanup completed.")
