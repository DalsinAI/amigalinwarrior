/* LinWarrior on GFOS: the sound engine under amiga/lw_al.c (see
 * lw_al_core.h). Apache-2.0, like LinWarrior.
 * Copyright (c) 2026 AmiCradle Team. */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "lw_al_core.h"

void lwa_init(lwa_state *s, unsigned long rate)
{
	memset(s, 0, sizeof *s);
	s->rate = rate ? rate : 22050;
	s->lat[2] = -1.0f;            /* OpenAL's default: looking down -z, */
	s->lup[1] = 1.0f;             /* up +y */
}

void lwa_free(lwa_state *s)
{
	int i;

	for (i = 0; i < LWA_MAX_BUFFERS; i++)
		free(s->buffers[i].pcm);
	memset(s->buffers, 0, sizeof s->buffers);
	memset(s->sources, 0, sizeof s->sources);
}

static unsigned long le32(const unsigned char *p)
{
	return p[0] | (unsigned long)p[1] << 8 | (unsigned long)p[2] << 16 | (unsigned long)p[3] << 24;
}

static unsigned int le16(const unsigned char *p)
{
	return p[0] | (unsigned int)p[1] << 8;
}

static unsigned int new_buffer(lwa_state *s)
{
	unsigned int i;

	for (i = 1; i < LWA_MAX_BUFFERS; i++)
		if (!s->buffers[i].pcm)
			return i;
	return 0;
}

unsigned int lwa_buffer_from_wav(lwa_state *s, const unsigned char *d, unsigned long size)
{
	const unsigned char *fmt = NULL, *pcm = NULL;
	unsigned long off, pcmlen = 0, frames, f, rate;
	unsigned int name, channels, bits, align;
	short *out;

	if (!d || size < 12 || memcmp(d, "RIFF", 4) || memcmp(d + 8, "WAVE", 4))
		return 0;
	for (off = 12; off + 8 <= size; ) {
		unsigned long len = le32(d + off + 4);
		const unsigned char *body = d + off + 8;

		if (len > size - off - 8)
			len = size - off - 8;          /* a short last chunk: what is there */
		if (!memcmp(d + off, "fmt ", 4) && len >= 16)
			fmt = body;
		else if (!memcmp(d + off, "data", 4)) {
			pcm = body;
			pcmlen = len;
		}
		off += 8 + len + (len & 1);
	}
	if (!fmt || !pcm || le16(fmt) != 1)    /* PCM only */
		return 0;
	channels = le16(fmt + 2);
	rate = le32(fmt + 4);
	bits = le16(fmt + 14);
	if ((channels != 1 && channels != 2) || (bits != 8 && bits != 16) || !rate)
		return 0;
	align = channels * (bits / 8);
	frames = pcmlen / align;
	if (!frames || !(name = new_buffer(s)))
		return 0;
	if (!(out = (short *)malloc(frames * sizeof *out)))
		return 0;
	for (f = 0; f < frames; f++) {
		const unsigned char *p = pcm + f * align;
		long v;

		if (bits == 8) {
			v = ((int)p[0] - 128) * 256;
			if (channels == 2)
				v = (v + ((int)p[1] - 128) * 256) / 2;
		} else {
			v = (short)le16(p);
			if (channels == 2)
				v = (v + (short)le16(p + 2)) / 2;
		}
		out[f] = (short)v;
	}
	s->buffers[name].pcm = out;
	s->buffers[name].frames = frames;
	s->buffers[name].rate = rate;
	return name;
}

unsigned int lwa_buffer_silence(lwa_state *s, unsigned long frames)
{
	unsigned int name = new_buffer(s);

	if (!name || !frames || !(s->buffers[name].pcm = (short *)calloc(frames, sizeof(short))))
		return 0;
	s->buffers[name].frames = frames;
	s->buffers[name].rate = s->rate;
	return name;
}

unsigned int lwa_source_new(lwa_state *s)
{
	unsigned int i;

	for (i = 1; i < LWA_MAX_SOURCES; i++) {
		lwa_source *src = &s->sources[i];

		if (!src->used) {
			memset(src, 0, sizeof *src);
			src->used = 1;
			src->gain = 1.0f;
			src->pitch = 1.0f;
			lwa_source_update(s, i);
			return i;
		}
	}
	return 0;
}

int lwa_source_valid(const lwa_state *s, unsigned int name)
{
	return name > 0 && name < LWA_MAX_SOURCES && s->sources[name].used;
}

void lwa_source_play(lwa_state *s, unsigned int name)
{
	lwa_source *src;

	if (!lwa_source_valid(s, name))
		return;
	src = &s->sources[name];
	src->ipos = 0;                 /* OpenAL: play restarts a playing source */
	src->frac = 0;
	lwa_source_update(s, name);
	src->playing = src->buffer && s->buffers[src->buffer].pcm;
}

void lwa_source_stop(lwa_state *s, unsigned int name)
{
	if (lwa_source_valid(s, name))
		s->sources[name].playing = 0;
}

static float dot(const float *a, const float *b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void lwa_spatial(const float lpos[3], const float lat[3], const float lup[3],
		const float spos[3], int relative, float gain, float *gl, float *gr)
{
	float v[3], right[3], dist, atten = 1.0f, pan = 0.0f, g;

	if (relative) {
		v[0] = spos[0]; v[1] = spos[1]; v[2] = spos[2];
	} else {
		v[0] = spos[0] - lpos[0]; v[1] = spos[1] - lpos[1]; v[2] = spos[2] - lpos[2];
	}
	dist = sqrtf(dot(v, v));
	if (dist > 1.0f)
		atten = 1.0f / (1.0f + (dist - 1.0f));   /* ref 1 / (ref + rolloff (d - ref)) */
	if (dist > 1e-4f) {
		float rl;

		/* right = at x up */
		right[0] = lat[1] * lup[2] - lat[2] * lup[1];
		right[1] = lat[2] * lup[0] - lat[0] * lup[2];
		right[2] = lat[0] * lup[1] - lat[1] * lup[0];
		rl = sqrtf(dot(right, right));
		if (rl > 1e-6f)
			pan = dot(v, right) / (rl * dist);
		if (pan > 1.0f) pan = 1.0f;
		if (pan < -1.0f) pan = -1.0f;
	}
	g = gain * atten;
	if (g > 1.0f) g = 1.0f;
	if (g < 0.0f) g = 0.0f;
	*gl = g * sqrtf((1.0f - pan) * 0.5f);
	*gr = g * sqrtf((1.0f + pan) * 0.5f);
}

static int to_vol(float g)
{
	long v = (long)(g * LWA_VOL_ONE + 0.5f);

	return v < 0 ? 0 : v > LWA_VOL_ONE ? LWA_VOL_ONE : (int)v;
}

void lwa_source_update(lwa_state *s, unsigned int name)
{
	lwa_source *src;
	float gl, gr, pitch;
	unsigned long rate;

	if (!lwa_source_valid(s, name))
		return;
	src = &s->sources[name];
	lwa_spatial(s->lpos, s->lat, s->lup, src->pos, src->relative, src->gain, &gl, &gr);
	src->vol_l = to_vol(gl);
	src->vol_r = to_vol(gr);
	pitch = src->pitch;
	if (pitch < 0.01f) pitch = 0.01f;
	if (pitch > 16.0f) pitch = 16.0f;
	rate = src->buffer && s->buffers[src->buffer].pcm ? s->buffers[src->buffer].rate : s->rate;
	src->step = (unsigned long)((double)rate * pitch * 65536.0 / (double)s->rate + 0.5);
	if (!src->step)
		src->step = 1;
}

void lwa_update_all(lwa_state *s)
{
	unsigned int i;

	for (i = 1; i < LWA_MAX_SOURCES; i++)
		if (s->sources[i].used)
			lwa_source_update(s, i);
}

void lwa_mix(lwa_state *s, short *out, unsigned long frames)
{
	static long acc[LWA_MIX_CHUNK * 2];

	while (frames) {
		unsigned long n = frames > LWA_MIX_CHUNK ? LWA_MIX_CHUNK : frames, f, i;

		memset(acc, 0, n * 2 * sizeof acc[0]);
		for (i = 1; i < LWA_MAX_SOURCES; i++) {
			lwa_source *src = &s->sources[i];
			const lwa_buffer *b;

			if (!src->used || !src->playing)
				continue;
			b = &s->buffers[src->buffer];
			if (!b->pcm) {
				src->playing = 0;
				continue;
			}
			for (f = 0; f < n; f++) {
				long s0, s1, v;

				if (src->ipos >= b->frames) {
					if (!src->looping) {
						src->playing = 0;
						break;
					}
					src->ipos %= b->frames;
				}
				s0 = b->pcm[src->ipos];
				if (src->ipos + 1 < b->frames)
					s1 = b->pcm[src->ipos + 1];
				else
					s1 = src->looping ? b->pcm[0] : 0;
				/* 16 x 14 bits and 16 x 13 bits: no overflow in a 32-bit long;
				 * GCC shifts negative numbers arithmetically on both targets */
				v = s0 + (((s1 - s0) * (long)(src->frac >> 2)) >> 14);
				acc[f * 2] += (v * src->vol_l) >> 12;
				acc[f * 2 + 1] += (v * src->vol_r) >> 12;
				src->frac += src->step;
				src->ipos += src->frac >> 16;
				src->frac &= 0xffff;
			}
		}
		for (f = 0; f < n * 2; f++) {
			long v = acc[f];

			out[f] = (short)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
		}
		out += n * 2;
		frames -= n;
	}
}
