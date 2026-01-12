#include "stdafx.h"
#include "Scope.h"
#include "ScopeObjectCollection.h"

void Scope::operator [] (ScopeObjectCollection coll) {
	coll.registerAll(mTemplate);
}