#ifndef CCMMC_HEADER_COMMON_H
#define CCMMC_HEADER_COMMON_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The name of this program, which can be used in error messages
#define prog_name ccmmc_main_name
extern const char *ccmmc_main_name;

// Calculate the length of an array
#define SIZEOF_ARRAY(x) (sizeof(x)/sizeof(x[0]))

// Make sure that we have a POSIX-compatible strerror_r function
#ifndef HAVE_STRERROR_R
# error "strerror_r() is not available"
#endif
#ifndef HAVE_DECL_STRERROR_R
# error "strerror_r() is not declared"
#endif
#ifdef STRERROR_R_CHAR_P
# ifdef _GNU_SOURCE
#  error "_GNU_SOURCE must not be defined"
# else
#  error "This program doesn't work with GNU-specific strerror_r()"
# endif
#endif

// Make multi-thread error reporting easier
#define ERR_LEN     1024
#define ERR_DECL    char err[ERR_LEN]
#define ERR_MSG     get_err_msg(errno, err, ERR_LEN)

static char *get_err_msg(int num, char *buf, size_t len) {
    int errno_save = errno;
    if (strerror_r (num, buf, len) != 0)
        snprintf (buf, len, "Unknown error %d", num);
    errno = errno_save;
    return buf;
}

#define ERR_FATAL_CHECK(result, function) \
    if (result == NULL) { \
        ERR_DECL; \
        fprintf(stderr, "%s: " #function ": %s\n", prog_name, ERR_MSG); \
        exit(1); \
    }

#endif
// vim: set sw=4 ts=4 sts=4 et:
