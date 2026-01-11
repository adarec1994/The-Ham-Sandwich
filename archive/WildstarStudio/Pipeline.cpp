#include "stdafx.h"
#include "Pipeline.h"

std::shared_ptr<Pipeline> Pipeline::gInstance;

void Pipeline::applyGeometry(InputGeometryPtr geometry) {
	mGeometry = geometry;
	mGeometry->bindElements();
}

void Pipeline::removeGeometry(InputGeometryPtr geometry) {
	if (geometry != mGeometry)
		return;

	mGeometry->unbindElements();
	mGeometry = nullptr;
}

void Pipeline::render(TextureInputPtr texInput) {
	if (mGeometry == nullptr || mGeometry->getVertexCount() == 0 || mGeometry->getTriangleCount() == 0)
		return;

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	auto program = mGeometry->getProgram();
	program->begin();

	if (texInput != nullptr)
		texInput->apply();

	mGeometry->getVertexBuffer()->apply();
	mGeometry->getIndexBuffer()->apply();

	if (mGeometry->getInstanceCount() == 0) {
		glDrawElements((GLenum) mGeometry->getLayout(), mGeometry->getIndexCount(),
			mGeometry->getIndexBuffer()->getIndexType(),
			(GLvoid*) (mGeometry->getStartIndex() * mGeometry->getIndexBuffer()->getIndexSize()));
	} else {
		if (mGeometry->getStartIndex() * mGeometry->getIndexBuffer()->getIndexSize() > 0x1000000) {
			std::stringstream strm;
			strm << mGeometry->getStartIndex();
			throw std::exception(strm.str().c_str());
		}
		glDrawElementsInstanced((GLenum) mGeometry->getLayout(), mGeometry->getIndexCount(),
			mGeometry->getIndexBuffer()->getIndexType(),
			(GLvoid*) (mGeometry->getStartIndex() * mGeometry->getIndexBuffer()->getIndexSize()),
			mGeometry->getInstanceCount());
	}

	if (texInput != nullptr)
		texInput->remove();

	program->end();

	mGeometry->getVertexBuffer()->remove();
	mGeometry->getIndexBuffer()->remove();

	glDisable(GL_DEPTH_TEST);
}