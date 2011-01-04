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

/* Poppler data dir */
#define POPPLER_DATADIR "poppler-data"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Enable word list support. */
#define TEXTOUT_WORD_LIST 1

/* Throw exceptions to deal with not enough memory and similar problems */
/* #undef USE_EXCEPTIONS */

#define snprintf _snprintf
#define unlink _unlink
#define fileno _fileno
#define setmode _setmode

