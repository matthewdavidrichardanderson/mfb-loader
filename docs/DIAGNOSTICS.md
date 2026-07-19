# Launch diagnostics

If MFB Loader cannot continue launching a game, it displays a row of bars and
repeats the same number of pulses with the Wii disc-slot light. The pulses are
followed by a longer pause before repeating.

| Bars or pulses | Meaning |
| ---: | --- |
| 1 | Invalid launch parameters, or Disable dithering was requested but no compatible patch location was found. |
| 2 | Failed to open `/dev/di` after reloading IOS. |
| 3 | Failed to read the disc ID. |
| 4 | Failed to reopen the game partition. |
| 5 | Failed to read or validate the encrypted partition header. |
| 6 | Failed to read the apploader header. |
| 7 | The apploader size was invalid, or MFB failed to read the apploader. |
| 8 | An apploader section was invalid, or MFB failed to read one. |
| 9 | Failed to open `/dev/es`, or the apploader returned no game entry point. |
| 10 | Failed to query the IOS ticket-view count. |
| 11 | The ticket-view count was invalid, or a loaded section overlapped MFB's protected memory. |
| 12 | Failed to read IOS ticket views, or requested horizontal scaling was not patched. |
| 13 | The IOS launch transaction failed, or the requested 480p pixel patch was not applied. |
| 14 | The reloaded IOS did not report ready, or the requested video mode was not patched. |
| 15 | The reloaded IOS version did not match the requested IOS, or another IOS completion failure occurred. |

Some counts have two meanings because the display supports 15 distinct stages.
The point reached during launch normally distinguishes them. For example, 13
bars immediately after selecting Launch disc means an IOS launch failure, not a
480p patch failure.

The full internal error code is written to memory at `0x800030F0`. The current
implementation is in
[`freestanding/source/launch.c`](../freestanding/source/launch.c).
