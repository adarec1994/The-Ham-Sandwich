#pragma once

#include "ScopeObject.h"

class ScopeObjectCollection
{
	std::list<ScopeObject*> mObjects;
public:
	void addObject(ScopeObject& obj) {
		mObjects.push_back(&obj);
	}

	ScopeObjectCollection& operator , (ScopeObject& obj) {
		addObject(obj);
		return *this;
	}

	void registerAll(v8::Handle<v8::ObjectTemplate> templ) {
		for (auto& obj : mObjects) {
			obj->onRegister(templ);
		}
	}
};