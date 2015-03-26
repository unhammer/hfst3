//       This program is free software: you can redistribute it and/or modify
//       it under the terms of the GNU General Public License as published by
//       the Free Software Foundation, version 3 of the License.
//
//       This program is distributed in the hope that it will be useful,
//       but WITHOUT ANY WARRANTY; without even the implied warranty of
//       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//       GNU General Public License for more details.
//
//       You should have received a copy of the GNU General Public License
//       along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef _FORMAT_SPECIFIERS_H_
#define _FORMAT_SPECIFIERS_H_

#if defined (_MSC_VER) || (__MINGW32__)
  #define SIZE_T_SPECIFIER    "%Iu"
  #define SSIZE_T_SPECIFIER   "%Id"
  #define PTRDIFF_T_SPECIFIER "%Id"
  #define LONG_LONG_SPECIFIER "%I64d"
#else
  #define SIZE_T_SPECIFIER    "%zu"
  #define SSIZE_T_SPECIFIER   "%zd"
  #define PTRDIFF_T_SPECIFIER "%zd"
  #define LONG_LONG_SPECIFIER "%lld"
#endif

#endif
