// Copyright (C) 2009-2018, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <anki/gr/gl/ShaderProgramImpl.h>
#include <anki/gr/Shader.h>
#include <anki/gr/gl/ShaderImpl.h>

namespace anki
{

static void deletePrograms(GLsizei n, const GLuint* progs)
{
	ANKI_ASSERT(n == 1);
	ANKI_ASSERT(progs);
	glDeleteProgram(*progs);
}

ShaderProgramImpl::~ShaderProgramImpl()
{
	destroyDeferred(getManager(), deletePrograms);
	m_refl.m_uniforms.destroy(getAllocator());
}

Error ShaderProgramImpl::initGraphics(ShaderPtr vert, ShaderPtr tessc, ShaderPtr tesse, ShaderPtr geom, ShaderPtr frag)
{
	m_glName = glCreateProgram();
	ANKI_ASSERT(m_glName != 0);

	glAttachShader(m_glName, static_cast<const ShaderImpl&>(*vert).getGlName());
	m_shaders[ShaderType::VERTEX] = vert;

	if(tessc)
	{
		glAttachShader(m_glName, static_cast<const ShaderImpl&>(*tessc).getGlName());
		m_shaders[ShaderType::TESSELLATION_CONTROL] = tessc;
	}

	if(tesse)
	{
		glAttachShader(m_glName, static_cast<const ShaderImpl&>(*tesse).getGlName());
		m_shaders[ShaderType::TESSELLATION_EVALUATION] = tesse;
	}

	if(geom)
	{
		glAttachShader(m_glName, static_cast<const ShaderImpl&>(*geom).getGlName());
		m_shaders[ShaderType::GEOMETRY] = geom;
	}

	glAttachShader(m_glName, static_cast<const ShaderImpl&>(*frag).getGlName());
	m_shaders[ShaderType::FRAGMENT] = frag;

	return link(static_cast<const ShaderImpl&>(*vert).getGlName(), static_cast<const ShaderImpl&>(*frag).getGlName());
}

Error ShaderProgramImpl::initCompute(ShaderPtr comp)
{
	m_glName = glCreateProgram();
	ANKI_ASSERT(m_glName != 0);

	glAttachShader(m_glName, static_cast<const ShaderImpl&>(*comp).getGlName());
	m_shaders[ShaderType::COMPUTE] = comp;

	return link(0, 0);
}

Error ShaderProgramImpl::link(GLuint vert, GLuint frag)
{
	Error err = Error::NONE;

	glLinkProgram(m_glName);
	GLint status = 0;
	glGetProgramiv(m_glName, GL_LINK_STATUS, &status);

	if(!status)
	{
		GLint infoLen = 0;
		GLint charsWritten = 0;
		DynamicArrayAuto<char> infoLogTxt(getAllocator());

		glGetProgramiv(m_glName, GL_INFO_LOG_LENGTH, &infoLen);

		infoLogTxt.create(infoLen + 1);

		glGetProgramInfoLog(m_glName, infoLen, &charsWritten, &infoLogTxt[0]);

		ANKI_GL_LOGE("Link error log follows (vs:%u, fs:%u):\n%s", vert, frag, &infoLogTxt[0]);

		err = Error::USER_DATA;
	}

	return err;
}

const ShaderProgramImplReflection& ShaderProgramImpl::getReflection()
{
	if(m_reflInitialized)
	{
		return m_refl;
	}

	GLint uniformCount = 0;
	glGetProgramiv(getGlName(), GL_ACTIVE_UNIFORMS, &uniformCount);

	if(uniformCount)
	{
		for(U i = 0; i < U(uniformCount); ++i)
		{
			// Get uniform info
			GLsizei len;
			GLenum type;
			GLint size;
			Array<char, 128> name;
			glGetActiveUniform(getGlName(), i, sizeof(name), &len, &size, &type, &name[0]);

			name[len] = '\0';
			GLint location = glGetUniformLocation(getGlName(), &name[0]);

			// Store those info
			ShaderVariableDataType akType = ShaderVariableDataType::NONE;
			U32 dataSize = 0;
			switch(type)
			{
			case GL_FLOAT_VEC4:
				akType = ShaderVariableDataType::VEC4;
				dataSize = 16;
				break;
			case GL_INT_VEC4:
				akType = ShaderVariableDataType::IVEC4;
				dataSize = 16;
				break;
			case GL_UNSIGNED_INT_VEC4:
				akType = ShaderVariableDataType::UVEC4;
				dataSize = 16;
				break;
			case GL_FLOAT_MAT4:
				akType = ShaderVariableDataType::MAT4;
				dataSize = 16 * 4;
				break;
			default:
				ANKI_ASSERT(!"Unsupported type");
			}

			ShaderProgramImplReflection::Uniform uni;
			uni.m_location = location;
			uni.m_type = akType;
			uni.m_arrSize = size;

			m_refl.m_uniforms.emplaceBack(getAllocator(), uni);
			m_refl.m_uniformDataSize += dataSize * uni.m_arrSize;
		}
	}

	m_reflInitialized = true;
	return m_refl;
}

} // end namespace anki
