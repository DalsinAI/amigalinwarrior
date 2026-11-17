/* The Amiga port's start: the stack, the version string, and the drawer
 * as the current directory (the game opens data/ by relative paths), set
 * before main().
 * Apache-2.0, like LinWarrior. Copyright (c) 2026 AmiCradle Team. */
#include <unistd.h>

/* A Shell gives 4 KB of stack; the world's builders recurse deeply.
 * libnix's startup sets this much up. */
unsigned long __stack = 1024 * 1024;
static const char version[] __attribute__((used)) =
    "$VER: LinWarrior 0.20 (17.11.2026)";

static void __attribute__((constructor)) amiga_drawer_dir(void)
{
    chdir("PROGDIR:");
}
