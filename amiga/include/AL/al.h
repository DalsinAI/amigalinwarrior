/* LinWarrior on GFOS: the part of OpenAL 1.1's al.h that LinWarrior uses,
 * for amiga/lw_al.c (an OpenAL subset on SDL 1.2's audio). Written from
 * the OpenAL 1.1 specification's names and values.
 * Apache-2.0, like LinWarrior. Copyright (c) 2026 AmiCradle Team. */
#ifndef AL_AL_H
#define AL_AL_H
#ifdef __cplusplus
extern "C" {
#endif

typedef char ALboolean;
typedef char ALchar;
typedef signed char ALbyte;
typedef unsigned char ALubyte;
typedef short ALshort;
typedef unsigned short ALushort;
typedef int ALint;
typedef unsigned int ALuint;
typedef int ALsizei;
typedef int ALenum;
typedef float ALfloat;
typedef double ALdouble;
typedef void ALvoid;

#define AL_NONE                 0
#define AL_FALSE                0
#define AL_TRUE                 1
#define AL_SOURCE_RELATIVE      0x202
#define AL_PITCH                0x1003
#define AL_POSITION             0x1004
#define AL_DIRECTION            0x1005
#define AL_VELOCITY             0x1006
#define AL_LOOPING              0x1007
#define AL_BUFFER               0x1009
#define AL_GAIN                 0x100A
#define AL_ORIENTATION          0x100F
#define AL_SOURCE_STATE         0x1010
#define AL_INITIAL              0x1011
#define AL_PLAYING              0x1012
#define AL_PAUSED               0x1013
#define AL_STOPPED              0x1014
#define AL_NO_ERROR             0
#define AL_INVALID_NAME         0xA001
#define AL_INVALID_ENUM         0xA002
#define AL_INVALID_VALUE        0xA003
#define AL_INVALID_OPERATION    0xA004
#define AL_OUT_OF_MEMORY        0xA005

ALenum alGetError(void);
void alGenSources(ALsizei n, ALuint *sources);
void alDeleteSources(ALsizei n, const ALuint *sources);
ALboolean alIsSource(ALuint source);
void alSourcef(ALuint source, ALenum param, ALfloat value);
void alSourcefv(ALuint source, ALenum param, const ALfloat *values);
void alSourcei(ALuint source, ALenum param, ALint value);
void alGetSourcei(ALuint source, ALenum param, ALint *value);
void alSourcePlay(ALuint source);
void alSourceStop(ALuint source);
void alListenerfv(ALenum param, const ALfloat *values);
void alDopplerFactor(ALfloat value);
void alSpeedOfSound(ALfloat value);

#ifdef __cplusplus
}
#endif
#endif
