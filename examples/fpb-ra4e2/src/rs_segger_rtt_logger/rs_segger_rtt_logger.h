/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * rs_segger_rtt_logger.h — INTERNAL (private) declarations for the rs_segger_rtt_logger RS.
 *
 * NOT a public API. The public "API" of this RS is the standard C library
 * console (printf / fgets / ...). These two functions are the portable RTT
 * implementation, called only by the picolibc stdio callbacks
 * (rs_segger_rtt_logger_picolibc.c: put / get).
 *
 * The header lives in src/rs_segger_rtt_logger/ alongside its implementation. It is
 * internal to the rs_segger_rtt_logger module: application code must not include it
 * or call these functions directly.
 **********************************************************************************************************************/
#ifndef RS_SEGGER_RTT_LOGGER_H
#define RS_SEGGER_RTT_LOGGER_H

/* Portable RTT console logic. Only the picolibc stdio callbacks (put/get) call
 * these; nothing else should. */
int rs_segger_rtt_logger_write_impl(int fd, const char * buf, int len);
int rs_segger_rtt_logger_read_impl(int fd, char * buf, int len);

#endif /* RS_SEGGER_RTT_LOGGER_H */
