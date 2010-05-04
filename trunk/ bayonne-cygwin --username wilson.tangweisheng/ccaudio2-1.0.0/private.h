/* private.h.  Generated from private.h.in by configure.  */
/* private.h.in.  Generated from configure.ac by autoheader.  */

#ifndef W32
#if defined(_WIN32) && defined(_MSC_VER)
#define W32
#endif
#if defined(__BORLANDC__) && defined(__Windows)
#define W32
#endif
#endif

#if !defined(__EXPORT) && defined(W32)
#define __EXPORT __declspec(dllexport)
#endif

#ifdef	W32
#include "w32/private.h"
#else	// w32 uses hardcoded config



/* linking flags */
#define AUDIO_LIBRARY "-lccaudio2 -ldl -lwinmm -lm"

/* define codec path */
#define CODEC_LIBPATH "/usr/local/lib/ccaudio2-1.0"

/* module flags */
#define CODEC_MODFLAGS "-module -shared -no-undefined -avoid-version"

/* Define to 1 if you have the <alloca.h> header file. */
#define HAVE_ALLOCA_H 1

/* Define to 1 if you have the <CoreAudio/CoreAudio.h> header file. */
/* #undef HAVE_COREAUDIO_COREAUDIO_H */

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* have endian header */
#define HAVE_ENDIAN_H 1

/* gsm header found */
/* #undef HAVE_GSM_GSM_H */

/* gsm default header */
/* #undef HAVE_GSM_H */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `malloc' library (-lmalloc). */
/* #undef HAVE_LIBMALLOC */

/* mach dybloader */
/* #undef HAVE_MACH_DYLD */

/* Define to 1 if you have the <mach-o/dyld.h> header file. */
/* #undef HAVE_MACH_O_DYLD_H */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* support for plugin modules */
#define HAVE_MODULES 1

/* posix threading header */
#define HAVE_PTHREAD_H 1

/* have shload plugins */
/* #undef HAVE_SHL_LOAD */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#define HAVE_STRCASECMP 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* solaris endian */
/* #undef HAVE_SYS_ISA_DEFS_H */

/* Define to 1 if you have the <sys/soundcard.h> header file. */
#define HAVE_SYS_SOUNDCARD_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* OSX Core Audio */
/* #undef OSX_AUDIO */

/* Name of package */
#define PACKAGE "ccaudio2"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME ""

/* Define to the full name and version of this package. */
#define PACKAGE_STRING ""

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME ""

/* Define to the version of this package. */
#define PACKAGE_VERSION ""

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "1.0.0"

/* endian byte order */
/* #undef __BYTE_ORDER */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to equivalent of C99 restrict keyword, or to nothing if this is not
   supported. Do not define if restrict is supported directly. */
#define restrict __restrict

/* Define to empty if the keyword `volatile' does not work. Warning: valid
   code using `volatile' can become incorrect without. Disable with care. */
/* #undef volatile */

#ifndef	NO_CPP

// hidden mutex class

#ifdef	HAVE_PTHREAD_H
#include <pthread.h>
#endif

class ccAudio_Mutex_
{
private:
#ifdef	HAVE_PTHREAD_H
        pthread_mutex_t mutex;
#endif

public:
        ccAudio_Mutex_();
        ~ccAudio_Mutex_();
        void enter(void);
        void leave(void);
}; 

#endif
#endif // end of internal configures




#if defined(HAVE_ENDIAN_H)
 #include <endian.h>
#elif defined(HAVE_SYS_ISA_DEFS_H)
 #include <sys/isa_defs.h>
 #ifdef	_LITTLE_ENDIAN
  #define	__BYTE_ORDER 1234
 #else
  #define	__BYTE_ORDER 4321
 #endif
 #if _ALIGNMENT_REQUIRED > 0
  #define	__BYTE_ALIGNMENT _MAX_ALIGNMENT
 #else
  #define	__BYTE_ALIGNMENT 1
 #endif
#endif

#ifndef	__LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321
#endif

#ifndef	__BYTE_ORDER
#define	__BYTE_ORDER 1234
#endif

#ifndef	__BYTE_ALIGNMENT
#if defined(SPARC) || defined(sparc)
#if defined(__arch64__) || defined(__sparcv9)
#define	__BYTE_ALIGNMENT 8
#else
#define	__BYTE_ALIGNMENT 4
#endif
#endif
#endif

#ifndef	__BYTE_ALIGNMENT
#define	__BYTE_ALIGNMENT 1
#endif

	


#ifndef	NO_CPP
#include <cstring>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#ifndef HAVE_SNPRINTF
#ifdef  WIN32
#define snprintf        _snprintf
#define vsnprintf       _vsnprintf
#endif
#endif

#ifdef HAVE_STRCASECMP
#ifndef stricmp
#define stricmp(x,y) strcasecmp(x,y)
#endif
#ifndef strnicmp
#define strnicmp(x,y,n) strncasecmp(x,y,n)
#endif
#endif


	
