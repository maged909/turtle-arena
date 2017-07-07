**Extraordinary Beat X** game code.

To use this you'll need the [Spearmint engine](https://github.com/zturtleman/spearmint).

  * On Windows, install [Cygwin and mingw-w64](https://github.com/zturtleman/spearmint/wiki/Compiling#windows).
  * Get the source for Spearmint and build it using `make`.
  * Get the source for this repo and build it using `make`.
  * Copy the [ebx-data](https://github.com/Extraordinary-Beat-X/ebx-data) to `ebx-code/build/release-mingw32-x86/baseebx/data.pk3dir`

If you put both projects in the same directory you can launch the game using;

    spearmint/build/release-mingw32-x86/spearmint_x86.exe +set fs_basepath "ebx-code/build/release-mingw32-x86/" +set fs_game "baseebx"

On Linux and OS X you'll need to put `./` before the command and substitute the correct platform and architecture (look in the build directory).

