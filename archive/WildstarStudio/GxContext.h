#pragma once

#include "Window.h"

SHARED_FWD(GxContext);

class GxContext
{
	static GxContextPtr gInstance;

	HGLRC mGxCtx;
	HDC mWndDC;

	void loadExtensions();

public:
	void init(WindowPtr wnd);

	void onFrame();

	static GxContextPtr getInstance() { 
		if (gInstance == nullptr) {
			gInstance = std::make_shared<GxContext>();
		}

		return gInstance; 
	}

};

#define sGxCtx (GxContext::getInstance())