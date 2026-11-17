/*
 * LinWarrior on GFOS: GFOpenGPU has no GLU, and LinWarrior calls two of its
 * functions, written here on GL itself: gluPerspective as GLU's
 * documentation gives it (a glFrustum), gluBuild2DMipmaps as the base level
 * with GL 1.4's GL_GENERATE_MIPMAP making the rest.
 * Apache-2.0, like LinWarrior. Copyright (c) 2026 AmiCradle Team.
 */
#ifndef LW_AMIGA_GLU_H
#define LW_AMIGA_GLU_H
#include <GL/gl.h>

#ifndef GL_GENERATE_MIPMAP
#define GL_GENERATE_MIPMAP 0x8191
#endif

static inline void gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
	GLdouble h = __builtin_tan(fovy * 3.14159265358979323846 / 360.0) * zNear;
	GLdouble w = h * aspect;

	glFrustum(-w, w, -h, h, zNear, zFar);
}

static inline GLint gluBuild2DMipmaps(GLenum target, GLint internalFormat, GLsizei width,
		GLsizei height, GLenum format, GLenum type, const void *data)
{
	glTexParameteri(target, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexImage2D(target, 0, internalFormat, width, height, 0, format, type, data);
	return 0;
}
#endif
