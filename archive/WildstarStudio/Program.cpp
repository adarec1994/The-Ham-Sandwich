#include "stdafx.h"
#include "Program.h"

std::stack<ProgramPtr> Program::gProgramStack;

Program::Program() {
	mVertexShader = glCreateShader(GL_VERTEX_SHADER);
	mPixelShader = glCreateShader(GL_FRAGMENT_SHADER);
	mProgram = glCreateProgram();
}

void Program::define(const std::wstring& name) {
	ProgramMacro macro = {
		name,
		L"",
		true
	};

	mMacros.push_back(macro);
}

void Program::define(const std::wstring& name, const std::wstring& value) {
	ProgramMacro macro = {
		name,
		value,
		false
	};

	mMacros.push_back(macro);
}

void Program::compileShader(const std::string& data, GLuint shader) {
	std::wstringstream shaderStrm;
	shaderStrm << L"#version 330\r\n";

	if (mMacros.size() > 0) {
		for (auto& macro : mMacros) {
			shaderStrm << L"#define " << macro.Name;
			if (macro.NoValue == false)
				shaderStrm << L" " << macro.Value;

			shaderStrm << L"\r\n";
		}
	}

	shaderStrm << L"\r\n";
	shaderStrm << String::toUnicode(data);

	std::string shaderData = String::toAnsi(shaderStrm.str());
	const char* str = shaderData.c_str();

	glShaderSource(shader, 1, &str, nullptr);
	glCompileShader(shader);
	int32 status = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		int32 infoLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen <= 0)
			throw std::exception("Unable to compile shader, no error information provided");

		std::vector<char> error(infoLen);
		glGetShaderInfoLog(shader, infoLen, nullptr, error.data());
		std::stringstream strm;
		strm << "Error compiling shader, info log:" << std::endl << error.data();
		throw std::exception(strm.str().c_str());
	}
}

void Program::compileFromResource(HRSRC rc, GLuint shader) {
	if (rc == nullptr)
		throw std::exception("Unable to find resource");

	HGLOBAL glob = LoadResource(GetModuleHandle(nullptr), rc);
	uint32 size = SizeofResource(GetModuleHandle(nullptr), rc);

	const char* code = (const char*) LockResource(glob);
	std::string shaderCode(code, code + size);
	compileShader(shaderCode, shader);
}

void Program::loadVertexShader(const std::string& data) {
	compileShader(data, mVertexShader);
}

void Program::loadVertexShader(const std::wstring& data) {
	loadVertexShader(String::toAnsi(data));
}

void Program::loadVertexShader(const std::string& resType, const std::string& resId) {
	HRSRC rc = FindResourceA(GetModuleHandle(nullptr), resId.c_str(), resType.c_str());
	compileFromResource(rc, mVertexShader);
}

void Program::loadVertexShader(const std::wstring& resType, const std::wstring& resId) {
	HRSRC rc = FindResourceW(GetModuleHandle(nullptr), resId.c_str(), resType.c_str());
	compileFromResource(rc, mVertexShader);
}

void Program::loadPixelShader(const std::string& data) {
	compileShader(data, mPixelShader);
}

void Program::loadPixelShader(const std::wstring& data) {
	loadPixelShader(String::toAnsi(data));
}

void Program::loadPixelShader(const std::string& resType, const std::string& resId) {
	HRSRC rc = FindResourceA(GetModuleHandle(nullptr), resId.c_str(), resType.c_str());
	compileFromResource(rc, mPixelShader);
}

void Program::loadPixelShader(const std::wstring& resType, const std::wstring& resId) {
	HRSRC rc = FindResourceW(GetModuleHandle(nullptr), resId.c_str(), resType.c_str());
	compileFromResource(rc, mPixelShader);
}

void Program::linkProgram() {
	glAttachShader(mProgram, mVertexShader);
	glAttachShader(mProgram, mPixelShader);
	glLinkProgram(mProgram);

	int32 status = 0;
	glGetProgramiv(mProgram, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		int32 infoLen = 0;
		glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen <= 0)
			throw std::exception("Unable to compile shader, no error information provided");

		std::vector<char> error(infoLen);
		glGetProgramInfoLog(mProgram, infoLen, nullptr, error.data());
		std::stringstream strm;
		strm << "Error linking program, info log:" << std::endl << error.data();
		throw std::exception(strm.str().c_str());
	}
}

void Program::begin() {
	glUseProgram(mProgram);
	gProgramStack.push(shared_from_this());
}

void Program::end() {
	if (gProgramStack.size() == 0)
		throw std::exception("gProgramStack.size() == 0");

	gProgramStack.pop();
	if (gProgramStack.size() == 0)
		glUseProgram(0);
	else
		glUseProgram(gProgramStack.top()->getId());
}

uint32 Program::getUniformIndex(const std::wstring& name) {
	auto itr = mUniformMap.find(name);
	if (itr != mUniformMap.end())
		return itr->second;

	GLint ret = glGetUniformLocation(mProgram, String::toAnsi(name).c_str());
	if (ret == -1)
		throw std::exception("glGetUniformLocation == -1");

	mUniformMap[name] = static_cast<uint32>(ret);
	return static_cast<uint32>(ret);
}

bool Program::tryGetUniformIndex(const std::wstring& name, uint32& index) {
	auto itr = mUniformMap.find(name);
	if (itr != mUniformMap.end()) {
		index = itr->second;
		return true;
	}

	GLint ret = glGetUniformLocation(mProgram, String::toAnsi(name).c_str());
	if (ret == -1)
		return false;

	index = static_cast<uint32>(ret);
	return true;
}

void Program::set(const std::wstring& name, const float& value) {
	set(getUniformIndex(name), value);
}

void Program::set(const std::wstring& name, const uint32& value) {
	set(getUniformIndex(name), value);
}

void Program::set(const std::wstring& name, const Vector3& value) {
	set(getUniformIndex(name), value);
}

void Program::set(const std::wstring& name, const Matrix& value, bool transpose) {
	set(getUniformIndex(name), value, transpose);
}

void Program::set(uint32 index, const float& value) {
	if (index > 0x7FFFFFFF)
		throw std::exception("index > 0x7FFFFFFF");

	glUniform1f(index, value);
}

void Program::set(uint32 index, const uint32& value) {
	if (index > 0x7FFFFFFF)
		throw std::exception("index > 0x7FFFFFFF");

	glUniform1i(index, value);
}

void Program::set(uint32 index, const Vector4& value) {
	if (index > 0x7FFFFFFF)
		throw std::exception("index > 0x7FFFFFFF");

	glUniform4fv(index, 1, &value.X);
}

void Program::set(uint32 index, const Vector3& value) {
	if (index > 0x7FFFFFFF)
		throw std::exception("index > 0x7FFFFFFF");

	glUniform3fv(index, 1, value);
}

void Program::set(uint32 index, const Vector2& value) {
	if (index > 0x7FFFFFFF)
		throw std::exception("index > 0x7FFFFFFF");

	glUniform2fv(index, 1, value);
}

void Program::set(uint32 index, const Matrix& matrix, bool transpose) {
	if (index > 0x7FFFFFFF)
		throw std::exception("index > 0x7FFFFFFF");

	glUniformMatrix4fv(index, 1, transpose, matrix);
}