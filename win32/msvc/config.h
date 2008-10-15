/* Use libjpeg instead of builtin jpeg decoder. */
#define ENABLE_LIBJPEG 1

/* Have FreeType2 include files */
#define HAVE_FREETYPE_H 1

/* Use splash for rendering. */
#define HAVE_SPLASH 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Enable multithreading support. */
#define MULTITHREADED 1

/* Name of package */
#define PACKAGE "poppler"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "poppler"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "poppler 0.10.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "poppler"

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.10.0"

/* Poppler data dir */
#define POPPLER_DATADIR "/usr/local/share/poppler"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Enable word list support. */
#define TEXTOUT_WORD_LIST 1

/* Throw exceptions to deal with not enough memory and similar problems */
/* #undef USE_EXCEPTIONS */

/* Version number of package */
#define VERSION "0.10.0"

#define snprintf _snprintf
#define unlink _unlink
#define fileno _fileno
#define setmode _setmode

