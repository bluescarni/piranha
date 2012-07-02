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

import _core

# Get a list of coefficients supported by series type named series_name.
def _get_cf_types(series_name):
	cf_list_getter = getattr(_core,'_' + series_name + '_get_coefficient_list')
	return [(t[1],type(t[0]) if not t[0] is None else None) for t in cf_list_getter()]

# Try to fetch a series types for series named series_name with coefficient cf_type.
def _get_series_type(series_name,cf_type):
	cf_list_getter = getattr(_core,'_' + series_name + '_get_coefficient_list')
	cfl = [(type(t[0]) if not t[0] is None else None,t[1],t[2]) for t in cf_list_getter()]
	# First let's search by type.
	cand = filter(lambda t: t[0] == cf_type if not t[0] is None else False,cfl)
	assert(len(cand) < 2)
	if len(cand) == 0:
		# Let's try to search by name if we had no matches.
		cand = filter(lambda t: t[1] == cf_type,cfl)
		assert(len(cand) < 2)
	if len(cand) == 0:
		raise TypeError('No series type available for this coefficient type.')
	# Build series name and get it from _core.
	s_name = filter(lambda s: s.startswith('_' + series_name + '_' + str(cand[0][2])),dir(_core))
	assert(len(s_name) == 1)
	return getattr(_core,s_name[0])
