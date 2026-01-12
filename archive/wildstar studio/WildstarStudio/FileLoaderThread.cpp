#include "stdafx.h"
#include "FileLoaderThread.h"

using namespace Awesomium;

FileLoaderThread::FileLoaderThread() {
	mIsRunning = true;
	mThread = std::thread(std::bind(&FileLoaderThread::asyncLoadProc, this));
}

FileLoaderThread::~FileLoaderThread() {
	
}

void FileLoaderThread::pushRequest(FileLoadRequest req) {
	std::lock_guard<std::mutex> l(mLoadLock);

	mRequests.push_back(req);
}

void FileLoaderThread::shutdown() {
	mIsRunning = false;
	mThread.join();
}

void FileLoaderThread::asyncLoadProc() {
	while (mIsRunning == true) {
		{
			std::lock_guard<std::mutex> l(mLoadLock);
			for (auto& req : mRequests) {
				std::wstringstream strm;

				wchar_t curDir[MAX_PATH];
				GetCurrentDirectory(MAX_PATH, curDir);
#ifdef _DEBUG
				strm << curDir << L"\\..\\Release\\UI\\" << req.fileName;
#else
				strm << curDir << L"\\UI\\" << req.fileName;
#endif

				std::tr2::sys::path fpath(strm.str());
				std::wstring ext = fpath.extension();
				std::transform(ext.begin(), ext.end(), ext.begin(), tolower);

				WebString mime;
				bool isBinary = false;

				if (ext == L".js") {
					mime = WSLit("text/javascript");
				} else if (ext == L".png") {
					isBinary = true;
					mime = WSLit("image/png");
				} else if (ext == L".css") {
					mime = WSLit("text/css");
				} else if (ext == L".otf") {
					isBinary = true;
					mime = WSLit("font/opentype");
				} else if (ext == L".gif") {
					isBinary = true;
					mime = WSLit("image/gif");
				} else if (ext == L".json") {
					mime = WSLit("text/json");
				} else if (ext == L"ttf") {
					isBinary = true;
					mime = WSLit("application/x-font-ttf");
				} else {
					mime = WSLit("text/html");
				}

				std::ifstream file(strm.str(), isBinary ? std::ios::binary : std::ios::in);
				if (file.is_open() == false) {
					req.callback(0, nullptr, WSLit(""));
					return;
				}

				std::vector<char> content;

				if (isBinary == false) {
					std::stringstream inStrm;
					std::string buffer;
					while (std::getline(file, buffer)) {
						inStrm << buffer << std::endl;
					}

					buffer = inStrm.str();
					content.resize(buffer.length());
					memset(content.data(), 0, content.size());
					memcpy(content.data(), buffer.c_str(), buffer.length());
				} else {
					file.seekg(0, std::ios::end);
					uint32 len = (uint32) file.tellg();
					file.seekg(0, std::ios::beg);
					content.resize(len);
					file.read(content.data(), len);
				}

				req.callback(content.size(), content.data(), mime);
			}

			mRequests.clear();
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(30));
	}
}