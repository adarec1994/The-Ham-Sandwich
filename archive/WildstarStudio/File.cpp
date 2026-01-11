#include "StdAfx.h"
#include "File.h"

using namespace Awesomium;

namespace FileHelper {
	std::wstring getExtension(std::wstring file) {
		std::tr2::sys::path p(file);

		return p.extension().wstring().substr(1);
	}

	WebString getExtension(const WebString& file) {
		std::wstring fname(reinterpret_cast<const wchar_t*>(file.data()), file.length());
		return WebString((const wchar16*)FileHelper::getExtension(fname).c_str());
	}
}