// Copyright (C) 2009-2022, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Gr/Vulkan/GrUpscalerImpl.h>
#include <AnKi/Gr/Vulkan/GrManagerImpl.h>
#include <AnKi/Gr/CommandBuffer.h>
#include <AnKi/Gr/Vulkan/CommandBufferImpl.h>

// Ngx specific
#if ANKI_DLSS
#include <ThirdParty/nvngx_dlss_sdk/sdk/include/nvsdk_ngx.h>
#include <ThirdParty/nvngx_dlss_sdk/sdk/include/nvsdk_ngx_helpers.h>
#include <ThirdParty/nvngx_dlss_sdk/sdk/include/nvsdk_ngx_vk.h>
#include <ThirdParty/nvngx_dlss_sdk/sdk/include/nvsdk_ngx_helpers_vk.h>
#endif

namespace anki {

GrUpscalerImpl::~GrUpscalerImpl()
{
	shutdown();
}

Error GrUpscalerImpl::initInternal(const GrUpscalerInitInfo& initInfo)
{
#if ANKI_DLSS
	if(initInfo.m_upscalerType != GrUpscalerType::DLSS_2)
	{
		ANKI_VK_LOGE("Currently only DLSS supported");
		return Error::FUNCTION_FAILED;
	}
	return initAsDLSS(initInfo);
#else
	return Error::FUNCTION_FAILED;
#endif
}

void GrUpscalerImpl::shutdown()
{
#if ANKI_DLSS
	shutdownDLSS();
#endif
}

// NGX related logic
#if ANKI_DLSS

// Use random ID
#	define ANKI_NGX_APP_ID 231313132
#	define ANKI_RW_LOGS_DIR L"./"

// See enum DLSSQualityMode
static NVSDK_NGX_PerfQuality_Value g_NVQUALITYMODES[] = {
	NVSDK_NGX_PerfQuality_Value_MaxPerf, // DISABLED
	NVSDK_NGX_PerfQuality_Value_MaxPerf, NVSDK_NGX_PerfQuality_Value_Balanced, NVSDK_NGX_PerfQuality_Value_MaxQuality};

static inline NVSDK_NGX_PerfQuality_Value getDlssQualityModeToNVQualityMode(GrUpscalerQualityMode mode)
{
	return g_NVQUALITYMODES[static_cast<U32>(mode)];
}

Error GrUpscalerImpl::initAsDLSS(const GrUpscalerInitInfo& initInfo)
{
	NVSDK_NGX_Result result = NVSDK_NGX_Result_Fail;

	const VkDevice vkdevice = getGrManagerImpl().getDevice();
	const VkPhysicalDevice vkphysicaldevice = getGrManagerImpl().getPhysicalDevice();
	const VkInstance vkinstance = getGrManagerImpl().getInstance();
	result = NVSDK_NGX_VULKAN_Init(ANKI_NGX_APP_ID, ANKI_RW_LOGS_DIR, vkinstance, vkphysicaldevice, vkdevice);

	m_ngxInitialized = !NVSDK_NGX_FAILED(result);
	if(!isNgxInitialized())
	{
		if(result == NVSDK_NGX_Result_FAIL_FeatureNotSupported || result == NVSDK_NGX_Result_FAIL_PlatformError)
		{
			ANKI_VK_LOGE("NVIDIA NGX not available on this hardware/platform., code = 0x%08x, info: %ls", result,
						 GetNGXResultAsString(result));
		}
		else
		{
			ANKI_VK_LOGE("Failed to initialize NGX, error code = 0x%08x, info: %ls", result,
						 GetNGXResultAsString(result));
		}

		return Error::FUNCTION_FAILED;
	}

	result = NVSDK_NGX_VULKAN_GetCapabilityParameters(&m_ngxParameters);

	if(NVSDK_NGX_FAILED(result))
	{
		ANKI_VK_LOGE("NVSDK_NGX_GetCapabilityParameters failed, code = 0x%08x, info: %ls", result,
					 GetNGXResultAsString(result));
		return Error::FUNCTION_FAILED;
	}

	// Currently, the SDK and this sample are not in sync.  The sample is a bit forward looking,
	// in this case.  This will likely be resolved very shortly, and therefore, the code below
	// should be thought of as needed for a smooth user experience.
#if defined(NVSDK_NGX_Parameter_SuperSampling_NeedsUpdatedDriver) \
	&& defined(NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMajor) \
	&& defined(NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMinor)

	// If NGX Successfully initialized then it should set those flags in return
	I32 needsUpdatedDriver = 0;
	U32 minDriverVersionMajor = 0;
	U32 minDriverVersionMinor = 0;
	const NVSDK_NGX_Result resultUpdatedDriver =
		m_ngxParameters->Get(NVSDK_NGX_Parameter_SuperSampling_NeedsUpdatedDriver, &needsUpdatedDriver);
	const NVSDK_NGX_Result resultMinDriverVersionMajor =
		m_ngxParameters->Get(NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMajor, &minDriverVersionMajor);
	const NVSDK_NGX_Result resultMinDriverVersionMinor =
		m_ngxParameters->Get(NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMinor, &minDriverVersionMinor);
	if(resultUpdatedDriver == NVSDK_NGX_Result_Success && resultMinDriverVersionMajor == NVSDK_NGX_Result_Success
	   && resultMinDriverVersionMinor == NVSDK_NGX_Result_Success)
	{
		if(needsUpdatedDriver)
		{
			ANKI_VK_LOGE("NVIDIA DLSS cannot be loaded due to outdated driver. Minimum Driver Version required : %u.%u",
						 minDriverVersionMajor, minDriverVersionMinor);

			return Error::FUNCTION_FAILED;
		}
		else
		{
			ANKI_VK_LOGI("NVIDIA DLSS Minimum driver version was reported as : %u.%u", minDriverVersionMajor,
						 minDriverVersionMinor);
		}
	}
	else
	{
		ANKI_VK_LOGI("NVIDIA DLSS Minimum driver version was not reported.");
	}

#endif

	I32 dlssAvailable = 0;
	const NVSDK_NGX_Result resultDLSS =
		m_ngxParameters->Get(NVSDK_NGX_Parameter_SuperSampling_Available, &dlssAvailable);
	if(resultDLSS != NVSDK_NGX_Result_Success || !dlssAvailable)
	{
		// More details about what failed (per feature init result)
		I32 featureInitResult = NVSDK_NGX_Result_Fail;
		NVSDK_NGX_Parameter_GetI(m_ngxParameters, NVSDK_NGX_Parameter_SuperSampling_FeatureInitResult,
								 &featureInitResult);
		ANKI_VK_LOGE("NVIDIA DLSS not available on this hardware/platform., FeatureInitResult = 0x%08x, info: %ls",
					 featureInitResult, GetNGXResultAsString(static_cast<NVSDK_NGX_Result>(featureInitResult)));
		return Error::FUNCTION_FAILED;
	}

	// Create the feature
	if(createDLSSFeature(initInfo.m_sourceImageResolution, initInfo.m_targetImageResolution,
						 initInfo.m_qualityMode)
	   != Error::NONE)
	{
		ANKI_VK_LOGE("Failed to create DLSS feature");
		return Error::FUNCTION_FAILED;
	}

	return Error::NONE;
}

void GrUpscalerImpl::shutdownDLSS()
{
	if(isNgxInitialized())
	{
		// WaitForIdle
		getManager().finish();

		if(m_dlssFeature != nullptr)
		{
			releaseDLSSFeature();
		}
		NVSDK_NGX_VULKAN_DestroyParameters(m_ngxParameters);
		NVSDK_NGX_VULKAN_Shutdown();
		m_ngxInitialized = false;
	}
}

Error GrUpscalerImpl::createDLSSFeature(const UVec2& srcRes, const UVec2& dstRes, const GrUpscalerQualityMode mode)
{
	ANKI_ASSERT(isNgxInitialized());

	const U32 creationNodeMask = 1;
	const U32 visibilityNodeMask = 1;
	// Next create features	(See NVSDK_NGX_DLSS_Feature_Flags in nvsdk_ngx_defs.h)
	const I32 dlssCreateFeatureFlags =
		NVSDK_NGX_DLSS_Feature_Flags_None | NVSDK_NGX_DLSS_Feature_Flags_MVLowRes | NVSDK_NGX_DLSS_Feature_Flags_IsHDR;

	const Error querySettingsResult = queryOptimalSettings(dstRes, mode, m_recommendedSettings);
	if(querySettingsResult != Error::NONE
	   || (srcRes < m_recommendedSettings.m_dynamicMinimumRenderSize
		   || srcRes > m_recommendedSettings.m_dynamicMaximumRenderSize))
	{
		ANKI_VK_LOGE("Selected DLSS mode or render resolution is not supported");
		return Error::FUNCTION_FAILED;
	}

	NVSDK_NGX_DLSS_Create_Params dlssCreateParams;
	memset(&dlssCreateParams, 0, sizeof(dlssCreateParams));
	dlssCreateParams.Feature.InWidth = srcRes.x();
	dlssCreateParams.Feature.InHeight = srcRes.y();
	dlssCreateParams.Feature.InTargetWidth = dstRes.x();
	dlssCreateParams.Feature.InTargetHeight = dstRes.y();
	dlssCreateParams.Feature.InPerfQualityValue =
		getDlssQualityModeToNVQualityMode(m_recommendedSettings.m_qualityMode);
	dlssCreateParams.InFeatureCreateFlags = dlssCreateFeatureFlags;

	// Create the feature with a tmp CmdBuffer
	CommandBufferInitInfo cmdbinit;
	cmdbinit.m_flags = CommandBufferFlag::GENERAL_WORK | CommandBufferFlag::SMALL_BATCH;
	const CommandBufferPtr cmdb = getManager().newCommandBuffer(cmdbinit);
	CommandBufferImpl& cmdbImpl = static_cast<CommandBufferImpl&>(*cmdb);

	cmdbImpl.beginRecordingExt();
	const NVSDK_NGX_Result resultDLSS = NGX_VULKAN_CREATE_DLSS_EXT(cmdbImpl.getHandle(), creationNodeMask, visibilityNodeMask,
															 &m_dlssFeature,
											m_ngxParameters, &dlssCreateParams);
	if(NVSDK_NGX_FAILED(resultDLSS))
	{
		ANKI_VK_LOGE("Failed to create DLSS Features = 0x%08x, info: %ls", resultDLSS,
					 GetNGXResultAsString(resultDLSS));
		return Error::FUNCTION_FAILED;
	}
	cmdb->flush();

	return Error::NONE;
}

void GrUpscalerImpl::releaseDLSSFeature()
{
	ANKI_ASSERT(isNgxInitialized());

	getManager().finish();

	const NVSDK_NGX_Result resultDLSS =
		(m_dlssFeature != nullptr) ? NVSDK_NGX_VULKAN_ReleaseFeature(m_dlssFeature) : NVSDK_NGX_Result_Success;
	if(NVSDK_NGX_FAILED(resultDLSS))
	{
		ANKI_VK_LOGE("Failed to NVSDK_NGX_D3D12_ReleaseFeature, code = 0x%08x, info: %ls", resultDLSS,
					 GetNGXResultAsString(resultDLSS));
	}

	m_dlssFeature = nullptr;
}

static Error queryNgxQualitySettings(const UVec2& displayRes, const NVSDK_NGX_PerfQuality_Value quality,
									 NVSDK_NGX_Parameter& parameters, DLSSRecommendedSettings& outSettings)
{
	const NVSDK_NGX_Result result = NGX_DLSS_GET_OPTIMAL_SETTINGS(
		&parameters, displayRes.x(), displayRes.y(), quality, &outSettings.m_recommendedOptimalRenderSize[0],
		&outSettings.m_recommendedOptimalRenderSize[1], &outSettings.m_dynamicMaximumRenderSize[0],
		&outSettings.m_dynamicMaximumRenderSize[1], &outSettings.m_dynamicMinimumRenderSize[0],
		&outSettings.m_dynamicMinimumRenderSize[1], &outSettings.m_recommendedSharpness);

	if(NVSDK_NGX_FAILED(result))
	{
		outSettings.m_recommendedOptimalRenderSize = displayRes;
		outSettings.m_dynamicMaximumRenderSize = displayRes;
		outSettings.m_dynamicMinimumRenderSize = displayRes;
		outSettings.m_recommendedSharpness = 0.0f;

		ANKI_VK_LOGW("Querying Settings failed! code = 0x%08x, info: %ls", result, GetNGXResultAsString(result));
		return Error::FUNCTION_FAILED;
	}
	return Error::NONE;
}

Error GrUpscalerImpl::queryOptimalSettings(const UVec2& displayRes, const GrUpscalerQualityMode mode,
										   DLSSRecommendedSettings& outRecommendedSettings)
{
	ANKI_ASSERT(mode < GrUpscalerQualityMode::COUNT);

	outRecommendedSettings.m_qualityMode = mode;
	Error err(Error::FUNCTION_FAILED);
	switch(mode)
	{
	case GrUpscalerQualityMode::PERFORMANCE:
		err = queryNgxQualitySettings(displayRes, NVSDK_NGX_PerfQuality_Value_MaxPerf, *m_ngxParameters,
									  outRecommendedSettings);
		break;
	case GrUpscalerQualityMode::BALANCED:
		err = queryNgxQualitySettings(displayRes, NVSDK_NGX_PerfQuality_Value_Balanced, *m_ngxParameters,
									  outRecommendedSettings);
		break;
	case GrUpscalerQualityMode::QUALITY:
		err = queryNgxQualitySettings(displayRes, NVSDK_NGX_PerfQuality_Value_MaxQuality, *m_ngxParameters,
									  outRecommendedSettings);
		break;
	default:
		break;
	}
	return err;
}
#endif // ANKI_DLSS

} // end namespace anki
