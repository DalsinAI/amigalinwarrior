/* LinWarrior on GFOS: the part of freealut's alut.h that LinWarrior uses
 * (amiga/lw_al.c: WAV files only). Apache-2.0, like LinWarrior.
 * Copyright (c) 2026 AmiCradle Team. */
#ifndef AL_ALUT_H
#define AL_ALUT_H
#include <AL/al.h>
#include <AL/alc.h>
#ifdef __cplusplus
extern "C" {
#endif

#define ALUT_LOADER_BUFFER 0x300
#define ALUT_LOADER_MEMORY 0x301

ALboolean alutInit(int *argcp, char **argv);
ALboolean alutInitWithoutContext(int *argcp, char **argv);
ALboolean alutExit(void);
ALuint alutCreateBufferFromFile(const char *fileName);
ALuint alutCreateBufferFromFileImage(const ALvoid *data, ALsizei length);
ALuint alutCreateBufferHelloWorld(void);
const char *alutGetMIMETypes(ALenum loader);

#ifdef __cplusplus
}
#endif
#endif
