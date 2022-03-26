**Turtle Arena** game code.

To use this you'll need the [Spearmint engine](https://github.com/zturtleman/spearmint).

  * On Windows, install [Cygwin and mingw-w64](https://github.com/zturtleman/spearmint/wiki/Compiling#windows).
  * Get the source for Spearmint and build it using `make`.
  * Get the source for this repo and build it using `make`.
  * Copy the [turtle-arena-data](https://github.com/Turtle-Arena/turtle-arena-data) to `turtle-arena-code/build/release-mingw32-x86/baseturtle/data.pk3dir`

If you put both projects in the same directory you can launch the game using;

    spearmint/build/release-mingw32-x86/spearmint_x86.exe +set fs_basepath "turtle-arena/build/release-mingw32-x86/" +set fs_game "baseturtle"

On Linux and OS X you'll need to put `./` before the command and substitute the correct platform and architecture (look in the build directory).

