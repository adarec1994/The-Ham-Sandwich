#include "stdafx.h"
#include "AreaRender.h"
#include "UIManager.h"

AreaRender::AreaRender() {

}

void AreaRender::resize(uint32 width, uint32 height) {
	glBindTexture(GL_TEXTURE_2D, mTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	mWidth = width;
	mHeight = height;
	glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	mCamera->setAspect((float) width / (float) height);

	mProj = Matrix::perspective(45.0f, (float) width / (float) height, 1.0f, 2000.0f);
}

void AreaRender::initGraphics(uint32 width, uint32 height) {
	mCamera = std::make_shared<PerspectivicCamera>();
	mCamera->setPosition(Vector3(-50.0f, 0.0f, 0.0f));
	mCamera->setTarget(Vector3(0.0f, 0.0f, 0.0f));
	mCamera->setNearClip(1.0f);
	mCamera->setFarClip(2000.0f);
	mCamera->setAspect((float) width / (float) height);

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

	mProj = Matrix::perspective(45.0f, (float) width / (float) height, 1.0f, 2000.0f);
}

void AreaRender::renderArea(AreaFilePtr area) {
	update();

	glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffer);

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	area->render(mView, mProj);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void AreaRender::update() {
	float diff = (GetTickCount() - mLastUpdate) / 1000.0f;
	mLastUpdate = GetTickCount();

	auto wnd = sUIMgr->getWindow()->getHandle();
	if (wnd != GetForegroundWindow()) {
		POINT curCursorPos;
		GetCursorPos(&curCursorPos);
		mLastCursor = curCursorPos;
		return;
	}

	BYTE state[256] = { 0 };
	GetKeyboardState(state);

	if ((state['W'] & 0x80) != 0) {
		moveForward(diff * 30);
	}

	if ((state['S'] & 0x80) != 0) {
		moveForward(diff * -30);
	}

	if ((state['A'] & 0x80) != 0) {
		moveSide(diff * 30);
	}

	if ((state['D'] & 0x80) != 0) {
		moveSide(diff * -30);
	}

	if ((state['Q'] & 0x80) != 0) {
		moveUp(diff * -30);
	}

	if ((state['E'] & 0x80) != 0) {
		moveUp(diff * 30);
	}

	POINT curCursorPos;
	GetCursorPos(&curCursorPos);

	if (GetKeyState(VK_RBUTTON) & 0x8000) {
		int dx = curCursorPos.x - mLastCursor.x;
		int dy = curCursorPos.y - mLastCursor.y;
		if (dx != 0) {
			yaw((-dx * 0.003f * 180.0f) / 3.141592f);
		}

		if (dy != 0) {
			pitch((-dy * 0.003f * 180.0f) / 3.141592f);
		}
	}

	mLastCursor = curCursorPos;
}

void AreaRender::onModelSelected(AreaFilePtr file) {
	mLastUpdate = GetTickCount();
	mPosition = Vector3(-50.0f, file->getAverageHeight() + 50.0f, 0.0f);
	mForward = Vector3(1.0f, 0.0f, 0.0f);
	mUp = Vector3(0.0f, -1.0f, 0.0f);
	mRight = Vector3(0, 0, 1);
	mView = Matrix::lookAt(mPosition, mPosition + mForward, mUp);
}

void AreaRender::moveUp(float amount) {
	move(Vector3(0, -1, 0) * amount);
}

void AreaRender::moveForward(float amount) {
	move(mForward * amount);
}

void AreaRender::moveSide(float amount) {
	move(mRight * amount);
}

void AreaRender::move(const Vector3& direction) {
	mPosition += direction;

	mView = Matrix::lookAt(mPosition, mPosition + mForward, mUp);
}

void AreaRender::pitch(float angle) {
	Matrix matRot = Matrix::rotation(angle, mRight);
	mForward = matRot * mForward;
	mForward.normalize();
	mUp = matRot * mUp;
	mUp.normalize();

	mView = Matrix::lookAt(mPosition, mPosition + mForward, mUp);
}

void AreaRender::yaw(float angle) {
	Matrix matRot = Matrix::rotation(angle, Vector3(0, -1, 0));
	mForward = matRot * mForward;
	mForward.normalize();
	mUp = matRot * mUp;
	mUp.normalize();
	mRight = matRot * mRight;
	mRight.normalize();

	mView = Matrix::lookAt(mPosition, mPosition + mForward, mUp);
}

void AreaRender::roll(float angle) {
	Matrix matRot = Matrix::rotation(angle, mForward);
	mUp = matRot * mForward;
	mUp.normalize();
	mRight = matRot * mRight;
	mRight.normalize();

	mView = Matrix::lookAt(mPosition, mPosition + mForward, mUp);
}
