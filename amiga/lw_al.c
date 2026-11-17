/* LinWarrior on GFOS: the OpenAL, ALC and ALUT calls LinWarrior makes
 * (amiga/include/AL), on SDL 1.2's audio (sdl12-compat on GFOpenUp's
 * SDL2.module). The sound itself is amiga/lw_al_core.c: WAV files read by
 * us (sdl12-compat's SDL_LoadWAV_RW does not come back on GFOpenGPU),
 * sources mixed in SDL's audio callback, with pitch, 3D pan and distance.
 * One device and one context, as LinWarrior opens them.
 * Apache-2.0, like LinWarrior. Copyright (c) 2026 AmiCradle Team. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>
#include "lw_al_core.h"

#define LW_RATE 22050

static lwa_state state;
static int open_audio;           /* SDL's audio is running */
static ALenum error;
static struct ALCdevice_struct { int unused; } the_device;
static struct ALCcontext_struct { int unused; } the_context;

static void set_error(ALenum e)
{
	if (error == AL_NO_ERROR)
		error = e;
}

static void lock(void)
{
	if (open_audio)
		SDL_LockAudio();
}

static void unlock(void)
{
	if (open_audio)
		SDL_UnlockAudio();
}

static void callback(void *user, Uint8 *stream, int len)
{
	(void)user;
	lwa_mix(&state, (short *)stream, (unsigned long)len / 4);
}

/* ---- ALC ---- */

ALCdevice *alcOpenDevice(const ALCchar *name)
{
	SDL_AudioSpec want, have;

	(void)name;
	lwa_init(&state, LW_RATE);
	if (!SDL_WasInit(SDL_INIT_AUDIO) && SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
		return &the_device;       /* no sound, but the game goes on */
	memset(&want, 0, sizeof want);
	want.freq = LW_RATE;
	want.format = AUDIO_S16SYS;
	want.channels = 2;
	want.samples = 1024;
	want.callback = callback;
	if (SDL_OpenAudio(&want, &have) == 0) {
		if (have.format == AUDIO_S16SYS && have.channels == 2) {
			lwa_init(&state, (unsigned long)have.freq);
			open_audio = 1;
			SDL_PauseAudio(0);
		} else
			SDL_CloseAudio();
	}
	return &the_device;
}

ALCboolean alcCloseDevice(ALCdevice *device)
{
	(void)device;
	if (open_audio) {
		SDL_PauseAudio(1);
		SDL_CloseAudio();
		open_audio = 0;
	}
	lwa_free(&state);
	return 1;
}

ALCcontext *alcCreateContext(ALCdevice *device, const ALCint *attrs)
{
	(void)device;
	(void)attrs;
	return &the_context;
}

ALCboolean alcMakeContextCurrent(ALCcontext *context)
{
	(void)context;
	return 1;
}

void alcDestroyContext(ALCcontext *context)
{
	(void)context;
}

/* ---- AL ---- */

ALenum alGetError(void)
{
	ALenum e = error;

	error = AL_NO_ERROR;
	return e;
}

void alGenSources(ALsizei n, ALuint *sources)
{
	ALsizei i;

	lock();
	for (i = 0; i < n; i++)
		if (!(sources[i] = lwa_source_new(&state)))
			set_error(AL_OUT_OF_MEMORY);
	unlock();
}

void alDeleteSources(ALsizei n, const ALuint *sources)
{
	ALsizei i;

	lock();
	for (i = 0; i < n; i++)
		if (lwa_source_valid(&state, sources[i]))
			state.sources[sources[i]].used = 0;
	unlock();
}

ALboolean alIsSource(ALuint source)
{
	return lwa_source_valid(&state, source) ? AL_TRUE : AL_FALSE;
}

void alSourcef(ALuint source, ALenum param, ALfloat value)
{
	lwa_source *src;

	if (!lwa_source_valid(&state, source)) {
		set_error(AL_INVALID_NAME);
		return;
	}
	src = &state.sources[source];
	lock();
	switch (param) {
	case AL_GAIN: src->gain = value < 0 ? 0 : value; break;
	case AL_PITCH: src->pitch = value; break;
	default: set_error(AL_INVALID_ENUM); break;
	}
	lwa_source_update(&state, source);
	unlock();
}

void alSourcefv(ALuint source, ALenum param, const ALfloat *v)
{
	lwa_source *src;

	if (!lwa_source_valid(&state, source)) {
		set_error(AL_INVALID_NAME);
		return;
	}
	src = &state.sources[source];
	lock();
	switch (param) {
	case AL_POSITION:
		src->pos[0] = v[0]; src->pos[1] = v[1]; src->pos[2] = v[2];
		lwa_source_update(&state, source);
		break;
	case AL_VELOCITY:
	case AL_DIRECTION:
		break;                    /* no Doppler, no cones */
	default:
		set_error(AL_INVALID_ENUM);
	}
	unlock();
}

void alSourcei(ALuint source, ALenum param, ALint value)
{
	lwa_source *src;

	if (!lwa_source_valid(&state, source)) {
		set_error(AL_INVALID_NAME);
		return;
	}
	src = &state.sources[source];
	lock();
	switch (param) {
	case AL_BUFFER:
		if (value < 0 || value >= LWA_MAX_BUFFERS || (value && !state.buffers[value].pcm))
			set_error(AL_INVALID_VALUE);
		else {
			src->buffer = (unsigned int)value;
			src->playing = 0;
			src->ipos = src->frac = 0;
			lwa_source_update(&state, source);
		}
		break;
	case AL_LOOPING: src->looping = value != AL_FALSE; break;
	case AL_SOURCE_RELATIVE: src->relative = value != AL_FALSE; lwa_source_update(&state, source); break;
	default: set_error(AL_INVALID_ENUM); break;
	}
	unlock();
}

void alGetSourcei(ALuint source, ALenum param, ALint *value)
{
	if (!lwa_source_valid(&state, source)) {
		set_error(AL_INVALID_NAME);
		return;
	}
	if (param == AL_SOURCE_STATE)
		*value = state.sources[source].playing ? AL_PLAYING : AL_STOPPED;
	else if (param == AL_BUFFER)
		*value = (ALint)state.sources[source].buffer;
	else
		set_error(AL_INVALID_ENUM);
}

void alSourcePlay(ALuint source)
{
	if (!lwa_source_valid(&state, source)) {
		set_error(AL_INVALID_NAME);
		return;
	}
	lock();
	lwa_source_play(&state, source);
	unlock();
}

void alSourceStop(ALuint source)
{
	lock();
	lwa_source_stop(&state, source);
	unlock();
}

void alListenerfv(ALenum param, const ALfloat *v)
{
	lock();
	switch (param) {
	case AL_POSITION:
		state.lpos[0] = v[0]; state.lpos[1] = v[1]; state.lpos[2] = v[2];
		break;
	case AL_ORIENTATION:
		state.lat[0] = v[0]; state.lat[1] = v[1]; state.lat[2] = v[2];
		state.lup[0] = v[3]; state.lup[1] = v[4]; state.lup[2] = v[5];
		break;
	case AL_VELOCITY:
		unlock();
		return;
	default:
		set_error(AL_INVALID_ENUM);
		unlock();
		return;
	}
	lwa_update_all(&state);
	unlock();
}

void alDopplerFactor(ALfloat value) { (void)value; }
void alSpeedOfSound(ALfloat value) { (void)value; }

/* ---- ALUT ---- */

ALboolean alutInit(int *argcp, char **argv)
{
	alcMakeContextCurrent(alcCreateContext(alcOpenDevice(NULL), NULL));
	return alutInitWithoutContext(argcp, argv);
}

ALboolean alutInitWithoutContext(int *argcp, char **argv)
{
	(void)argcp;
	(void)argv;
	return AL_TRUE;
}

ALboolean alutExit(void)
{
	return AL_TRUE;
}

ALuint alutCreateBufferFromFileImage(const ALvoid *data, ALsizei length)
{
	ALuint name;

	if (!data || length <= 0) {
		set_error(AL_INVALID_VALUE);
		return AL_NONE;
	}
	lock();
	name = lwa_buffer_from_wav(&state, (const unsigned char *)data, (unsigned long)length);
	unlock();
	if (!name)
		set_error(AL_INVALID_VALUE);
	return name;
}

ALuint alutCreateBufferFromFile(const char *path)
{
	FILE *f = fopen(path, "rb");
	long size;
	unsigned char *data;
	ALuint name = AL_NONE;

	if (!f) {
		set_error(AL_INVALID_VALUE);
		return AL_NONE;
	}
	if (fseek(f, 0, SEEK_END) == 0 && (size = ftell(f)) > 0 && fseek(f, 0, SEEK_SET) == 0 &&
			(data = (unsigned char *)malloc((size_t)size)) != NULL) {
		if (fread(data, 1, (size_t)size, f) == (size_t)size)
			name = alutCreateBufferFromFileImage(data, (ALsizei)size);
		free(data);
	}
	fclose(f);
	return name;
}

ALuint alutCreateBufferHelloWorld(void)
{
	ALuint name;

	/* freealut says "Hello, world!"; this is a tenth of a second of quiet */
	lock();
	name = lwa_buffer_silence(&state, state.rate / 10);
	unlock();
	return name;
}

const char *alutGetMIMETypes(ALenum loader)
{
	(void)loader;
	return "audio/x-wav";
}
