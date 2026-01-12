#pragma once

namespace FileHelper {
	std::wstring getExtension(std::wstring file);
	Awesomium::WebString getExtension(const Awesomium::WebString& file);
}