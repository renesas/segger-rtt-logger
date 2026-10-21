/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * segger_rtt_logger_app.c — app_segger_rtt_logger use-case.
 *
 * Demonstrates that ordinary application code using the STANDARD C CONSOLE
 * (printf / fgets) works unchanged over SEGGER RTT — no UART and no
 * RTOS. The rs_segger_rtt_logger RS defines the picolibc stdin /
 * stdout / stderr streams that route stdio to SEGGER RTT channel 0.
 *
 * Behaviour (per specification):
 *   1. Print a banner.
 *   2. Read a complete, bounded input line from the RTT Viewer.
 *   3. Echo it back as:  "You printed: <input>"
 *      One entered line produces exactly one response. A line longer than
 *      APP_CONSOLE_LINE_MAX - 1 characters is truncated: the rest of the line
 *      is discarded and the echo ends with " (truncated)".
 *
 * USE-CASE RULES (same as any APP in this repo):
 *   - The use-case calls the (standard C) console API. It never touches the
 *     transport directly — the RS owns the stdio streams.
 *   - No dynamic allocation: the input line uses a fixed, bounded buffer.
 **********************************************************************************************************************/

#include "segger_rtt_logger_app.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* Maximum bounded input line length, including the NUL terminator. */
#define APP_CONSOLE_LINE_MAX   (128)

/* Read and discard the rest of the current input line, up to and including the
 * newline. Returns true if any character other than CR / LF was discarded, i.e.
 * the line really was longer than the buffer (a line that exactly fills the
 * buffer leaves only its CR / LF behind, which is not a truncation). */
static bool segger_rtt_logger_app_discard_rest_of_line(void)
{
    bool discarded = false;
    int  c         = getchar();

    while (('\n' != c) && (EOF != c))
    {
        if ('\r' != c)
        {
            discarded = true;
        }
        c = getchar();
    }

    return discarded;
}

void segger_rtt_logger_app_run(void)
{
    char line[APP_CONSOLE_LINE_MAX];
    bool truncated;

    /* The RS streams are unbuffered: every character goes straight to the RTT
     * buffer, so the banner and each echo reach the RTT Viewer immediately,
     * without setvbuf() or fflush(). */
    printf("System Init\r\n");
    printf("Segger RTT Logger ready. Type a line and press Enter.\r\n");

    for (;;)
    {
        /* Read one complete, bounded line. fgets() stops at newline or when the
         * buffer is full, and always NUL-terminates — no overflow. */
        if (NULL != fgets(line, (int)sizeof line, stdin))
        {
            /* No newline in the buffer: fgets() stopped because the buffer is
             * full. Consume the rest of the line here, so that it does not come
             * back as a second (or empty) echo on the next fgets().
             * strcspn() (already needed below) finds the newline: on some cores
             * the C library's strchr() is a much larger, speed-optimised
             * routine (about 400 B on Cortex-M85). */
            truncated = false;
            if ('\0' == line[strcspn(line, "\n")])
            {
                truncated = segger_rtt_logger_app_discard_rest_of_line();
            }

            /* Strip the trailing CR/LF the terminal appends. */
            line[strcspn(line, "\r\n")] = '\0';

            printf("You printed: %s%s\r\n", line, truncated ? " (truncated)" : "");
        }
    }
}
