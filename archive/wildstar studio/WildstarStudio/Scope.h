#pragma once

#include "ScopeObject.h"
#include "ScopeObjectCollection.h"

class Scope
{
protected:
	v8::Handle<v8::ObjectTemplate> mTemplate;

public:
	Scope(v8::Handle<v8::ObjectTemplate> scope) {
		mTemplate = scope;
	}

	void operator [] (ScopeObject& obj) {
		obj.onRegister(mTemplate);
	}

	void operator [] (ScopeObjectCollection coll);

	v8::Handle<v8::ObjectTemplate> getTemplate() { return mTemplate; }
};