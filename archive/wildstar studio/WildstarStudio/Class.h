#pragma once

#include "ClassInstance.h"
#include "ScopeObject.h"
#include "Bases.h"

template<typename T>
class Class : public ScopeObject
{
	template<typename... Bases>
	class base_helper
	{
	public:
		base_helper(Class<T>* ) { }
		void operator()() { }
	};

	template<typename Base, typename... Rem>
	class base_helper<Base, Rem...>
	{
	public:
		base_helper(Class<T>* t) {
			t->expand_bases<Base>();
			base_helper<Rem...> bh(t);
		}
	};

	ClassInstance<T>* mInstance;
	std::wstring mName;

	template<typename Base>
	void expand_bases() {
		mInstance->def_base<Base>();
	}

public:
	Class(const std::wstring& name) {
		mName = name;
		mInstance = new ClassInstance<T>(name);
	}

	template<typename... Classes>
	Class(const std::wstring& name, Bases<Classes...> base) {
		mName = name;
		mInstance = new ClassInstance<T>(name);

		base_helper<Classes...> bh(this);
	}

	void onRegister(v8::Handle<v8::ObjectTemplate> scope) {
		scope->Set(v8::String::New((const uint16_t*) mName.c_str(), mName.length()), mInstance->getTemplate());
	}

	template<typename... Args>
	Class<T>& constructor() {
		std::function<std::shared_ptr<T> (Args...)> crFun = [](Args... args) { return std::make_shared<T>(args...); };
		mInstance->def_constructor(crFun);
		return *this;
	}

	template<typename R, typename... Args, typename S>
	Class<T>& function(const std::wstring& name, S s) {
		std::function<R (std::shared_ptr<T>, Args...)> fun = [s](std::shared_ptr<T> _this, Args... args) { return s(_this, args...); };
		mInstance->def_function(name, fun);
		return *this;
	}


	template<typename R, typename... Args>
	Class<T>& function(const std::wstring& name, R (T::*func)(Args...)) {
		std::function<R(std::shared_ptr<T>, Args...)> fun = [func](std::shared_ptr<T> _this, Args... args) { return (_this.get()->*func)(args...); };
		mInstance->def_function(name, fun);
		return *this;
	}

	template<typename R, typename... Args>
	Class<T>& static_function(const std::wstring& name, std::function<R (Args...)> fun) {
		mInstance->def_function(name, fun, false);
		return *this;
	}

	template<typename R, typename... Args>
	Class<T>& static_function(const std::wstring& name, R (*fun)(Args...)) {
		mInstance->def_function(name, std::function<R (Args...)>([fun](Args... args) { return fun(args...); }), false);
		return *this;
	}

	template<typename R>
	Class<T>& property(const std::wstring& name, std::function<R (std::shared_ptr<T>)> getter, std::function<void (std::shared_ptr<T>, R)> setter = std::function<void (std::shared_ptr<T>, R)>()) {
		mInstance->def_property<R>(name, getter, setter);
		return *this;
	}

	template<typename R>
	Class<T>& property(const std::wstring& name, R(T::*getter)(), void(T::*setter)(R) = nullptr) {
		std::function<R (std::shared_ptr<T>)> _getter = [getter](std::shared_ptr<T> t) { return (t.get()->*getter)(); };
		std::function<void (std::shared_ptr<T>, R)> _setter;
		if (setter != nullptr) {
			_setter = [setter](std::shared_ptr<T> t, R value) { (t.get()->*setter)(value); };
		}

		return property<R>(name, _getter, _setter);
	}
};