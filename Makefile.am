# vim: set sw=8 ts=8 sts=8 noet:

NULL =

EXTRA_DIST = autogen.sh

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

BUILT_SOURCES = lexer.h
