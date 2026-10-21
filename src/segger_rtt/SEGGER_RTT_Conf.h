/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * SEGGER_RTT_Conf.h — repository RTT configuration for the rs_segger_rtt_logger RS.
 *
 * Renesas custom configuration. This file is NOT part of the SEGGER sources:
 * it replaces Config/SEGGER_RTT_Conf.h of SEGGER RTT V8.58
 * (github.com/SEGGERMicro/RTT). In V8.58 SEGGER_RTT_ConfDefaults.h includes
 * this file first and then supplies a default only for each setting that is
 * not defined here, so the values below take precedence. SEGGER_RTT.c,
 * SEGGER_RTT.h and SEGGER_RTT_ConfDefaults.h in this folder are the unmodified
 * SEGGER files — see README.md in this folder for their source.
 *
 * Scope
 * -----
 * Bidirectional console transport over SEGGER RTT, channel 0 only:
 *   Up-buffer   0  "Terminal"  BUFFER_SIZE_UP   bytes  target  -> J-Link RTT Viewer  (stdout/stderr)
 *   Down-buffer 0  "Terminal"  BUFFER_SIZE_DOWN bytes  J-Link  -> target             (stdin)
 *   Control-block header       ~64 B
 *
 * This configuration is deliberately minimal:
 *   - Static allocation only; no malloc.
 *   - Bare-metal locking (CMSIS PRIMASK save/restore); NO FreeRTOS dependency.
 *   - Single up/down channel; no multi-channel, no printf backend.
 **********************************************************************************************************************/

#ifndef SEGGER_RTT_CONF_H
#define SEGGER_RTT_CONF_H

/*──────────────────────────────────────────────────────────────────────────────
 * Channel count: only channel 0 ("Terminal") is used.
 *────────────────────────────────────────────────────────────────────────────*/
#define SEGGER_RTT_MAX_NUM_UP_BUFFERS    (1U)
#define SEGGER_RTT_MAX_NUM_DOWN_BUFFERS  (1U)

/*──────────────────────────────────────────────────────────────────────────────
 * Buffer sizes.
 *  - UP   (target -> host): 1024 B holds a comfortable burst of formatted
 *    output before J-Link drains it.
 *  - DOWN (host -> target):   64 B is ample for interactive line input.
 * Both may be overridden from the build system.
 *────────────────────────────────────────────────────────────────────────────*/
#ifndef BUFFER_SIZE_UP
  #define BUFFER_SIZE_UP   (1024U)  /* target -> J-Link (stdout / stderr) */
#endif
#ifndef BUFFER_SIZE_DOWN
  #define BUFFER_SIZE_DOWN   (64U)  /* J-Link -> target (stdin)           */
#endif

/*──────────────────────────────────────────────────────────────────────────────
 * Locking.
 *
 * The RTT ring-buffer critical sections are short. This RS is a bare-metal
 * console driven from thread / main-loop context, so the lock only needs to
 * protect the shared control block against a preempting interrupt.
 *
 * We use the official SEGGER macro convention: LOCK opens a brace-scoped block
 * and declares a local that captures the current PRIMASK; UNLOCK restores it
 * and closes the block. Saving/restoring PRIMASK (rather than an unconditional
 * enable) keeps interrupts masked if the caller already had them masked, and
 * the scoped local means the lock is re-entrant and needs no static storage.
 * This also matches the interface of the official J-Link RTT sources, so those
 * can replace this file without touching SEGGER_RTT.c.
 *
 * LOCK / UNLOCK must therefore be used as a balanced pair with no early return
 * between them (SEGGER_RTT.c is written single-exit to honour this).
 *
 * Host (non-ARM) builds, or builds that define RS_SEGGER_RTT_HOST_TEST, use an
 * empty brace pair — no interrupts to mask on the host.
 *────────────────────────────────────────────────────────────────────────────*/
#if defined(RS_SEGGER_RTT_HOST_TEST) || !defined(__ARM_ARCH)

  /* Host (non-ARM) builds: empty balanced block, no masking. */
  #define SEGGER_RTT_LOCK()     {
  #define SEGGER_RTT_UNLOCK()   }

#else

  #include "cmsis_compiler.h"   /* __get_PRIMASK / __set_PRIMASK / __disable_irq */

  #define SEGGER_RTT_LOCK()     { unsigned _rtt_lock_state = __get_PRIMASK(); __disable_irq();
  #define SEGGER_RTT_UNLOCK()   __set_PRIMASK(_rtt_lock_state); }

#endif

/*──────────────────────────────────────────────────────────────────────────────
 * Default channel-0 write mode.
 * NO_BLOCK_TRIM: if the up-buffer is full the write is trimmed (never blocks).
 * A detached J-Link therefore never stalls the CPU; the retarget layer reports
 * the truthful (possibly shorter) accepted count to the caller.
 *────────────────────────────────────────────────────────────────────────────*/
#define SEGGER_RTT_MODE_DEFAULT    SEGGER_RTT_MODE_NO_BLOCK_TRIM

#endif /* SEGGER_RTT_CONF_H */
