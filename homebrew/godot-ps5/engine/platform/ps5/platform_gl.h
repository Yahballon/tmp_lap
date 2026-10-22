// SPDX-License-Identifier: MIT
#pragma once
// Isolated archive-only stage: use Godot 4.7.2's own pinned GLAD headers.
// The actual EGL/GL implementation is NOT initialized by this header.
#ifndef GL_API_ENABLED
#define GL_API_ENABLED
#endif
#ifndef GLES_API_ENABLED
#define GLES_API_ENABLED
#endif
#ifndef GLAD_GLES2
#define GLAD_GLES2
#endif
#include <thirdparty/glad/glad/gl.h>
