#include "UI_Utils.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <codecvt>
#include <locale>

// Added for Texture Loading
#include <glad/glad.h>
#include <stb_image.h>

#ifdef _WIN32
#include <windows.h>
#endif

std::string wstring_to_utf8(const std::wstring& str)
{
    if (str.empty()) return {};
#ifdef _WIN32
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0, NULL, NULL);
    std::string out(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.size(), out.data(), size_needed, NULL, NULL);
    return out;
#else
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.to_bytes(str);
#endif
}

std::string ToLowerCopy(std::string s)
{
    for (auto& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

bool EndsWithNoCase(const std::string& s, const std::string& suffix)
{
    if (s.size() < suffix.size()) return false;
    const size_t off = s.size() - suffix.size();
    for (size_t i = 0; i < suffix.size(); ++i)
    {
        const auto a = static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(s[off + i])));
        const auto b = static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(suffix[i])));
        if (a != b) return false;
    }
    return true;
}

std::string GetExtLower(const std::string& name)
{
    std::filesystem::path p(name);
    std::string e = p.extension().string();
    if (e.empty()) return "";
    return ToLowerCopy(e);
}

bool ContainsLowerFast(const std::string& hayLower, const std::string& needleLower)
{
    if (needleLower.empty()) return true;
    return hayLower.find(needleLower) != std::string::npos;
}

namespace UI_Utils {

    ImTextureID LoadTexture(const char* filename)
    {
        int width, height, nrChannels;
        unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);

        if (!data)
            return 0;

        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        GLenum format = GL_RGB;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);

        return (ImTextureID)(intptr_t)textureID;
    }
}