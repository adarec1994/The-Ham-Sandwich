#pragma once

#include "Vector3.h"
#include "Matrix.h"

class Program;

typedef std::shared_ptr<Program> ProgramPtr;

class Program : public std::enable_shared_from_this<Program>
{
	struct ProgramMacro
	{
		std::wstring Name;
		std::wstring Value;
		bool NoValue;
	};

	static std::stack<ProgramPtr> gProgramStack;
	GLuint mVertexShader;
	GLuint mPixelShader;
	GLuint mProgram;
	std::map<std::wstring, uint32> mUniformMap;
	std::list<ProgramMacro> mMacros;

	void compileShader(const std::string& data, GLuint shader);
	void compileFromResource(HRSRC rc, GLuint shader);

public:
	Program();

	void define(const std::wstring& name, const std::wstring& value);
	void define(const std::wstring& name);

	void loadVertexShader(const std::wstring& data);
	void loadVertexShader(const std::string& data);
	void loadVertexShader(const std::string& resType, const std::string& resId);
	void loadVertexShader(const std::wstring& resType, const std::wstring& resId);

	void loadPixelShader(const std::wstring& data);
	void loadPixelShader(const std::string& data);
	void loadPixelShader(const std::string& resType, const std::string& resId);
	void loadPixelShader(const std::wstring& resType, const std::wstring& resId);

	void set(const std::wstring& name, const float& value);
	void set(const std::wstring& name, const uint32& value);
	void set(const std::wstring& name, const Vector3& value);
	void set(const std::wstring& name, const Vector4& value);
	void set(const std::wstring& name, const Matrix& value, bool transpose = true);

	void set(uint32 index, const float& value);
	void set(uint32 index, const uint32& value);
	void set(uint32 index, const Vector2& value);
	void set(uint32 index, const Vector3& value);
	void set(uint32 index, const Vector4& value);
	void set(uint32 index, const Matrix& value, bool transpose = true);

	uint32 getUniformIndex(const std::wstring& name);
	bool tryGetUniformIndex(const std::wstring& name, uint32& index);

	void linkProgram();

	void begin();
	void end();

	uint32 getId() const { return mProgram; }
};