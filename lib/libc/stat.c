/**
 *******************************************************************************
 * @file    stat.c
 * @author  Olli Vanhoja
 * @brief   File status functions.
 * @section LICENSE
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <stdarg.h>
#if 0
#include <string.h>
#endif
#include <kstring.h>
#include <time.h>
#define strlen(x) strlenn(x, 4096) /* TODO REMOVE ME */
#include <syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

int chmod(const char * path, mode_t mode)
{
    int err;
    struct _fs_chmod_args args = {
        .mode = mode
    };

    args.fd = open(path, O_WRONLY);
    if (args.fd < 0)
        return -1;

    err = syscall(SYSCALL_FS_CHMOD, &args);

    close(args.fd);

    return err;
}

int fchmodat(int fd, const char * path, mode_t mode, int flag)
{
    int err;
    struct _fs_chmod_args args = {
        .mode = mode
    };

    args.fd = openat(fd, path, O_WRONLY, flag);
    if (args.fd < 0)
        return -1;

    err = syscall(SYSCALL_FS_CHMOD, &args);

    close(args.fd);

    return err;
}

int fchmod(int fildes, mode_t mode)
{
    struct _fs_chmod_args args = {
        .fd = fildes,
        .mode = mode
    };

    return syscall(SYSCALL_FS_CHMOD, &args);
}

int fstat(int fildes, struct stat * buf)
{
    struct _fs_stat_args args = {
        .fd = fildes,
        .buf = buf,
        .flags = AT_FDARG | O_EXEC
    };

    return syscall(SYSCALL_FS_STAT, &args);
}

int fstatat(int fd, const char * restrict path,
            struct stat * restrict buf, int flag)
{
    struct _fs_stat_args args = {
        .fd = fd,
        .path = path,
        .path_len = strlen(path) + 1,
        .buf = buf,
        .flags = AT_FDARG | flag
    };

    return syscall(SYSCALL_FS_STAT, &args);
}

int lstat(const char * restrict path, struct stat * restrict buf)
{
    struct _fs_stat_args args = {
        .path = path,
        .path_len = strlen(path) + 1,
        .buf = buf,
        .flags = AT_SYMLINK_NOFOLLOW
    };

    return syscall(SYSCALL_FS_STAT, &args);
}

int stat(const char * restrict path, struct stat * restrict buf)
{
    struct _fs_stat_args args = {
        .path = path,
        .path_len = strlen(path) + 1,
        .buf = buf
    };

    return syscall(SYSCALL_FS_STAT, &args);
}

int mkdir(const char * path, mode_t mode)
{
    struct _fs_mkdir_args args = {
        .path = path,
        .path_len = strlen(path) + 1,
        .mode = mode
    };

    return syscall(SYSCALL_FS_MKDIR, &args);
}

int rmdir(const char * path)
{
    struct _fs_rmdir_args args = {
        .path = path,
        .path_len = strlen(path) + 1
    };

    return syscall(SYSCALL_FS_RMDIR, &args);
}
