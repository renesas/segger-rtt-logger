/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * hal_entry.c — FPB-RA0L1 bare-metal entry for the Segger RTT Logger example.
 *
 * Execution model
 * ---------------
 * No-RTOS (bare-metal) FSP project. After reset the FSP startup performs clock
 * and BSP bring-up, then the RASC-generated ra_gen/main.c calls hal_entry().
 * We hand control straight to the application use-case.
 *
 * All console I/O (printf / fgets) is retargeted to SEGGER RTT channel 0 by the
 * rs_segger_rtt_logger RS, which defines the picolibc stdin / stdout /
 * stderr streams. SEGGER RTT needs no peripheral, pin or clock configuration, so
 * there is no HAL setup in the I/O path here.
 *
 * This file replaces the RASC-generated src/hal_entry.cpp: the project is kept
 * C-only, and segger_rtt_logger_app_run() is a C function.
 **********************************************************************************************************************/

#include "hal_data.h"

/* Application use-case (src/app/segger_rtt_logger_app.c). */
#include "segger_rtt_logger_app.h"

void hal_entry(void)
{
    segger_rtt_logger_app_run();   /* never returns */
}
