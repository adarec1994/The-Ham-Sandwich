#include "AreaRender.h"
#include "AreaFile.h"
#include "TerrainShader.h"
#include <vector>
#include <string>

AreaRender::AreaRender() = default;

AreaRender::~AreaRender()
{
    if (mShaderProgram) glDeleteProgram(mShaderProgram);
}

void AreaRender::init()
{
    if (mShaderProgram != 0)
    {
        glDeleteProgram(mShaderProgram);
        mShaderProgram = 0;
    }

    mShaderProgram = TerrainShader::CreateProgram();
    if (mShaderProgram == 0)
        return;

    TerrainShader::GetUniforms(mShaderProgram, mUniforms);

    glUseProgram(mShaderProgram);

    glUniform1i(mUniforms.blendMap, 0);
    glUniform1i(mUniforms.colorMap, 1);
    glUniform1i(mUniforms.layer0, 2);
    glUniform1i(mUniforms.layer1, 3);
    glUniform1i(mUniforms.layer2, 4);
    glUniform1i(mUniforms.layer3, 5);
    glUniform1i(mUniforms.layer0Normal, 6);
    glUniform1i(mUniforms.layer1Normal, 7);
    glUniform1i(mUniforms.layer2Normal, 8);
    glUniform1i(mUniforms.layer3Normal, 9);

    glUniform4f(mUniforms.baseColor, 1.0f, 1.0f, 1.0f, 1.0f);
    glUniform4f(mUniforms.highlightColor, 0.0f, 0.0f, 0.0f, 0.0f);
    glUniform4f(mUniforms.texScale, 8.0f, 8.0f, 8.0f, 8.0f);

    AreaChunkRender::geometryInit(mShaderProgram);

    glUseProgram(0);
}