#ifndef __ISNAN_H__
#define __ISNAN_H__

/*
 * Temporary fix for various misdefinitions of isnan().
 * isnan() is becoming undef'd in some .h files. 
 * #include thislast in your .cpp file to get it right.
 *
 * Authors:
 *   Inkscape groupies and obsessive-compulsives
 *
 * Copyright (C) 1999-2002 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <math.h>



#ifdef __APPLE__

/* MacOSX definition */
#define isNaN(a) (__isnan(a))

#else

#ifdef WIN32

/* Win32 definition */
#define isNaN(a) (_isnan(a))

#else

/* Linux definition */
#define isNaN(a) (isnan(a))

#endif /* WIN32 */

#endif /* __APPLE__ */





#endif /* __ISNAN_H__ */



