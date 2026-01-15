#pragma once
#include <glad/glad.h>
#include "TerrainShader.h"

class AreaRender
{
	GLuint mShaderProgram = 0;
	TerrainShader::Uniforms mUniforms;

public:
	AreaRender();
	~AreaRender();

	void init();
	[[nodiscard]] GLuint getProgram() const { return mShaderProgram; }
	[[nodiscard]] const TerrainShader::Uniforms& getUniforms() const { return mUniforms; }
};