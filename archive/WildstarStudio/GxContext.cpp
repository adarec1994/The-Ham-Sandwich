#include "stdafx.h"
#include "GxContext.h"
#include "UIManager.h"
#include "TextureManager.h"

GxContextPtr GxContext::gInstance = nullptr;

void GxContext::init(WindowPtr wnd) {
	mWndDC = GetDC(wnd->getHandle());

	PIXELFORMATDESCRIPTOR pfd = { 0 };
	pfd.cAlphaBits = 8;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;
	pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.nSize = sizeof(pfd);

	int format = ChoosePixelFormat(mWndDC, &pfd);
	if (format == 0) {
		throw std::runtime_error("Unable to find a suitable pixel format");
	}

	BOOL result = SetPixelFormat(mWndDC, format, &pfd);
	if (result == FALSE) {
		throw std::runtime_error("Unable to find a suitable pixel format");
	}

	mGxCtx = wglCreateContext(mWndDC);
	wglMakeCurrent(mWndDC, mGxCtx);

	glClearColor(0, 0, 0, 1);
	glViewport(0, 0, wnd->getWidth(), wnd->getHeight());

	loadExtensions();

	sTexMgr->init();
	sUIMgr->init(wnd->getWidth(), wnd->getHeight(), wnd);
}

void GxContext::onFrame() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	sUIMgr->onFrame();

	SwapBuffers(mWndDC);
}

#define PROC(name) name = (decltype(name))wglGetProcAddress(#name)

void GxContext::loadExtensions() {
	PROC(glActiveTexture); // GL_ARB_texture_compression
	PROC(glCompressedTexImage2D); // GL_ARB_texture_compression
	PROC(glDebugMessageCallbackAMD); // dynamic
	PROC(glDebugMessageEnableAMD); // dynamic
	PROC(glDebugMessageCallbackARB); // dynamic
	PROC(glDebugMessageControlARB); // dynamic
	PROC(glBindBuffer); // GL_ARB_vertex_buffer_object
	PROC(glBufferData); // GL_ARB_vertex_buffer_object
	PROC(glGenBuffers); // GL_ARB_vertex_buffer_object
	PROC(glDeleteBuffers); // GL_ARB_vertex_buffer_object
	PROC(glGetAttribLocation); // GL_ARB_vertex_shader
	PROC(glVertexAttrib3f); // GL_ARB_vertex_shader
	PROC(glVertexAttrib3fv); // GL_ARB_vertex_shader
	PROC(glUniform1f); // GL_ARB_shader_objects
	PROC(glUniform3f); // GL_ARB_shader_objects
	PROC(glUniform3fv); // GL_ARB_shader_objects
	PROC(glUniform4fv);
	PROC(glUniform2fv); // GL_ARB_shader_objects
	PROC(glUniformMatrix4fv); // GL_ARB_shader_objects
	PROC(glCreateProgram);  // GL_ARB_shader_objects
	PROC(glShaderSource); // GL_ARB_shader_objects
	PROC(glCreateShader); // GL_ARB_shader_objects
	PROC(glLinkProgram); // GL_ARB_shader_objects
	PROC(glAttachShader); // GL_ARB_shader_objects
	PROC(glGetShaderInfoLog); // GL_ARB_shader_objects
	PROC(glGetProgramInfoLog); // GL_ARB_shader_objects
	PROC(glGetShaderiv); // GL_ARB_shader_objects
	PROC(glGetProgramiv); // GL_ARB_vertex_program
	PROC(glUseProgram); // GL_ARB_shader_objects
	PROC(glCompileShader); // GL_ARB_shader_objects
	PROC(glBindAttribLocation); // GL_ARB_vertex_shader
	PROC(glVertexAttribPointer); // GL_ARB_vertex_shader
	PROC(glGetUniformLocation); // GL_ARB_shader_objects
	PROC(glUniform1i); // GL_ARB_shader_objects
	PROC(glEnableVertexAttribArray); // GL_ARB_vertex_shader
	PROC(glDisableVertexAttribArray); // GL_ARB_vertex_shader
	PROC(glGenerateMipmap); // GL_SGIS_generate_mipmap
	PROC(glFramebufferRenderbuffer);
	PROC(glBindFramebuffer);
	PROC(glBindRenderbuffer);
	PROC(glFramebufferTexture);
	PROC(glGenRenderbuffers);
	PROC(glGenFramebuffers);
	PROC(glRenderbufferStorage);
	PROC(glDrawBuffers);
	PROC(glCheckFramebufferStatus);
}