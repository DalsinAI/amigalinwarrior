/* LinWarrior on GFOS: the part of OpenAL 1.1's alc.h that LinWarrior uses
 * (amiga/lw_al.c). Apache-2.0, like LinWarrior.
 * Copyright (c) 2026 AmiCradle Team. */
#ifndef AL_ALC_H
#define AL_ALC_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct ALCdevice_struct ALCdevice;
typedef struct ALCcontext_struct ALCcontext;
typedef char ALCboolean;
typedef char ALCchar;
typedef int ALCint;

ALCdevice *alcOpenDevice(const ALCchar *devicename);
ALCboolean alcCloseDevice(ALCdevice *device);
ALCcontext *alcCreateContext(ALCdevice *device, const ALCint *attrlist);
ALCboolean alcMakeContextCurrent(ALCcontext *context);
void alcDestroyContext(ALCcontext *context);

#ifdef __cplusplus
}
#endif
#endif
