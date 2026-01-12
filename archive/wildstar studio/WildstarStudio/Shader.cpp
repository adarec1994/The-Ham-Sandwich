#include "stdafx.h"
#include "Shader.h"
#include <d3d/d3dcompiler.h>
#include "String.h"

pD3DDisassemble fD3DDisassemble;

Shader::Shader(FileEntryPtr file) : mFile(file) {
	mFile = file;
	sIOMgr->getArchive()->getFileData(file, mContent);
	mStream = std::make_shared<BinStream>(mContent);
}

bool Shader::load() {
	if (fD3DDisassemble == nullptr) {
		HMODULE hDll = LoadLibrary(L"d3dcompiler_43.dll");
		if (hDll == nullptr) {
			return false;
		}

		fD3DDisassemble = (pD3DDisassemble) GetProcAddress(hDll, "D3DDisassemble");
		if (fD3DDisassemble == nullptr) {
			return false;
		}
	}

	mHeader = mStream->read<ShaderHeader>();
	if (mHeader.signature != 0x54475453) {
		mPermutations.push_back(L"Please open the files with the suffix *.ps_3_0.sho/*.ps_4_0.sho/*.vs_3_0.sho/*.vs_4_0.sho and not the root *.sho.");
		return true;
	}

	if (mHeader.numPermutations == 0) {
		return true;
	}

	for (uint32 i = 0; i < mHeader.numPermutations; ++i) {
		mStream->seek(0x50 + mHeader.ofsPermutations + i * sizeof(ShaderPermute));
		ShaderPermute perm = mStream->read<ShaderPermute>();
		if (perm.numBytes > 0) {
			mStream->seek(0x50 + mHeader.ofsPermutations + mHeader.numPermutations * sizeof(ShaderPermute) + perm.offset);
			std::vector<uint8> content(static_cast<std::size_t>(perm.numBytes));
			mStream->read(content.data(), perm.numBytes);
			ID3DBlob* pBlob = nullptr;
			auto res = fD3DDisassemble(content.data(), content.size(), 0, nullptr, &pBlob);
			if (SUCCEEDED(res)) {
				std::vector<char> textData(pBlob->GetBufferSize());
				memcpy(textData.data(), pBlob->GetBufferPointer(), textData.size());
				textData.push_back('\0');
				mPermutations.push_back(String::toUnicode(textData.data()));
				pBlob->Release();
			}
		}
	}

	return true;
}