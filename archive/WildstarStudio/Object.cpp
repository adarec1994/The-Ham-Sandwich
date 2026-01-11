#include "stdafx.h"
#include "Object.h"

Object::Object(v8::Handle<v8::Object> obj) {
	mObject = obj;
}

Object::Object() {
	mObject = v8::Object::New(v8::Isolate::GetCurrent());
}