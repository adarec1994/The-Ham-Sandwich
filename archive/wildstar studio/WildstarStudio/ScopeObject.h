#pragma once

class ScopeObjectCollection;

class ScopeObject
{
public:
	virtual void onRegister(v8::Handle<v8::ObjectTemplate> scope) = 0;

	ScopeObjectCollection operator , (ScopeObject& other);
};