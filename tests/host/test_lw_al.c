/* Host tests of amiga/lw_al_core.c, the sound under the port's OpenAL calls.
 * Built for the PC and for 68k Linux by tests/host/test_all.sh, which runs
 * both and compares what they print: the WAV reader and the mixer must give
 * the same samples on either byte order.
 *
 *   test_lw_al [WAV...]
 *
 * Checks (each prints "ok N name" or "FAIL N name", then a total):
 * the reader on made-up WAVs (8-bit, 16-bit, stereo, odd chunks, not WAV),
 * play/loop/stop, pitch (a source at pitch 2 ends in half the time), the
 * spatial gains (in front: equal; to the right: right louder; behind the
 * listener turned round: the other way; far: quieter; relative: follows the
 * listener; held to 1.0), and the mixer's clipping. Then for every WAV
 * named, its frames, rate and a hash of the samples read, and a hash of a
 * mix of all of them playing together at assorted gains and pitches.
 * Apache-2.0, like LinWarrior. Copyright (c) 2026 AmiCradle Team. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lw_al_core.h"

static int checks, failures;

static void check(int ok, const char *name)
{
	checks++;
	if (!ok)
		failures++;
	printf("%s %d %s\n", ok ? "ok" : "FAIL", checks, name);
}

static void put32(unsigned char *p, unsigned long v)
{
	p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24;
}

static void put16(unsigned char *p, unsigned v)
{
	p[0] = v; p[1] = v >> 8;
}

/* a WAV: CH channels of BITS bits at RATE, FRAMES frames of DATA, and an
 * extra chunk of XLEN bytes (odd: padded) before the data */
static unsigned long make_wav(unsigned char *w, unsigned ch, unsigned bits, unsigned long rate,
		const unsigned char *data, unsigned long frames, unsigned long xlen)
{
	unsigned long dlen = frames * ch * (bits / 8), off = 12;

	memcpy(w, "RIFF", 4);
	memcpy(w + 8, "WAVE", 4);
	memcpy(w + off, "fmt ", 4); put32(w + off + 4, 16);
	put16(w + off + 8, 1); put16(w + off + 10, ch); put32(w + off + 12, rate);
	put32(w + off + 16, rate * ch * (bits / 8)); put16(w + off + 20, ch * (bits / 8)); put16(w + off + 22, bits);
	off += 24;
	if (xlen) {
		memcpy(w + off, "LIST", 4); put32(w + off + 4, xlen);
		memset(w + off + 8, 'x', xlen);
		off += 8 + xlen + (xlen & 1);
	}
	memcpy(w + off, "data", 4); put32(w + off + 4, dlen);
	memcpy(w + off + 8, data, dlen);
	off += 8 + dlen;
	put32(w + 4, off - 8);
	return off;
}

static unsigned long hash_shorts(const short *p, unsigned long n)
{
	unsigned long h = 2166136261UL, i;

	for (i = 0; i < n; i++) {
		unsigned v = (unsigned short)p[i];

		h = ((h ^ (v & 0xff)) * 16777619UL) & 0xffffffffUL;
		h = ((h ^ (v >> 8)) * 16777619UL) & 0xffffffffUL;
	}
	return h;
}

static unsigned char *read_file(const char *path, unsigned long *size)
{
	FILE *f = fopen(path, "rb");
	unsigned char *d;
	long n;

	if (!f)
		return NULL;
	fseek(f, 0, SEEK_END);
	n = ftell(f);
	fseek(f, 0, SEEK_SET);
	d = (unsigned char *)malloc(n > 0 ? n : 1);
	if (d && fread(d, 1, n, f) != (size_t)n) {
		free(d);
		d = NULL;
	}
	fclose(f);
	*size = n;
	return d;
}

static lwa_state st;

static void reader_checks(void)
{
	unsigned char w[512], d[64];
	unsigned int b;
	unsigned long n;

	lwa_init(&st, 22050);
	/* 16-bit mono: -2, 1000, -32768, 32767 */
	put16(d, 0xfffe); put16(d + 2, 1000); put16(d + 4, 0x8000); put16(d + 6, 0x7fff);
	n = make_wav(w, 1, 16, 11025, d, 4, 0);
	b = lwa_buffer_from_wav(&st, w, n);
	check(b && st.buffers[b].frames == 4 && st.buffers[b].rate == 11025 &&
		st.buffers[b].pcm[0] == -2 && st.buffers[b].pcm[1] == 1000 &&
		st.buffers[b].pcm[2] == -32768 && st.buffers[b].pcm[3] == 32767, "16-bit mono, little-endian samples");
	/* 8-bit unsigned: 0, 128, 255 */
	d[0] = 0; d[1] = 128; d[2] = 255;
	n = make_wav(w, 1, 8, 22050, d, 3, 0);
	b = lwa_buffer_from_wav(&st, w, n);
	check(b && st.buffers[b].pcm[0] == -32768 && st.buffers[b].pcm[1] == 0 && st.buffers[b].pcm[2] == 32512,
		"8-bit unsigned to signed 16");
	/* 16-bit stereo: (100, 300) and (-100, -301) average */
	put16(d, 100); put16(d + 2, 300); put16(d + 4, (unsigned)-100 & 0xffff); put16(d + 6, (unsigned)-301 & 0xffff);
	n = make_wav(w, 2, 16, 44100, d, 2, 0);
	b = lwa_buffer_from_wav(&st, w, n);
	check(b && st.buffers[b].frames == 2 && st.buffers[b].pcm[0] == 200 && st.buffers[b].pcm[1] == -200,
		"16-bit stereo mixed down to mono");
	/* an odd-length chunk before the data */
	put16(d, 1234); put16(d + 2, 4321);
	n = make_wav(w, 1, 16, 8000, d, 2, 5);
	b = lwa_buffer_from_wav(&st, w, n);
	check(b && st.buffers[b].frames == 2 && st.buffers[b].pcm[1] == 4321, "odd chunk skipped with its pad byte");
	/* not a WAV, not PCM, truncated */
	check(!lwa_buffer_from_wav(&st, (const unsigned char *)"RIFF....AVIfjunk", 16), "not WAVE refused");
	n = make_wav(w, 1, 16, 8000, d, 2, 0);
	put16(w + 20, 3);                        /* format 3: float */
	check(!lwa_buffer_from_wav(&st, w, n), "not PCM refused");
	n = make_wav(w, 1, 16, 8000, d, 2, 0);
	b = lwa_buffer_from_wav(&st, w, n - 2);  /* the data chunk cut short */
	check(b && st.buffers[b].frames == 1, "short data chunk: the whole frames there are");
	check(!lwa_buffer_from_wav(&st, NULL, 0), "nothing refused");
	lwa_free(&st);
}

static void play_checks(void)
{
	unsigned char w[4096], d[2000];
	short out[4000];
	unsigned int b, s, s2;
	unsigned long i, n;

	for (i = 0; i < 1000; i++)
		put16(d + i * 2, 10000);
	n = make_wav(w, 1, 16, 22050, d, 1000, 0);
	lwa_init(&st, 22050);
	b = lwa_buffer_from_wav(&st, w, n);
	s = lwa_source_new(&st);
	check(s && lwa_source_valid(&st, s) && !lwa_source_valid(&st, 0) && !lwa_source_valid(&st, s + 1),
		"source names");
	st.sources[s].buffer = b;
	st.sources[s].relative = 1;              /* at the listener: centre */
	lwa_source_play(&st, s);
	lwa_mix(&st, out, 500);
	check(st.sources[s].playing && out[0] == out[1] && out[0] > 7000 && out[0] < 7100,
		"centre: both channels at 1/sqrt 2");
	lwa_mix(&st, out, 600);
	check(!st.sources[s].playing && out[2 * 499] != 0 && out[2 * 500] == 0 && out[2 * 599] == 0,
		"stops at its end, then silence");
	st.sources[s].looping = 1;
	lwa_source_play(&st, s);
	lwa_mix(&st, out, 2000);
	check(st.sources[s].playing && out[2 * 1999] > 7000, "looping goes on");
	lwa_source_stop(&st, s);
	lwa_mix(&st, out, 10);
	check(!st.sources[s].playing && out[0] == 0, "stop");
	st.sources[s].looping = 0;
	st.sources[s].pitch = 2.0f;
	lwa_source_play(&st, s);
	lwa_mix(&st, out, 499);
	check(st.sources[s].playing, "pitch 2: playing at 499");
	lwa_mix(&st, out, 2);
	check(!st.sources[s].playing, "pitch 2: done after 500");
	/* two loud sources clip, not wrap */
	st.sources[s].pitch = 1.0f;
	st.sources[s].gain = 1.0f;
	s2 = lwa_source_new(&st);
	st.sources[s2] = st.sources[s];
	for (i = 0; i < 1000; i++)
		st.buffers[b].pcm[i] = 30000;
	lwa_source_play(&st, s);
	lwa_source_play(&st, s2);
	lwa_mix(&st, out, 4);
	check(out[0] == 32767 && out[1] == 32767, "two loud sources clip at 32767");
	/* rates: an 11025 Hz buffer on a 22050 Hz device lasts twice as long */
	lwa_free(&st);
	lwa_init(&st, 22050);
	n = make_wav(w, 1, 16, 11025, d, 1000, 0);
	b = lwa_buffer_from_wav(&st, w, n);
	s = lwa_source_new(&st);
	st.sources[s].buffer = b;
	lwa_source_play(&st, s);
	lwa_mix(&st, out, 1999);
	check(st.sources[s].playing, "11025 Hz on 22050: playing at 1999");
	lwa_mix(&st, out, 2);
	check(!st.sources[s].playing, "11025 Hz on 22050: done after 2000");
	lwa_free(&st);
}

static void spatial_checks(void)
{
	float lpos[3] = { 0, 0, 0 }, at[3] = { 0, 0, -1 }, up[3] = { 0, 1, 0 };
	float back[3] = { 0, 0, 1 };
	float front[3] = { 0, 0, -0.5f }, right[3] = { 3, 0, 0 }, far[3] = { 0, 0, -10 };
	float big[3] = { 100, 0, 0 };
	float gl, gr, gl2, gr2;

	lwa_spatial(lpos, at, up, front, 0, 1.0f, &gl, &gr);
	check(gl == gr && gl > 0.70f && gl < 0.71f, "in front, near: centre at full gain");
	lwa_spatial(lpos, at, up, right, 0, 1.0f, &gl, &gr);
	check(gr > 0.33f && gr < 0.34f && gl < 1e-3f, "to the right at 3: right only, 1/3");
	lwa_spatial(lpos, back, up, right, 0, 1.0f, &gl2, &gr2);
	check(gl2 > 0.33f && gr2 < 1e-3f, "listener turned round: now on the left");
	lwa_spatial(lpos, at, up, far, 0, 1.0f, &gl, &gr);
	check(gl == gr && gl > 0.0706f && gl < 0.0709f, "in front at 10: 1/10 of centre");
	lwa_spatial(right, at, up, front, 1, 1.0f, &gl, &gr);
	check(gl == gr && gl > 0.70f, "relative: follows the listener");
	lwa_spatial(lpos, at, up, big, 0, 10.0f, &gl, &gr);
	check(gr > 0.099f && gr < 0.1f, "gain 10 at 100: 10/100");
	lwa_spatial(lpos, at, up, front, 0, 10.0f, &gl, &gr);
	check(gl * gl + gr * gr < 1.0001f, "gain held to 1.0");
}

int main(int argc, char **argv)
{
	int i;
	unsigned int srcs[64];
	int nsrc = 0;
	short *mix;
	static const float gains[] = { 1.0f, 0.5f, 0.25f, 0.75f };
	static const float pitches[] = { 1.0f, 1.5f, 0.75f, 1.25f };

	reader_checks();
	play_checks();
	spatial_checks();
	printf("%s: %d checks, %d failed\n", failures ? "FAIL" : "PASS", checks, failures);

	/* the game's WAVs: read, then all played at once and mixed */
	lwa_init(&st, 22050);
	for (i = 1; i < argc && nsrc < 64; i++) {
		unsigned long size;
		unsigned char *d = read_file(argv[i], &size);
		const char *base = strrchr(argv[i], '/');
		unsigned int b = d ? lwa_buffer_from_wav(&st, d, size) : 0;

		base = base ? base + 1 : argv[i];
		if (!b) {
			printf("WAV %s unreadable\n", base);
			free(d);
			continue;
		}
		printf("WAV %s frames=%lu rate=%lu hash=%08lx\n", base, st.buffers[b].frames,
			st.buffers[b].rate, hash_shorts(st.buffers[b].pcm, st.buffers[b].frames));
		free(d);
		srcs[nsrc] = lwa_source_new(&st);
		st.sources[srcs[nsrc]].buffer = b;
		st.sources[srcs[nsrc]].relative = 1;
		st.sources[srcs[nsrc]].gain = gains[nsrc % 4];
		st.sources[srcs[nsrc]].pitch = pitches[nsrc % 4];
		st.sources[srcs[nsrc]].looping = nsrc & 1;
		lwa_source_play(&st, srcs[nsrc]);
		nsrc++;
	}
	if (nsrc) {
		mix = (short *)malloc(22050 * 2 * sizeof *mix);
		for (i = 0; i < 3; i++) {
			lwa_mix(&st, mix, 22050);
			printf("MIX second %d hash=%08lx\n", i + 1, hash_shorts(mix, 22050 * 2));
		}
		for (i = 0; i < nsrc; i++)
			printf("SRC %d vol=%d,%d step=%lu\n", i, st.sources[srcs[i]].vol_l,
				st.sources[srcs[i]].vol_r, st.sources[srcs[i]].step);
		free(mix);
	}
	lwa_free(&st);
	return failures != 0;
}
