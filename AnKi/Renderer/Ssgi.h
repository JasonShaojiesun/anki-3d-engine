// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Renderer/RendererObject.h>
#include <AnKi/Resource/ImageResource.h>
#include <AnKi/Gr.h>

namespace anki
{

/// @addtogroup renderer
/// @{

/// Screen space global illumination.
class Ssgi : public RendererObject
{
public:
	Ssgi(Renderer* r)
		: RendererObject(r)
	{
		registerDebugRenderTarget("SSGI");
	}

	~Ssgi();

	ANKI_USE_RESULT Error init(const ConfigSet& cfg);

	/// Populate the rendergraph.
	void populateRenderGraph(RenderingContext& ctx);

	RenderTargetHandle getRt() const
	{
		return m_runCtx.m_finalRt;
	}

	void getDebugRenderTarget(CString rtName, RenderTargetHandle& handle,
							  ShaderProgramPtr& optionalShaderProgram) const override
	{
		ANKI_ASSERT(rtName == "SSGI");
		handle = m_runCtx.m_finalRt;
	}

private:
	class
	{
	public:
		ShaderProgramResourcePtr m_prog;
		Array<ShaderProgramPtr, 4> m_grProg;
		RenderTargetDescription m_rtDescr;
		ImageResourcePtr m_noiseImage;
		U32 m_maxSteps = 32;
		U32 m_firstStepPixels = 16;
		U32 m_depthLod = 0;
	} m_main;

	class
	{
	public:
		ShaderProgramResourcePtr m_prog;
		Array2d<ShaderProgramPtr, 2, 4> m_grProg;
	} m_denoise;

	class
	{
	public:
		TexturePtr m_rt;
		ShaderProgramResourcePtr m_prog;
		Array<ShaderProgramPtr, 4> m_grProg;
		Bool m_rtImportedOnce = false;
	} m_recontruction;

	class
	{
	public:
		Array<RenderTargetHandle, 2> m_intermediateRts;
		RenderTargetHandle m_finalRt;
		RenderingContext* m_ctx = nullptr;
	} m_runCtx;

	ANKI_USE_RESULT Error initInternal(const ConfigSet& cfg);

	void run(const RenderingContext& ctx, RenderPassWorkContext& rgraphCtx);
	void runVBlur(RenderPassWorkContext& rgraphCtx);
	void runHBlur(RenderPassWorkContext& rgraphCtx);
	void runRecontruct(RenderPassWorkContext& rgraphCtx);
};
/// @}

} // namespace anki
