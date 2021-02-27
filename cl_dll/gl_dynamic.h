#pragma once
#ifndef GL_DYNAMIC_H
#define GL_DYNAMIC_H

#include "mod_features.h"

#if FEATURE_FOG
#define CLDLL_FOG
#endif

#ifdef CLDLL_FOG

#if defined (_WIN32)

#include "in_defs.h"

#else // _WIN32

#define APIENTRY // __stdcall ?

#endif // _WIN32

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

typedef void (APIENTRY *GLAPI_glEnable)(GLenum cap);
typedef void (APIENTRY *GLAPI_glDisable)(GLenum cap);
typedef void (APIENTRY *GLAPI_glFogi)(GLenum pname, GLint param);
typedef void (APIENTRY *GLAPI_glFogf)(GLenum pname, GLfloat param);
typedef void (APIENTRY *GLAPI_glFogfv)(GLenum pname, const GLfloat *params);
typedef void (APIENTRY *GLAPI_glHint)(GLenum target, GLenum mode);
typedef void (APIENTRY *GLAPI_glGetIntegerv)(GLenum pname, GLint* params);

extern GLAPI_glEnable GL_glEnable;
extern GLAPI_glDisable GL_glDisable;
extern GLAPI_glFogi GL_glFogi;
extern GLAPI_glFogf GL_glFogf;
extern GLAPI_glFogfv GL_glFogfv;
extern GLAPI_glHint GL_glHint;
extern GLAPI_glGetIntegerv GL_glGetIntegerv;

#endif

#endif // GL_DYNAMIC_H
