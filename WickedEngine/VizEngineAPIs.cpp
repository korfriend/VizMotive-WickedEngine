//#pragma warning(disable:4819)
#include "VizEngineAPIs.h" 
#include "WickedEngine.h"

#include "wiGraphicsDevice_DX12.h"
#include "wiGraphicsDevice_Vulkan.h"

#include "../Editor/ModelImporter.h"

#include <iostream>

using namespace wi::ecs;
using namespace wi::scene;

#define CANVAS_INIT_W 16u
#define CANVAS_INIT_H 16u
#define CANVAS_INIT_DPI 96.f

#pragma region // global enumerations
enum class FileType
{
	INVALID,
	OBJ,
	GLTF,
	GLB,
	VRM,
	IMAGE,
	VIDEO,
	SOUND,
};

static wi::unordered_map<std::string, FileType> filetypes = {
	{"OBJ", FileType::OBJ},
	{"GLTF", FileType::GLTF},
	{"GLB", FileType::GLB},
	{"VRM", FileType::VRM},
};

static wi::unordered_map<std::string, vzm::COMPONENT_TYPE> vmcomptypes = {
	{typeid(vzm::VmBaseComponent).name(), vzm::COMPONENT_TYPE::BASE},
	{typeid(vzm::VmAnimation).name(), vzm::COMPONENT_TYPE::ANIMATION},
	{typeid(vzm::VmActor).name(), vzm::COMPONENT_TYPE::ACTOR},
	{typeid(vzm::VmMesh).name(), vzm::COMPONENT_TYPE::GEOMETRY},
	{typeid(vzm::VmMaterial).name(), vzm::COMPONENT_TYPE::MATERIAL},
	{typeid(vzm::VmLight).name(), vzm::COMPONENT_TYPE::LIGHT},
	{typeid(vzm::VmEmitter).name(), vzm::COMPONENT_TYPE::EMITTER},
	{typeid(vzm::VmWeather).name(), vzm::COMPONENT_TYPE::WEATHER},
	{typeid(vzm::VmCamera).name(), vzm::COMPONENT_TYPE::CAMERA},
	{typeid(vzm::VmCollider).name(), vzm::COMPONENT_TYPE::COLLIDER},
};

enum class CompType
{
	INVALID,
	NameComponent_,
	LayerComponent_,
	TransformComponent_,
	HierarchyComponent_,
	MaterialComponent_,
	MeshComponent_,
	ImpostorComponent_,
	ObjectComponent_,
	LightComponent_,
	CameraComponent_,
	EnvironmentProbeComponent_,
	AnimationComponent_,
	EmittedParticleSystem_,
	ColliderComponent_,
	WeatherComponent_,
};

static wi::unordered_map<std::string, CompType> comptypes = {
	{typeid(wi::scene::NameComponent).name(), CompType::NameComponent_},
	{typeid(wi::scene::LayerComponent).name(), CompType::LayerComponent_},
	{typeid(wi::scene::TransformComponent).name(), CompType::TransformComponent_},
	{typeid(wi::scene::HierarchyComponent).name(), CompType::HierarchyComponent_},
	{typeid(wi::scene::MaterialComponent).name(), CompType::MaterialComponent_},
	{typeid(wi::scene::MeshComponent).name(), CompType::MeshComponent_},
	{typeid(wi::scene::ObjectComponent).name(), CompType::ObjectComponent_},
	{typeid(wi::scene::LightComponent).name(), CompType::LightComponent_},
	{typeid(wi::scene::CameraComponent).name(), CompType::CameraComponent_},
	{typeid(wi::scene::EnvironmentProbeComponent).name(), CompType::EnvironmentProbeComponent_},
	{typeid(wi::scene::AnimationComponent).name(), CompType::AnimationComponent_},
	{typeid(wi::scene::ColliderComponent).name(), CompType::ColliderComponent_},
	{typeid(wi::scene::WeatherComponent).name(), CompType::WeatherComponent_},
	{typeid(wi::EmittedParticleSystem).name(), CompType::EmittedParticleSystem_},
};
#pragma endregion

static bool g_is_display = true;
auto fail_ret = [](const std::string& err_str, const bool _warn = false)
	{
		if (g_is_display) 
		{
			wi::backlog::post(err_str, _warn ? wi::backlog::LogLevel::Warning : wi::backlog::LogLevel::Error);
		}
		return false;
	};

namespace vzm
{
	std::atomic_bool profileFrameFinished = { true };

	void TransformPoint(const float posSrc[3], const float mat[16], const bool rowMajor, float posDst[3])
	{
		XMVECTOR p = XMLoadFloat3((XMFLOAT3*)posSrc);
		XMMATRIX m(mat);
		if (!rowMajor) {
			m = XMMatrixTranspose(m);
		}
		p = XMVector3TransformCoord(p, m);
		XMStoreFloat3((XMFLOAT3*)posDst, p);
	}
	void TransformVector(const float vecSrc[3], const float mat[16], const bool rowMajor, float vecDst[3])
	{
		XMVECTOR v = XMLoadFloat3((XMFLOAT3*)vecSrc);
		XMMATRIX m(mat);
		if (!rowMajor) {
			m = XMMatrixTranspose(m);
		}
		v = XMVector3TransformNormal(v, m);
		XMStoreFloat3((XMFLOAT3*)vecDst, v);
	}
	void ComputeBoxTransformMatrix(const float cubeScale[3], const float posCenter[3],
		const float yAxis[3], const float zAxis[3], const bool rowMajor, float mat[16], float matInv[16])
	{
		XMVECTOR vec_scale = XMLoadFloat3((XMFLOAT3*)cubeScale);
		XMVECTOR pos_eye = XMLoadFloat3((XMFLOAT3*)posCenter);
		XMVECTOR vec_up = XMLoadFloat3((XMFLOAT3*)yAxis);
		XMVECTOR vec_view = -XMLoadFloat3((XMFLOAT3*)zAxis);
		XMMATRIX ws2cs = VZMatrixLookTo(pos_eye, vec_view, vec_up);
		vec_scale = XMVectorReciprocal(vec_scale);
		XMMATRIX scale = XMMatrixScaling(XMVectorGetX(vec_scale), XMVectorGetY(vec_scale), XMVectorGetZ(vec_scale));
		//glm::fmat4x4 translate = glm::translate(glm::fvec3(0.5f));

		XMMATRIX ws2cs_unit = ws2cs * scale; // row major
		*(XMMATRIX*)mat = rowMajor ? ws2cs_unit : XMMatrixTranspose(ws2cs_unit); // note that our renderer uses row-major
		*(XMMATRIX*)matInv = XMMatrixInverse(NULL, ws2cs_unit);
	}

	using namespace wi;

#pragma region // VZM DATA STRUCTURES
	class VzmRenderer : public wi::RenderPath3D
	{
	private:
		uint32_t prev_width = 0;
		uint32_t prev_height = 0;
		float prev_dpi = 96;
		bool prev_colorspace_conversion_required = false;

		VmCamera* vmCam = nullptr;
		TimeStamp timeStamp_vmUpdate;

		void TryResizeRenderTargets()
		{
			wi::graphics::GraphicsDevice* graphicsDevice = wi::graphics::GetDevice();
			if (graphicsDevice == nullptr)
				return;

			if (swapChain.IsValid())
			{
				colorspace = graphicsDevice->GetSwapChainColorSpace(&swapChain);
			}
			colorspace_conversion_required = colorspace == wi::graphics::ColorSpace::HDR10_ST2084;

			bool requireUpdateRenderTarget = prev_width != width || prev_height != height || prev_dpi != dpi
				|| prev_colorspace_conversion_required != colorspace_conversion_required;
			if (!requireUpdateRenderTarget)
				return;

			init(width, height, dpi);
			swapChain = {};
			renderResult = {};
			renderInterResult = {};

			auto CreateRenderTarget = [&](wi::graphics::Texture& renderTexture, const bool isInterResult)
				{
					if (!renderTexture.IsValid())
					{
						wi::graphics::TextureDesc desc;
						desc.width = width;
						desc.height = height;
						desc.bind_flags = wi::graphics::BindFlag::RENDER_TARGET | wi::graphics::BindFlag::SHADER_RESOURCE;
						if (!isInterResult)
						{
							// we assume the main GUI and engine use the same GPU device
							// wi::graphics::ResourceMiscFlag::SHARED_ACROSS_ADAPTER;
							desc.misc_flags = wi::graphics::ResourceMiscFlag::SHARED;
							desc.format = wi::graphics::Format::R10G10B10A2_UNORM;
						}
						else
						{
							desc.format = wi::graphics::Format::R11G11B10_FLOAT;
						}
						bool success = graphicsDevice->CreateTexture(&desc, nullptr, &renderTexture);
						assert(success);

						graphicsDevice->SetName(&renderTexture, (isInterResult ? "VzmRenderer::renderInterResult_" : "VzmRenderer::renderResult_") + camEntity);
					}
				};
			if (colorspace_conversion_required)
			{
				CreateRenderTarget(renderInterResult, true);
			}
			if (swapChain.IsValid())
			{
				// dojo to do ... create swapchain...
				// a window handler required 
			}
			else
			{
				CreateRenderTarget(renderResult, false);
			}

			Start(); // call ResizeBuffers();
			prev_width = width;
			prev_height = height;
			prev_dpi = dpi;
			prev_colorspace_conversion_required = colorspace_conversion_required;
		}

	public:

		uint64_t FRAMECOUNT = 0;

		float deltaTime = 0;
		float deltaTimeAccumulator = 0;
		float targetFrameRate = 60;		//
		bool frameskip = true;
		bool framerate_lock = false;	//
		wi::Timer timer;

		int fps_avg_counter = 0;
		float		time;
		float		time_previous;

		wi::FadeManager fadeManager;

		// render target 을 compose... 
		Entity camEntity = INVALID_ENTITY; // for searching 
		bool colorspace_conversion_required = false;
		wi::graphics::ColorSpace colorspace = wi::graphics::ColorSpace::SRGB;

		// note swapChain and renderResult are exclusive
		wi::graphics::SwapChain swapChain;
		wi::graphics::Texture renderResult;
		// renderInterResult is valid only when swapchain's color space is ColorSpace::HDR10_ST2084
		wi::graphics::Texture renderInterResult;

		// display all-time engine information text
		InfoDisplayer infoDisplay;
		std::string infodisplay_str;
		float deltatimes[20] = {};

		// wi::renderer options
		int ShadowProps2D = 1024; // (int max_resolution);
		int ShadowPropsCube = 256; // (int max_resolution);
		float ShadowsEnabled = true; 
		bool WireRender = false; 
		bool DrawDebugBoneLines = false; // (bool param);
		bool DrawDebugPartitionTree = false; // (bool param);
		bool DrawDebugEnvProbes = false; 
		bool DrawDebugEmitters = false; // (bool param);
		bool DrawDebugForceFields = false; // (bool param);
		bool DrawDebugCameras = false; // (bool param);
		bool DrawDebugColliders = false; // (bool param);
		bool DrawGridHelper = false; 
		bool DrawVoxelHelper = false;
		int DrawVoxelHelperClipMapLevel = 0;
		bool DebugLightCulling = false;
		bool AdvancedLightCulling = true;
		bool VariableRateShadingClassification = false;
		bool VariableRateShadingClassificationDebug = false;
		bool TemporalAAEnabled = false;
		bool TemporalAADebugEnabled = false;
		bool FreezeCullingCameraEnabled = false;
		bool VXGIEnabled = false;
		bool VXGIReflectionsEnabled = false;
		float GameSpeed = 1.f; 
		uint32_t RaytraceBounceCount = 3u; 
		bool RaytraceDebugBVHVisualizerEnabled = false; 
		bool RaytracedShadowsEnabled = false; 
		bool TessellationEnabled = true; 
		bool DisableAlbedoMaps = false; 
		bool ForceDiffuseLighting = false; 
		bool ScreenSpaceShadowsEnabled = false; 
		bool SurfelGIEnabled = false; 
		SURFEL_DEBUG SurfelGIDebugEnabled = SURFEL_DEBUG_NONE; // (SURFEL_DEBUG value);
		bool DDGIEnabled = false; 
		bool DDGIDebugEnabled = false; 
		uint32_t DDGIRayCount = 128u; // (uint32_t value);
		float DDGIBlendSpeed = 0.02f; 
		float GIBoost = 1.f;

		bool DisplayProfile = false; // this is only for profiling canvas

		// kind of initializer
		void Load() override
		{
			infoDisplay.active = true;
			infoDisplay.fpsinfo = true;
			infoDisplay.pipeline_count = true;
			infoDisplay.resolution = true;
			infoDisplay.heap_allocation_counter = true;
			infoDisplay.logical_size = true;

			this->ClearSprites();
			this->ClearFonts();

			// remove...
			setSSREnabled(false);
			setFXAAEnabled(false);
			setReflectionsEnabled(true);
			setRaytracedReflectionsEnabled(true);
			setFSR2Enabled(true);
			setEyeAdaptionEnabled(false);
			setEyeAdaptionKey(0.1f);
			setEyeAdaptionRate(0.5f);

			//setSceneUpdateEnabled(true); // for multiple main-cameras support

			wi::font::UpdateAtlas(GetDPIScaling());

			RenderPath3D::Load();

			assert(width > 0 && height > 0);
			{
				const float fadeSeconds = 0.f;
				wi::Color fadeColor = wi::Color(0, 0, 0, 255);
				// Fade manager will activate on fadeout
				fadeManager.Clear();
				fadeManager.Start(fadeSeconds, fadeColor, [this]() {
					Start();
					});

				fadeManager.Update(0); // If user calls ActivatePath without fadeout, it will be instant
			}
		}

		void UpdateRendererOptions()
		{
			wi::renderer::SetShadowProps2D(ShadowProps2D);
			wi::renderer::SetShadowPropsCube(ShadowPropsCube);
			wi::renderer::SetShadowsEnabled(ShadowsEnabled);
			wi::renderer::SetWireRender(WireRender);
			wi::renderer::SetToDrawDebugBoneLines(DrawDebugBoneLines);
			wi::renderer::SetToDrawDebugPartitionTree(DrawDebugPartitionTree);
			wi::renderer::SetToDrawDebugEnvProbes(DrawDebugEnvProbes);
			wi::renderer::SetToDrawDebugEmitters(DrawDebugEmitters);
			wi::renderer::SetToDrawDebugForceFields(DrawDebugForceFields);
			wi::renderer::SetToDrawDebugCameras(DrawDebugCameras);
			wi::renderer::SetToDrawDebugColliders(DrawDebugColliders);
			wi::renderer::SetToDrawGridHelper(DrawGridHelper);
			wi::renderer::SetToDrawVoxelHelper(DrawVoxelHelper, DrawVoxelHelperClipMapLevel);
			wi::renderer::SetDebugLightCulling(DebugLightCulling);
			wi::renderer::SetAdvancedLightCulling(AdvancedLightCulling);
			wi::renderer::SetVariableRateShadingClassification(VariableRateShadingClassification);
			wi::renderer::SetVariableRateShadingClassificationDebug(VariableRateShadingClassificationDebug);
			wi::renderer::SetTemporalAAEnabled(TemporalAAEnabled);
			wi::renderer::SetTemporalAADebugEnabled(TemporalAADebugEnabled);
			wi::renderer::SetFreezeCullingCameraEnabled(FreezeCullingCameraEnabled);
			wi::renderer::SetVXGIEnabled(VXGIEnabled);
			wi::renderer::SetVXGIReflectionsEnabled(VXGIReflectionsEnabled);
			wi::renderer::SetGameSpeed(GameSpeed);
			wi::renderer::SetRaytraceBounceCount(RaytraceBounceCount);
			wi::renderer::SetRaytraceDebugBVHVisualizerEnabled(RaytraceDebugBVHVisualizerEnabled);
			wi::renderer::SetRaytracedShadowsEnabled(RaytracedShadowsEnabled);
			wi::renderer::SetTessellationEnabled(TessellationEnabled);
			wi::renderer::SetDisableAlbedoMaps(DisableAlbedoMaps);
			wi::renderer::SetForceDiffuseLighting(ForceDiffuseLighting);
			wi::renderer::SetScreenSpaceShadowsEnabled(ScreenSpaceShadowsEnabled);
			wi::renderer::SetSurfelGIEnabled(SurfelGIEnabled);
			wi::renderer::SetSurfelGIDebugEnabled(SurfelGIDebugEnabled);
			wi::renderer::SetDDGIEnabled(DDGIEnabled);
			wi::renderer::SetDDGIDebugEnabled(DDGIDebugEnabled);
			wi::renderer::SetDDGIRayCount(DDGIRayCount);
			wi::renderer::SetDDGIBlendSpeed(DDGIBlendSpeed);
			wi::renderer::SetGIBoost(GIBoost);
		}

		void Update(float dt) override
		{
			if (vmCam)
			{
				vmCam->IsActivated = true;
			}
			UpdateRendererOptions();
			RenderPath3D::Update(dt);	// calls RenderPath2D::Update(dt);
		}

		void PostUpdate() override
		{
			RenderPath2D::PostUpdate();
			RenderPath3D::PostUpdate();
		}

		void FixedUpdate() override
		{
			RenderPath2D::FixedUpdate();
		}

		void Compose(wi::graphics::CommandList cmd)
		{
			using namespace wi::graphics;

			wi::graphics::GraphicsDevice* graphicsDevice = GetDevice();
			if (!graphicsDevice)
				return;

			auto range = wi::profiler::BeginRangeCPU("Compose");

			wi::RenderPath3D::Compose(cmd);

			if (fadeManager.IsActive())
			{
				// display fade rect
				wi::image::Params fx;
				fx.enableFullScreen();
				fx.color = fadeManager.color;
				fx.opacity = fadeManager.opacity;
				wi::image::Draw(nullptr, fx, cmd);
			}

			// Draw the information display
			if (infoDisplay.active)
			{
				infodisplay_str.clear();
				if (infoDisplay.watermark)
				{
					infodisplay_str += "Wicked Engine ";
					infodisplay_str += wi::version::GetVersionString();
					infodisplay_str += " ";

#if defined(_ARM)
					infodisplay_str += "[ARM]";
#elif defined(_WIN64)
					infodisplay_str += "[64-bit]";
#elif defined(_WIN32)
					infodisplay_str += "[32-bit]";
#endif // _ARM

#ifdef PLATFORM_UWP
					infodisplay_str += "[UWP]";
#endif // PLATFORM_UWP

#ifdef WICKEDENGINE_BUILD_DX12
					if (dynamic_cast<GraphicsDevice_DX12*>(graphicsDevice))
					{
						infodisplay_str += "[DX12]";
					}
#endif // WICKEDENGINE_BUILD_DX12
#ifdef WICKEDENGINE_BUILD_VULKAN
					if (dynamic_cast<GraphicsDevice_Vulkan*>(graphicsDevice))
					{
						infodisplay_str += "[Vulkan]";
					}
#endif // WICKEDENGINE_BUILD_VULKAN
#ifdef PLATFORM_PS5
					if (dynamic_cast<GraphicsDevice_PS5*>(graphicsDevice.get()))
					{
						infodisplay_str += "[PS5]";
					}
#endif // PLATFORM_PS5

#ifdef _DEBUG
					infodisplay_str += "[DEBUG]";
#endif // _DEBUG
					if (graphicsDevice->IsDebugDevice())
					{
						infodisplay_str += "[debugdevice]";
					}
					infodisplay_str += "\n";
				}
				if (infoDisplay.device_name)
				{
					infodisplay_str += "Device: ";
					infodisplay_str += graphicsDevice->GetAdapterName();
					infodisplay_str += "\n";
				}
				if (infoDisplay.resolution)
				{
					infodisplay_str += "Resolution: ";
					infodisplay_str += std::to_string(GetPhysicalWidth());
					infodisplay_str += " x ";
					infodisplay_str += std::to_string(GetPhysicalHeight());
					infodisplay_str += " (";
					infodisplay_str += std::to_string(int(GetDPI()));
					infodisplay_str += " dpi)\n";
				}
				if (infoDisplay.logical_size)
				{
					infodisplay_str += "Logical Size: ";
					infodisplay_str += std::to_string(int(GetLogicalWidth()));
					infodisplay_str += " x ";
					infodisplay_str += std::to_string(int(GetLogicalHeight()));
					infodisplay_str += "\n";
				}
				if (infoDisplay.colorspace)
				{
					infodisplay_str += "Color Space: ";
					ColorSpace colorSpace = colorspace; // graphicsDevice->GetSwapChainColorSpace(&swapChain);
					switch (colorSpace)
					{
					default:
					case wi::graphics::ColorSpace::SRGB:
						infodisplay_str += "sRGB";
						break;
					case wi::graphics::ColorSpace::HDR10_ST2084:
						infodisplay_str += "ST.2084 (HDR10)";
						break;
					case wi::graphics::ColorSpace::HDR_LINEAR:
						infodisplay_str += "Linear (HDR)";
						break;
					}
					infodisplay_str += "\n";
				}
				if (infoDisplay.fpsinfo)
				{
					deltatimes[fps_avg_counter++ % arraysize(deltatimes)] = deltaTime;
					float displaydeltatime = deltaTime;
					if (fps_avg_counter > arraysize(deltatimes))
					{
						float avg_time = 0;
						for (int i = 0; i < arraysize(deltatimes); ++i)
						{
							avg_time += deltatimes[i];
						}
						displaydeltatime = avg_time / arraysize(deltatimes);
					}

					infodisplay_str += std::to_string(int(std::round(1.0f / displaydeltatime))) + " FPS\n";
				}
				if (infoDisplay.heap_allocation_counter)
				{
					infodisplay_str += "Heap allocations per frame: ";
#ifdef WICKED_ENGINE_HEAP_ALLOCATION_COUNTER
					infodisplay_str += std::to_string(number_of_heap_allocations.load());
					infodisplay_str += " (";
					infodisplay_str += std::to_string(size_of_heap_allocations.load());
					infodisplay_str += " bytes)\n";
					number_of_heap_allocations.store(0);
					size_of_heap_allocations.store(0);
#else
					infodisplay_str += "[disabled]\n";
#endif // WICKED_ENGINE_HEAP_ALLOCATION_COUNTER
				}
				if (infoDisplay.pipeline_count)
				{
					infodisplay_str += "Graphics pipelines active: ";
					infodisplay_str += std::to_string(graphicsDevice->GetActivePipelineCount());
					infodisplay_str += "\n";
				}

				wi::font::Params params = wi::font::Params(
					4,
					4,
					infoDisplay.size,
					wi::font::WIFALIGN_LEFT,
					wi::font::WIFALIGN_TOP,
					wi::Color::White(),
					wi::Color::Shadow()
				);
				params.shadow_softness = 0.4f;

				// Explanation: this compose pass is in LINEAR space if display output is linear or HDR10
				//	If HDR10, the HDR10 output mapping will be performed on whole image later when drawing to swapchain
				if (colorspace != ColorSpace::SRGB)
				{
					params.enableLinearOutputMapping(9);
				}

				params.cursor = wi::font::Draw(infodisplay_str, params, cmd);

				// VRAM:
				{
					GraphicsDevice::MemoryUsage vram = graphicsDevice->GetMemoryUsage();
					bool warn = false;
					if (vram.usage > vram.budget)
					{
						params.color = wi::Color::Error();
						warn = true;
					}
					else if (float(vram.usage) / float(vram.budget) > 0.9f)
					{
						params.color = wi::Color::Warning();
						warn = true;
					}
					if (infoDisplay.vram_usage || warn)
					{
						params.cursor = wi::font::Draw("VRAM usage: " + std::to_string(vram.usage / 1024 / 1024) + "MB / " + std::to_string(vram.budget / 1024 / 1024) + "MB\n", params, cmd);
						params.color = wi::Color::White();
					}
				}

				// Write warnings below:
				params.color = wi::Color::Warning();
#ifdef _DEBUG
				params.cursor = wi::font::Draw("Warning: This is a [DEBUG] build, performance will be slow!\n", params, cmd);
#endif
				if (graphicsDevice->IsDebugDevice())
				{
					params.cursor = wi::font::Draw("Warning: Graphics is in [debugdevice] mode, performance will be slow!\n", params, cmd);
				}

				// Write errors below:
				params.color = wi::Color::Error();
				if (wi::renderer::GetShaderMissingCount() > 0)
				{
					params.cursor = wi::font::Draw(std::to_string(wi::renderer::GetShaderMissingCount()) + " shaders missing! Check the backlog for more information!\n", params, cmd);
				}
				if (wi::renderer::GetShaderErrorCount() > 0)
				{
					params.cursor = wi::font::Draw(std::to_string(wi::renderer::GetShaderErrorCount()) + " shader compilation errors! Check the backlog for more information!\n", params, cmd);
				}


				if (infoDisplay.colorgrading_helper)
				{
					wi::image::Draw(wi::texturehelper::getColorGradeDefault(), wi::image::Params(0, 0, 256.0f / GetDPIScaling(), 16.0f / GetDPIScaling()), cmd);
				}
			}
			
			wi::backlog::Draw(*this, cmd, colorspace);
			wi::profiler::EndRange(range); // Compose

			if (DisplayProfile)
			{
				wi::profiler::DrawData(*this, 4, 60, cmd, colorspace);
			}
		}

		void RenderFinalize()
		{
			using namespace wi::graphics;

			wi::graphics::GraphicsDevice* graphicsDevice = GetDevice();
			if (!graphicsDevice)
				return;

			// Begin final compositing:
			CommandList cmd = graphicsDevice->BeginCommandList();
			wi::image::SetCanvas(*this);
			wi::font::SetCanvas(*this);

			Viewport viewport;
			viewport.width = (float)width;
			viewport.height = (float)height;
			graphicsDevice->BindViewports(1, &viewport, cmd);

			if (colorspace_conversion_required)
			{
				RenderPassImage rp[] = {
					RenderPassImage::RenderTarget(&renderInterResult, RenderPassImage::LoadOp::CLEAR),
				};
				graphicsDevice->RenderPassBegin(rp, arraysize(rp), cmd);
			}
			else
			{
				// If swapchain is SRGB or Linear HDR, it can be used for blending
				//	- If it is SRGB, the render path will ensure tonemapping to SDR
				//	- If it is Linear HDR, we can blend trivially in linear space
				renderInterResult = {};
				if (swapChain.IsValid())
				{
					graphicsDevice->RenderPassBegin(&swapChain, cmd);
				}
				else
				{
					RenderPassImage rp[] = {
						RenderPassImage::RenderTarget(&renderResult, RenderPassImage::LoadOp::CLEAR),
					};
					graphicsDevice->RenderPassBegin(rp, arraysize(rp), cmd);
				}
			}

			Compose(cmd);
			graphicsDevice->RenderPassEnd(cmd);

			if (colorspace_conversion_required)
			{
				// In HDR10, we perform a final mapping from linear to HDR10, into the swapchain
				if (swapChain.IsValid())
				{
					graphicsDevice->RenderPassBegin(&swapChain, cmd);
				}
				else
				{
					RenderPassImage rp[] = {
						RenderPassImage::RenderTarget(&renderResult, RenderPassImage::LoadOp::CLEAR),
					};
					graphicsDevice->RenderPassBegin(rp, arraysize(rp), cmd);
				}
				wi::image::Params fx;
				fx.enableFullScreen();
				fx.enableHDR10OutputMapping();
				wi::image::Draw(&renderInterResult, fx, cmd);
				graphicsDevice->RenderPassEnd(cmd);
			}

			if (DisplayProfile)
			{
				profileFrameFinished = true;
				wi::profiler::EndFrame(cmd); // cmd must be assigned before SubmitCommandLists
			}
			graphicsDevice->SubmitCommandLists();
		}

		void WaitRender()
		{
			wi::graphics::GraphicsDevice* graphicsDevice = wi::graphics::GetDevice();
			if (!graphicsDevice)
				return;

			wi::graphics::CommandList cmd = graphicsDevice->BeginCommandList();
			if (swapChain.IsValid())
			{
				graphicsDevice->RenderPassBegin(&swapChain, cmd);
			}
			else
			{
				wi::graphics::RenderPassImage rt[] = {
					wi::graphics::RenderPassImage::RenderTarget(&renderResult, wi::graphics::RenderPassImage::LoadOp::CLEAR)
				};
				graphicsDevice->RenderPassBegin(rt, 1, cmd);
			}

			wi::graphics::Viewport viewport;
			viewport.width = (float)width;
			viewport.height = (float)height;
			graphicsDevice->BindViewports(1, &viewport, cmd);
			if (wi::initializer::IsInitializeFinished(wi::initializer::INITIALIZED_SYSTEM_FONT))
			{
				wi::backlog::DrawOutputText(*this, cmd, colorspace);
			}
			graphicsDevice->RenderPassEnd(cmd);
			graphicsDevice->SubmitCommandLists();
		}

		bool UpdateVmCamera(const VmCamera* _vmCam = nullptr)
		{
			// note Scene::Merge... rearrange the components
			camera = scene->cameras.GetComponent(camEntity);
			if (!getSceneUpdateEnabled())
			{
				scene->camera = *camera;
			}

			if (_vmCam != nullptr)
			{
				vmCam = (VmCamera*)_vmCam;
				camEntity = vmCam->componentVID;
			}
			if (vmCam)
			{
				std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(timeStamp_vmUpdate - vmCam->timeStamp);
				if (time_span.count() > 0)
				{
					return true;
				}
			}
			TryResizeRenderTargets();

			// ???? DOJO TO DO... SetPose 에서 해 보기... flickering 동작 확인
			TransformComponent* transform = scene->transforms.GetComponent(camEntity);
			if (transform)
			{
				transform->UpdateTransform();
			}
			else
			{
				camera->UpdateCamera();
			}
			timeStamp_vmUpdate = std::chrono::high_resolution_clock::now();

			return true;
		}

		VmCamera* GetVmCamera()
		{
			return vmCam;
		}
	};

	struct VzmScene : Scene
	{
		VID sceneVid = INVALID_VID;
		VmWeather vmWeather;
		std::string name;

		float deltaTime = 0;
		wi::Timer timer;
	};

	class SceneManager
	{
	private:
		std::unique_ptr<wi::graphics::GraphicsDevice> graphicsDevice;

		// archive
		std::map<VID, VzmScene> scenes;						// <SceneEntity, Scene>
		// one camera component to one renderer
		std::map<VID, VzmRenderer> renderers;				// <CamEntity, VzmRenderer> (including camera and scene)
		wi::unordered_map<VID, std::unique_ptr<VmBaseComponent>> vmComponents;

	public:
		void Initialize(::vzm::ParamMap<std::string>& argument)
		{
			// device creation
			// User can also create a graphics device if custom logic is desired, but they must do before this function!
			wi::platform::window_type window = argument.GetParam("window", wi::platform::window_type(nullptr));
			if (graphicsDevice == nullptr)
			{
				using namespace wi::graphics;

				ValidationMode validationMode = ValidationMode::Disabled;
				if (argument.GetParam("debugdevice", false))
				{
					validationMode = ValidationMode::Enabled;
				}
				if (argument.GetParam("gpuvalidation", false))
				{
					validationMode = ValidationMode::GPU;
				}
				if (argument.GetParam("gpu_verbose", false))
				{
					validationMode = ValidationMode::Verbose;
				}

				GPUPreference preference = GPUPreference::Discrete;
				if (argument.GetParam("igpu", false))
				{
					preference = GPUPreference::Integrated;
				}

				bool use_dx12 = wi::arguments::HasArgument("dx12");
				bool use_vulkan = wi::arguments::HasArgument("vulkan");

#ifndef WICKEDENGINE_BUILD_DX12
				if (use_dx12) {
					wi::helper::messageBox("The engine was built without DX12 support!", "Error");
					use_dx12 = false;
				}
#endif // WICKEDENGINE_BUILD_DX12
#ifndef WICKEDENGINE_BUILD_VULKAN
				if (use_vulkan) {
					wi::helper::messageBox("The engine was built without Vulkan support!", "Error");
					use_vulkan = false;
				}
#endif // WICKEDENGINE_BUILD_VULKAN

				if (!use_dx12 && !use_vulkan)
				{
#if defined(WICKEDENGINE_BUILD_DX12)
					use_dx12 = true;
#elif defined(WICKEDENGINE_BUILD_VULKAN)
					use_vulkan = true;
#else
					wi::backlog::post("No rendering backend is enabled! Please enable at least one so we can use it as default", wi::backlog::LogLevel::Error);
					assert(false);
#endif
				}
				assert(use_dx12 || use_vulkan);

				if (use_vulkan)
				{
#ifdef WICKEDENGINE_BUILD_VULKAN
					wi::renderer::SetShaderPath(wi::renderer::GetShaderPath() + "spirv/");
					graphicsDevice = std::make_unique<GraphicsDevice_Vulkan>(window, validationMode, preference);
#endif
				}
				else if (use_dx12)
				{
#ifdef WICKEDENGINE_BUILD_DX12
					wi::renderer::SetShaderPath(wi::renderer::GetShaderPath() + "hlsl6/");
					graphicsDevice = std::make_unique<GraphicsDevice_DX12>(validationMode, preference);
#endif
				}
			}
			wi::graphics::GetDevice() = graphicsDevice.get();

			wi::initializer::InitializeComponentsAsync();
			//wi::initializer::InitializeComponentsImmediate();

			// Reset all state that tests might have modified:
			wi::eventhandler::SetVSync(false);
			//wi::renderer::SetToDrawGridHelper(false);
			//wi::renderer::SetTemporalAAEnabled(false);
			//wi::renderer::ClearWorld(wi::scene::GetScene());
		}

		// Runtime can create a new entity with this
		inline VID CreateSceneEntity(const std::string& name)
		{
			if (GetFirstSceneByName(name))
			{
				wi::backlog::post(name + " is already registered as a scene!", backlog::LogLevel::Error);
				return INVALID_ENTITY;
			}

			Entity ett = CreateEntity();

			if (ett != INVALID_ENTITY) {
				VzmScene& scene = scenes[ett];
				wi::renderer::ClearWorld(scene);
				scene.weather = WeatherComponent();
				scene.weather.ambient = XMFLOAT3(0.9f, 0.9f, 0.9f);
				wi::Color default_sky_zenith = wi::Color(30, 40, 60, 200);
				scene.weather.zenith = default_sky_zenith;
				wi::Color default_sky_horizon = wi::Color(10, 10, 20, 220); // darker elements will lerp towards this
				scene.weather.horizon = default_sky_horizon;
				scene.weather.fogStart = std::numeric_limits<float>::max();
				scene.weather.fogDensity = 0;
				{
					scene.vmWeather.componentVID = ett;
					scene.vmWeather.compType = COMPONENT_TYPE::WEATHER;
				}
				scene.name = name;
				scene.sceneVid = ett;
			}

			return ett;
		}
		inline VzmRenderer* CreateRenderer(const VID camEntity)
		{
			auto it = renderers.find(camEntity);
			assert(it == renderers.end());

			VzmRenderer* renderer = &renderers[camEntity];
			renderer->camEntity = camEntity;

			for (auto it = scenes.begin(); it != scenes.end(); it++)
			{
				VzmScene* scene = &it->second;
				CameraComponent* camera = scene->cameras.GetComponent(camEntity);
				if (camera)
				{
					renderer->scene = scene;
					renderer->camera = camera;
					return renderer;
				}
			}
			renderers.erase(camEntity);
			return nullptr;
		}

		inline size_t GetVidsByName(const std::string& name, std::vector<VID>& vids)
		{
			for (auto it = scenes.begin(); it != scenes.end(); it++)
			{
				VzmScene& scene = it->second;
				for (Entity ett : scene.names.GetEntityArray())
				{
					NameComponent* nameComp = scene.names.GetComponent(ett);
					if (nameComp != nullptr)
					{
						vids.push_back(ett);
					}
				}
			}
			return (uint32_t)vids.size();
		}
		inline VID GetFirstVidByName(const std::string& name)
		{
			for (auto it = scenes.begin(); it != scenes.end(); it++)
			{
				Entity ett = it->second.Entity_FindByName(name);
				if (ett != INVALID_ENTITY)
				{
					return ett;
				}
				if (it->second.name == name)
				{
					return it->second.sceneVid;
				}
			}
			return INVALID_VID;
		}
		inline std::string GetNameByVid(const VID vid)
		{
			for (auto it = scenes.begin(); it != scenes.end(); it++)
			{
				NameComponent* nameComp = it->second.names.GetComponent(vid);
				if (nameComp)
				{
					return nameComp->name;
				}
			}
			auto it = scenes.find(vid);
			if (it != scenes.end())
			{
				return it->second.name;
			}
			return "";
		}
		inline VzmScene* GetScene(const VID sid)
		{
			auto it = scenes.find(sid);
			if (it == scenes.end())
			{
				return nullptr;
			}
			return &it->second;
		}
		inline VzmScene* GetFirstSceneByName(const std::string& name)
		{
			for (auto it = scenes.begin(); it != scenes.end(); it++)
			{
				VzmScene* scene = &it->second;
				if (scene->name == name)
				{
					return scene;
				}
			}
			return nullptr;
		}
		inline std::map<VID, VzmScene>* GetScenes()
		{
			return &scenes;
		}
		inline VzmRenderer* GetRenderer(const VID camEntity)
		{
			auto it = renderers.find(camEntity);
			if (it == renderers.end())
			{
				return nullptr;
			}
			return &it->second;
		}
		inline VzmRenderer* GetFirstRendererByName(const std::string& name)
		{
			return GetRenderer(GetFirstVidByName(name));
		}

		template <typename VMCOMP>
		inline VMCOMP* CreateVmComp(const VID vid)
		{
			auto it = vmComponents.find(vid);
			assert(it == vmComponents.end());

			std::string typeName = typeid(VMCOMP).name();
			COMPONENT_TYPE compType = vmcomptypes[typeName];
			if (compType == COMPONENT_TYPE::UNDEFINED)
			{
				return nullptr;
			}

			for (auto sit = scenes.begin(); sit != scenes.end(); sit++)
			{
				VzmScene* scene = &sit->second;
				wi::unordered_set<Entity> entities;
				scene->FindAllEntities(entities);
				auto it = entities.find(vid);
				if (it != entities.end())
				{
					VMCOMP vmComp;
					vmComp.componentVID = vid;
					vmComp.compType = compType;
					vmComponents.insert(std::make_pair(vid, std::make_unique<VMCOMP>(vmComp)));
					return (VMCOMP*)vmComponents[vid].get();
				}
			}
			return nullptr;
		}

		template <typename VMCOMP>
		inline VMCOMP* GetVmComp(const VID vid)
		{
			auto it = vmComponents.find(vid);
			if (it == vmComponents.end())
			{
				return nullptr;
			}
			return (VMCOMP*)it->second.get();
		}
		template <typename COMP>
		inline COMP* GetEngineComp(const VID vid)
		{
			std::string typeName = typeid(COMP).name();
			CompType compType = comptypes[typeName];
			COMP* comp = nullptr;
			for (auto it = scenes.begin(); it != scenes.end(); it++)
			{
				VzmScene* scene = &it->second;
				switch (compType)
				{
				case CompType::CameraComponent_:
					comp = (COMP*)scene->cameras.GetComponent(vid); break;
				case CompType::ObjectComponent_:
					comp = (COMP*)scene->objects.GetComponent(vid); break;
				case CompType::TransformComponent_:
					comp = (COMP*)scene->transforms.GetComponent(vid); break;
				case CompType::HierarchyComponent_:
					comp = (COMP*)scene->hierarchy.GetComponent(vid); break;
				case CompType::NameComponent_:
					comp = (COMP*)scene->names.GetComponent(vid); break;
				case CompType::LightComponent_:
					comp = (COMP*)scene->lights.GetComponent(vid); break;
				case CompType::AnimationComponent_:
					comp = (COMP*)scene->animations.GetComponent(vid); break;
				case CompType::EmittedParticleSystem_:
					comp = (COMP*)scene->emitters.GetComponent(vid); break;
				case CompType::ColliderComponent_:
					comp = (COMP*)scene->colliders.GetComponent(vid); break;
				case CompType::WeatherComponent_:
					comp = (COMP*)scene->weathers.GetComponent(vid); break;
				default: assert(0 && "Not allowed GetComponent");  return nullptr;
				}
				if (comp) break;
			}

			return comp;
		}

		inline void RemoveEntity(const VID vid)
		{
			VzmScene* scene = GetScene(vid);
			if (scene)
			{
				wi::unordered_set<Entity> entities;
				scene->FindAllEntities(entities);

				for (auto it = entities.begin(); it != entities.end(); it++)
				{
					renderers.erase(*it);
					vmComponents.erase(*it);
				}
				scenes.erase(vid);
			}
			else
			{

				for (auto it = scenes.begin(); it != scenes.end(); it++)
				{
					VzmScene* scene = &it->second;
					scene->Entity_Remove(vid);
				}
				renderers.erase(vid);
				vmComponents.erase(vid);
			}
		}
	};
#pragma endregion

	SceneManager sceneManager;
}


#define COMP_GET(COMP, PARAM, RET) COMP* PARAM = sceneManager.GetEngineComp<COMP>(componentVID); if (!PARAM) return RET;

namespace vzm 
{
#pragma region // VmBaseComponent
	void VmBaseComponent::GetWorldPosition(float v[3])
	{
		COMP_GET(TransformComponent, transform, );
		*(XMFLOAT3*)v = transform->GetPosition();
	}
	void VmBaseComponent::GetWorldForward(float v[3])
	{
		COMP_GET(TransformComponent, transform, );
		*(XMFLOAT3*)v = transform->GetForward();
	}
	void VmBaseComponent::GetWorldRight(float v[3])
	{
		COMP_GET(TransformComponent, transform, );
		*(XMFLOAT3*)v = transform->GetRight();
	}
	void VmBaseComponent::GetWorldUp(float v[3])
	{
		COMP_GET(TransformComponent, transform, );
		*(XMFLOAT3*)v = transform->GetUp();
	}
	void VmBaseComponent::GetLocalTransform(float mat[16], const bool rowMajor)
	{
		COMP_GET(TransformComponent, transform, );
		XMMATRIX _mat = !rowMajor? transform->GetLocalMatrix() : XMMatrixTranspose(transform->GetLocalMatrix());
		XMStoreFloat4x4((XMFLOAT4X4*)mat, _mat);
	}
	void VmBaseComponent::GetWorldTransform(float mat[16], const bool rowMajor)
	{
		COMP_GET(TransformComponent, transform, );
		XMMATRIX _mat = XMLoadFloat4x4(&transform->world);
		if (rowMajor)
		{
			_mat = XMMatrixTranspose(_mat);
		}
		XMStoreFloat4x4((XMFLOAT4X4*)mat, _mat);
	}
	void VmBaseComponent::GetLocalInvTransform(float mat[16], const bool rowMajor)
	{
		COMP_GET(TransformComponent, transform, );
		XMMATRIX _mat = !rowMajor ? transform->GetLocalMatrix() : XMMatrixTranspose(transform->GetLocalMatrix());
		_mat = XMMatrixInverse(nullptr, _mat);
		XMStoreFloat4x4((XMFLOAT4X4*)mat, _mat);
	}
	void VmBaseComponent::GetWorldInvTransform(float mat[16], const bool rowMajor)
	{
		COMP_GET(TransformComponent, transform, );
		XMMATRIX _mat = XMLoadFloat4x4(&transform->world);
		if (rowMajor)
		{
			_mat = XMMatrixTranspose(_mat);
		}
		_mat = XMMatrixInverse(nullptr, _mat);
		XMStoreFloat4x4((XMFLOAT4X4*)mat, _mat);
	}
	void VmBaseComponent::SetTranslate(const float value[3])
	{
		COMP_GET(TransformComponent, transform, );
		transform->Translate(*(XMFLOAT3*)value);
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmBaseComponent::SetScale(const float value[3])
	{
		COMP_GET(TransformComponent, transform, );
		transform->Scale(*(XMFLOAT3*)value);
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmBaseComponent::SetQuaternion(const float value[4])
	{
		COMP_GET(TransformComponent, transform, );
		transform->Rotate(*(XMFLOAT4*)value);
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmBaseComponent::SetMatrix(const float value[16], const bool additiveTransform, const bool rowMajor)
	{
		COMP_GET(TransformComponent, transform, );
		if (!additiveTransform)
		{
			transform->ClearTransform();
		}
		XMMATRIX mat = XMLoadFloat4x4((XMFLOAT4X4*)value);
		if (rowMajor)
		{
			mat = XMMatrixTranspose(mat);
		}
		transform->MatrixTransform(mat);
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	VID VmBaseComponent::GetParentVid()
	{
		COMP_GET(HierarchyComponent, hierarchy, INVALID_VID);
		return hierarchy->parentID;
	}
#pragma endregion 

#pragma region // VmRenderer
#define RENDERER_GET VzmRenderer* vrenderer = (VzmRenderer*)renderer;
	void VmRenderer::SetShadowProps2D(int max_resolution)
	{
		RENDERER_GET;
		vrenderer->ShadowProps2D = max_resolution;
	}
	void VmRenderer::SetShadowPropsCube(int max_resolution)
	{
		RENDERER_GET;
		vrenderer->ShadowPropsCube = max_resolution;
	}
	void VmRenderer::SetShadowsEnabled(bool value)
	{
		RENDERER_GET;
		vrenderer->ShadowsEnabled = value;
	}
	bool VmRenderer::GetShadowsEnabled()
	{
		RENDERER_GET;
		return vrenderer->ShadowsEnabled;
	}
	void VmRenderer::SetWireRender(bool value)
	{
		RENDERER_GET;
		vrenderer->WireRender = value;
	}
	bool VmRenderer::IsWireRender()
	{
		RENDERER_GET;
		return vrenderer->WireRender;
	}
	void VmRenderer::SetToDrawDebugBoneLines(bool param){}
	bool VmRenderer::GetToDrawDebugBoneLines()
	{
		RENDERER_GET;
		return vrenderer->DrawDebugBoneLines;
	}
	void VmRenderer::SetToDrawDebugPartitionTree(bool param){}
	bool VmRenderer::GetToDrawDebugPartitionTree()
	{
		RENDERER_GET;
		return vrenderer->DrawDebugPartitionTree;
	}
	bool VmRenderer::GetToDrawDebugEnvProbes()
	{
		RENDERER_GET;
		return vrenderer->DrawDebugEnvProbes;
	}
	void VmRenderer::SetToDrawDebugEnvProbes(bool value){}
	void VmRenderer::SetToDrawDebugEmitters(bool param){}
	bool VmRenderer::GetToDrawDebugEmitters()
	{
		RENDERER_GET;
		return vrenderer->DrawDebugEmitters;
	}
	void VmRenderer::SetToDrawDebugForceFields(bool param){}
	bool VmRenderer::GetToDrawDebugForceFields()
	{
		RENDERER_GET;
		return vrenderer->DrawDebugForceFields;
	}
	void VmRenderer::SetToDrawDebugCameras(bool param){}
	bool VmRenderer::GetToDrawDebugCameras()
	{
		RENDERER_GET;
		return vrenderer->DrawDebugCameras;
	}
	void VmRenderer::SetToDrawDebugColliders(bool param){}
	bool VmRenderer::GetToDrawDebugColliders()
	{
		RENDERER_GET;
		return vrenderer->DrawDebugColliders;
	}
	bool VmRenderer::GetToDrawGridHelper()
	{
		RENDERER_GET;
		return vrenderer->DrawGridHelper;
	}
	void VmRenderer::SetToDrawGridHelper(bool value)
	{
		RENDERER_GET;
		vrenderer->DrawGridHelper = true;
	}
	bool VmRenderer::GetToDrawVoxelHelper()
	{
		RENDERER_GET;
		return vrenderer->DrawVoxelHelper;
	}
	void VmRenderer::SetToDrawVoxelHelper(bool value, int clipmap_level){}
	void VmRenderer::SetDebugLightCulling(bool enabled){}
	bool VmRenderer::GetDebugLightCulling()
	{
		RENDERER_GET;
		return vrenderer->DebugLightCulling;
	}
	void VmRenderer::SetAdvancedLightCulling(bool enabled){}
	bool VmRenderer::GetAdvancedLightCulling()
	{
		RENDERER_GET;
		return vrenderer->AdvancedLightCulling;
	}
	void VmRenderer::SetVariableRateShadingClassification(bool enabled){}
	bool VmRenderer::GetVariableRateShadingClassification()
	{
		RENDERER_GET;
		return vrenderer->VariableRateShadingClassification;
	}
	void VmRenderer::SetVariableRateShadingClassificationDebug(bool enabled){}
	bool VmRenderer::GetVariableRateShadingClassificationDebug()
	{
		RENDERER_GET;
		return vrenderer->VariableRateShadingClassificationDebug;
	}
	void VmRenderer::SetTemporalAAEnabled(bool enabled){}
	bool VmRenderer::GetTemporalAAEnabled()
	{
		RENDERER_GET;
		return vrenderer->TemporalAAEnabled;
	}
	void VmRenderer::SetTemporalAADebugEnabled(bool enabled){}
	bool VmRenderer::GetTemporalAADebugEnabled()
	{
		RENDERER_GET;
		return vrenderer->TemporalAADebugEnabled;
	}
	void VmRenderer::SetFreezeCullingCameraEnabled(bool enabled){}
	bool VmRenderer::GetFreezeCullingCameraEnabled()
	{
		RENDERER_GET;
		return vrenderer->FreezeCullingCameraEnabled;
	}
	void VmRenderer::SetVXGIEnabled(bool enabled){}
	bool VmRenderer::GetVXGIEnabled()
	{
		RENDERER_GET;
		return vrenderer->VXGIEnabled;
	}
	void VmRenderer::SetVXGIReflectionsEnabled(bool enabled){}
	bool VmRenderer::GetVXGIReflectionsEnabled()
	{
		RENDERER_GET;
		return vrenderer->VXGIReflectionsEnabled;
	}
	void VmRenderer::SetGameSpeed(float value){}
	float VmRenderer::GetGameSpeed()
	{
		RENDERER_GET;
		return vrenderer->GameSpeed;
	}
	void VmRenderer::SetRaytraceBounceCount(uint32_t bounces){}
	uint32_t VmRenderer::GetRaytraceBounceCount()
	{
		RENDERER_GET;
		return vrenderer->RaytraceBounceCount;
	}
	void VmRenderer::SetRaytraceDebugBVHVisualizerEnabled(bool value){}
	bool VmRenderer::GetRaytraceDebugBVHVisualizerEnabled()
	{
		RENDERER_GET;
		return vrenderer->RaytraceDebugBVHVisualizerEnabled;
	}
	void VmRenderer::SetRaytracedShadowsEnabled(bool value){}
	bool VmRenderer::GetRaytracedShadowsEnabled()
	{
		RENDERER_GET;
		return vrenderer->RaytracedShadowsEnabled;
	}
	void VmRenderer::SetTessellationEnabled(bool value){}
	bool VmRenderer::GetTessellationEnabled()
	{
		RENDERER_GET;
		return vrenderer->TessellationEnabled;
	}
	void VmRenderer::SetDisableAlbedoMaps(bool value){}
	bool VmRenderer::IsDisableAlbedoMaps()
	{
		RENDERER_GET;
		return vrenderer->DisableAlbedoMaps;
	}
	void VmRenderer::SetForceDiffuseLighting(bool value){}
	bool VmRenderer::IsForceDiffuseLighting()
	{
		RENDERER_GET;
		return vrenderer->ForceDiffuseLighting;
	}
	void VmRenderer::SetScreenSpaceShadowsEnabled(bool value){}
	bool VmRenderer::GetScreenSpaceShadowsEnabled()
	{
		RENDERER_GET;
		return vrenderer->ScreenSpaceShadowsEnabled;
	}
	void VmRenderer::SetSurfelGIEnabled(bool value){}
	bool VmRenderer::GetSurfelGIEnabled()
	{
		RENDERER_GET;
		return vrenderer->SurfelGIEnabled;
	}
	void VmRenderer::SetSurfelGIDebugEnabled(SURFEL_DEBUG value){}
	VmRenderer::SURFEL_DEBUG VmRenderer::GetSurfelGIDebugEnabled()
	{
		RENDERER_GET;
		return (VmRenderer::SURFEL_DEBUG)vrenderer->SurfelGIDebugEnabled;
	}
	void VmRenderer::SetDDGIEnabled(bool value){}
	bool VmRenderer::GetDDGIEnabled()
	{
		RENDERER_GET;
		return vrenderer->DDGIEnabled;
	}
	void VmRenderer::SetDDGIDebugEnabled(bool value){}
	bool VmRenderer::GetDDGIDebugEnabled()
	{
		RENDERER_GET;
		return vrenderer->DDGIDebugEnabled;
	}
	void VmRenderer::SetDDGIRayCount(uint32_t value){}
	uint32_t VmRenderer::GetDDGIRayCount()
	{
		RENDERER_GET;
		return vrenderer->DDGIRayCount;
	}
	void VmRenderer::SetDDGIBlendSpeed(float value){}
	float VmRenderer::GetDDGIBlendSpeed()
	{
		RENDERER_GET;
		return vrenderer->DDGIBlendSpeed;
	}
	void VmRenderer::SetGIBoost(float value){}
	float VmRenderer::GetGIBoost()
	{
		RENDERER_GET;
		return vrenderer->GIBoost;
	}
#pragma endregion

#pragma region // VmCamera
	void VmCamera::SetPose(const float pos[3], const float view[3], const float up[3])
	{
		//wi::jobsystem::context ctx;
		//wi::jobsystem::Execute(ctx, [&](wi::jobsystem::JobArgs args) {
			CameraComponent* camComponent = sceneManager.GetEngineComp<CameraComponent>(componentVID);
			TransformComponent* transform = sceneManager.GetEngineComp<TransformComponent>(componentVID);
			if (!camComponent || !renderer || !transform) return;

			// up vector correction
			XMVECTOR _eye = XMLoadFloat3((XMFLOAT3*)pos);
			XMVECTOR _view = XMVector3Normalize(XMLoadFloat3((XMFLOAT3*)view));
			XMVECTOR _up = XMLoadFloat3((XMFLOAT3*)up);
			XMVECTOR _right = XMVector3Cross(_view, _up);
			_up = XMVector3Normalize(XMVector3Cross(_right, _view));

			// note the pose info is defined in WS
			XMMATRIX ws2cs = VZMatrixLookTo(_eye, _view, _up);
			XMMATRIX cs2ws = XMMatrixInverse(nullptr, ws2cs);

			HierarchyComponent* parent = sceneManager.GetEngineComp<HierarchyComponent>(componentVID);
			XMMATRIX parent2ws = XMMatrixIdentity();
			while (parent)
			{
				TransformComponent* transformParent = sceneManager.GetEngineComp<TransformComponent>(parent->parentID);
				parent = sceneManager.GetEngineComp<HierarchyComponent>(parent->parentID);
				if (transformParent)
				{
					parent2ws *= transformParent->GetLocalMatrix();
				}
			}

			XMMATRIX local = cs2ws * XMMatrixInverse(nullptr, parent2ws);
			transform->ClearTransform();
			transform->MatrixTransform(local); // ls2ws

			// note camComponent will be updated during the scene update
			// avoid to directly update the eye, at, and view, which are defined in WS
			timeStamp = std::chrono::high_resolution_clock::now();
		//});
		//wi::jobsystem::Wait(ctx);
	}
	void VmCamera::SetPerspectiveProjection(const float zNearP, const float zFarP, const float fovY, const float aspectRatio)
	{
		CameraComponent* camComponent = sceneManager.GetEngineComp<CameraComponent>(componentVID);
		if (!camComponent) return;
		// aspectRatio is W / H
		camComponent->CreatePerspective(aspectRatio, 1.f, zNearP, zFarP, fovY);
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmCamera::SetCanvasSize(const float w, const float h, const float dpi)
	{
		CameraComponent* camComponent = sceneManager.GetEngineComp<CameraComponent>(componentVID);
		if (!camComponent || !renderer) return;
		camComponent->CreatePerspective(w, h, camComponent->zNearP, camComponent->zFarP, camComponent->fov);
		((VzmRenderer*)renderer)->init(std::max((uint32_t)w, 16u), std::max((uint32_t)h, 16u), dpi);
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmCamera::GetPose(float pos[3], float view[3], float up[3])
	{
		//wi::jobsystem::context ctx;
		//wi::jobsystem::Execute(ctx, [&](wi::jobsystem::JobArgs args) {
			CameraComponent* camComponent = sceneManager.GetEngineComp<CameraComponent>(componentVID);
			if (!camComponent) return;

			TransformComponent* transform = sceneManager.GetEngineComp<TransformComponent>(componentVID);
			if (pos) *(XMFLOAT3*)pos = camComponent->Eye;
			if (view) *(XMFLOAT3*)view = camComponent->At;
			if (up) *(XMFLOAT3*)up = camComponent->Up;
		//});
		//wi::jobsystem::Wait(ctx);
	}
	void VmCamera::GetPerspectiveProjection(float* zNearP, float* zFarP, float* fovY, float* aspectRatio)
	{
		CameraComponent* camComponent = sceneManager.GetEngineComp<CameraComponent>(componentVID);
		if (!camComponent) return;
		if (zNearP) *zNearP = camComponent->zNearP;
		if (zFarP) *zFarP = camComponent->zFarP;
		if (fovY) *fovY = camComponent->fov;
		if (aspectRatio) *aspectRatio = camComponent->width / camComponent->height;
	}
	void VmCamera::GetCanvasSize(float* w, float* h, float* dpi)
	{
		if (!renderer) return;
		if (w) *w = (float)((VzmRenderer*)renderer)->width;
		if (h) *h = (float)((VzmRenderer*)renderer)->height;
		if (dpi) *dpi = ((VzmRenderer*)renderer)->dpi;
	}
#pragma endregion 

#pragma region // VmActor
#pragma endregion

#pragma region // VmLight
	void VmLight::SetColor(const float value[3])
	{
		COMP_GET(LightComponent, light, );
		light->color = *(XMFLOAT3*)value;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmLight::SetIntensity(const float value)
	{
		COMP_GET(LightComponent, light, );
		light->intensity = value;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmLight::SetRange(const float value)
	{
		COMP_GET(LightComponent, light, );
		light->range = value;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmLight::SetConeOuterRange(const float value)
	{
		COMP_GET(LightComponent, light, );
		light->outerConeAngle = value;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmLight::SetConeInnerRange(const float value)
	{
		COMP_GET(LightComponent, light, );
		light->innerConeAngle = value;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmLight::SetRadius(const float value)
	{
		COMP_GET(LightComponent, light, );
		light->radius = value;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmLight::SetLength(const float value)
	{
		COMP_GET(LightComponent, light, );
		light->length = value;
		timeStamp = std::chrono::high_resolution_clock::now();
	}

	void VmLight::SetCastShadow(const bool value)
	{
		COMP_GET(LightComponent, light, );
		light->SetCastShadow(value);
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmLight::SetVolumetricsEnabled(const bool value)
	{
		COMP_GET(LightComponent, light, );
		light->SetVolumetricsEnabled(value);
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmLight::SetVisualizerEnabled(const bool value)
	{
		COMP_GET(LightComponent, light, );
		light->SetVisualizerEnabled(value);
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmLight::SetStatic(const bool value)
	{
		COMP_GET(LightComponent, light, );
		light->SetStatic(value);
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmLight::SetVolumetricCloudsEnabled(const bool value)
	{
		COMP_GET(LightComponent, light, );
		light->SetVolumetricCloudsEnabled(value);
		timeStamp = std::chrono::high_resolution_clock::now();
	}

	bool VmLight::IsCastingShadow()
	{
		COMP_GET(LightComponent, light, false);
		return light->IsCastingShadow();
	}
	bool VmLight::IsVolumetricsEnabled()
	{
		COMP_GET(LightComponent, light, false);
		return light->IsVolumetricsEnabled();
	}
	bool VmLight::IsVisualizerEnabled()
	{
		COMP_GET(LightComponent, light, false);
		return light->IsVisualizerEnabled();
	}
	bool VmLight::IsStatic()
	{
		COMP_GET(LightComponent, light, false);
		return light->IsStatic();
	}
	bool VmLight::IsVolumetricCloudsEnabled()
	{
		COMP_GET(LightComponent, light, false);
		return light->IsVolumetricCloudsEnabled();
	}

	float VmLight::GetRange()
	{
		COMP_GET(LightComponent, light, -1.f);
		return light->GetRange();
	}
	void VmLight::SetType(const LightType val)
	{
		COMP_GET(LightComponent, light, );
		light->SetType((LightComponent::LightType)val);
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	VmLight::LightType VmLight::GetType()
	{
		COMP_GET(LightComponent, light, VmLight::LightType::DIRECTIONAL);
		return (VmLight::LightType)light->GetType();
	}
#pragma endregion 

#pragma region // VmCollider
	void VmCollider::SetCPUEnabled(const bool value)
	{
		COMP_GET(ColliderComponent, collider, );
		collider->SetCPUEnabled(value);
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmCollider::SetGPUEnabled(const bool value)
	{
		COMP_GET(ColliderComponent, collider, );
		collider->SetGPUEnabled(value);
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	bool VmCollider::IsCPUEnabled()
	{
		COMP_GET(ColliderComponent, collider, false);
		return collider->IsCPUEnabled();
	}
	bool VmCollider::IsGPUEnabled()
	{
		COMP_GET(ColliderComponent, collider, false);
		return collider->IsGPUEnabled();
	}
	float VmCollider::GetRadius()
	{
		COMP_GET(ColliderComponent, collider, 0);
		return collider->radius;
	}
	void VmCollider::SetRadius(const float r)
	{
		COMP_GET(ColliderComponent, collider, );
		collider->radius = r;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmCollider::GetOffset(float offset[3])
	{
		COMP_GET(ColliderComponent, collider, );
		*(XMFLOAT3*)offset = collider->offset;
	}
	void VmCollider::SetOffset(const float offset[3])
	{
		COMP_GET(ColliderComponent, collider, );
		collider->offset = *(XMFLOAT3*)offset;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmCollider::GetTail(float tail[3])
	{
		COMP_GET(ColliderComponent, collider, );
		*(XMFLOAT3*)tail = collider->tail;
	}
	void VmCollider::SetTail(const float tail[3])
	{
		COMP_GET(ColliderComponent, collider, );
		collider->tail = *(XMFLOAT3*)tail;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	VmCollider::Shape VmCollider::GetShape()
	{
		COMP_GET(ColliderComponent, collider, VmCollider::Shape::Sphere);
		return (VmCollider::Shape)collider->shape;
	}
	void VmCollider::SetShape(const Shape shape)
	{
		COMP_GET(ColliderComponent, collider, );
		collider->shape = (wi::scene::ColliderComponent::Shape)shape;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
#pragma endregion

#pragma region // VmEmitter
	void VmEmitter::Burst(int num)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->Burst(num);
	}
	void VmEmitter::Restart()
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->Restart();
	}
	void VmEmitter::GetStatistics(ParticleCounters& statistics)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		::ParticleCounters pc = emitter->GetStatistics();
		statistics = *(VmEmitter::ParticleCounters*)&pc;
	}
	VmEmitter::PARTICLESHADERTYPE VmEmitter::GetShaderType()
	{
		COMP_GET(EmittedParticleSystem, emitter, VmEmitter::PARTICLESHADERTYPE::SOFT);
		return (PARTICLESHADERTYPE)emitter->shaderType;
	}
	VID VmEmitter::GetMeshVid()
	{
		COMP_GET(EmittedParticleSystem, emitter, INVALID_VID);
		return emitter->meshID;
	}
	void VmEmitter::SetMeshVid(const VID vid)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->meshID = vid;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	float VmEmitter::GetFixedTimeStep()
	{
		COMP_GET(EmittedParticleSystem, emitter, -1.f);
		return emitter->FIXED_TIMESTEP;
	}
	void VmEmitter::SetFixedTimeStep(const float FIXED_TIMESTEP)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->FIXED_TIMESTEP = FIXED_TIMESTEP;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	float VmEmitter::GetSize()
	{
		COMP_GET(EmittedParticleSystem, emitter, 0.f);
		return emitter->size;
	}
	void VmEmitter::SetSize(const float size)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->size = size;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	float VmEmitter::GetRandomFactor()
	{
		COMP_GET(EmittedParticleSystem, emitter, 0.f);
		return emitter->random_factor;
	}
	void VmEmitter::SetRandomFactor(const float random_factor)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->random_factor = random_factor;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	float VmEmitter::GetNormalFactor()
	{
		COMP_GET(EmittedParticleSystem, emitter, 0.f);
		return emitter->normal_factor;
	}
	void VmEmitter::SetNormalFactor(const float normal_factor)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->normal_factor = normal_factor;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	float VmEmitter::GetCount()
	{
		COMP_GET(EmittedParticleSystem, emitter, 0.f);
		return emitter->count;
	}
	void VmEmitter::SetCount(const float count)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->count = count;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	float VmEmitter::GetLife()
	{
		COMP_GET(EmittedParticleSystem, emitter, 0.f);
		return emitter->life;
	}
	void VmEmitter::SetLife(const float life)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->life = life;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	float VmEmitter::GetRandomLife()
	{
		COMP_GET(EmittedParticleSystem, emitter, 0.f);
		return emitter->random_life;
	}
	void VmEmitter::SetRandomLife(const float random_life)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->random_life = random_life;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmEmitter::GetScaleXY(float* scaleX, float* scaleY)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		if (scaleX) *scaleX = emitter->scaleX;
		if (scaleY) *scaleY = emitter->scaleY;
	}
	void VmEmitter::SetScaleXY(const float scaleX, const float scaleY)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->scaleX = scaleX;
		emitter->scaleY = scaleY;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	float VmEmitter::GetRotation()
	{
		COMP_GET(EmittedParticleSystem, emitter, 0.f);
		return emitter->rotation;
	}
	void VmEmitter::SetRotation(const float rotation)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->rotation = rotation;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	float VmEmitter::GetMotionBlurAmount()
	{
		COMP_GET(EmittedParticleSystem, emitter, 0.f);
		return emitter->motionBlurAmount;
	}
	void VmEmitter::SetMotionBlurAmount(const float motionBlurAmount)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->motionBlurAmount = motionBlurAmount;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	float VmEmitter::GetMass()
	{
		COMP_GET(EmittedParticleSystem, emitter, 0.f);
		return emitter->mass;
	}
	void VmEmitter::SetMass(const float mass)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->mass = mass;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	float VmEmitter::GetRandomColor()
	{
		COMP_GET(EmittedParticleSystem, emitter, 0.f);
		return emitter->random_color;
	}
	void VmEmitter::SetRandomColor(const float random_color)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->random_color = random_color;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmEmitter::GetVelocity(float velocity[3])
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		*(XMFLOAT3*)velocity = emitter->velocity;
	}
	void VmEmitter::SetVelocity(const float velocity[3])
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->velocity = *(XMFLOAT3*)velocity;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmEmitter::GetGravity(float gravity[3])
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		*(XMFLOAT3*)gravity = emitter->gravity;
	}
	void VmEmitter::SetGravity(const float gravity[3])
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->gravity = *(XMFLOAT3*)gravity;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	float VmEmitter::GetDrag()
	{
		COMP_GET(EmittedParticleSystem, emitter, 0.f);
		return emitter->drag;
	}
	void VmEmitter::SetDrag(const float drag)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->drag = drag;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	float VmEmitter::GetRestitution()
	{
		COMP_GET(EmittedParticleSystem, emitter, 0.f);
		return emitter->restitution;
	}
	void VmEmitter::SetRestitution(const float restitution)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->restitution = restitution;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmEmitter::GetSPHProps(float* SPH_h, float* SPH_K, float* SPH_p0, float* SPH_e)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		if (SPH_h) *SPH_h = emitter->SPH_h;
		if (SPH_K) *SPH_K = emitter->SPH_K;
		if (SPH_p0) *SPH_p0 = emitter->SPH_p0;
		if (SPH_e) *SPH_e = emitter->SPH_e;
	}
	void VmEmitter::SetSPHProps(const float SPH_h, const float SPH_K, const float SPH_p0, const float SPH_e)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->SPH_h = SPH_h;
		emitter->SPH_K = SPH_K;
		emitter->SPH_p0 = SPH_p0;
		emitter->SPH_e = SPH_e;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmEmitter::GetSpriteSheetProps(uint32_t* framesX, uint32_t* framesY, uint32_t* frameCount, uint32_t* frameStart)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		if (framesX) *framesX = emitter->framesX;
		if (framesY) *framesY = emitter->framesY;
		if (frameCount) *frameCount = emitter->frameCount;
		if (frameStart) *frameStart = emitter->frameStart;
	}
	void VmEmitter::SetSpriteSheetProps(const uint32_t framesX, const uint32_t framesY, const uint32_t frameCount, const uint32_t frameStart)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->framesX = framesX;
		emitter->framesY = framesY;
		emitter->frameCount = frameCount;
		emitter->frameStart = frameStart;
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmEmitter::SetMaxParticleCount(uint32_t value)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->SetMaxParticleCount(value);
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	uint32_t VmEmitter::GetMaxParticleCount()
	{
		COMP_GET(EmittedParticleSystem, emitter, 0u);
		return emitter->GetMaxParticleCount();
	}
	uint64_t VmEmitter::GetMemorySizeInBytes()
	{
		COMP_GET(EmittedParticleSystem, emitter, 0u);
		return emitter->GetMemorySizeInBytes();
	}
	bool VmEmitter::IsDebug()
	{
		COMP_GET(EmittedParticleSystem, emitter, false);
		return emitter->IsDebug();
	}
	bool VmEmitter::IsPaused()
	{
		COMP_GET(EmittedParticleSystem, emitter, false);
		return emitter->IsPaused();
	}
	bool VmEmitter::IsSorted()
	{
		COMP_GET(EmittedParticleSystem, emitter, false);
		return emitter->IsSorted();
	}
	bool VmEmitter::IsDepthCollisionEnabled()
	{
		COMP_GET(EmittedParticleSystem, emitter, false);
		return emitter->IsDepthCollisionEnabled();
	}
	bool VmEmitter::IsSPHEnabled()
	{
		COMP_GET(EmittedParticleSystem, emitter, false);
		return emitter->IsSPHEnabled();
	}
	bool VmEmitter::IsVolumeEnabled()
	{
		COMP_GET(EmittedParticleSystem, emitter, false);
		return emitter->IsVolumeEnabled();
	}
	bool VmEmitter::IsFrameBlendingEnabled()
	{
		COMP_GET(EmittedParticleSystem, emitter, false);
		return emitter->IsFrameBlendingEnabled();
	}
	bool VmEmitter::IsCollidersDisabled()
	{
		COMP_GET(EmittedParticleSystem, emitter, false);
		return emitter->IsCollidersDisabled();
	}
	bool VmEmitter::IsTakeColorFromMesh()
	{
		COMP_GET(EmittedParticleSystem, emitter, false);
		return emitter->IsTakeColorFromMesh();
	}
	void VmEmitter::SetDebug(const bool value)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->SetDebug(value);
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmEmitter::SetPaused(const bool value)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->SetPaused(value);
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmEmitter::SetSorted(const bool value)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->SetSorted(value);
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmEmitter::SetDepthCollisionEnabled(const bool value)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->SetDepthCollisionEnabled(value);
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmEmitter::SetSPHEnabled(const bool value)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->SetSPHEnabled(value);
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmEmitter::SetVolumeEnabled(const bool value)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->SetVolumeEnabled(value);
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmEmitter::SetFrameBlendingEnabled(const bool value)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->SetFrameBlendingEnabled(value);
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmEmitter::SetCollidersDisabled(const bool value)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->SetCollidersDisabled(value);
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmEmitter::SetTakeColorFromMesh(const bool value)
	{
		COMP_GET(EmittedParticleSystem, emitter, );
		emitter->SetTakeColorFromMesh(value);
		timeStamp = std::chrono::high_resolution_clock::now();
	}
#pragma endregion

#pragma region // VmWeather
	void VmWeather::SetWeatherPreset(const uint32_t index)
	{
		VzmScene* scene = sceneManager.GetScene(componentVID);
		if (scene == nullptr)
		{
			return;
		}

		if (scene->weathers.GetComponentArray().size() <= index)
		{
			return;
		}
		scene->weather = scene->weathers.GetComponentArray()[index];
		timeStamp = std::chrono::high_resolution_clock::now();
	}

#define GetWeatherParam(IS_NAME) VzmScene* scene = sceneManager.GetScene(componentVID); \
	if (scene == nullptr) return false;  return scene->weather.IS_NAME();

	bool VmWeather::IsOceanEnabled()
	{
		GetWeatherParam(IsOceanEnabled);
	}
	bool VmWeather::IsRealisticSky()
	{
		GetWeatherParam(IsRealisticSky);
	}
	bool VmWeather::IsVolumetricClouds()
	{
		GetWeatherParam(IsVolumetricClouds);
	}
	bool VmWeather::IsHeightFog()
	{
		GetWeatherParam(IsHeightFog);
	}
	bool VmWeather::IsVolumetricCloudsCastShadow()
	{
		GetWeatherParam(IsVolumetricCloudsCastShadow);
	}
	bool VmWeather::IsOverrideFogColor()
	{
		GetWeatherParam(IsOverrideFogColor);
	}
	bool VmWeather::IsRealisticSkyAerialPerspective()
	{
		GetWeatherParam(IsRealisticSkyAerialPerspective);
	}
	bool VmWeather::IsRealisticSkyHighQuality()
	{
		GetWeatherParam(IsRealisticSkyHighQuality);
	}
	bool VmWeather::IsRealisticSkyReceiveShadow()
	{
		GetWeatherParam(IsRealisticSkyReceiveShadow);
	}
	bool VmWeather::IsVolumetricCloudsReceiveShadow()
	{
		GetWeatherParam(IsVolumetricCloudsReceiveShadow);
	}

#define GetWeatherComp  VzmScene* scene = sceneManager.GetScene(componentVID); \
	WeatherComponent* weather = nullptr; \
	if (scene != nullptr) { weather = &scene->weather; } else { COMP_GET(WeatherComponent, w, ); weather = w; } 

#define SetWeatherEnabled(FN_NAME) GetWeatherComp; weather->FN_NAME(value); timeStamp = std::chrono::high_resolution_clock::now();

	void VmWeather::SetOceanEnabled(const bool value)
	{
		SetWeatherEnabled(SetOceanEnabled);
	}
	void VmWeather::SetRealisticSky(const bool value)
	{
		SetWeatherEnabled(SetRealisticSky);
	}
	void VmWeather::SetVolumetricClouds(const bool value)
	{
		SetWeatherEnabled(SetVolumetricClouds);
	}
	void VmWeather::SetHeightFog(const bool value)
	{
		SetWeatherEnabled(SetHeightFog);
	}
	void VmWeather::SetVolumetricCloudsCastShadow(const bool value)
	{
		SetWeatherEnabled(SetVolumetricCloudsCastShadow);
	}
	void VmWeather::SetOverrideFogColor(const bool value)
	{
		SetWeatherEnabled(SetOverrideFogColor);
	}
	void VmWeather::SetRealisticSkyAerialPerspective(const bool value)
	{
		SetWeatherEnabled(SetRealisticSkyAerialPerspective);
	}
	void VmWeather::SetRealisticSkyHighQuality(const bool value)
	{
		SetWeatherEnabled(SetRealisticSkyHighQuality);
	}
	void VmWeather::SetRealisticSkyReceiveShadow(const bool value)
	{
		SetWeatherEnabled(SetRealisticSkyReceiveShadow);
	}
	void VmWeather::SetVolumetricCloudsReceiveShadow(const bool value)
	{
		SetWeatherEnabled(SetVolumetricCloudsReceiveShadow);
	}

#define SetWeatherParamT(PARAM, TYPE) GetWeatherComp; weather->PARAM = *(TYPE*)value; timeStamp = std::chrono::high_resolution_clock::now();
#define SetWeatherParam(PARAM) GetWeatherComp; weather->PARAM = value; 	timeStamp = std::chrono::high_resolution_clock::now();

	void VmWeather::SetSunColor(const float value[3])
	{
		SetWeatherParamT(sunColor, XMFLOAT3);
	}
	void VmWeather::SetDirectionColor(const float value[3])
	{
		SetWeatherParamT(sunDirection, XMFLOAT3);
	}
	void VmWeather::SetSkyExposure(const float value)
	{
		SetWeatherParam(skyExposure);
	}
	void VmWeather::SetHorizonColor(const float value[3])
	{
		SetWeatherParamT(horizon, XMFLOAT3);
	}
	void VmWeather::SetZenithColor(const float value[3])
	{
		SetWeatherParamT(zenith, XMFLOAT3);
	}
	void VmWeather::SetAmbient(const float value[3])
	{
		SetWeatherParamT(ambient, XMFLOAT3);
	}
	void VmWeather::SetFogStart(const float value)
	{
		SetWeatherParam(fogStart);
	}
	void VmWeather::SetFogDensity(const float value)
	{
		SetWeatherParam(fogDensity);
	}
	void VmWeather::SetFogHightStart(const float value)
	{
		SetWeatherParam(fogHeightStart);
	}
	void VmWeather::SetFogHightEnd(const float value)
	{
		SetWeatherParam(fogHeightEnd);
	}
	void VmWeather::SetWindDirection(const float value[3])
	{
		SetWeatherParamT(windDirection, XMFLOAT3);
	}
	void VmWeather::SetWindowRandomness(const float value)
	{
		SetWeatherParam(windRandomness);
	}
	void VmWeather::SetWindWaveSize(const float value)
	{
		SetWeatherParam(windWaveSize);
	}
	void VmWeather::SetWindSpeed(const float value)
	{
		SetWeatherParam(windSpeed);
	}
	void VmWeather::SetStars(const float value)
	{
		SetWeatherParam(stars);
	}
	void VmWeather::SetGravity(const float value[3])
	{
		SetWeatherParamT(gravity, XMFLOAT3);
	}
	void VmWeather::SetSkyRotation(const float value)
	{
		SetWeatherParam(sky_rotation);
	}
	void VmWeather::SetRainAmount(const float value)
	{
		SetWeatherParam(rain_amount);
	}
	void VmWeather::SetRainLength(const float value)
	{
		SetWeatherParam(rain_length);
	}
	void VmWeather::SetRainSpeed(const float value)
	{
		SetWeatherParam(rain_speed);
	}
	void VmWeather::SetRainScale(const float value)
	{
		SetWeatherParam(rain_scale);
	}
	void VmWeather::SetRainSplashScale(const float value)
	{
		SetWeatherParam(rain_splash_scale);
	}
	void VmWeather::SetRainColor(const float value[4])
	{
		SetWeatherParamT(rain_color, XMFLOAT4);
	}
#pragma endregion

#pragma region // VmAnimation
	bool VmAnimation::IsPlaying()
	{
		COMP_GET(AnimationComponent, aniComponent, false);
		return aniComponent->IsPlaying();
	}
	bool VmAnimation::IsLooped()
	{
		COMP_GET(AnimationComponent, aniComponent, false);
		return aniComponent->IsLooped();
	}
	float VmAnimation::GetLength()
	{
		COMP_GET(AnimationComponent, aniComponent, 0);
		return aniComponent->GetLength();
	}
	bool VmAnimation::IsEnded()
	{
		COMP_GET(AnimationComponent, aniComponent, false);
		return aniComponent->IsEnded();
	}
	bool VmAnimation::IsRootMotion()
	{
		COMP_GET(AnimationComponent, aniComponent, false);
		return aniComponent->IsRootMotion();
	}
	void VmAnimation::Play()
	{
		COMP_GET(AnimationComponent, aniComponent, );
		aniComponent->Play();
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmAnimation::Pause()
	{
		COMP_GET(AnimationComponent, aniComponent, );
		aniComponent->Pause();
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmAnimation::Stop()
	{
		COMP_GET(AnimationComponent, aniComponent, );
		aniComponent->Stop();
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmAnimation::SetLooped(const bool value)
	{
		COMP_GET(AnimationComponent, aniComponent, );
		aniComponent->SetLooped(value);
		timeStamp = std::chrono::high_resolution_clock::now();
	}
	void VmAnimation::SetRootMotion(const bool value)
	{
		COMP_GET(AnimationComponent, aniComponent, );
		value ? aniComponent->RootMotionOn() : aniComponent->RootMotionOff();
		timeStamp = std::chrono::high_resolution_clock::now();
	}
#pragma endregion

	// Resource Pool
#pragma region // VmMesh
#pragma endregion

#pragma region // VmMaterial
#pragma endregion
}

namespace vzm
{
	struct SafeReleaseChecker
	{
		SafeReleaseChecker() {};
		bool destroyed = false;
		~SafeReleaseChecker()
		{
			if (!destroyed)
			{
				std::cout << "MUST CALL DeinitEngineLib before finishing the application!" << std::endl;
				DeinitEngineLib();
			}
			std::cout << "Safely finished ^^" << std::endl;
		};
	};
	static std::unique_ptr<SafeReleaseChecker> safeReleaseChecker;

	VZRESULT InitEngineLib(const std::string& coreName, const std::string& logFileName)
	{
		static bool initialized = false;
		if (initialized)
		{
			wi::backlog::post("Already initialized!");
			return VZ_WARNNING;
		}

		safeReleaseChecker = std::make_unique<SafeReleaseChecker>();

		auto ext_video = wi::resourcemanager::GetSupportedVideoExtensions();
		for (auto& x : ext_video)
		{
			filetypes[x] = FileType::VIDEO;
		}
		auto ext_sound = wi::resourcemanager::GetSupportedSoundExtensions();
		for (auto& x : ext_sound)
		{
			filetypes[x] = FileType::SOUND;
		}
		auto ext_image = wi::resourcemanager::GetSupportedImageExtensions();
		for (auto& x : ext_image)
		{
			filetypes[x] = FileType::IMAGE;
		}

		ParamMap<std::string> arguments;
		sceneManager.Initialize(arguments);

		// With this mode, file data for resources will be kept around. This allows serializing embedded resource data inside scenes
		wi::resourcemanager::SetMode(wi::resourcemanager::Mode::ALLOW_RETAIN_FILEDATA);

		wi::backlog::setFontColor(wi::Color(130, 210, 220, 255));

		// DOJO TO DO
		// ACTIVATED RENDERER CHECKER ==> this is for info display

		return VZ_OK;
	}

	VZRESULT DeinitEngineLib()
	{
		if (safeReleaseChecker.get() == nullptr)
		{
			wi::backlog::post("MUST CALL vzm::InitEngineLib before calling vzm::DeinitEngineLib()", backlog::LogLevel::Error);
			return VZ_WARNNING;
		}
		wi::jobsystem::ShutDown();
		// DOJO adds for explicit release of COM-based components
		wi::audio::Deinitialize(); // note audio is based on COM, so explicitly destruction is required!
		safeReleaseChecker->destroyed = true;
		return VZ_OK;
	}

	VID GetFirstVidByName(const std::string& name)
	{
		return sceneManager.GetFirstVidByName(name);
	}

	size_t GetVidsByName(const std::string& name, std::vector<VID>& vids)
	{
		return sceneManager.GetVidsByName(name, vids);
	}

	bool GetNameByVid(const VID vid, std::string& name)
	{
		name = sceneManager.GetNameByVid(vid);
		return name != "";
	}

	void RemoveComponent(const VID vid)
	{
		sceneManager.RemoveEntity(vid);
	}

	VID NewScene(const std::string& sceneName)
	{
		Scene* scene = sceneManager.GetFirstSceneByName(sceneName);
		if (scene != nullptr)
		{
			return INVALID_ENTITY;
		}

		return sceneManager.CreateSceneEntity(sceneName);
	}

	bool MoveToParent(const Entity entity, const Entity parentEntity, Scene* scene)
	{
		assert(entity != parentEntity);
		if (parentEntity != INVALID_ENTITY)
		{
			for (auto& entry : scene->componentLibrary.entries)
			{
				if (entry.second.component_manager->Contains(parentEntity))
				{
					scene->hierarchy.Create(entity).parentID = parentEntity;
					return true;
				}
			}
		}
		return false;
	}

	VID NewSceneComponent(const COMPONENT_TYPE compType, const VID sceneVid, const std::string& compName, const VID parentVid, VmBaseComponent** baseComp)
	{
		VzmScene* scene = sceneManager.GetScene(sceneVid);
		if (scene == nullptr)
		{
			return INVALID_ENTITY;
		}
		Entity ett = INVALID_ENTITY;
		switch (compType)
		{
		case COMPONENT_TYPE::ACTOR:
		{
			ett = scene->Entity_CreateObject(compName);
			VmActor* vmActor = sceneManager.CreateVmComp<VmActor>(ett);
			if (baseComp) *baseComp = vmActor;
			break;
		}
		case COMPONENT_TYPE::CAMERA:
		{
			ett = scene->Entity_CreateCamera(compName, CANVAS_INIT_W, CANVAS_INIT_H);
			VmCamera* vmCam = sceneManager.CreateVmComp<VmCamera>(ett);

			CameraComponent* camComponent = scene->cameras.GetComponent(ett);
			VzmRenderer* renderer = sceneManager.CreateRenderer(ett);
			renderer->scene = scene;
			assert(renderer->camera == camComponent);

			vmCam->compType = compType;
			vmCam->componentVID = ett;
			vmCam->renderer = (void*)renderer;
			renderer->UpdateVmCamera(vmCam);
			renderer->init(CANVAS_INIT_W, CANVAS_INIT_H, CANVAS_INIT_DPI);
			renderer->Load(); // Calls renderer->Start()
			if (baseComp) *baseComp = renderer->GetVmCamera();
			break;
		}
		case COMPONENT_TYPE::LIGHT:
		{
			ett = scene->Entity_CreateLight(compName);// , XMFLOAT3(0, 3, 0), XMFLOAT3(1, 1, 1), 2, 60);
			VmLight* vmLight = sceneManager.CreateVmComp<VmLight>(ett);
			if (baseComp) *baseComp = vmLight;
			break;
		}
		case COMPONENT_TYPE::EMITTER:
		{
			ett = scene->Entity_CreateEmitter(compName);
			VmEmitter* vmEmitter = sceneManager.CreateVmComp<VmEmitter>(ett);
			if (baseComp) *baseComp = vmEmitter;
			break;
		}
		case COMPONENT_TYPE::MATERIAL:
		{
			ett = scene->Entity_CreateMaterial(compName);
			VmMaterial* vmMat = sceneManager.CreateVmComp<VmMaterial>(ett);
			if (baseComp) *baseComp = vmMat;
			break;
		}
		case COMPONENT_TYPE::COLLIDER:
		{
			ett = CreateEntity();
			scene->names.Create(ett) = compName;
			scene->colliders.Create(ett);
			scene->transforms.Create(ett);
			VmCollider* vmCollider = sceneManager.CreateVmComp<VmCollider>(ett);
			if (baseComp) *baseComp = vmCollider;
			break;
		}
		case COMPONENT_TYPE::WEATHER:
		{
			ett = CreateEntity();
			scene->names.Create(ett) = compName;
			scene->weathers.Create(ett);
			VmWeather* vmWeather = sceneManager.CreateVmComp<VmWeather>(ett);
			if (baseComp) *baseComp = vmWeather;
			break;
		}
		case COMPONENT_TYPE::ANIMATION:
		default:
			return INVALID_ENTITY;
		}
		MoveToParent(ett, parentVid, scene);
		return ett;
	}

	VID AppendComponentTo(const VID vid, const VID parentVid)
	{
		HierarchyComponent* hierarchy = sceneManager.GetEngineComp<HierarchyComponent>(vid);
		if (hierarchy)
		{
			if (hierarchy->parentID == parentVid)
			{
				auto scenes = sceneManager.GetScenes();
				for (auto it = scenes->begin(); it != scenes->end(); it++)
				{
					VzmScene* scene = &it->second;
					if (scene->hierarchy.GetComponent(vid))
					{
						return scene->sceneVid;
					}
				}
				assert("There must be a scene containing the hierarchy entity");
			}
		}

		auto scenes = sceneManager.GetScenes();
		bool ret = false;
		for (auto it = scenes->begin(); it != scenes->end(); it++)
		{
			VzmScene* scene = &it->second;
			if (MoveToParent(vid, parentVid, scene))
			{
				return scene->sceneVid;
			}
		}
		return INVALID_VID;
	}

	VmBaseComponent* GetComponent(const COMPONENT_TYPE compType, const VID vid)
	{
		switch (compType)
		{
		case COMPONENT_TYPE::BASE: return sceneManager.GetVmComp<VmBaseComponent>(vid);
		case COMPONENT_TYPE::CAMERA: return sceneManager.GetVmComp<VmCamera>(vid);
		case COMPONENT_TYPE::ACTOR: return sceneManager.GetVmComp<VmActor>(vid);
		case COMPONENT_TYPE::LIGHT: return sceneManager.GetVmComp<VmLight>(vid);
		case COMPONENT_TYPE::EMITTER: return sceneManager.GetVmComp<VmEmitter>(vid);
		case COMPONENT_TYPE::WEATHER: return sceneManager.GetVmComp<VmWeather>(vid);
		case COMPONENT_TYPE::ANIMATION: return sceneManager.GetVmComp<VmAnimation>(vid);
		default: break;
		}
		return nullptr;
	}

	uint32_t GetSceneCompoenentVids(const COMPONENT_TYPE compType, const VID sceneVid, std::vector<VID>& vids)
	{
		Scene* scene = sceneManager.GetScene(sceneVid);
		if (scene == nullptr)
		{
			return 0u;
		}
		switch (compType)
		{
		case COMPONENT_TYPE::CAMERA: vids = scene->cameras.GetEntityArray(); break;
		case COMPONENT_TYPE::ACTOR: vids = scene->objects.GetEntityArray(); break;
		case COMPONENT_TYPE::LIGHT: vids = scene->lights.GetEntityArray(); break;
		case COMPONENT_TYPE::EMITTER: vids = scene->emitters.GetEntityArray(); break;
		case COMPONENT_TYPE::WEATHER: vids = scene->weathers.GetEntityArray(); break;
		case COMPONENT_TYPE::ANIMATION: vids = scene->animations.GetEntityArray(); break;
		default: break;
		}
		return (uint32_t)vids.size();
	}

	VmWeather* GetSceneActivatedWeather(const VID sceneVid)
	{
		VzmScene* scene = sceneManager.GetScene(sceneVid);
		if (scene == nullptr)
		{
			return nullptr;
		}
		return &scene->vmWeather;
	}

	void LoadFileIntoNewSceneAsync(const std::string& file, const std::string& rootName, const std::string& sceneName, const std::function<void(VID sceneVid, VID rootVid)>& callback)
	{
		struct loadingJob
		{
			wi::Timer timer;
			wi::jobsystem::context ctx;
			// input param
			std::string rootName;
			std::string sceneName;
			std::string file;
			std::function<void(VID sceneVid, VID rootVid)> callback;

			bool isFinished = false;
		};

		static uint32_t jobIndex = 0;
		static std::map<uint32_t, loadingJob> loadingJobStore;
		bool isBusy = false;
		for (auto& it : loadingJobStore)
		{
			if (!it.second.isFinished)
			{
				isBusy = true;
				break;
			}
		}
		if (!isBusy)
		{
			loadingJobStore.clear();
			jobIndex = 0;
		}

		loadingJob& jobInfo = loadingJobStore[jobIndex++];
		jobInfo.file = file;
		jobInfo.rootName = rootName;
		jobInfo.sceneName = sceneName;
		jobInfo.callback = callback;

		wi::backlog::post("");
		wi::jobsystem::Execute(jobInfo.ctx, [&](wi::jobsystem::JobArgs args) {
			VID rootVid = INVALID_VID;
			VID sceneVid = LoadFileIntoNewScene(jobInfo.file, jobInfo.rootName, jobInfo.sceneName, &rootVid);
			if (jobInfo.callback != nullptr)
			{
				jobInfo.callback(sceneVid, rootVid);
			}
			});
		std::thread([&jobInfo] {
			wi::jobsystem::Wait(jobInfo.ctx);
			wi::backlog::post("\n[vzm::LoadMeshModelAsync] GLTF Loading (" + std::to_string((int)std::round(jobInfo.timer.elapsed())) + " ms)");
			}).detach();
	}

	VID LoadFileIntoNewScene(const std::string& file, const std::string& rootName, const std::string& sceneName, VID* rootVid)
	{
		VID sid = sceneManager.CreateSceneEntity(sceneName);
		VzmScene* scene = sceneManager.GetScene(sid);
		if (scene == nullptr)
		{
			return INVALID_ENTITY;
		}

		Entity rootEntity = INVALID_ENTITY;
		// loading.. with file

		std::string extension = wi::helper::toUpper(wi::helper::GetExtensionFromFileName(file));
		FileType type = FileType::INVALID;
		auto it = filetypes.find(extension);
		if (it != filetypes.end())
		{
			type = it->second;
		}
		if (type == FileType::INVALID)
			return INVALID_ENTITY;

		if (type == FileType::OBJ) // wavefront-obj
		{
			rootEntity = ImportModel_OBJ(file, *scene);	// reassign transform components
		}
		else if (type == FileType::GLTF || type == FileType::GLB || type == FileType::VRM) // gltf, vrm
		{
			rootEntity = ImportModel_GLTF(file, *scene);
		}
		scene->names.GetComponent(rootEntity)->name = rootName;

		if (rootVid) *rootVid = rootEntity;

		return sid;
	}

	VZRESULT MergeScenes(const VID srcSceneVid, const VID dstSceneVid)
	{
		Scene* srcScene = sceneManager.GetScene(srcSceneVid);
		Scene* dstScene = sceneManager.GetScene(dstSceneVid);
		if (srcScene == nullptr || dstScene == nullptr)
		{
			wi::backlog::post("Invalid Scene", wi::backlog::LogLevel::Error);
			return VZ_FAIL;
		}

		// base
		wi::vector<Entity> transformEntities = srcScene->transforms.GetEntityArray();

		// camera wirh renderer
		wi::vector<Entity> camEntities = srcScene->cameras.GetEntityArray();
		// actors
		wi::vector<Entity> aniEntities = srcScene->animations.GetEntityArray();
		wi::vector<Entity> objEntities = srcScene->objects.GetEntityArray();
		wi::vector<Entity> lightEntities = srcScene->lights.GetEntityArray();
		wi::vector<Entity> emitterEntities = srcScene->emitters.GetEntityArray();

		// resources
		wi::vector<Entity> meshEntities = srcScene->meshes.GetEntityArray();
		wi::vector<Entity> materialEntities = srcScene->materials.GetEntityArray();

		dstScene->Merge(*srcScene);

		for (Entity& ett : camEntities)
		{
			VmCamera* vmCam = sceneManager.CreateVmComp<VmCamera>(ett);
			CameraComponent* camComponent = dstScene->cameras.GetComponent(ett);
			VzmRenderer* renderer = sceneManager.CreateRenderer(ett);
			renderer->scene = dstScene;
			assert(renderer->camera == camComponent);

			vmCam->compType = COMPONENT_TYPE::CAMERA;
			vmCam->componentVID = ett;
			vmCam->renderer = (void*)renderer;
			renderer->UpdateVmCamera(vmCam);
			renderer->init(CANVAS_INIT_W, CANVAS_INIT_H, CANVAS_INIT_DPI);
			renderer->Load(); // Calls renderer->Start()
		}

		// actors
		{
			for (Entity& ett : aniEntities)
			{
				sceneManager.CreateVmComp<VmAnimation>(ett);
			}
			for (Entity& ett : objEntities)
			{
				sceneManager.CreateVmComp<VmActor>(ett);
			}
			for (Entity& ett : lightEntities)
			{
				sceneManager.CreateVmComp<VmLight>(ett);
			}
			for (Entity& ett : emitterEntities)
			{
				sceneManager.CreateVmComp<VmEmitter>(ett);
			}
		}

		// must be posterior to actors
		for (Entity& ett : transformEntities)
		{
			if (!sceneManager.GetVmComp<VmBaseComponent>(ett))
			{
				sceneManager.CreateVmComp<VmBaseComponent>(ett);
			}
		}

		// resources
		{
			for (Entity& ett : meshEntities)
			{
				sceneManager.CreateVmComp<VmMesh>(ett);
			}
			for (Entity& ett : materialEntities)
			{
				sceneManager.CreateVmComp<VmMaterial>(ett);
			}
		}
		//static Scene scene_resPool;
		//scene_resPool.meshes.Merge(dstScene->meshes);
		//scene_resPool.Update(0);
		//int gg = 0;
		return VZ_OK;
	}

	VZRESULT Render(const VID camVid, const bool updateScene)
	{
		VzmRenderer* renderer = sceneManager.GetRenderer(camVid);
		if (renderer == nullptr)
		{
			return VZ_FAIL;
		}

		wi::font::UpdateAtlas(renderer->GetDPIScaling());

		renderer->UpdateVmCamera();

		// DOJO TO DO : CHECK updateScene across cameras belonging to a scene and force to use a oldest one...
		renderer->setSceneUpdateEnabled(updateScene || renderer->FRAMECOUNT == 0);
		if (!updateScene)
		{
			renderer->scene->camera = *renderer->camera;
		}

		if (!wi::initializer::IsInitializeFinished())
		{
			// Until engine is not loaded, present initialization screen...
			renderer->WaitRender();
			return VZ_JOB_WAIT;
		}

		if (profileFrameFinished)
		{
			profileFrameFinished = false;
			wi::profiler::BeginFrame();
		}

		VzmScene* scene = (VzmScene*)renderer->scene;

		{
			// for frame info.
			renderer->deltaTime = float(std::max(0.0, renderer->timer.record_elapsed_seconds()));
			const float target_deltaTime = 1.0f / renderer->targetFrameRate;
			if (renderer->framerate_lock && renderer->deltaTime < target_deltaTime)
			{
				wi::helper::QuickSleep((target_deltaTime - renderer->deltaTime) * 1000);
				renderer->deltaTime += float(std::max(0.0, renderer->timer.record_elapsed_seconds()));
			}

			scene->deltaTime = float(std::max(0.0, scene->timer.record_elapsed_seconds()));
		}


		//wi::input::Update(nullptr, *renderer);
		// Wake up the events that need to be executed on the main thread, in thread safe manner:
		wi::eventhandler::FireEvent(wi::eventhandler::EVENT_THREAD_SAFE_POINT, 0);
		renderer->fadeManager.Update(renderer->deltaTime);

		renderer->PreUpdate(); // current to previous

		// Fixed time update:
		{
			auto range = wi::profiler::BeginRangeCPU("Fixed Update");
			if (renderer->frameskip)
			{
				renderer->deltaTimeAccumulator += renderer->deltaTime;
				if (renderer->deltaTimeAccumulator > 10)
				{
					// application probably lost control, fixed update would take too long
					renderer->deltaTimeAccumulator = 0;
				}

				const float targetFrameRateInv = 1.0f / renderer->targetFrameRate;
				while (renderer->deltaTimeAccumulator >= targetFrameRateInv)
				{
					renderer->FixedUpdate();
					renderer->deltaTimeAccumulator -= targetFrameRateInv;
				}
			}
			else
			{
				renderer->FixedUpdate();
			}
			wi::profiler::EndRange(range); // Fixed Update
		}
		{
			// use scene->deltaTime
			auto range = wi::profiler::BeginRangeCPU("Update");
			wi::backlog::Update(*renderer, scene->deltaTime);
			renderer->Update(scene->deltaTime);
			renderer->PostUpdate();
			wi::profiler::EndRange(range); // Update

			// we ill use the separate framecount for each renderer (not global device)
			//
			renderer->FRAMECOUNT++;
			renderer->frameCB.frame_count = (uint)renderer->FRAMECOUNT;
			//renderer->frameCB.delta_time = renderer->deltaTime;
			// note here frameCB's time is computed based on the scene timeline
			//renderer->frameCB.time_previous = renderer->frameCB.time;
			//renderer->frameCB.time = scene->deltaTimeAccumulator;
		}

		{
			auto range = wi::profiler::BeginRangeCPU("Render");
			scene->dt = renderer->deltaTime;
			renderer->Render();
			wi::profiler::EndRange(range); // Render
		}
		renderer->RenderFinalize();

		return VZ_OK;
	}

	void ReloadShader()
	{
		wi::renderer::ReloadShaders();
	}

	VID DisplayEngineProfiling(const int w, const int h, const bool displayProfile, const bool displayEngineStates)
	{
		static bool isFirstCall = true;
		static VID sceneVid = sceneManager.CreateSceneEntity("__VZM_ENGINE_INTERNAL__");
		VzmScene* sceneInternalState = sceneManager.GetScene(sceneVid);
		static Entity canvasEtt = sceneInternalState->Entity_CreateCamera("INFO_CANVAS", w, h);
		static VzmRenderer* sysInfoRenderer = sceneManager.CreateRenderer(canvasEtt);

		if (isFirstCall)
		{
			sysInfoRenderer->init(w, h, CANVAS_INIT_DPI);

			sysInfoRenderer->infoDisplay.active = true;
			sysInfoRenderer->infoDisplay.watermark = true;
			//sysInfoRenderer->infoDisplay.fpsinfo = true;
			//sysInfoRenderer->infoDisplay.resolution = true;
			//sysInfoRenderer->infoDisplay.colorspace = true;
			sysInfoRenderer->infoDisplay.device_name = true;
			sysInfoRenderer->infoDisplay.vram_usage = true;
			sysInfoRenderer->infoDisplay.heap_allocation_counter = true;

			sysInfoRenderer->DisplayProfile = true;
			wi::profiler::SetEnabled(true);

			{
				const float fadeSeconds = 0.f;
				wi::Color fadeColor = wi::Color(0, 0, 0, 255);
				// Fade manager will activate on fadeout
				sysInfoRenderer->fadeManager.Clear();
				sysInfoRenderer->fadeManager.Start(fadeSeconds, fadeColor, []() {
					sysInfoRenderer->Start();
					});

				sysInfoRenderer->fadeManager.Update(0); // If user calls ActivatePath without fadeout, it will be instant
			}
			isFirstCall = false;
		}

		sysInfoRenderer->camEntity = canvasEtt;
		sysInfoRenderer->width = w;
		sysInfoRenderer->height = h;
		sysInfoRenderer->UpdateVmCamera();

		sysInfoRenderer->setSceneUpdateEnabled(false);
		sysInfoRenderer->scene->camera = *sysInfoRenderer->camera;

		wi::font::UpdateAtlas(sysInfoRenderer->GetDPIScaling());

		if (!wi::initializer::IsInitializeFinished())
		{
			// Until engine is not loaded, present initialization screen...
			//sysInfoRenderer->WaitRender();
			return VZ_JOB_WAIT;
		}

		if (profileFrameFinished)
		{
			profileFrameFinished = false;
			wi::profiler::BeginFrame();
		}
		sysInfoRenderer->RenderFinalize(); // set profileFrameFinished to true inside

		return (VID)canvasEtt;
	}

	void* GetGraphicsSharedRenderTarget(const int camVid, const void* graphicsDev2, const void* srv_desc_heap2, const int descriptor_index, uint32_t* w, uint32_t* h)
	{
		VzmRenderer* renderer = sceneManager.GetRenderer(camVid);
		if (renderer == nullptr)
		{
			return nullptr;
		}

		if (w) *w = renderer->width;
		if (h) *h = renderer->height;

		wi::graphics::GraphicsDevice* graphicsDevice = wi::graphics::GetDevice();
		if (graphicsDevice == nullptr) return nullptr;
		//return graphicsDevice->OpenSharedResource(graphicsDev2, const_cast<wi::graphics::Texture*>(&renderer->GetRenderResult()));
		//return graphicsDevice->OpenSharedResource(graphicsDev2, &renderer->rtPostprocess);
		return graphicsDevice->OpenSharedResource(graphicsDev2, srv_desc_heap2, descriptor_index, const_cast<wi::graphics::Texture*>(&renderer->renderResult));
	}
}

#include "vzArcBall.h"

namespace vzm
{
	std::unordered_map<OrbitalControl*, arcball::ArcBall> map_arcballs;

	OrbitalControl::OrbitalControl()
	{
		map_arcballs[this] = arcball::ArcBall();
	}
	OrbitalControl::~OrbitalControl()
	{
		map_arcballs.erase(this);
	}

	void OrbitalControl::SetTargetCam(const VID _camVid, const float stage_center[3], const float stage_radius)
	{
		auto itr = map_arcballs.find(this);
		if (itr == map_arcballs.end())
			return wi::backlog::post("NOT AVAILABLE ARCBALL!", wi::backlog::LogLevel::Error);

		camVid = _camVid;
		arcball::ArcBall& arc_ball = itr->second;
		XMVECTOR _stage_center = XMLoadFloat3((XMFLOAT3*)stage_center);
		arc_ball.FitArcballToSphere(_stage_center, stage_radius);
		arc_ball.__is_set_stage = true;
	}
	void compute_screen_matrix(XMMATRIX& ps2ss, const float w, const float h)
	{
		XMMATRIX matTranslate = XMMatrixTranslation(1.f, -1.f, 0.f);
		XMMATRIX matScale = XMMatrixScaling(0.5f * w, -0.5f * h, 1.f);

		XMMATRIX matTranslateSampleModel = XMMatrixTranslation(-0.5f, -0.5f, 0.f);

		ps2ss = matTranslate * matScale * matTranslateSampleModel;
	}
	bool OrbitalControl::Start(const float pos_xy[2], const float sensitivity)
	{
		auto itr = map_arcballs.find(this);
		if (itr == map_arcballs.end())
		{
			return fail_ret("NOT AVAILABLE ARCBALL!");
		}
		arcball::ArcBall& arc_ball = itr->second;
		if (!arc_ball.__is_set_stage)
		{
			return fail_ret("NO INITIALIZATION IN THIS ARCBALL!");
		}

		CameraComponent* cam = sceneManager.GetEngineComp<CameraComponent>(camVid);
		TransformComponent* transform = sceneManager.GetEngineComp<TransformComponent>(camVid);
		if (cam == nullptr || transform == nullptr)
		{
			return fail_ret("NO VALID CAMERA!!");
		}

		// Orbital Camera
		// we assume the transform->world is not dirty state
		XMMATRIX matWorld = XMLoadFloat4x4(&transform->world);

		arcball::CameraState cam_pose;
		cam_pose.isPerspective = true;
		cam_pose.np = cam->zNearP;
		cam_pose.posCamera = cam->Eye;
		cam_pose.vecView = cam->At;
		cam_pose.vecUp = cam->Up;

		XMMATRIX ws2cs, cs2ps, ps2ss;
		ws2cs = cam->GetView();
		float aspect = (float)cam->canvas.width / (float)cam->canvas.height;
		cs2ps = VZMatrixPerspectiveFov(XM_PIDIV4, aspect, cam->zNearP, cam->zFarP);
		compute_screen_matrix(ps2ss, (float)cam->canvas.width, (float)cam->canvas.height);
		cam_pose.matWS2SS = ws2cs * cs2ps * ps2ss;
		cam_pose.matSS2WS = XMMatrixInverse(NULL, cam_pose.matWS2SS);

		arc_ball.StartArcball((float)pos_xy[0], (float)pos_xy[1], cam_pose, 10.f * sensitivity);

		return true;
	}
	bool OrbitalControl::Move(const float pos_xy[2])
	{
		auto itr = map_arcballs.find(this);
		if (itr == map_arcballs.end())
			return fail_ret("NOT AVAILABLE ARCBALL!");
		arcball::ArcBall& arc_ball = itr->second;
		if (!arc_ball.__is_set_stage)
			return fail_ret("NO INITIALIZATION IN THIS ARCBALL!");

		XMMATRIX mat_tr;
		arc_ball.MoveArcball(mat_tr, (float)pos_xy[0], (float)pos_xy[1], true);

		arcball::CameraState cam_pose_begin = arc_ball.GetCameraStateSetInStart();
		XMVECTOR vEye = XMVector3TransformCoord(XMLoadFloat3(&cam_pose_begin.posCamera), mat_tr);
		XMVECTOR vView = XMVector3TransformNormal(XMLoadFloat3(&cam_pose_begin.vecView), mat_tr);
		XMVECTOR vUp = XMVector3TransformNormal(XMLoadFloat3(&cam_pose_begin.vecUp), mat_tr);

		// Set Pose //...
		VmCamera* vCam = sceneManager.GetVmComp<VmCamera>(camVid);
		XMFLOAT3 pos, view, up;
		XMStoreFloat3(&pos, vEye);
		XMStoreFloat3(&view, XMVector3Normalize(vView));
		XMStoreFloat3(&up, XMVector3Normalize(vUp));
		vCam->SetPose(__FP pos, __FP view, __FP up);

		return true;
	}
	bool OrbitalControl::PanMove(const float pos_xy[2])
	{
		auto itr = map_arcballs.find(this);
		if (itr == map_arcballs.end())
			return fail_ret("NOT AVAILABLE ARCBALL!");
		arcball::ArcBall& arc_ball = itr->second;
		if (!arc_ball.__is_set_stage)
			return fail_ret("NO INITIALIZATION IN THIS ARCBALL!");

		arcball::CameraState cam_pose_begin = arc_ball.GetCameraStateSetInStart();

		XMMATRIX& mat_ws2ss = cam_pose_begin.matWS2SS;
		XMMATRIX& mat_ss2ws = cam_pose_begin.matSS2WS;

		VmCamera* vCam = sceneManager.GetVmComp<VmCamera>(camVid);

		if (!cam_pose_begin.isPerspective)
		{
			XMVECTOR pos_eye_ws = XMLoadFloat3(&cam_pose_begin.posCamera);
			XMVECTOR pos_eye_ss = XMVector3TransformCoord(pos_eye_ws, mat_ws2ss);
		
			XMFLOAT3 v = XMFLOAT3((float)pos_xy[0] - arc_ball.__start_x, (float)pos_xy[1] - arc_ball.__start_y, 0);
			XMVECTOR diff_ss = XMLoadFloat3(&v);
		
			pos_eye_ss = pos_eye_ss - diff_ss; // Think Panning! reverse camera moving
			pos_eye_ws = XMVector3TransformCoord(pos_eye_ss, mat_ss2ws);

			XMFLOAT3 pos;
			XMStoreFloat3(&pos, pos_eye_ws);
			vCam->SetPose(__FP pos, __FP cam_pose_begin.vecView, __FP cam_pose_begin.vecUp);
		}
		else
		{
			XMFLOAT3 f = XMFLOAT3((float)pos_xy[0], (float)pos_xy[1], 0);
			XMVECTOR pos_cur_ss = XMLoadFloat3(&f);
			f = XMFLOAT3(arc_ball.__start_x, arc_ball.__start_y, 0);
			XMVECTOR pos_old_ss = XMLoadFloat3(&f);
			XMVECTOR pos_cur_ws = XMVector3TransformCoord(pos_cur_ss, mat_ss2ws);
			XMVECTOR pos_old_ws = XMVector3TransformCoord(pos_old_ss, mat_ss2ws);
			XMVECTOR diff_ws = pos_cur_ws - pos_old_ws;
		
			if (XMVectorGetX(XMVector3Length(diff_ws)) < DBL_EPSILON)
			{
				vCam->SetPose(__FP cam_pose_begin.posCamera, __FP cam_pose_begin.vecView, __FP cam_pose_begin.vecUp);
				return true;
			}
		
			//cout << "-----0> " << glm::length(diff_ws) << endl;
			//cout << "-----1> " << pos.x << ", " << pos.y << endl;
			//cout << "-----2> " << arc_ball.__start_x << ", " << arc_ball.__start_y << endl;
			XMVECTOR pos_center_ws = arc_ball.GetCenterStage();
			XMVECTOR vec_eye2center_ws = pos_center_ws - XMLoadFloat3(&cam_pose_begin.posCamera);
		
			float panningCorrected = XMVectorGetX(XMVector3Length(diff_ws)) * XMVectorGetX(XMVector3Length(vec_eye2center_ws)) / cam_pose_begin.np;
		
			diff_ws = XMVector3Normalize(diff_ws);
			XMVECTOR v = XMLoadFloat3(&cam_pose_begin.posCamera) - XMVectorScale(diff_ws, panningCorrected);
			XMFLOAT3 pos;
			XMStoreFloat3(&pos, v);
			vCam->SetPose(__FP pos, __FP cam_pose_begin.vecView, __FP cam_pose_begin.vecUp);
		}
		return true;
	}
}
