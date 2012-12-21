#pragma once

#include "GLCommon.hpp"

struct FramebufferGL
{
	GLuint fbo;
	GLuint * colorTexId;
	GLuint   depthTexId;
	GLenum * drawBuffers;
	int width;
	int height;
	int outCount;
};

int  build_framebuffer(FramebufferGL & fb, int width, int height, int outCount);
int  build_framebuffer(FramebufferGL & fb, int width, int height, GLuint c_colorTexId, GLuint c_depthTexId);
int  destroy_framebuffer(FramebufferGL & fb);
