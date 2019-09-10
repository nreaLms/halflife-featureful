#pragma once
#ifndef GL_DYNAMIC_H
#define GL_DYNAMIC_H

#define CLDLL_FOG

#ifdef CLDLL_FOG

#if defined (_WIN32)

#include <windows.h>

#else // _WIN32

#define APIENTRY // __stdcall ?

#endif // _WIN32

#include <GL/gl.h>

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
