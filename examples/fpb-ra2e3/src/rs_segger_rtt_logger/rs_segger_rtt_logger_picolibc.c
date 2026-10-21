/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * rs_segger_rtt_logger_picolibc.c — picolibc stdio binding for rs_segger_rtt_logger (LLVM / Arm Toolchain for Embedded).
 *
 * picolibc's tinystdio implements every stream as a FILE object with per-character put / get /
 * flush callbacks, and the C library expects the application (or a host library such as
 * libsemihost) to define the global stdin / stdout / stderr objects. This file defines them and
 * connects the callbacks to the RTT logic in rs_segger_rtt_logger.c:
 *
 *     printf / puts / putchar   ->  put(c, stdout|stderr)  ->  rs_segger_rtt_logger_write_impl(1|2, &c, 1)
 *     getchar / fgets / scanf   ->  get(stdin)             ->  rs_segger_rtt_logger_read_impl(0, &c, 1)
 *
 * The streams are unbuffered: every character goes straight to the SEGGER RTT ring buffer, which
 * is the only buffer in the path. No heap is used.
 *
 * Partial-write policy: RTT channel 0 runs in NO_BLOCK_TRIM mode. When a character cannot be
 * accepted (buffer full / no host draining), put returns EOF — it never reports a character as
 * written when it was not.
 *
 * Do not link libsemihost or libdummyhost: they define stdin / stdout / stderr as well.
 **********************************************************************************************************************/

#include <stdio.h>

#if !defined(__PICOLIBC__)
 #error "rs_segger_rtt_logger_picolibc.c requires picolibc (LLVM / Arm Toolchain for Embedded)."
#endif

#include "rs_segger_rtt_logger.h"

/* File descriptors understood by rs_segger_rtt_logger_write_impl / rs_segger_rtt_logger_read_impl. */
#define RS_SEGGER_RTT_LOGGER_PICOLIBC_FD_STDIN     (0)
#define RS_SEGGER_RTT_LOGGER_PICOLIBC_FD_STDOUT    (1)
#define RS_SEGGER_RTT_LOGGER_PICOLIBC_FD_STDERR    (2)

static int rs_segger_rtt_logger_picolibc_put(int fd, char c)
{
    /* Truthful policy: report EOF if RTT did not accept the character. */
    if (1 != rs_segger_rtt_logger_write_impl(fd, &c, 1))
    {
        return EOF;
    }

    return (int) (unsigned char) c;
}

static int rs_segger_rtt_logger_picolibc_putc_stdout(char c, FILE * f)
{
    (void) f;

    return rs_segger_rtt_logger_picolibc_put(RS_SEGGER_RTT_LOGGER_PICOLIBC_FD_STDOUT, c);
}

static int rs_segger_rtt_logger_picolibc_putc_stderr(char c, FILE * f)
{
    (void) f;

    return rs_segger_rtt_logger_picolibc_put(RS_SEGGER_RTT_LOGGER_PICOLIBC_FD_STDERR, c);
}

static int rs_segger_rtt_logger_picolibc_getc(FILE * f)
{
    char c = 0;

    (void) f;

    /* Blocks until at least one byte is available (getchar() semantics). */
    if (1 != rs_segger_rtt_logger_read_impl(RS_SEGGER_RTT_LOGGER_PICOLIBC_FD_STDIN, &c, 1))
    {
        return EOF;
    }

    return (int) (unsigned char) c;
}

static int rs_segger_rtt_logger_picolibc_flush(FILE * f)
{
    (void) f;

    /* Nothing to flush: the SEGGER RTT ring buffer is the only buffer in the path. */
    return 0;
}

static FILE g_rs_segger_rtt_logger_stdin = FDEV_SETUP_STREAM(NULL,
                                                       rs_segger_rtt_logger_picolibc_getc,
                                                       NULL,
                                                       _FDEV_SETUP_READ);

static FILE g_rs_segger_rtt_logger_stdout = FDEV_SETUP_STREAM(rs_segger_rtt_logger_picolibc_putc_stdout,
                                                        NULL,
                                                        rs_segger_rtt_logger_picolibc_flush,
                                                        _FDEV_SETUP_WRITE);

static FILE g_rs_segger_rtt_logger_stderr = FDEV_SETUP_STREAM(rs_segger_rtt_logger_picolibc_putc_stderr,
                                                        NULL,
                                                        rs_segger_rtt_logger_picolibc_flush,
                                                        _FDEV_SETUP_WRITE);

FILE * const stdin  = &g_rs_segger_rtt_logger_stdin;
FILE * const stdout = &g_rs_segger_rtt_logger_stdout;
FILE * const stderr = &g_rs_segger_rtt_logger_stderr;
