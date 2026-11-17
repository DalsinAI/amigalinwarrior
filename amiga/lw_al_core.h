/* LinWarrior on GFOS: the sound engine under amiga/lw_al.c's OpenAL calls.
 * Pure C with no SDL, so the host tests run it on the PC and on 68k Linux:
 * a PCM WAV reader (8 or 16 bit, mono or stereo, any rate, read byte by
 * byte, so the same on either byte order), sources with gain, pitch,
 * position and looping, OpenAL's inverse-distance rolloff and a
 * constant-power stereo pan from the listener's position and orientation,
 * and a mixer that resamples every source to the device rate in 16.16
 * fixed point and mixes them to signed 16-bit stereo.
 * Apache-2.0, like LinWarrior. Copyright (c) 2026 AmiCradle Team. */
#ifndef LW_AL_CORE_H
#define LW_AL_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#define LWA_MAX_BUFFERS 128
#define LWA_MAX_SOURCES 128
#define LWA_VOL_ONE 4096          /* a channel volume of 1.0 */
#define LWA_MIX_CHUNK 1024        /* frames mixed per pass */

typedef struct {
	short *pcm;                   /* mono, signed 16 bits, native order */
	unsigned long frames;
	unsigned long rate;
} lwa_buffer;

typedef struct {
	int used;
	unsigned int buffer;          /* 0 = none */
	float gain, pitch;
	float pos[3];
	int relative, looping, playing;
	unsigned long ipos, frac;     /* where it is: frame, and 1/65536ths */
	unsigned long step;           /* frames per device frame, 16.16 */
	int vol_l, vol_r;             /* 0 .. LWA_VOL_ONE */
} lwa_source;

typedef struct {
	unsigned long rate;           /* the device's */
	float lpos[3], lat[3], lup[3];
	lwa_buffer buffers[LWA_MAX_BUFFERS];   /* [0] unused: name 0 is none */
	lwa_source sources[LWA_MAX_SOURCES];
} lwa_state;

void lwa_init(lwa_state *s, unsigned long rate);
void lwa_free(lwa_state *s);

/* a WAV file in memory into a new buffer: its name, or 0 */
unsigned int lwa_buffer_from_wav(lwa_state *s, const unsigned char *data, unsigned long size);
/* a buffer of silence (ALUT's hello-world stand-in): its name, or 0 */
unsigned int lwa_buffer_silence(lwa_state *s, unsigned long frames);

unsigned int lwa_source_new(lwa_state *s);          /* its name, or 0 */
int lwa_source_valid(const lwa_state *s, unsigned int name);
void lwa_source_play(lwa_state *s, unsigned int name);
void lwa_source_stop(lwa_state *s, unsigned int name);

/* gains for the left and right channel of a source at spos, as OpenAL's
 * default inverse-distance-clamped model (reference distance 1, rolloff
 * 1) and a constant-power pan; the total is held to 1.0, as OpenAL's
 * default maximum gain does */
void lwa_spatial(const float lpos[3], const float lat[3], const float lup[3],
		const float spos[3], int relative, float gain, float *gl, float *gr);

/* the source's volumes and step from its gain, pitch and position */
void lwa_source_update(lwa_state *s, unsigned int name);
/* every playing source's (the listener moved) */
void lwa_update_all(lwa_state *s);

/* mix FRAMES stereo frames into OUT (interleaved left, right) */
void lwa_mix(lwa_state *s, short *out, unsigned long frames);

#ifdef __cplusplus
}
#endif
#endif
