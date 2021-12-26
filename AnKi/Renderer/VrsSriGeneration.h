// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Renderer/RendererObject.h>

namespace anki {

/// @addtogroup renderer
/// @{

/// Computes the shading rate image to be used by a number of passes.
class VrsSriGeneration : public RendererObject
{
public:
	VrsSriGeneration(Renderer* r);
	~VrsSriGeneration();

	ANKI_USE_RESULT Error init();

	/// Populate the rendergraph.
	void populateRenderGraph(RenderingContext& ctx);

	RenderTargetHandle getSriRt() const
	{
		return m_runCtx.m_rt;
	}

	U32 getSriTexelDimension() const
	{
		return m_sriTexelDimension;
	}

public:
	ShaderProgramResourcePtr m_prog;
	ShaderProgramPtr m_grProg;

	ShaderProgramResourcePtr m_visualizeProg;
	ShaderProgramPtr m_visualizeGrProg;

	RenderTargetDescription m_rtDescr;
	FramebufferDescription m_fbDescr;

	static constexpr U32 m_sriTexelDimension = 16;

	class
	{
	public:
		RenderTargetHandle m_rt;
	} m_runCtx;

	ANKI_USE_RESULT Error initInternal();

	void getDebugRenderTarget(CString rtName, RenderTargetHandle& handle,
							  ShaderProgramPtr& optionalShaderProgram) const override;
};
/// @}

} // end namespace anki
