#pragma once
#include "shaders/ShaderInterop_Renderer.h"

namespace wi::enums
{
	// Common blendmodes used across multiple systems
	enum BLENDMODE
	{
		BLENDMODE_OPAQUE,
		BLENDMODE_ALPHA,
		BLENDMODE_PREMULTIPLIED,
		BLENDMODE_ADDITIVE,
		BLENDMODE_MULTIPLY,
		BLENDMODE_COUNT
	};

	// Do not alter order or value because it is bound to lua manually!
	//	(Previously this was RENDERTYPE enum)
	enum FILTER
	{
		// Include nothing:
		FILTER_NONE = 0,

		// Object filtering types:
		FILTER_OPAQUE = 1 << 0,
		FILTER_TRANSPARENT = 1 << 1,
		FILTER_WATER = 1 << 2,
		FILTER_NAVIGATION_MESH = 1 << 3,
		FILTER_OBJECT_ALL = FILTER_OPAQUE | FILTER_TRANSPARENT | FILTER_WATER | FILTER_NAVIGATION_MESH,

		// Other filtering types:
		FILTER_COLLIDER = 1 << 4,

		// Include everything:
		FILTER_ALL = ~0,

		// These are deprecated namings:
		RENDERTYPE_VOID = 0,
		RENDERTYPE_OPAQUE = FILTER_OPAQUE,
		RENDERTYPE_TRANSPARENT = FILTER_TRANSPARENT,
		RENDERTYPE_WATER = FILTER_WATER,
		RENDERTYPE_ALL = FILTER_ALL
	};

	enum RENDERPASS
	{
		RENDERPASS_MAIN,
		RENDERPASS_PREPASS,
		RENDERPASS_ENVMAPCAPTURE,
		RENDERPASS_SHADOW,
		RENDERPASS_SHADOWCUBE,
		RENDERPASS_VOXELIZE,
		RENDERPASS_COUNT
	};

	// There are two different kinds of stencil refs:
	//	ENGINE	: managed by the engine systems (STENCILREF enum values between 0-15)
	//	USER	: managed by the user (raw numbers between 0-15)
	enum STENCILREF_MASK
	{
		STENCILREF_MASK_ENGINE = 0x0F,
		STENCILREF_MASK_USER = 0xF0,
		STENCILREF_MASK_ALL = STENCILREF_MASK_ENGINE | STENCILREF_MASK_USER,
	};

	// engine stencil reference values. These can be in range of [0, 15].
	// Do not alter order or value because it is bound to lua manually!
	enum STENCILREF
	{
		STENCILREF_EMPTY = 0,
		STENCILREF_DEFAULT = 1,
		STENCILREF_CUSTOMSHADER = 2,
		STENCILREF_OUTLINE = 3,
		STENCILREF_CUSTOMSHADER_OUTLINE = 4,
		STENCILREF_LAST = 15
	};

	// constant buffers
	enum CBTYPES
	{
		CBTYPE_FRAME,
		CBTYPE_COUNT
	};

	// resource buffers (StructuredBuffer, Buffer, etc.)
	enum RBTYPES
	{
		RBTYPE_ENTITYARRAY,
		RBTYPE_MATRIXARRAY,
		RBTYPE_COUNT
	};

	// textures
	enum TEXTYPES
	{
		TEXTYPE_2D_VOLUMETRICCLOUDS_SHADOW,
		TEXTYPE_2D_SKYATMOSPHERE_TRANSMITTANCELUT,
		TEXTYPE_2D_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT,
		TEXTYPE_2D_SKYATMOSPHERE_SKYVIEWLUT,
		TEXTYPE_2D_SKYATMOSPHERE_SKYLUMINANCELUT,
		TEXTYPE_3D_SKYATMOSPHERE_CAMERAVOLUMELUT,
		TEXTYPE_2D_SHEENLUT,
		TEXTYPE_3D_WIND,
		TEXTYPE_COUNT
	};

	// shaders
	enum SHADERTYPE
	{
		// vertex shaders
		VSTYPE_OBJECT_DEBUG,
		VSTYPE_OBJECT_COMMON,
		VSTYPE_OBJECT_SIMPLE,
		VSTYPE_OBJECT_PREPASS,
		VSTYPE_OBJECT_PREPASS_ALPHATEST,
		VSTYPE_OBJECT_COMMON_TESSELLATION,
		VSTYPE_OBJECT_PREPASS_TESSELLATION,
		VSTYPE_OBJECT_PREPASS_ALPHATEST_TESSELLATION,
		VSTYPE_OBJECT_SIMPLE_TESSELLATION,

		VSTYPE_SHADOW,
		VSTYPE_SHADOW_ALPHATEST,
		VSTYPE_SHADOW_TRANSPARENT,
		VSTYPE_SHADOWCUBEMAPRENDER,
		VSTYPE_SHADOWCUBEMAPRENDER_ALPHATEST,
		VSTYPE_SHADOWCUBEMAPRENDER_TRANSPARENT,

		VSTYPE_IMPOSTOR,
		VSTYPE_VERTEXCOLOR,
		VSTYPE_VOLUMETRICLIGHT_DIRECTIONAL,
		VSTYPE_VOLUMETRICLIGHT_POINT,
		VSTYPE_VOLUMETRICLIGHT_SPOT,
		VSTYPE_LIGHTVISUALIZER_SPOTLIGHT,
		VSTYPE_LIGHTVISUALIZER_POINTLIGHT,
		VSTYPE_SKY,
		VSTYPE_ENVMAP,
		VSTYPE_ENVMAP_SKY,
		VSTYPE_SPHERE,
		VSTYPE_OCCLUDEE,
		VSTYPE_VOXELIZER,
		VSTYPE_VOXEL,
		VSTYPE_FORCEFIELDVISUALIZER_POINT,
		VSTYPE_FORCEFIELDVISUALIZER_PLANE,
		VSTYPE_RENDERLIGHTMAP,
		VSTYPE_RAYTRACE_SCREEN,
		VSTYPE_POSTPROCESS,
		VSTYPE_LENSFLARE,
		VSTYPE_DDGI_DEBUG,


		// pixel shaders
		PSTYPE_OBJECT_PERMUTATION_BEGIN,
		PSTYPE_OBJECT_PERMUTATION_END = PSTYPE_OBJECT_PERMUTATION_BEGIN + SHADERTYPE_BIN_COUNT,
		PSTYPE_OBJECT_TRANSPARENT_PERMUTATION_BEGIN,
		PSTYPE_OBJECT_TRANSPARENT_PERMUTATION_END = PSTYPE_OBJECT_TRANSPARENT_PERMUTATION_BEGIN + SHADERTYPE_BIN_COUNT,
		PSTYPE_IMPOSTOR,

		PSTYPE_OBJECT_HOLOGRAM,

		PSTYPE_OBJECT_DEBUG,
		PSTYPE_OBJECT_PAINTRADIUS,
		PSTYPE_OBJECT_SIMPLE,
		PSTYPE_OBJECT_PREPASS,
		PSTYPE_OBJECT_PREPASS_ALPHATEST,
		PSTYPE_IMPOSTOR_PREPASS,
		PSTYPE_IMPOSTOR_SIMPLE,

		PSTYPE_SHADOW_ALPHATEST,
		PSTYPE_SHADOW_TRANSPARENT,
		PSTYPE_SHADOW_WATER,

		PSTYPE_VERTEXCOLOR,
		PSTYPE_LIGHTVISUALIZER,
		PSTYPE_VOLUMETRICLIGHT_DIRECTIONAL,
		PSTYPE_VOLUMETRICLIGHT_POINT,
		PSTYPE_VOLUMETRICLIGHT_SPOT,
		PSTYPE_SKY_STATIC,
		PSTYPE_SKY_DYNAMIC,
		PSTYPE_SUN,
		PSTYPE_ENVMAP,
		PSTYPE_ENVMAP_SKY_STATIC,
		PSTYPE_ENVMAP_SKY_DYNAMIC,
		PSTYPE_CUBEMAP,
		PSTYPE_CAPTUREIMPOSTOR,
		PSTYPE_VOXELIZER,
		PSTYPE_VOXEL,
		PSTYPE_FORCEFIELDVISUALIZER,
		PSTYPE_RENDERLIGHTMAP,
		PSTYPE_RAYTRACE_DEBUGBVH,
		PSTYPE_DOWNSAMPLEDEPTHBUFFER,
		PSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL,
		PSTYPE_POSTPROCESS_OUTLINE,
		PSTYPE_LENSFLARE,
		PSTYPE_DDGI_DEBUG,
		PSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_UPSAMPLE,


		// geometry shaders
		GSTYPE_SHADOWCUBEMAPRENDER_EMULATION,
		GSTYPE_SHADOWCUBEMAPRENDER_ALPHATEST_EMULATION,
		GSTYPE_SHADOWCUBEMAPRENDER_TRANSPARENT_EMULATION,
		GSTYPE_ENVMAP_EMULATION,
		GSTYPE_ENVMAP_SKY_EMULATION,
		GSTYPE_VOXELIZER,
		GSTYPE_VOXEL,


		// hull shaders
		HSTYPE_OBJECT,
		HSTYPE_OBJECT_PREPASS,
		HSTYPE_OBJECT_PREPASS_ALPHATEST,
		HSTYPE_OBJECT_SIMPLE,



		// domain shaders
		DSTYPE_OBJECT,
		DSTYPE_OBJECT_PREPASS,
		DSTYPE_OBJECT_PREPASS_ALPHATEST,
		DSTYPE_OBJECT_SIMPLE,



		// compute shaders
		CSTYPE_LUMINANCE_PASS1,
		CSTYPE_LUMINANCE_PASS2,
		CSTYPE_SHADINGRATECLASSIFICATION,
		CSTYPE_SHADINGRATECLASSIFICATION_DEBUG,
		CSTYPE_TILEFRUSTUMS,
		CSTYPE_LIGHTCULLING,
		CSTYPE_LIGHTCULLING_DEBUG,
		CSTYPE_LIGHTCULLING_ADVANCED,
		CSTYPE_LIGHTCULLING_ADVANCED_DEBUG,
		CSTYPE_RESOLVEMSAADEPTHSTENCIL,
		CSTYPE_VXGI_OFFSETPREV,
		CSTYPE_VXGI_TEMPORAL,
		CSTYPE_VXGI_SDF_JUMPFLOOD,
		CSTYPE_VXGI_RESOLVE_DIFFUSE,
		CSTYPE_VXGI_RESOLVE_SPECULAR,
		CSTYPE_SKYATMOSPHERE_TRANSMITTANCELUT,
		CSTYPE_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT,
		CSTYPE_SKYATMOSPHERE_SKYVIEWLUT,
		CSTYPE_SKYATMOSPHERE_SKYLUMINANCELUT,
		CSTYPE_SKYATMOSPHERE_CAMERAVOLUMELUT,
		CSTYPE_GENERATEMIPCHAIN2D_UNORM4,
		CSTYPE_GENERATEMIPCHAIN2D_FLOAT4,
		CSTYPE_GENERATEMIPCHAIN3D_UNORM4,
		CSTYPE_GENERATEMIPCHAIN3D_FLOAT4,
		CSTYPE_GENERATEMIPCHAINCUBE_UNORM4,
		CSTYPE_GENERATEMIPCHAINCUBE_FLOAT4,
		CSTYPE_GENERATEMIPCHAINCUBEARRAY_UNORM4,
		CSTYPE_GENERATEMIPCHAINCUBEARRAY_FLOAT4,
		CSTYPE_FILTERENVMAP,
		CSTYPE_COPYTEXTURE2D_UNORM4,
		CSTYPE_COPYTEXTURE2D_FLOAT4,
		CSTYPE_COPYTEXTURE2D_UNORM4_BORDEREXPAND,
		CSTYPE_COPYTEXTURE2D_FLOAT4_BORDEREXPAND,
		CSTYPE_SKINNING,
		CSTYPE_RAYTRACE,
		CSTYPE_PAINT_TEXTURE,
		CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_FLOAT1,
		CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_FLOAT3,
		CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_FLOAT4,
		CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_UNORM1,
		CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_UNORM4,
		CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_WIDE_FLOAT1,
		CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_WIDE_FLOAT3,
		CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_WIDE_FLOAT4,
		CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_WIDE_UNORM1,
		CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_WIDE_UNORM4,
		CSTYPE_POSTPROCESS_BLUR_BILATERAL_FLOAT1,
		CSTYPE_POSTPROCESS_BLUR_BILATERAL_FLOAT3,
		CSTYPE_POSTPROCESS_BLUR_BILATERAL_FLOAT4,
		CSTYPE_POSTPROCESS_BLUR_BILATERAL_UNORM1,
		CSTYPE_POSTPROCESS_BLUR_BILATERAL_UNORM4,
		CSTYPE_POSTPROCESS_BLUR_BILATERAL_WIDE_FLOAT1,
		CSTYPE_POSTPROCESS_BLUR_BILATERAL_WIDE_FLOAT3,
		CSTYPE_POSTPROCESS_BLUR_BILATERAL_WIDE_FLOAT4,
		CSTYPE_POSTPROCESS_BLUR_BILATERAL_WIDE_UNORM1,
		CSTYPE_POSTPROCESS_BLUR_BILATERAL_WIDE_UNORM4,
		CSTYPE_POSTPROCESS_SSAO,
		CSTYPE_POSTPROCESS_HBAO,
		CSTYPE_POSTPROCESS_MSAO_PREPAREDEPTHBUFFERS1,
		CSTYPE_POSTPROCESS_MSAO_PREPAREDEPTHBUFFERS2,
		CSTYPE_POSTPROCESS_MSAO_INTERLEAVE,
		CSTYPE_POSTPROCESS_MSAO,
		CSTYPE_POSTPROCESS_MSAO_BLURUPSAMPLE,
		CSTYPE_POSTPROCESS_MSAO_BLURUPSAMPLE_BLENDOUT,
		CSTYPE_POSTPROCESS_MSAO_BLURUPSAMPLE_PREMIN,
		CSTYPE_POSTPROCESS_MSAO_BLURUPSAMPLE_PREMIN_BLENDOUT,
		CSTYPE_POSTPROCESS_RTREFLECTION,
		CSTYPE_POSTPROCESS_RTDIFFUSE,
		CSTYPE_POSTPROCESS_RTDIFFUSE_SPATIAL,
		CSTYPE_POSTPROCESS_RTDIFFUSE_TEMPORAL,
		CSTYPE_POSTPROCESS_RTDIFFUSE_BILATERAL,
		CSTYPE_POSTPROCESS_SSR_TILEMAXROUGHNESS_HORIZONTAL,
		CSTYPE_POSTPROCESS_SSR_TILEMAXROUGHNESS_VERTICAL,
		CSTYPE_POSTPROCESS_SSR_KICKJOBS,
		CSTYPE_POSTPROCESS_SSR_DEPTHHIERARCHY,
		CSTYPE_POSTPROCESS_SSR_RAYTRACE,
		CSTYPE_POSTPROCESS_SSR_RAYTRACE_EARLYEXIT,
		CSTYPE_POSTPROCESS_SSR_RAYTRACE_CHEAP,
		CSTYPE_POSTPROCESS_SSR_RESOLVE,
		CSTYPE_POSTPROCESS_SSR_TEMPORAL,
		CSTYPE_POSTPROCESS_SSR_BILATERAL,
		CSTYPE_POSTPROCESS_LIGHTSHAFTS,
		CSTYPE_POSTPROCESS_DEPTHOFFIELD_TILEMAXCOC_HORIZONTAL,
		CSTYPE_POSTPROCESS_DEPTHOFFIELD_TILEMAXCOC_VERTICAL,
		CSTYPE_POSTPROCESS_DEPTHOFFIELD_NEIGHBORHOODMAXCOC,
		CSTYPE_POSTPROCESS_DEPTHOFFIELD_KICKJOBS,
		CSTYPE_POSTPROCESS_DEPTHOFFIELD_PREPASS,
		CSTYPE_POSTPROCESS_DEPTHOFFIELD_PREPASS_EARLYEXIT,
		CSTYPE_POSTPROCESS_DEPTHOFFIELD_MAIN,
		CSTYPE_POSTPROCESS_DEPTHOFFIELD_MAIN_EARLYEXIT,
		CSTYPE_POSTPROCESS_DEPTHOFFIELD_MAIN_CHEAP,
		CSTYPE_POSTPROCESS_DEPTHOFFIELD_POSTFILTER,
		CSTYPE_POSTPROCESS_DEPTHOFFIELD_UPSAMPLE,
		CSTYPE_POSTPROCESS_MOTIONBLUR_TILEMAXVELOCITY_HORIZONTAL,
		CSTYPE_POSTPROCESS_MOTIONBLUR_TILEMAXVELOCITY_VERTICAL,
		CSTYPE_POSTPROCESS_MOTIONBLUR_NEIGHBORHOODMAXVELOCITY,
		CSTYPE_POSTPROCESS_MOTIONBLUR_KICKJOBS,
		CSTYPE_POSTPROCESS_MOTIONBLUR,
		CSTYPE_POSTPROCESS_MOTIONBLUR_EARLYEXIT,
		CSTYPE_POSTPROCESS_MOTIONBLUR_CHEAP,
		CSTYPE_POSTPROCESS_BLOOMSEPARATE,
		CSTYPE_POSTPROCESS_AERIALPERSPECTIVE,
		CSTYPE_POSTPROCESS_AERIALPERSPECTIVE_CAPTURE,
		CSTYPE_POSTPROCESS_AERIALPERSPECTIVE_CAPTURE_MSAA,
		CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_SHAPENOISE,
		CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_DETAILNOISE,
		CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_CURLNOISE,
		CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_WEATHERMAP,
		CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_RENDER,
		CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_RENDER_CAPTURE,
		CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_RENDER_CAPTURE_MSAA,
		CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_REPROJECT,
		CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_SHADOW_RENDER,
		CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_SHADOW_FILTER,
		CSTYPE_POSTPROCESS_FXAA,
		CSTYPE_POSTPROCESS_TEMPORALAA,
		CSTYPE_POSTPROCESS_SHARPEN,
		CSTYPE_POSTPROCESS_TONEMAP,
		CSTYPE_POSTPROCESS_UNDERWATER,
		CSTYPE_POSTPROCESS_FSR_UPSCALING,
		CSTYPE_POSTPROCESS_FSR_SHARPEN,
		CSTYPE_POSTPROCESS_FSR2_AUTOGEN_REACTIVE_PASS,
		CSTYPE_POSTPROCESS_FSR2_COMPUTE_LUMINANCE_PYRAMID_PASS,
		CSTYPE_POSTPROCESS_FSR2_PREPARE_INPUT_COLOR_PASS,
		CSTYPE_POSTPROCESS_FSR2_RECONSTRUCT_PREVIOUS_DEPTH_PASS,
		CSTYPE_POSTPROCESS_FSR2_DEPTH_CLIP_PASS,
		CSTYPE_POSTPROCESS_FSR2_LOCK_PASS,
		CSTYPE_POSTPROCESS_FSR2_ACCUMULATE_PASS,
		CSTYPE_POSTPROCESS_FSR2_RCAS_PASS,
		CSTYPE_POSTPROCESS_CHROMATIC_ABERRATION,
		CSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL_FLOAT1,
		CSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL_UNORM1,
		CSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL_FLOAT4,
		CSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL_UNORM4,
		CSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL_UINT4,
		CSTYPE_POSTPROCESS_DOWNSAMPLE4X,
		CSTYPE_POSTPROCESS_NORMALSFROMDEPTH,
		CSTYPE_POSTPROCESS_SCREENSPACESHADOW,
		CSTYPE_POSTPROCESS_RTSHADOW,
		CSTYPE_POSTPROCESS_RTSHADOW_DENOISE_TILECLASSIFICATION,
		CSTYPE_POSTPROCESS_RTSHADOW_DENOISE_FILTER,
		CSTYPE_POSTPROCESS_RTSHADOW_DENOISE_TEMPORAL,
		CSTYPE_POSTPROCESS_RTAO,
		CSTYPE_POSTPROCESS_RTAO_DENOISE_TILECLASSIFICATION,
		CSTYPE_POSTPROCESS_RTAO_DENOISE_FILTER,
		CSTYPE_SURFEL_COVERAGE,
		CSTYPE_SURFEL_INDIRECTPREPARE,
		CSTYPE_SURFEL_RAYTRACE,
		CSTYPE_SURFEL_UPDATE,
		CSTYPE_SURFEL_GRIDOFFSETS,
		CSTYPE_SURFEL_BINNING,
		CSTYPE_SURFEL_INTEGRATE,
		CSTYPE_VISIBILITY_RESOLVE,
		CSTYPE_VISIBILITY_RESOLVE_MSAA,
		CSTYPE_VISIBILITY_INDIRECT_PREPARE,
		CSTYPE_VISIBILITY_SKY,
		CSTYPE_VISIBILITY_VELOCITY,
		CSTYPE_VISIBILITY_SURFACE_REDUCED_PERMUTATION_BEGIN,
		CSTYPE_VISIBILITY_SURFACE_REDUCED_PERMUTATION_END = CSTYPE_VISIBILITY_SURFACE_REDUCED_PERMUTATION_BEGIN + SHADERTYPE_BIN_COUNT,
		CSTYPE_VISIBILITY_SURFACE_PERMUTATION_BEGIN,
		CSTYPE_VISIBILITY_SURFACE_PERMUTATION_END = CSTYPE_VISIBILITY_SURFACE_PERMUTATION_BEGIN + SHADERTYPE_BIN_COUNT,
		CSTYPE_VISIBILITY_SHADE_PERMUTATION_BEGIN,
		CSTYPE_VISIBILITY_SHADE_PERMUTATION_END = CSTYPE_VISIBILITY_SHADE_PERMUTATION_BEGIN + SHADERTYPE_BIN_COUNT,
		CSTYPE_DDGI_RAYTRACE,
		CSTYPE_DDGI_UPDATE,
		CSTYPE_DDGI_UPDATE_DEPTH,
		CSTYPE_TERRAIN_VIRTUALTEXTURE_UPDATE_BASECOLORMAP,
		CSTYPE_TERRAIN_VIRTUALTEXTURE_UPDATE_NORMALMAP,
		CSTYPE_TERRAIN_VIRTUALTEXTURE_UPDATE_SURFACEMAP,
		CSTYPE_MESHLET_PREPARE,
		CSTYPE_IMPOSTOR_PREPARE,
		CSTYPE_VIRTUALTEXTURE_TILEREQUESTS,
		CSTYPE_VIRTUALTEXTURE_TILEALLOCATE,
		CSTYPE_VIRTUALTEXTURE_RESIDENCYUPDATE,
		CSTYPE_WIND,


		// raytracing pipelines:
		RTTYPE_RTREFLECTION,


		SHADERTYPE_COUNT,
	};

	// input layouts
	enum ILTYPES
	{
		ILTYPE_OBJECT_DEBUG,
		ILTYPE_RENDERLIGHTMAP,
		ILTYPE_VERTEXCOLOR,
		ILTYPE_POSITION,
		ILTYPE_COUNT
	};
	// rasterizer states
	enum RSTYPES
	{
		RSTYPE_FRONT,
		RSTYPE_BACK,
		RSTYPE_DOUBLESIDED,
		RSTYPE_WIRE,
		RSTYPE_WIRE_SMOOTH,
		RSTYPE_WIRE_DOUBLESIDED,
		RSTYPE_WIRE_DOUBLESIDED_SMOOTH,
		RSTYPE_SHADOW,
		RSTYPE_SHADOW_DOUBLESIDED,
		RSTYPE_OCCLUDEE,
		RSTYPE_VOXELIZE,
		RSTYPE_SKY,
		RSTYPE_COUNT
	};
	// depth-stencil states
	enum DSSTYPES
	{
		DSSTYPE_DEFAULT,
		DSSTYPE_TRANSPARENT,
		DSSTYPE_SHADOW,
		DSSTYPE_DEPTHDISABLED,
		DSSTYPE_DEPTHREAD,
		DSSTYPE_DEPTHREADEQUAL,
		DSSTYPE_ENVMAP,
		DSSTYPE_CAPTUREIMPOSTOR,
		DSSTYPE_WRITEONLY,
		DSSTYPE_HOLOGRAM,
		DSSTYPE_COUNT
	};
	// blend states
	enum BSTYPES
	{
		BSTYPE_OPAQUE,
		BSTYPE_TRANSPARENT,
		BSTYPE_ADDITIVE,
		BSTYPE_PREMULTIPLIED,
		BSTYPE_COLORWRITEDISABLE,
		BSTYPE_MULTIPLY,
		BSTYPE_TRANSPARENTSHADOW,
		BSTYPE_COUNT
	};

	enum SAMPLERTYPES
	{
		// Can be changed by user
		SAMPLER_OBJECTSHADER,

		// Persistent samplers
		// These are bound once and are alive forever
		SAMPLER_LINEAR_CLAMP,
		SAMPLER_LINEAR_WRAP,
		SAMPLER_LINEAR_MIRROR,
		SAMPLER_POINT_CLAMP,
		SAMPLER_POINT_WRAP,
		SAMPLER_POINT_MIRROR,
		SAMPLER_ANISO_CLAMP,
		SAMPLER_ANISO_WRAP,
		SAMPLER_ANISO_MIRROR,
		SAMPLER_CMP_DEPTH,

		SAMPLER_COUNT,
	};

}
