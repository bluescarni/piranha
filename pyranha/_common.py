# -*- coding: iso-8859-1 -*-
# Copyright (C) 2009-2011 by Francesco Biscani
# bluescarni@gmail.com
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the
# Free Software Foundation, Inc.,
# 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

from __future__ import absolute_import as _ai

from . import _core

# Get a list of coefficients supported by series type named series_name.
def _get_cf_types(series_name):
	cf_list_getter = getattr(_core,'_' + series_name + '_get_coefficient_list')
	return [(t[1],type(t[0]) if not t[0] is None else None) for t in cf_list_getter()]

# Try to fetch a series types for series named series_name with coefficient cf_type.
def _get_series_type(series_name,cf_type):
	cf_list_getter = getattr(_core,'_' + series_name + '_get_coefficient_list')
	cfl = [(type(t[0]) if not t[0] is None else None,t[1],t[2]) for t in cf_list_getter()]
	# First let's search by type.
	cand = list(filter(lambda t: t[0] == cf_type if not t[0] is None else False,cfl))
	assert(len(cand) < 2)
	if len(cand) == 0:
		# Let's try to search by name if we had no matches.
		cand = list(filter(lambda t: t[1] == cf_type,cfl))
		assert(len(cand) < 2)
	if len(cand) == 0:
		raise TypeError('no series type available for this coefficient type')
	# Build series name and get it from _core.
	s_name = list(filter(lambda s: s.startswith('_' + series_name + '_' + str(cand[0][2])),dir(_core)))
	assert(len(s_name) == 1)
	return getattr(_core,s_name[0])

# Function to be run at module unload to clear registered custom derivatives.
# The rationale is that custom derivatives will contain Python objects, and we
# want to remove them before the C++ library exits.
def _cleanup_custom_derivatives(series_name):
	import re
	s_names = list(filter(lambda s: re.match('\_' + series_name + '\_\d+',s),dir(_core)))
	for s in s_names:
		print('Unregistering custom derivatives for: ' + s)
		s_type = getattr(_core,s)
		s_type.unregister_all_custom_derivatives()

# Wrapper for the evaluate() method. Will first check input dict,
# and then try to invoke the underlying C++ exposed method.
def _evaluate_wrapper(self,d):
	# Type checks.
	if not isinstance(d,dict):
		raise TypeError('evaluation dictionary must be a dict object')
	if len(d) == 0:
		raise ValueError('evaluation dictionary cannot be empty')
	if not all([isinstance(k,str) for k in d]):
		raise TypeError('all keys in the evaluation dictionary must be string objects')
	t_set = set([type(d[k]) for k in d])
	if not len(t_set) == 1:
		raise TypeError('all values in the evaluation dictionary must be of the same type')
	try:
		return self._evaluate(d,d[list(d.keys())[0]])
	except TypeError:
		raise TypeError('cannot evaluate with values of type ' + type(d[list(d.keys())[0]]).__name__)

# Register the evaluate wrapper for a particular series.
def _register_evaluate_wrapper(series_name):
	import re
	s_names = list(filter(lambda s: re.match('\_' + series_name + '\_\d+',s),dir(_core)))
	for s in s_names:
		s_type = getattr(_core,s)
		setattr(s_type,'evaluate',_evaluate_wrapper)

# Render a series in png format using latex + dvipng.
# Code adapted from and inspired by:
# http://xyne.archlinux.ca/projects/tex2png
def _repr_png_(self):
	from tempfile import mkdtemp, NamedTemporaryFile
	from subprocess import Popen, PIPE, STDOUT
	from shlex import split
	from shutil import rmtree
	from os.path import join
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
		return retval
	finally:
		# No matter what happens, always remove the temp directory with all the content.
		rmtree(tempd_name)

# Register the png representation method for a particular series.
def _register_repr_png(series_name):
	import re
	s_names = list(filter(lambda s: re.match('\_' + series_name + '\_\d+',s),dir(_core)))
	for s in s_names:
		s_type = getattr(_core,s)
		setattr(s_type,'_repr_png_',_repr_png_)

# Register the latex representation method for a particular series.
def _register_repr_latex(series_name):
	import re
	s_names = list(filter(lambda s: re.match('\_' + series_name + '\_\d+',s),dir(_core)))
	for s in s_names:
		s_type = getattr(_core,s)
		setattr(s_type,'_repr_latex_',lambda self: r'\[ ' + self._latex_() + r' \]')
