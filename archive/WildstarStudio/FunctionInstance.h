#pragma once

#include "ArgumentMatch.h"
#include "ObjectWrap.h"

class FunctionInstance
{
	static void __cdecl internalFunctionCallback(const v8::FunctionCallbackInfo<v8::Value>& info);
	std::wstring mName;
	v8::Handle<v8::FunctionTemplate> mFunction;

#pragma region Overload Types (GenericFunctionOverload, FunctionOverload)

	class GenericFunctionOverload
	{
	public:
		virtual uint32 matchScore(const v8::FunctionCallbackInfo<v8::Value>& info) = 0;
		virtual void execute(const v8::FunctionCallbackInfo<v8::Value>& info) = 0;
	};

	template<typename Ret, typename... Args>
	class FunctionOverload : public GenericFunctionOverload
	{
		std::function<Ret (Args...)> mFunction;
		bool mMemberFunction;

		template<uint32 index, typename T, typename... Rem>
		void matchArgument(const v8::FunctionCallbackInfo<v8::Value>& info, int32& curScore) {
			auto match = Argument::match<T>(info[index]);
			if (match == ArgumentMatch::None) {
				curScore = 0;
				return;
			} else if (match == ArgumentMatch::Partial) {
				curScore -= 99 / (sizeof...(Args));
			}

			matchArgument<index + 1, Rem...>(info, curScore);
		}

		template<uint32 size>
		void matchArgument(const v8::FunctionCallbackInfo<v8::Value>& info, int32& curScore) {

		}

		template<uint32 index, typename T, typename... Rem, typename... Cur>
		void expand(const v8::FunctionCallbackInfo<v8::Value>& info, const Cur&... args) {
			expand<index + 1, Rem...>(info, args..., ObjectWrap::unwrap<T>(info[index]));
		}

		template<uint32 index, typename... Rem>
		void expand(const v8::FunctionCallbackInfo<v8::Value>& info, const Rem&... args) {
			make_res<Ret>(info, args...);
		}

		template<typename R>
		typename std::enable_if<!std::is_same<R, void>::value, void>::type make_res(const v8::FunctionCallbackInfo<v8::Value>& info, const Args&... args) {
			v8::TryCatch tc;
			auto res = mFunction(args...);
			if (tc.HasCaught()) {
				tc.ReThrow();
				return;
			}

			info.GetReturnValue().Set(ObjectWrap::wrap<R>(res));
			
		}

		template<typename R>
		typename std::enable_if<std::is_same<R, void>::value, void>::type make_res(const v8::FunctionCallbackInfo<v8::Value>& info, const Args&... args) {
			v8::TryCatch tc;
			mFunction(args...);
			if (tc.HasCaught()) {
				tc.ReThrow();
			} else {
				info.GetReturnValue().Set(v8::Undefined());
			}
		}

		template<typename T, typename... Rem>
		uint32 matchScoreMember(const v8::FunctionCallbackInfo<v8::Value>& info) {
			int32 score = 100;

			if (info.This()->IsUndefined()) {
				return 0;
			}

			if (Argument::match<T>(info.This()) != ArgumentMatch::Exact) {
				return 0;
			}

			if ((info.Length() + 1) < sizeof...(Args)) {
				score = 0;
			} else {
				matchArgument<0, Rem...>(info, score);
			}

			if ((info.Length() + 1) > sizeof...(Args) && score > 1) {
				score /= 2;
			}

			if (score < 0) {
				score = 0;
			}

			return (uint32) score;
		}

		template<typename T, typename... Rem>
		void executeMember(const v8::FunctionCallbackInfo<v8::Value>& info) {
			expand<0, Rem...>(info, ObjectWrap::unwrap<T>(info.This()));
		}

	public:
		FunctionOverload(std::function<Ret (Args...)> fun, bool memberFunction = false) {
			mFunction = fun;
			mMemberFunction = memberFunction;
		}

		template<typename... Rem>
		uint32 matchScore_impl(const v8::FunctionCallbackInfo<v8::Value>& info) {
			if (mMemberFunction == true) {
				return matchScoreMember<Rem...>(info);
			}

			int32 score = 100;
			if (info.Length() < sizeof...(Rem)) {
				score = 0;
			} else {
				matchArgument<0, Rem...>(info, score);
			}

			if (info.Length() > sizeof...(Rem) && score > 1) {
				score /= 2;
			}

			if (score < 0) {
				score = 0;
			}

			return (uint32) score;
		}

		template<>
		uint32 matchScore_impl<>(const v8::FunctionCallbackInfo<v8::Value>& info) {
			if (mMemberFunction == true) {
				return 0;
			}

			if (info.Length() > 0) {
				return 50;
			} else {
				return 100;
			}
		}

		uint32 matchScore(const v8::FunctionCallbackInfo<v8::Value>& info) {
			return matchScore_impl<Args...>(info);
		}

		template<typename... Rem>
		void execute_impl(const v8::FunctionCallbackInfo<v8::Value>& info) {
			if (mMemberFunction == true) {
				return executeMember<Rem...>(info);
			}

			expand<0, Rem...>(info);
		}

		template<>
		void execute_impl<>(const v8::FunctionCallbackInfo<v8::Value>& info) {
			v8::TryCatch tc;
			auto res = mFunction();
			if (tc.HasCaught()) {
				tc.ReThrow();
			} else {
				info.GetReturnValue().Set(ObjectWrap::wrap<Ret>(res));
			}
		}

		void execute(const v8::FunctionCallbackInfo<v8::Value>& info) {
			execute_impl<Args...>(info);
		}
	};
#pragma endregion

	std::list<std::shared_ptr<GenericFunctionOverload>> mOverloads;

	void invokeFunction(const v8::FunctionCallbackInfo<v8::Value>& info);
	
	std::list<std::function<bool (const v8::FunctionCallbackInfo<v8::Value>& info)>> mConstraints;
public:
	FunctionInstance();

	template<typename Res, typename... Args>
	void addOverload(std::function<Res (Args...)> fun) {
		mOverloads.push_back(std::make_shared<FunctionOverload<Res, Args...>>(fun));
	}

	template<typename Res, typename... Args>
	void addMemberOverload(std::function<Res(Args...)> fun) {
		mOverloads.push_back(std::make_shared<FunctionOverload<Res, Args...>>(fun, true));
	}

	template<typename T>
	void addConstraint(T t) {
		mConstraints.push_back([t](const v8::FunctionCallbackInfo<v8::Value>& info) { return t(info); });
	}

	v8::Handle<v8::FunctionTemplate> getTemplate() const { return mFunction; }
};