#include "stdafx.h"
#include "ScopeObjectCollection.h"
#include "ScopeObject.h"

ScopeObjectCollection ScopeObject::operator , (ScopeObject& other) {
	ScopeObjectCollection ret;
	ret.addObject(*this);
	ret.addObject(other);
	return ret;
}