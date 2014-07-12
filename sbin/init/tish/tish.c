/**
 *******************************************************************************
 * @file    tish.c
 * @author  Olli Vanhoja
 * @brief   Tiny Init Shell for debugging in init.
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

#include <sys/types.h>
#include <kstring.h>
#include <libkern.h>
#include <zeke.h>
#include <errno.h>
#include <unistd.h>
#include <syscall.h>
#include "tish.h"

extern void tish_sysctl_cmd(char ** args);
extern void tish_uname(char ** args);
extern void tish_ikut(char ** args);
extern void tish_debug(char ** args);
extern void tish_ls(char ** args);
extern void tish_touch(char ** args);
extern void tish_mkdir(char ** args);
extern void tish_mount(char ** args);

struct builtin {
    void (*fn)(char ** args);
    char name[10];
};

/* Builtins */

static void cd(char ** args);
static void uptime(char ** args);
static void reg(char ** args);
static void tish_exit(char ** args);
static void help(char ** args);

struct builtin cmdarr[] = {
        {cd, "cd"},
        {tish_sysctl_cmd, "sysctl"},
        {tish_uname, "uname"},
        {tish_ikut, "ikut"},
        {uptime, "uptime"},
        {tish_ls, "ls"},
        {tish_touch, "touch"},
        {tish_mkdir, "mkdir"},
        {tish_mount, "mount"},
        {reg, "reg"},
        {tish_debug, "debug"},
        {tish_exit, "exit"},
        {help, "help"}
};

int tish_eof;

/* Static functions */
static char * gline(char * str, int num);

int tish(void)
{
    char * cmd;
    char line[MAX_LEN];
    char * strtok_lasts;
    int err;

    while (1) {
        puts("# ");
        if (!gline(line, MAX_LEN))
            break;

        if ((cmd = kstrtok(line, DELIMS, &strtok_lasts))) {
            errno = 0;

            for (int i = 0; i < num_elem(cmdarr); i++) {
                if (!strcmp(cmd, cmdarr[i].name)) {
                    cmdarr[i].fn(&strtok_lasts);
                    goto get_errno;
                }
            }

            puts("I don't know how to execute\n");

get_errno:
            if ((err = errno)) {
                ksprintf(line, sizeof(line), "\nFailed, errno: %u\n", err);
                puts(line);
            }

            if (tish_eof)
                return 0;
        }
    }

    return 0;
}

static void cd(char ** args)
{
    //char * arg = kstrtok(0, DELIMS, args);
    puts("cd not implemented\n");
#if 0
    if (!arg)
        fprintf(stderr, "cd missing argument.\n");
    else
        chdir(arg);
#endif
}

static void uptime(char ** args)
{
    uint32_t loads[3];
    char buf[40];

    syscall(SYSCALL_SCHED_GET_LOADAVG, loads);

    ksprintf(buf, sizeof(buf), "load average: %u, %u, %u\n", loads[0], loads[1], loads[2]);
    puts(buf);
}

static void reg(char ** args)
{
    char * arg = kstrtok(0, DELIMS, args);
    char buf[40] = "Invalid argument\n";

    if (!strcmp(arg, "sp")) {
        void * sp;

        __asm__ ("mov %[result], sp" : [result] "=r" (sp));
        ksprintf(buf, sizeof(buf), "sp = %p\n", sp);
    } else if (!strcmp(arg, "cpsr")) {
        uint32_t mode;

        __asm__ ("mrs     %0, cpsr" : "=r" (mode));
        ksprintf(buf, sizeof(buf), "cpsr = %x\n", mode);
    }

    puts(buf);
}

static void tish_exit(char ** args)
{
    tish_eof = 1;
}

static void help(char ** args)
{
    char buf[20];

    for (int i = 0; i < num_elem(cmdarr); i++) {
        ksprintf(buf, sizeof(buf), "%s ", cmdarr[i].name);
        puts(buf);
    }
    puts("\n");
}

/* TODO Until we have some actual standard IO subsystem working the following
 * is the best hack I can think of. */
#define _ugetc() bcm2835_uart_ugetc()

static char * gline(char * str, int num)
{
    int err, i = 0;
    char ch;
    char buf[2] = {'\0', '\0'};

    while (1) {
        err = read(STDIN_FILENO, &ch, sizeof(ch));
        if (err <= 0) {
            msleep(150); /* TODO blocking instead of polling */
            continue;
        }

        /* Handle backspace */
        if (ch == 127) {
            if (i > 0) {
                i--;
                puts("\b \b");
            }
            continue;
        }

        /* TODO Handle arrow keys and delete */

        /* Handle return */
        if (ch == '\n' || ch == '\r' || i == num) {
            str[i] = '\0';
            buf[0] = '\n';
            puts(buf);
            return str;
        } else {
            str[i] = ch;
        }

        buf[0] = ch;
        puts(buf);
        i++;
    }
}
