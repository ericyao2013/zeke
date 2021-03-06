/**
 *******************************************************************************
 * @file    paths.h
 * @author  Olli Vanhoja
 * @brief
 * @section LICENSE
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 */

/**
 * @addtogroup LIBC
 * @{
 */

/**
 * @addtogroup paths
 * @{
 */

#ifndef _PATHS_H_
#define _PATHS_H_

/**
 * Default search path.
 */
#define _PATH_DEFPATH   "/usr/bin:/bin:"

#define _PATH_PASSWD    "/etc/passwd"   /*!< passwd file location. */
#define _PATH_SHADOW    "/etc/shadow"   /*!< shadow file location. */
#define _PATH_GROUP     "/etc/group"    /*!< group file location. */

/**
 * All standard utilities path.
 */
#define _PATH_STDPATH   "/usr/bin:/bin:/usr/sbin:/sbin:"

/* Utils */
#define _PATH_BSHELL    "/bin/sh"       /*!< Default shell location. */
#define _PATH_ECHO      "/bin/echo"     /*!< Echo command location. */

#endif /* _PATHS_H_ */

/**
 * @}
 */

/**
 * @}
 */
