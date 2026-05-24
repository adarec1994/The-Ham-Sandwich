#include "DebugLog.h"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <thread>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace
{
    std::mutex gLogMutex;
    bool gInitialized = false;

    std::string Timestamp()
    {
        const auto now = std::chrono::system_clock::now();
        const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
        const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::tm localTime{};
        localtime_s(&localTime, &nowTime);

        std::ostringstream ss;
        ss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S")
           << "." << std::setw(3) << std::setfill('0') << millis.count();
        return ss.str();
    }

    void WriteLineUnlocked(const std::string& category, const std::string& message)
    {
        std::ofstream out("ham_loader_debug.log", std::ios::app);
        if (!out.is_open()) return;

        out << "[" << Timestamp() << "]"
            << "[tid " << std::this_thread::get_id() << "]"
            << "[" << category << "] "
            << message << "\n";
    }

    LONG WINAPI HandleUnhandledException(EXCEPTION_POINTERS* info)
    {
        std::lock_guard<std::mutex> lock(gLogMutex);

        std::ostringstream ss;
        ss << "Unhandled SEH exception";
        if (info && info->ExceptionRecord)
        {
            ss << " code=0x" << std::hex << info->ExceptionRecord->ExceptionCode
               << " address=0x" << reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress);
        }

        WriteLineUnlocked("Crash", ss.str());
        return EXCEPTION_EXECUTE_HANDLER;
    }

    void HandleTerminate()
    {
        try
        {
            auto ex = std::current_exception();
            if (ex)
                std::rethrow_exception(ex);

            DebugLog::Write("Crash", "std::terminate called with no active exception");
        }
        catch (const std::exception& e)
        {
            DebugLog::WriteException("Crash", e);
        }
        catch (...)
        {
            DebugLog::WriteUnknownException("Crash");
        }

        std::abort();
    }
}

namespace DebugLog
{
    void Init()
    {
        {
            std::lock_guard<std::mutex> lock(gLogMutex);
            if (gInitialized) return;
            gInitialized = true;

            std::ofstream out("ham_loader_debug.log", std::ios::app);
            if (out.is_open())
            {
                out << "\n========== session start " << Timestamp() << " ==========\n";
            }
        }

        SetUnhandledExceptionFilter(HandleUnhandledException);
        std::set_terminate(HandleTerminate);
    }

    void Write(const std::string& category, const std::string& message)
    {
        std::lock_guard<std::mutex> lock(gLogMutex);
        WriteLineUnlocked(category, message);
    }

    void WriteException(const std::string& category, const std::exception& e)
    {
        Write(category, std::string("exception: ") + e.what());
    }

    void WriteUnknownException(const std::string& category)
    {
        Write(category, "unknown exception");
    }
}
