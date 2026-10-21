# SEGGER RTT — third-party source

This folder holds the SEGGER Real-Time Transfer (RTT) engine used by the Segger RTT Logger RS, plus the repository's own RTT configuration.

## Origin

| Item | Value |
|---|---|
| Component | SEGGER RTT (Real-Time Transfer) |
| Version | **V8.58** — latest official release |
| Source | SEGGER's official repository [github.com/SEGGERMicro/RTT](https://github.com/SEGGERMicro/RTT), tag [V8.58](https://github.com/SEGGERMicro/RTT/tree/V8.58) (commit `4d8feab`) |
| Download (V8.58) | [Source code (zip)](https://github.com/SEGGERMicro/RTT/archive/refs/tags/V8.58.zip) · [Source code (tar.gz)](https://github.com/SEGGERMicro/RTT/archive/refs/tags/V8.58.tar.gz) |
| License | BSD (SEGGER), see `LICENSE.md` in this folder |

## Files

| File | Status |
|---|---|
| `SEGGER_RTT.c` | **Unmodified** SEGGER file — `RTT/SEGGER_RTT.c` of V8.58 |
| `SEGGER_RTT.h` | **Unmodified** SEGGER file — `RTT/SEGGER_RTT.h` of V8.58 |
| `SEGGER_RTT_ConfDefaults.h` | **Unmodified** SEGGER file — `RTT/SEGGER_RTT_ConfDefaults.h` of V8.58. Supplies the default of every RTT setting that `SEGGER_RTT_Conf.h` does not define |
| `LICENSE.md` | **Unmodified** SEGGER license — `LICENSE.md` of V8.58. Kept because the V8.58 source files no longer carry the license text in their header |
| `SEGGER_RTT_Conf.h` | **Renesas custom configuration** — replaces the repository's `Config/SEGGER_RTT_Conf.h`: channel 0 only, 1024 B up / 64 B down, `SEGGER_RTT_MODE_NO_BLOCK_TRIM`, CMSIS PRIMASK locking, no RTOS |

How the configuration works in V8.58: `SEGGER_RTT.h` includes `SEGGER_RTT_ConfDefaults.h`, which first includes `SEGGER_RTT_Conf.h` and then defines a default only for each setting that is still undefined (`#ifndef`). The values in `SEGGER_RTT_Conf.h` therefore take precedence.

Files of the SEGGER repository that are deliberately **not** included: `RTT/SEGGER_RTT_printf.c` (the RS uses picolibc stdio instead), `RTT/SEGGER_RTT_ASM_ARMv7M.S` (the C implementation is used), `Syscalls/*` (`rs_segger_rtt_logger_picolibc.c` is the only stdio binding) and `Examples/*`.

## Updating to another SEGGER RTT version

1. Download the tag of the new version from [github.com/SEGGERMicro/RTT](https://github.com/SEGGERMicro/RTT/tags).
2. Replace `SEGGER_RTT.c`, `SEGGER_RTT.h`, `SEGGER_RTT_ConfDefaults.h` and `LICENSE.md` with the files of that tag. Do not modify them (SEGGER recommends against changes).
3. Keep `SEGGER_RTT_Conf.h`, and check `SEGGER_RTT_ConfDefaults.h` for new or renamed settings.
4. Update the version in this file and in the project `README.md`.
