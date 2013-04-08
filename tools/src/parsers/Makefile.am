## Process this file with automake to produce Makefile.in

## Copyright (C) 2013 University of Helsinki

## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.

## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
## GNU General Public License for more details.

## You should have received a copy of the GNU General Public License
## along with this program. If not, see <http://www.gnu.org/licenses/>.

SUBDIRS=test

BUILT_SOURCES=xfst-parser.h xfst-parser.cc xfst-lexer.cc

AM_YFLAGS=-d
AM_LFLAGS=--outfile=$(LEX_OUTPUT_ROOT).c

AM_CPPFLAGS = -I${top_srcdir}/libhfst/src -I${top_srcdir}/libhfst/src/parsers -I${top_srcdir}/tools/src $(GLIB_CPPFLAGS) -Wno-deprecated

if WANT_XFST2FST
MAYBE_XFST=hfst-xfst2fst
endif

noinst_HEADERS = XfstCompiler.h xfst-utils.h xfst-parser.h

HFST_COMMON_SRC=../hfst-program-options.cc ../hfst-commandline.cc ../hfst-tool-metadata.cc

bin_PROGRAMS=$(MAYBE_XFST)
hfst_xfst2fst_SOURCES = xfst-lexer.ll xfst-parser.yy xfst-utils.cc XfstCompiler.cc hfst-xfst2fst.cc $(HFST_COMMON_SRC)
hfst_xfst2fst_LDADD = $(top_builddir)/libhfst/src/libhfst.la $(GLIB_LIBS) -lreadline

#xfst-parser.h: xfst-parser.cc

xfst-parser.cc: xfst-parser.yy

xfst-lexer.cc: xfst-lexer.ll