#include "stdafx.h"
#include "Common.h"
#include "Class.h"
#include "UIManager.h"
#include "Object.h"
#include "Value.h"
#include "Array.h"
#include "ObjectWrap.inl"
#include "TypedArray.h"

class Console
{
	static void makeIdent(std::wstringstream& strm, uint32 numIdent) {
		for (uint32 i = 0; i < numIdent; ++i) {
			strm << L"\t";
		}
	}

	static std::wstring toString(ValuePtr val, uint32 ident) {
		std::wstringstream msgStrm;

		if (val->is<std::wstring>()) {
			makeIdent(msgStrm, ident);
			msgStrm << val->as<std::wstring>();
		} else if (val->is<int64>()) {
			uint64 value = val->as<int64>();
			makeIdent(msgStrm, ident);
			msgStrm << value;
		} else if (val->is<double>()) {
			double value = val->as<double>();
			makeIdent(msgStrm, ident);
			msgStrm << value;
		} else if (val->is<ArrayPtr>()) {
			makeIdent(msgStrm, ident);
			msgStrm << L"[ ";
			ArrayPtr arr = val->as<ArrayPtr>();
			for (uint32 i = 0; i < arr->length(); ++i) {
				if (i != 0) {
					msgStrm << L", ";
				}

				msgStrm << toString(arr->get<ValuePtr>(i), 0);
			}
		} else if (val->is<TypedArrayPtr>()) {
			makeIdent(msgStrm, ident);
			msgStrm << L"[TypedArray arr]";
		} else if (val->is<ObjectPtr>()) {
			msgStrm << L" { " << std::endl;
			std::list<std::wstring> props;
			auto obj = val->as<ObjectPtr>();
			obj->enumProperties(props);
			uint32 i = 0;
			for (auto& prop : props) {
				makeIdent(msgStrm, ident + 1);
				msgStrm << prop << L": ";
				msgStrm << toString(obj->get<ValuePtr>(prop), ident + 1);
				if (i != props.size() - 1) {
					msgStrm << L",";
				}
				msgStrm << std::endl;
				++i;
			}
			makeIdent(msgStrm, ident);
			msgStrm << L"} ";
		}

		return msgStrm.str();
	}
public:
	static void log(ValuePtr val) {
		std::wstring desc = toString(val, 0);
		std::wstringstream strm;
		Awesomium::JSArray args = Awesomium::JSArray(1);
		args.Insert(Awesomium::JSValue(Awesomium::WebString((const wchar16*) desc.c_str(), desc.length())), 0);
		auto strEsc = args[0].ToString();
		std::wstring str((const wchar_t*) strEsc.data(), strEsc.length());
		strm << L"logConsoleMessage('";
		
		for (auto& itr : str) {
			switch (itr) {
			case '\'':
				strm << L"\\\'";
				break;

			case '\n':
				strm << L"<br />";
				break;

			case '\\':
				strm << L"\\\\";
				break;

			default:
				strm << itr;
			}
		}

		strm << L"');";
		sUIMgr->executeJavascript(strm.str());
	}
};

void Common::onRegister(Scope& scope) {
	scope
	[
		Class<Console>(L"console")
			.static_function(L"log", &Console::log)
	];
}