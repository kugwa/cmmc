# vim: set sw=8 ts=8 sts=8 noet:

NULL =

EXTRA_DIST = autogen.sh

bin_PROGRAMS = scanner

scanner_LFLAGS = --header-file=lexer.h
scanner_SOURCES =	\
	main.c		\
	lexer.h		\
	lexer.l		\
	symbol-table.h	\
	symbol-table.c	\
	$(NULL)

lexer.h: scanner-lexer.c

BUILT_SOURCES = lexer.h
