#pragma once

class FileLoadRequest
{
public:
	std::wstring fileName;
	std::function<void (uint32, void*, Awesomium::WebString)> callback;
};

class FileLoaderThread
{
	std::thread mThread;
	std::mutex mLoadLock;
	volatile bool mIsRunning;

	std::list<FileLoadRequest> mRequests;

	void asyncLoadProc();
public:
	FileLoaderThread();
	~FileLoaderThread();

	void pushRequest(FileLoadRequest req);

	void shutdown();
};