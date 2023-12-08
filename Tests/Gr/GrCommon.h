// Copyright (C) 2009-2023, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Gr.h>
#include <AnKi/ShaderCompiler.h>
#include <AnKi/ShaderCompiler/ShaderProgramParser.h>
#include <AnKi/ShaderCompiler/Dxc.h>
#include <Tests/Framework/Framework.h>

namespace anki {

inline ShaderPtr createShader(CString src, ShaderType type, GrManager& gr, ConstWeakArray<ShaderSpecializationConstValue> specVals = {})
{
	ShaderCompilerString header;
	ShaderCompilerOptions compilerOptions;
	ShaderProgramParser::generateAnkiShaderHeader(type, compilerOptions, header);
	header += src;
	ShaderCompilerDynamicArray<U8> spirv;
	ShaderCompilerString errorLog;

	const Error err = compileHlslToSpirv(header, type, false, spirv, errorLog);
	if(err)
	{
		ANKI_TEST_LOGE("Compile error:\n%s", errorLog.cstr());
	}
	ANKI_TEST_EXPECT_NO_ERR(err);

	ShaderInitInfo initInf(type, spirv);
	initInf.m_constValues = specVals;

	return gr.newShader(initInf);
}

} // end namespace anki
