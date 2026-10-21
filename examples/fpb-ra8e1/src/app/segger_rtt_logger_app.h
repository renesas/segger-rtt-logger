/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * segger_rtt_logger_app.h — public interface of the app_segger_rtt_logger use-case.
 **********************************************************************************************************************/

#ifndef SEGGER_RTT_LOGGER_APP_H
#define SEGGER_RTT_LOGGER_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Run the console demo: print a banner, then echo each input line. Never returns. */
void segger_rtt_logger_app_run(void);

#ifdef __cplusplus
}
#endif

#endif /* SEGGER_RTT_LOGGER_APP_H */
