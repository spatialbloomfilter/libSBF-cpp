/*
    Spatial Bloom Filter C++ Library (libSBF-cpp)
    Copyright (C) 2017  Luca Calderoni, Dario Maio,
                        University of Bologna
    Copyright (C) 2017  Paolo Palmieri,
                        Cranfield University

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPORT_H
#define LIBEXPORT_H

#if __GNUC__ >= 4
  #define DLL_PUBLIC __attribute__ ((visibility ("default")))
  #define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
#else
  #define DLL_PUBLIC
  #define DLL_LOCAL
#endif

#ifdef __cplusplus
extern "C" {}
#endif

#endif /* LIBEXPORT_H */

