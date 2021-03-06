/*
 * isalnum(int)
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <ctype.h>
#include <sys/_PDCLIB_locale.h>

int isalnum(int c)
{
    return (_PDCLIB_threadlocale()->_CType[c].flags &
            (_PDCLIB_CTYPE_ALPHA | _PDCLIB_CTYPE_DIGIT));
}
