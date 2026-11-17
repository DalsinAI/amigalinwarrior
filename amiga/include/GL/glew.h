/*
 * LinWarrior on GFOS: GLEW is not used. GFOpenGPU's libGL exports the whole
 * GL API, extensions included, so the game's GL calls link straight to it;
 * this header stands in for GLEW's (LinWarrior includes it inside its own
 * namespace GL, so it pulls in nothing but GL's own headers and GLU's).
 * Apache-2.0, like LinWarrior. Copyright (c) 2026 AmiCradle Team.
 */
#ifndef LW_AMIGA_GLEW_H
#define LW_AMIGA_GLEW_H

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <GL/gl.h>
#include <GL/glext.h>

#define GLEW_OK 0
#define GLEW_VERSION_2_1 1
#define GLEW_GET_FUN(x) x
static GLboolean glewExperimental __attribute__((unused)) = GL_FALSE;
static inline GLenum glewInit(void) { return GLEW_OK; }
static inline const GLubyte *glewGetErrorString(GLenum e) { (void)e; return (const GLubyte *)"no GLEW on GFOS"; }
static inline const GLubyte *glewGetString(GLenum e) { (void)e; return (const GLubyte *)"GFOS"; }

#include <GL/glu.h>
#endif
