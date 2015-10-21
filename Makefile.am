# vim: set sw=8 ts=8 sts=8 noet:

NULL =

EXTRA_DIST = autogen.sh Makefile.simple

bin_PROGRAMS = scanner
noinst_LIBRARIES = liblexer.a

scanner_CFLAGS = $(WARN_CFLAGS)
scanner_SOURCES =	\
	main.c		\
	lexer.h		\
	symbol-table.h	\
	symbol-table.c	\
	$(NULL)
scanner_LDADD =		\
	liblexer.a	\
	$(NULL)

liblexer_a_LFLAGS = --header-file=lexer.h
liblexer_a_SOURCES =	\
	lexer.l		\
	$(NULL)
lexer.h: liblexer_a-lexer.c

BUILT_SOURCES = lexer.h

submit_dir_name = hw
submit_tarball_prefix = b01902054_b01902062
submit: dist
	tar -zxf $(DIST_ARCHIVES)
	mv $(distdir) $(submit_dir_name)
	cp -a $(submit_dir_name)/Makefile.simple $(submit_dir_name)/Makefile
	rm $(submit_dir_name)/liblexer_a-lexer.c
	rm $(submit_dir_name)/lexer.h
	ver=`git rev-list HEAD | wc -l` && tar -jpcf \
	    $(submit_tarball_prefix)_ver$$ver.tar.bz2 $(submit_dir_name)
	rm -rf $(submit_dir_name)
