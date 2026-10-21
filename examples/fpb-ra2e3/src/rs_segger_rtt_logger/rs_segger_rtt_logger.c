/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * rs_segger_rtt_logger.c — rs_segger_rtt_logger RS (Renesas Software): standard C stdio over SEGGER RTT.
 *
 * Job-to-be-done
 * --------------
 * Make the standard C library console (printf / fprintf / puts / putchar /
 * getchar / scanf / fgets on stdin / stdout / stderr) work over SEGGER RTT
 * instead of UART, WITHOUT changing any application code.
 *
 * How it works
 * ------------
 * Toolchain: LLVM (Arm Toolchain for Embedded) with picolibc. picolibc's
 * tinystdio routes every stream through per-character put/get callbacks of the
 * stdin / stdout / stderr FILE objects. Those objects are defined in
 * rs_segger_rtt_logger_picolibc.c, whose callbacks call the RTT logic in THIS file,
 * which routes every stream to SEGGER RTT channel 0:
 *
 *     printf / fputs / putchar  ->  put(c)  ->  rs_segger_rtt_logger_write_impl(1|2, ...)  ->  SEGGER_RTT_Write(0, ...)
 *     getchar / fgets / scanf   ->  get()   ->  rs_segger_rtt_logger_read_impl (0, ...)    ->  SEGGER_RTT_GetKey()
 *
 * No UART, no dynamic allocation, and no mandatory RTOS.
 * The SEGGER RTT ring buffer is the single buffer in the path (no duplicate
 * buffering). This file does not depend on the C library's stdio internals.
 *
 * Partial-write policy (truthful)
 * -------------------------------
 * Channel 0 runs in NO_BLOCK_TRIM mode, so SEGGER_RTT_Write() may accept fewer
 * bytes than requested (or zero, when no J-Link/Viewer is draining the buffer).
 * rs_segger_rtt_logger_write_impl() returns the ACTUAL number of bytes accepted
 * (0..len); it never fabricates a full-length success and never silently
 * discards output. If a write iteration makes no progress, it stops and returns
 * the truthful count so far, so the caller observes a short write rather than a
 * spin or a lie.
 *
 * Read semantics
 * --------------
 * rs_segger_rtt_logger_read_impl() blocks until at least one byte is available, then
 * returns all currently-available bytes up to len. It does NOT wait to fill the
 * full requested length — matching POSIX read() and the getchar()/fgets()
 * contract.
 *
 * While no input is available it waits in a loop:
 *   - Bare-metal (BSP_CFG_RTOS == 0): busy-wait, as before.
 *   - FreeRTOS (BSP_CFG_RTOS == 2): vTaskDelay(1) on every pass, so the waiting
 *     task blocks for one tick and lower-priority tasks and the idle task can
 *     run. Input (getchar / fgets / scanf) must then be called from a task after
 *     the scheduler has started — never from an ISR or a critical section.
 **********************************************************************************************************************/

#include "rs_segger_rtt_logger.h"
#include "SEGGER_RTT.h"

/* The FSP BSP configuration (BSP_CFG_RTOS) exists only in target builds. */
#if defined(__ARM_ARCH) && !defined(RS_SEGGER_RTT_HOST_TEST)
 #include "bsp_api.h"
#endif

#if defined(BSP_CFG_RTOS) && (2 == BSP_CFG_RTOS)
 #include "FreeRTOS.h"
 #include "task.h"
#endif

#include <errno.h>
#include <stddef.h>

/*──────────────────────────────────────────────────────────────────────────────
 * Configuration
 *────────────────────────────────────────────────────────────────────────────*/
#define RS_SEGGER_RTT_LOGGER_CHANNEL     (0U)   /* single "Terminal" channel */

#define RS_SEGGER_RTT_LOGGER_FD_STDIN    (0)
#define RS_SEGGER_RTT_LOGGER_FD_STDOUT   (1)
#define RS_SEGGER_RTT_LOGGER_FD_STDERR   (2)

/*──────────────────────────────────────────────────────────────────────────────
 * rs_segger_rtt_logger_write_impl — stdout / stderr -> RTT up-buffer (target -> J-Link).
 *
 * Called by the stdout / stderr put callbacks (rs_segger_rtt_logger_picolibc.c).
 *
 * @return  Number of bytes actually accepted by RTT (0..len), or -1 with errno
 *          set for an unsupported file descriptor.
 *────────────────────────────────────────────────────────────────────────────*/
int rs_segger_rtt_logger_write_impl(int fd, const char * buf, int len)
{
    const char * p;
    unsigned     remaining;
    unsigned     total;
    unsigned     n;

    if ((RS_SEGGER_RTT_LOGGER_FD_STDOUT != fd) && (RS_SEGGER_RTT_LOGGER_FD_STDERR != fd))
    {
        errno = EBADF;
        return -1;
    }
    if (NULL == buf)
    {
        errno = EFAULT;
        return -1;
    }
    if (len <= 0)
    {
        return 0;
    }

    /* No explicit init: SEGGER_RTT_Write() self-initialises the control block on
     * first use via the library's guarded INIT() (it inits only once, when the
     * "SEGGER RTT" ID is absent). Calling SEGGER_RTT_Init() here would be wrong
     * with the official engine — that entry point re-runs _DoInit() every call
     * and would wipe WrOff/RdOff, discarding buffered data on every write. */

    /* Cast is safe here: the len <= 0 guard above guarantees len >= 1. */
    p         = buf;
    remaining = (unsigned)len;
    total     = 0U;

    while (remaining > 0U)
    {
        n = SEGGER_RTT_Write(RS_SEGGER_RTT_LOGGER_CHANNEL, p, remaining);
        if (0U == n)
        {
            /* Buffer full / no host draining: report the truthful count so far
             * (may be 0). Do not spin and do not claim bytes were written. */
            break;
        }
        p         += n;
        total     += n;
        remaining -= n;
    }

    return (int)total;
}

/*──────────────────────────────────────────────────────────────────────────────
 * rs_segger_rtt_logger_read_impl — stdin <- RTT down-buffer (J-Link -> target).
 *
 * Called by the stdin get callback (rs_segger_rtt_logger_picolibc.c).
 *
 * Blocks until at least one byte is available, then returns all currently
 * available bytes up to len (never waits to fill the whole request).
 *
 * @return  Number of bytes read (>= 1 for len > 0), 0 for len == 0, or -1 with
 *          errno set for an unsupported file descriptor.
 *────────────────────────────────────────────────────────────────────────────*/
int rs_segger_rtt_logger_read_impl(int fd, char * buf, int len)
{
    int count;
    int c;

    if (RS_SEGGER_RTT_LOGGER_FD_STDIN != fd)
    {
        errno = EBADF;
        return -1;
    }
    if (NULL == buf)
    {
        errno = EFAULT;
        return -1;
    }
    if (len <= 0)
    {
        return 0;
    }

    /* No explicit init: SEGGER_RTT_HasKey()/_GetKey() self-initialise via the
     * library's guarded INIT(). SEGGER_RTT_Init() must NOT be called here — the
     * official entry point re-runs _DoInit() unconditionally and would reset the
     * down-buffer offsets, dropping keystrokes already sent by the host. */

    /* Block for the first byte (matches getchar() / blocking read()). */
    while (0 == SEGGER_RTT_HasKey())
    {
#if defined(BSP_CFG_RTOS) && (2 == BSP_CFG_RTOS)
        /* FreeRTOS: block for one tick so lower-priority tasks can run. */
        vTaskDelay((TickType_t) 1);
#else
        /* Bare-metal: busy-wait. */
#endif
    }

    /* Drain what is currently available, up to len. Do not wait for more. */
    count = 0;
    while (count < len)
    {
        c = SEGGER_RTT_GetKey();
        if (c < 0)
        {
            break;  /* nothing more available right now */
        }
        buf[count] = (char)c;
        ++count;
    }

    return count;
}