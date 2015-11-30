# vim: set sw=8 ts=8 sts=8 noet:

NULL =

EXTRA_DIST = autogen.sh Makefile.simple

bin_PROGRAMS = parser
noinst_LIBRARIES = libparser.a

AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src

parser_CFLAGS = $(WARN_CFLAGS)
parser_SOURCES =		\
	src/main.c		\
	src/common.h		\
	src/draw.c		\
	src/ast.h		\
	src/ast.c		\
	src/symbol-table.h	\
	src/symbol-table.c	\
	$(NULL)
parser_LDADD =			\
	libparser.a		\
	$(NULL)

libparser_a_YFLAGS = -d
libparser_a_SOURCES =		\
	src/lexer.l		\
	src/parser.y		\
	$(NULL)

BUILT_SOURCES =				\
	src/lexer.c			\
	src/libparser_a-parser.h	\
	$(NULL)

submit_dir_name = hw
submit_tarball_prefix = b01902054_b01902062
submit: dist
	tar -zxf $(DIST_ARCHIVES)
	mv $(distdir) $(submit_dir_name)
	cp -a $(submit_dir_name)/Makefile.simple $(submit_dir_name)/Makefile
	rm $(submit_dir_name)/src/lexer.c
	rm $(submit_dir_name)/src/libparser_a-parser.h
	rm $(submit_dir_name)/src/libparser_a-parser.c
	ver=`git rev-list HEAD | wc -l` && tar -jpcf \
	    $(submit_tarball_prefix)_ver$$ver.tar.bz2 $(submit_dir_name)
	rm -rf $(submit_dir_name)
