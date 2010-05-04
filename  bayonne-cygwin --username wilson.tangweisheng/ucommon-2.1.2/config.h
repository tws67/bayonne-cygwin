/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* has clocked nanosleep */
/* #undef HAVE_CLOCK_NANOSLEEP */

/* Define to 1 if you have the <dirent.h> header file. */
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <endian.h> header file. */
#define HAVE_ENDIAN_H 1

/* has ftok */
#define HAVE_FTOK 1

/* have getaddrinfo */
#define HAVE_GETADDRINFO 1

/* have gethostbyname2 */
#define HAVE_GETHOSTBYNAME2 1

/* have inet_ntop */
#define HAVE_INET_NTOP 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <linux/version.h> header file. */
/* #undef HAVE_LINUX_VERSION_H */

/* has localtime_r */
#define HAVE_LOCALTIME_R 1

/* Define to 1 if you have the <mach/clock.h> header file. */
/* #undef HAVE_MACH_CLOCK_H */

/* Define to 1 if you have the <mach-o/dyld.h> header file. */
/* #undef HAVE_MACH_O_DYLD_H */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* has nanosleep */
#define HAVE_NANOSLEEP 1

/* Define to 1 if you have the <poll.h> header file. */
#define HAVE_POLL_H 1

/* has posix clock functions */
#define HAVE_POSIXTIME 1

/* can specify access options */
#define HAVE_POSIX_FADVISE 1

/* posix memory alignment */
#define HAVE_POSIX_MEMALIGN 1

/* "pthread clocking" */
/* #undef HAVE_PTHREAD_CONDATTR_SETCLOCK */

/* "pthread delay" */
/* #undef HAVE_PTHREAD_DELAY */

/* "pthread delay np" */
/* #undef HAVE_PTHREAD_DELAY_NP */

/* "pthread concurrency" */
#define HAVE_PTHREAD_SETCONCURRENCY 1

/* "pthread scheduling" */
/* #undef HAVE_PTHREAD_SETSCHEDPRIO */

/* "pthread yield" */
/* #undef HAVE_PTHREAD_YIELD */

/* alternate dyn loader */
/* #undef HAVE_SHL_LOAD */

/* shared memory open */
#define HAVE_SHM_OPEN 1

/* have socketpair in libsocket */
#define HAVE_SOCKETPAIR 1

/* Define to 1 if you have the <stdexcept> header file. */
#define HAVE_STDEXCEPT 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* string icmp */
/* #undef HAVE_STRICMP */

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* string istr */
/* #undef HAVE_STRISTR */

/* system config */
#define HAVE_SYSCONF 1

/* Define to 1 if you have the <sys/filio.h> header file. */
/* #undef HAVE_SYS_FILIO_H */

/* Define to 1 if you have the <sys/mman.h> header file. */
#define HAVE_SYS_MMAN_H 1

/* Define to 1 if you have the <sys/poll.h> header file. */
#define HAVE_SYS_POLL_H 1

/* Define to 1 if you have the <sys/resource.h> header file. */
#define HAVE_SYS_RESOURCE_H 1

/* Define to 1 if you have the <sys/shm.h> header file. */
#define HAVE_SYS_SHM_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/timeb.h> header file. */
#define HAVE_SYS_TIMEB_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Name of package */
#define PACKAGE "ucommon"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME ""

/* Define to the full name and version of this package. */
#define PACKAGE_STRING ""

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME ""

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION ""

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "2.1.2"

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to the equivalent of the C99 'restrict' keyword, or to
   nothing if this is not supported.  Do not define if restrict is
   supported directly.  */
#define restrict __restrict
/* Work around a bug in Sun C++: it does not support _Restrict or
   __restrict__, even though the corresponding Sun C compiler ends up with
   "#define restrict _Restrict" or "#define restrict __restrict__" in the
   previous line.  Perhaps some future version of Sun C++ will work with
   restrict; if so, hopefully it defines __RESTRICT like Sun C does.  */
#if defined __SUNPRO_CC && !defined __RESTRICT
# define _Restrict
# define __restrict__
#endif

/* Define to empty if the keyword `volatile' does not work. Warning: valid
   code using `volatile' can become incorrect without. Disable with care. */
/* #undef volatile */


#include <ucommon/export.h>

