/**
 *******************************************************************************
 * @file    mount.h
 * @author  Olli Vanhoja
 * @brief   Mount or dismount a file system.
 * @section LICENSE
 * Copyright (c) 2020 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2014 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#ifndef MOUNT_H
#define MOUNT_H

#include <sys/types/_fsblkcnt_t.h>
#include <sys/types/_fsfilcnt_t.h>

#define MNT_RDONLY      0x0001 /*!< Read only. */
#define MNT_SYNCHRONOUS 0x0002 /*!< Synchronous writes. */
#define MNT_ASYNC       0x0040 /*!< Asynchronous writes. */
#define MNT_NOEXEC      0x0004 /*!< No exec for the file system. */
#define MNT_NOSUID      0x0008 /*!< Set uid bits not honored. */
#define MNT_NOATIME     0x0100 /*!< Don't update file access times. */

#include <sys/types/_mfsnamelen.h>

struct statfs {
    unsigned long   f_bsize;    /*!< File system block size. */
    unsigned long   f_frsize;   /*!< Fragment size. */
    fsblkcnt_t      f_blocks;   /*!< Size of fs in f_frsize units. */
    fsblkcnt_t      f_bfree;    /*!< Number of free blocks */
    fsblkcnt_t      f_bavail;   /*!< Number of free blocks available to
                                 *   non-privileged process. */
    fsfilcnt_t      f_files;    /*!< Total number of inodes. */
    fsfilcnt_t      f_ffree;    /*!< Total number of free inodes. */
    fsfilcnt_t      f_favail;   /*!< Total number of free inodes to
                                 *   non-privileged process. */
    unsigned long   f_fsid;     /*!< Filesystem ID. */
    unsigned long   f_flag;     /*!< Mount flags. */
    unsigned long   f_namemax;  /*!< Maximum filename length. */
    char            f_fsname[MFSNAMELEN]; /*!< File system name. */
};

#if defined(__SYSCALL_DEFS__) || defined(KERNEL_INTERNAL)
#include <stddef.h>
#include <stdint.h>

/**
 * Arguments struct for SYSCALL_FS_MOUNT
 */
struct _fs_mount_args {
    const char * source;
    size_t source_len; /*!< in bytes */
    const char * target;
    size_t target_len; /*!< in bytes */
    const char fsname[MFSNAMELEN];
    uint32_t flags;
    const char * parm;
    size_t parm_len; /*!< in bytes */
};

/**
 * Arguments struct for SYSCALL_FS_UMOUNT
 */
struct _fs_umount_args {
    const char * target;
    size_t target_len; /*!< in bytes */
};

/** Arguments for SYSCALL_FS_STATFS */
struct _fs_statfs_args {
    int fd;
    const char * path;
    size_t path_len;
    struct statfs * buf;
    unsigned flags;
};

/** Arguments for SYSCALL_FS_GETFSSTAT */
struct _fs_getfsstat_args {
    struct statfs * buf;
    size_t bufsize;
    unsigned flags;
};
#endif /* __SYSCALL_DEFS__ */

#ifndef KERNEL_INTERNAL
__BEGIN_DECLS

/**
 * The mount() system call attaches the file system specified by source to the
 * directory specified by target.
 * @throws + EFAULT     One of the argument pointers is outside of the process's
 *                      allocated address space.
 *         + ENOMEM     Not enough memory available to mount a new file system.
 *         + ENOENT     Mount target doesn't exist.
 *         + ENOTSUP    File system type is not supported.
 *         + ENODEV     Mount failed. TODO ??
 */
int mount(const char * source, const char * target, const char * type,
          int flags, char * parms);

int umount(const char * target);

int fstatfs(int fildes, struct statfs * buf);
int fstatfsat(int fildes, const char * path, struct statfs * buf);
int statfs(const char * restrict path, struct statfs * restrict buf);

/**
 * Get list of all mounted file systems.
 * @param buf       is a pointer to the buffer; NULL if peeking the required buffer size.
 * @param bufsize   is the buffer size; 0 if peeking the required buffer size.
 * @param flags     No flags specified.
 * @returns Returns the size of stats written in bytes if successful;
 *          Otherwise returns -1 and sets errno.
 */
int getfsstat(struct statfs * buf, long bufsize, int flags);

__END_DECLS
#endif /* !KERNEL_INTERNAL */

#endif /* MOUNT_H */

/**
 * @}
 */
