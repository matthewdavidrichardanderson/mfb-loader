# MFB Loader

MFB Loader is a Wii homebrew application for loading physical Wii discs with
custom video settings. It keeps the game's stock IOS and physical-disc read
path so load behaviour and load times remain consistent with the Disc Channel.

This is development software. Back up important data before use, and expect
some games or video modes to be incompatible. MFB Loader does not include game
images, encryption keys, Nintendo software, or other copyrighted game data.

Current video options include:

- video output and aspect ratio;
- framebuffer/VI horizontal scaling;
- deflicker filtering;
- the 480p pixel-clock patch;
- region handling.

## Status

The current build loads physical Wii discs and applies video settings while the
game is loaded. Framebuffer scaling, deflicker control, supported aspect-ratio
patching, and the two known 480p pixel-fix variants are connected to the game
launch path.

Auto settings leave the corresponding game behaviour unchanged. Forced output
standards and full region-free handling remain under development.

At launch, MFB reads the IOS requested by the disc, reloads that stock IOS, and
hands control to the game's apploader. It does not use cIOS, USB/WBFS disc
redirection, read-limit removal, or IOS reload blocking.

Controls: Up/Down selects an option, Left/Right changes it, A advances a value,
Plus opens diagnostics, and Home exits. Diagnostics remain in memory and mirror
to USB Gecko when one is detected. Settings are loaded from and saved to
`settings.cfg` in the app directory.

GameCube controller: D-pad selects or changes options, A advances a value, Z
opens or returns from diagnostics, B returns from diagnostics, and Start exits
to the Homebrew Channel.

## Install

Copy `apps/mfb-loader/` from a packaged release to the `apps/` directory on
the SD card used by the Homebrew Channel. Insert an original Wii game disc,
start MFB Loader, choose the desired options, and launch the disc.

## Build

Install devkitPPC, set `DEVKITPRO` and `DEVKITPPC`, then run:

```sh
./tools/bootstrap-dependencies.sh
make
```

The bootstrap script fetches and builds the pinned libogc2 and libfat revisions.
The Makefile intentionally refuses to fall back to devkitPro libogc.

On Windows, run this from the devkitPro MSYS2 shell. The result is
`mfb-loader.dol`.

Run `make package` to create `dist/`. Copy the contents of that directory to
the root of an SD card for the Homebrew Channel.

## License

MFB Loader is licensed under the GNU General Public License version 2 only.
See [LICENSE](LICENSE).

Third-party components retain their respective licenses. Their notices are
listed in [THIRD_PARTY_NOTICES.txt](THIRD_PARTY_NOTICES.txt). If you redistribute
binaries, you must also satisfy the GPL's corresponding-source requirements.

Wii and Nintendo are trademarks of Nintendo. This independent homebrew project
is not affiliated with, authorized by, or endorsed by Nintendo.
