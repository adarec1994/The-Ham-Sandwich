#pragma once
#include <glad/glad.h>

class AreaRender
{
	GLuint mShaderProgram = 0;

public:
	AreaRender();
	~AreaRender();

	void init();
	[[nodiscard]] GLuint getProgram() const { return mShaderProgram; }
};