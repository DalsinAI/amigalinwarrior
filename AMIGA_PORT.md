LinWarrior for GFOS
===================

LinWarrior 3D, Benjamin Pickhardt's (hackcraft.de) mech game: walk a mech
across a procedural landscape and city, fight other mechs and turrets.
Upstream calls it an unfinished development version; this is its last
commit, on the way to r20. For AmigaOS 3.2 with GFOpenUp, on a 68040 or
68060 with an FPU.

Running it
----------

- Double-click LinWarrior. It opens a 640 x 480 screen through GFOpenUp's
  SDL and OpenGL (LIBS:GFOpenGPU/SDL2.module, GL.module), which is where
  the drawing goes: to the host's GPU on AmiCradle.
- From a Shell: LinWarrior --help lists the options (--resolution 800x600,
  --window, --mission N, --lockmouse 0).
- Controls: see Controls.

What the port changes
---------------------

- Sound: LinWarrior uses OpenAL and ALUT. amiga/lw_al.c is our own
  version of the 20-odd calls it makes, on SDL 1.2's audio:
  amiga/lw_al_core.c reads the WAV files (byte by byte, the same on either
  byte order), and mixes the sources in SDL's audio callback, resampled
  to the device rate in fixed point, with pitch, OpenAL's distance rolloff
  and a stereo pan from the listener's position and orientation. No
  Doppler. It reads WAVs itself because sdl12-compat's SDL_LoadWAV_RW does
  not come back on GFOpenGPU.
- GLEW and GLU: GFOpenGPU's libGL exports the GL calls, extensions
  included, so amiga/include/GL/glew.h only stands in for GLEW's header;
  amiga/include/GL/glu.h has the two GLU calls the game makes
  (gluPerspective, gluBuild2DMipmaps) written on GL.
- XML: Expat, built with GFOpenXML's 68k configuration.
- amiga/patches/0001-amigaos.patch:
  - Main.cpp: started from its icon, or with only options, it runs the
    game (upstream wants "game" as the first argument);
  - GameConfig.h: 640 x 480 by default (upstream 800 x 600);
  - GameMain.cpp: SDL_OPENGL for the screen (sdl12-compat, GFOpenUp's
    SDL 1.2, does not do SDL_OPENGLBLIT);
  - Filesystem.cpp: data/ in the drawer only, not the Unix install paths;
  - Texfile.cpp: the TGA header's width and height read and written byte
    by byte on every machine (upstream swapped them on big-endian ones,
    which is wrong for TGA);
  - macros.h, Appender.cpp: <time.h> included where upstream relied on
    glibc pulling it in.
- The drawer leaves out the Blender files of the models (18 MB): the game
  reads the .md5mesh files made from them.

Tests
-----

sh tests/host/test_all.sh builds tests/host/test_lw_al.c with the sound
core for the PC and for big-endian 68k Linux (qemu-m68k -cpu m68040). Its
25 checks cover the WAV reader (8 and 16 bit, stereo, odd chunks, cut
files, not WAV), play, loop, stop, pitch, sample rates, the 3D gains and
clipping; then it reads every WAV the game ships and mixes them all
together for three seconds. The PC and the 68k must print the same, and
do. tests/host/test_lha.py tests the archive writer.

What only a run on an Amiga shows (the screen, the GL drawing, the sound
through SDL, the mouse and keys) needs a lab.

Building
--------

sh package/build-amiga.sh with STOVE= an os32-gcc16-open6 stove: it
fetches upstream and Expat at package/linwarrior.lock's commits, applies
the patch, builds, and writes the LhA and the GFOpenUp part to OUT=.

Team 3
