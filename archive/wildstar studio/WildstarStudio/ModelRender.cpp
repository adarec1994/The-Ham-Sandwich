#include "stdafx.h"
#include "ModelRender.h"
#include "GlExt.h"
#include "GxContext.h"
#include "Window.h"
#include "UIManager.h"

ModelRender::ModelRender() {
	mWidth = 0;
	mHeight = 0;
	mRotation = 0.0f;
	mLastUpdate = std::chrono::high_resolution_clock::now();
	mIsRotating = true;
	mPointMode = false;
	mUseNormal = false;
	mI3Sector = 0;
	mI3Geometry = std::make_shared<InputGeometry>();
	mI3Geometry->addElement(VertexSemantic::Position, 0, 3);
	mI3Geometry->addElement(VertexSemantic::Normal, 0, 3);
	mI3Geometry->addElement(VertexSemantic::TexCoord, 0, 2);

	mM3Geometry = std::make_shared<InputGeometry>();
	mM3Geometry->addElement(VertexSemantic::Position, 0, 3);
	mM3Geometry->addElement(VertexSemantic::Normal, 0, 3);
	mM3Geometry->addElement(VertexSemantic::TexCoord, 0, 2);
}

void ModelRender::initGraphics(uint32 width, uint32 height) {
	mWidth = width;
	mHeight = height;

	glGenFramebuffers(1, &mFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);

	glGenTextures(1, &mTexture);
	glBindTexture(GL_TEXTURE_2D, mTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glGenRenderbuffers(1, &mDepthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffer);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTexture, 0);
	GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, buffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		throw std::runtime_error("Unable to create off screen render texture");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	mI3Program = std::make_shared<Program>();
	mI3Program->loadVertexShader(L"GLSL", L"I3VERTEX");
	mI3Program->loadPixelShader(L"GLSL", L"I3FRAGMENT");
	mI3Program->linkProgram();

	mI3Uniform.matProj = mI3Program->getUniformIndex(L"matProj");
	mI3Uniform.matView = mI3Program->getUniformIndex(L"matView");
	mI3Uniform.matWorld = mI3Program->getUniformIndex(L"matWorld");

	mI3Geometry->setProgram(mI3Program);
	mI3Geometry->setVertexLayout(VertexLayout::Triangles);
	mI3Geometry->setInstanceCount(0);
	mI3Geometry->setStartIndex(0);
	mI3Geometry->setStride(sizeof(I3ModelVertex));

	mI3Geometry->finalize();

	mI3TexInput = std::make_shared<TextureInput>();
	mI3TexInput->attachToProgram(mI3Program);

	mM3Program = std::make_shared<Program>();
	mM3Program->loadVertexShader(L"GLSL", L"M3VERTEX");
	mM3Program->loadPixelShader(L"GLSL", L"M3FRAGMENT");
	mM3Program->linkProgram();

	mM3Uniform.matProj = mM3Program->getUniformIndex(L"matProj");
	mM3Uniform.matView = mM3Program->getUniformIndex(L"matView");
	mM3Uniform.matWorld = mM3Program->getUniformIndex(L"matWorld");

	mM3Geometry->setProgram(mM3Program);
	mM3Geometry->setVertexLayout(VertexLayout::Triangles);
	mM3Geometry->setInstanceCount(0);
	mM3Geometry->setStartIndex(0);
	mM3Geometry->setStride(sizeof(M3ModelVertex));

	mM3Geometry->finalize();

	mM3TexInput = std::make_shared<TextureInput>();
	mM3TexInput->attachToProgram(mM3Program);
}

void ModelRender::resize(uint32 width, uint32 height) {
	glBindTexture(GL_TEXTURE_2D, mTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	mWidth = width;
	mHeight = height;
	glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void ModelRender::stopRotation() {
	mIsRotating = false;
}

void ModelRender::restartRotation() {
	mLastUpdate = std::chrono::high_resolution_clock::now();
	mIsRotating = true;
}

void ModelRender::resetRotation() {
	mRotation = 0.0f;
}

void ModelRender::renderModel(I3ModelPtr model) {
	static float tanFov = tanf(22.5f * 3.141592f / 180.0f);

	float dstHeight = model->getSectors()[mI3Sector]->getHeight() * 0.75f;
	float dstWidth = dstHeight / tanFov;

	glViewport(0, 0, mWidth, mHeight);

	glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffer);

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	Matrix matView = Matrix::lookAt(
		Vector3(0, model->getSectors()[mI3Sector]->getHeight() / 2.0f + model->getSectors()[mI3Sector]->getMinPos(), std::max(model->getSectors()[mI3Sector]->getMaxDistance() * 2.0f, dstWidth)),
		Vector3(0, model->getSectors()[mI3Sector]->getHeight() / 2.0f + +model->getSectors()[mI3Sector]->getMinPos(), 0),
		Vector3(0, -1, 0));

	if (mIsRotating) {
		auto now = std::chrono::high_resolution_clock::now();
		auto diff = now - mLastUpdate;
		mLastUpdate = now;

		mRotation += (std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() / 1000.0f) * 90.0f;
	}

	Matrix matRot = Matrix::rotation(mRotation, Vector3::UnitY);

	mI3Program->begin();
	mI3Program->set(mI3Uniform.matView, matView);	
	mI3Program->set(mI3Uniform.matWorld, matRot);

	Matrix matProj = Matrix::perspective(45.0f, (float) mWidth / (float) mHeight, 0.5f, 10000.0f);
	mI3Program->set(mI3Uniform.matProj, matProj);
	mI3Program->end();

	glColor3f(1.0f, 0.5f, 0.25f);
	model->renderSector(mI3Sector, mI3Geometry, mI3TexInput);
	glColor3f(1, 1, 1);

	glFlush();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
}

void ModelRender::renderModel(M3ModelPtr model) {
	static float tanFov = tanf(22.5f * 3.141592f / 180.0f);

	float dstHeight = model->getMaxHeight() * 0.75f;
	float dstWidth = dstHeight / tanFov;

	glViewport(0, 0, mWidth, mHeight);

	glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffer);

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	Matrix matView = Matrix::lookAt(
		Vector3(0, -model->getMaxHeight() / 2.0f, std::max(model->getMaxDistance() * 2.0f, dstWidth)),
		Vector3(0, -model->getMaxHeight() / 2.0f, 0.0f),
		Vector3(0, 1, 0)
	);

	glEnable(GL_DEPTH_TEST);


	if (mIsRotating) {
		auto now = std::chrono::high_resolution_clock::now();
		auto diff = now - mLastUpdate;
		mLastUpdate = now;

		mRotation += (std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() / 1000.0f) * 90.0f;
	}

	Matrix matRot = Matrix::rotation(mRotation, Vector3::UnitY);

	mM3Program->begin();
	mM3Program->set(mM3Uniform.matView, matView);
	mM3Program->set(mM3Uniform.matWorld, matRot);

	Matrix matProj = Matrix::perspective(45.0f, (float) mWidth / (float) mHeight, 0.5f, 10000.0f);
	mM3Program->set(mM3Uniform.matProj, matProj);
	mM3Program->end();

	model->render(mM3Geometry, mM3TexInput);

	glFlush();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
}