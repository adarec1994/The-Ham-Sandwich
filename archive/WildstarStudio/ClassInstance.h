#pragma once

#include "FunctionInstance.h"

class GenericSharedPtrWrap
{
public:
	virtual ~GenericSharedPtrWrap() {

	}
};

template<typename T>
class SharedPtrWrap : public GenericSharedPtrWrap
{
	std::shared_ptr<T> mPtr;
public:
	SharedPtrWrap(std::shared_ptr<T> ptr) {
		mPtr = ptr;
	}

	~SharedPtrWrap() {

	}

	std::shared_ptr<T> getPointer() const { return mPtr; }
};

class GenericClassInstance
{
protected:
	static std::map<const std::type_info*, GenericClassInstance*> mInstanceMap;

	class GenericUpcastFunction
	{
	
	};

	template<typename T>
	class UpcastFunction : public GenericUpcastFunction
	{
		std::function<std::shared_ptr<T>(v8::Handle<v8::Object>)> mCaster;
	public:
		UpcastFunction(std::function<std::shared_ptr<T>(v8::Handle<v8::Object>)> caster) {
			mCaster = caster;
		}

		std::shared_ptr<T> upcast(v8::Handle<v8::Object> obj) { return mCaster(obj); }
	};

	std::map<const std::type_info*, GenericUpcastFunction*> mBases;

public:
	template<typename T>
	bool canUpcast() {
		const std::type_info& typeInfo = typeid(T);
		return mBases.find(&typeInfo) != mBases.end();
	}

	template<typename T>
	std::shared_ptr<T> upcast(v8::Handle<v8::Object> obj) {
		const std::type_info& typeInfo = typeid(T);
		auto itr = mBases.find(&typeInfo);
		if (itr == mBases.end()) {
			return nullptr;
		}

		return ((UpcastFunction<T>*)itr->second)->upcast(obj);
	}

	virtual v8::Handle<v8::Value> createInstance(GenericSharedPtrWrap& ptr) = 0;

	template<typename T>
	static GenericClassInstance* getTypeInstance() {
		const std::type_info* ti = &typeid(T);
		auto itr = mInstanceMap.find(ti);
		if (itr == mInstanceMap.end()) {
			return nullptr;
		}

		return itr->second;
	}
};

template<typename T>
class ClassInstance : public GenericClassInstance
{
	class PropertyData
	{
		ClassInstance<T>* mClassInstance;
		uint32 mClassHash;
		std::wstring mName;
	public:
		PropertyData(ClassInstance<T>* inst, uint32 hash, const std::wstring& str) {
			mClassHash = hash;
			mClassInstance = inst;
			mName = str;
		}

		uint32 getHash() const { return mClassHash; }
		ClassInstance<T>* getInstance() const { return mClassInstance; }
		std::wstring getName() const { return mName; }
	};

	class GenericProperty
	{

	};

	template<typename Prop>
	class Property : public GenericProperty
	{
		std::function<Prop(std::shared_ptr<T>)> mGetter;
		std::function<void(std::shared_ptr<T>, Prop)> mSetter;
	public:
		Property(std::function<Prop(std::shared_ptr<T>)> getter) {
			mGetter = getter;
		}

		Property(std::function<Prop(std::shared_ptr<T>)> getter, std::function<void(std::shared_ptr<T>, Prop)> setter) {
			mGetter = getter;
			mSetter = setter;
		}

		void setSetter(std::function<void(std::shared_ptr<T>, Prop)> setter) {
			mSetter = setter;
		}

		Prop getValue(std::shared_ptr<T> t) {
			return mGetter(t);
		}

		void setValue(std::shared_ptr<T> t, Prop value) {
			if (mSetter) {
				mSetter(t, value);
			} else {
				v8::ThrowException(v8::Exception::TypeError(v8::String::New("Property is readonly")));
				return;
			}
		}
	};

	std::unique_ptr<FunctionInstance> mConstructor;
	v8::Handle<v8::ObjectTemplate> mPrototype;
	v8::Handle<v8::FunctionTemplate> mTemplate;
	std::map<std::wstring, std::unique_ptr<FunctionInstance>> mMethods;
	std::map<std::wstring, std::unique_ptr<FunctionInstance>> mStaticMethods;
	std::map<std::wstring, std::shared_ptr<GenericProperty>> mProperties;

	template<typename Prop>
	static void __cdecl __propertyGetCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& value) {
		if (value.Data()->IsExternal() == false) {
			value.GetReturnValue().Set(v8::Undefined());
			return;
		}

		auto ext = value.Data().As<v8::External>();
		PropertyData* data = (PropertyData*) ext->Value();

		if (data->getHash() != typeid(T).hash_code()) {
			value.GetReturnValue().Set(v8::Undefined());
			return;
		}

		auto inst = data->getInstance();
		auto str = data->getName();

		auto itr = inst->mProperties.find(str);
		if (itr == inst->mProperties.end()) {
			value.GetReturnValue().Set(v8::Undefined());
			return;
		}

		std::shared_ptr<Property<Prop>> prop = std::static_pointer_cast<Property<Prop>>(itr->second);
		value.GetReturnValue().Set(ObjectWrap::wrap<Prop>(prop->getValue(ObjectWrap::unwrap<std::shared_ptr<T>>(value.This()))));
	}

	template<typename Prop>
	static void __cdecl __propertySetCallback(v8::Local<v8::String> name, v8::Local<v8::Value> propValue, const v8::PropertyCallbackInfo<void>& value) {
		if (value.Data()->IsExternal() == false) {
			return;
		}

		auto ext = value.Data().As<v8::External>();
		PropertyData* data = (PropertyData*) ext->Value();

		if (data->getHash() != typeid(T).hash_code()) {
			return;
		}

		auto inst = data->getInstance();
		auto str = data->getName();

		auto itr = inst->mProperties.find(str);
		if (itr == inst->mProperties.end()) {
			return;
		}

		std::shared_ptr<Property<Prop>> prop = std::static_pointer_cast<Property<Prop>>(itr->second);
		prop->setValue(ObjectWrap::unwrap<std::shared_ptr<T>>(value.This()), ObjectWrap::unwrap<Prop>(propValue));
	}

	static void __cdecl __gcCallback(v8::Isolate* isolate, v8::Persistent<v8::Object>* pers, std::shared_ptr<T>* inst) {
		delete inst;
		pers->Dispose();
	}

	v8::Handle<v8::Value> createObjectInstance(std::shared_ptr<T> inst) {
		std::shared_ptr<T>* t = new std::shared_ptr<T>(inst);
		
		v8::TryCatch tc;
		auto obj = mPrototype->NewInstance();
		if (tc.HasCaught()) {
			delete t;
			tc.ReThrow();
			return v8::Undefined();
		}

		obj->SetInternalField(0, v8::Integer::New(typeid(T).hash_code()));
		obj->SetInternalField(1, v8::External::New(t));
		obj->SetInternalField(2, v8::External::New(this));

		v8::Persistent<v8::Object> pers(v8::Isolate::GetCurrent(), obj);
		pers.MakeWeak(t, __gcCallback);

		return obj;
	}

	v8::Handle<v8::Value> createInstance(GenericSharedPtrWrap& ptr) {
		try {
			SharedPtrWrap<T>& spw = dynamic_cast<SharedPtrWrap<T>&>(ptr);
			return createObjectInstance(spw.getPointer());
		} catch (std::bad_cast&) {
			throw;
		}
	}

public:
	ClassInstance(const std::wstring& className) {
		mInstanceMap[&typeid(T)] = this;
		mConstructor = std::unique_ptr<FunctionInstance>(new FunctionInstance());
		mConstructor->addConstraint(
			[](const v8::FunctionCallbackInfo<v8::Value>& info) { 
				if (info.IsConstructCall() == false) {
					v8::ThrowException(v8::Exception::TypeError(v8::String::New("Type cannot be called without new!")));
					return false;
				}

				return true;
			}
		);

		mTemplate = mConstructor->getTemplate();
		mPrototype = mTemplate->PrototypeTemplate();
		mPrototype->SetInternalFieldCount(3);
	}

	template<typename Base>
	void def_base() {
		mBases[&typeid(Base)] = new UpcastFunction<Base>(
			[](v8::Handle<v8::Object> obj) {
				std::shared_ptr<T>* ptr = (std::shared_ptr<T>*) obj->GetInternalField(1).As<v8::External>()->Value();
				return std::static_pointer_cast<Base>(*ptr);
			}
		);
	}

	template<typename... Args>
	void def_constructor(std::function<std::shared_ptr<T> (Args...)> creationFunction) {
		mConstructor->addOverload(std::function<v8::Handle<v8::Value> (Args...)>([this, creationFunction](Args... args) { return createObjectInstance(creationFunction(args...)); }));
	}

	template<typename Ret, typename... Args>
	void def_function(const std::wstring& name, std::function<Ret (std::shared_ptr<T>, Args...)> fun, bool member = true) {
		auto itr = mMethods.find(name);
		if (itr == mMethods.end()) {
			mMethods[name] = std::unique_ptr<FunctionInstance>(new FunctionInstance());
			itr = mMethods.find(name);
			mPrototype->Set(v8::String::New((const uint16_t*) name.c_str(), name.length()), itr->second->getTemplate());
		}

		if (member) {
			itr->second->addMemberOverload(fun);
		} else {
			itr->second->addOverload(fun);
		}
	}

	template<typename Ret, typename... Args>
	void def_function(const std::wstring& name, std::function<Ret(Args...)> fun, bool member = true) {
		auto itr = mStaticMethods.find(name);
		if (itr == mStaticMethods.end()) {
			mStaticMethods[name] = std::unique_ptr<FunctionInstance>(new FunctionInstance());
			itr = mStaticMethods.find(name);
			mTemplate->Set(v8::String::New((const uint16_t*) name.c_str(), name.length()), itr->second->getTemplate());
		}

		if (member) {
			itr->second->addMemberOverload(fun);
		} else {
			itr->second->addOverload(fun);
		}
	}

	template<typename Prop>
	void def_property(const std::wstring& name, Prop (T::*value), bool readonly) {
		std::shared_ptr<Property<Prop>> prop = std::make_shared<Property<Prop>>([value](std::shared_ptr<T> t) { return t.get()->*value; });
		std::shared_ptr<GenericProperty> gprop = std::static_pointer_cast<GenericProperty>(prop);
		mProperties[name] = gprop;

		PropertyData* data = new PropertyData(this, typeid(T).hash_code(), name);

		v8::AccessorSetterCallback setter = nullptr;
		if (readonly == false) {
			prop->setSetter([value](std::shared_ptr<T> t, Prop val) { t.get()->*value = val; });
			setter = &ClassInstance<T>::__propertySetCallback<typename Prop>;
		}

		mPrototype->SetAccessor(v8::String::New((const uint16_t*) name.c_str(), name.length()), &ClassInstance<T>::__propertyGetCallback<typename Prop>, setter, v8::External::New(data));
	}

	template<typename Prop>
	void def_property(const std::wstring& name, std::function<Prop (std::shared_ptr<T>)> getter, std::function<void(std::shared_ptr<T>, Prop)> setter) {
		std::shared_ptr<Property<Prop>> prop = std::make_shared<Property<Prop>>(getter);
		std::shared_ptr<GenericProperty> gprop = std::static_pointer_cast<GenericProperty>(prop);
		mProperties[name] = gprop;

		PropertyData* data = new PropertyData(this, typeid(T).hash_code(), name);

		v8::AccessorSetterCallback _setter = nullptr;
		if (setter) {
			prop->setSetter(setter);
			_setter = &ClassInstance<T>::__propertySetCallback<typename Prop>;
		}

		mPrototype->SetAccessor(v8::String::New((const uint16_t*) name.c_str(), name.length()), &ClassInstance<T>::__propertyGetCallback<typename Prop>, _setter, v8::External::New(data));
	}

	v8::Handle<v8::FunctionTemplate> getTemplate() const { return mTemplate; }
};

#include "ObjectWrap.inl"