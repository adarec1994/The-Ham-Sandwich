#include "stdafx.h"
#include "FunctionInstance.h"

FunctionInstance::FunctionInstance() {
	mFunction = v8::FunctionTemplate::New(&FunctionInstance::internalFunctionCallback, v8::External::New(this));
}

void FunctionInstance::internalFunctionCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
	if (info.Data()->IsExternal() == false) {
		v8::ThrowException(v8::String::New("internal error: native function call does not provide an external value."));
		return;
	}

	auto ext = info.Data().As<v8::External>();
	if (ext->Value() == nullptr) {
		v8::ThrowException(v8::String::New("internal error: native function call does not provide an external value."));
		return;
	}

	FunctionInstance* funcInstance = (FunctionInstance*) ext->Value();
	funcInstance->invokeFunction(info);
}

void FunctionInstance::invokeFunction(const v8::FunctionCallbackInfo<v8::Value>& info) {
	for (auto& cst : mConstraints) {
		if (cst(info) == false) {
			return;
		}
	}

	std::shared_ptr<GenericFunctionOverload> bestMatch;
	uint32 bestScore = 0;

	for (auto& overload : mOverloads) {
		uint32 score = overload->matchScore(info);
		if (score == 0) {
			continue;
		}

		if (bestMatch == nullptr || score > bestScore) {
			bestScore = score;
			bestMatch = overload;
		}
	}

	if (bestMatch == nullptr) {
		v8::ThrowException(v8::Exception::TypeError(v8::String::New("Could not match function overload for function.")));
		return;
	}

	bestMatch->execute(info);
}