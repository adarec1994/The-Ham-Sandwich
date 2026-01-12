#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>

class AreaRender
{
	GLuint mShaderProgram = 0;

public:
	AreaRender();
	~AreaRender();

	void init();
	GLuint getProgram() const { return mShaderProgram; }
};