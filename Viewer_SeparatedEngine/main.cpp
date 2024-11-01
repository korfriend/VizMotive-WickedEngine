// Dear ImGui: standalone example application for DirectX 12
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// Important: to compile on 32-bit systems, the DirectX12 backend requires code to be compiled with '#define ImTextureID ImU64'.
// This is because we need ImTextureID to carry a 64-bit value and by default ImTextureID is defined as void*.
// This define is set in the example .vcxproj file and need to be replicated in your app or by adding it to your imconfig.h file.

#include "VizEngineAPIs.h"

#include "wrl/client.h"
using namespace Microsoft::WRL;

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <tchar.h>

#include <spdlog/spdlog.h>

// math using GLM
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/vector_angle.hpp"

#define GLM_F3 *(glm::fvec3*)

#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

struct FrameContext
{
	ID3D12CommandAllocator* CommandAllocator;
	UINT64                  FenceValue;
};

// Data
static int const                    NUM_FRAMES_IN_FLIGHT = 3;
static FrameContext                 g_frameContext[NUM_FRAMES_IN_FLIGHT] = {};
static UINT                         g_frameIndex = 0;

static int const                    NUM_BACK_BUFFERS = 3;
static ID3D12Device* g_pd3dDevice = nullptr;
static ID3D12DescriptorHeap* g_pd3dRtvDescHeap = nullptr;
static ID3D12DescriptorHeap* g_pd3dSrvDescHeap = nullptr;
static ID3D12CommandQueue* g_pd3dCommandQueue = nullptr;
static ID3D12GraphicsCommandList* g_pd3dCommandList = nullptr;
static ID3D12Fence* g_fence = nullptr;
static HANDLE                       g_fenceEvent = nullptr;
static UINT64                       g_fenceLastSignaledValue = 0;
static IDXGISwapChain3* g_pSwapChain = nullptr;
static HANDLE                       g_hSwapChainWaitableObject = nullptr;
static ID3D12Resource* g_mainRenderTargetResource[NUM_BACK_BUFFERS] = {};
static D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void WaitForLastSubmittedFrame();
FrameContext* WaitForNextFrameResources();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

std::shared_ptr<spdlog::logger> g_apiLogger = spdlog::default_logger();
// Main code
int main(int, char**)
{
	g_apiLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");


	// Create application window
	//ImGui_ImplWin32_EnableDpiAwareness();
	WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
	::RegisterClassExW(&wc);
	HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX12 Example", WS_OVERLAPPEDWINDOW, 30, 30, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

	vzm::InitEngineLib();
//	vzm::NewScene("my scene");

	// Initialize Direct3D
	if (!CreateDeviceD3D(hwnd))
	{
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	// Show the window
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX12_Init(g_pd3dDevice, NUM_FRAMES_IN_FLIGHT,
		DXGI_FORMAT_R11G11B10_FLOAT, g_pd3dSrvDescHeap,
		g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
		g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != nullptr);

	// Our state
	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// Main loop
	bool done = false;
	while (!done)
	{
		// Poll and handle messages (inputs, window resize, etc.)
		// See the WndProc() function below for our to dispatch events to the Win32 backend.
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}
		if (done)
			break;

		// Start the Dear ImGui frame
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		{
			static bool callbackLoadModel_Finished = false;
			static VID sid = 0, sid_file = 0, cid = 0, cid_ani = 0;
			static VID eid = 0, lid = 0, ani_id = 0, aid_body = 0;
			static vzm::OrbitalControl orbitControl, orbitControlAni;
			static ImVec2 wh(512, 512);
			if (sid == 0)
			{
				// TO DO //
				// 1. mesh to resource pool
				// 2. material to resource pool
				// 3. check another material (different from the mesh's original) to objectcomponent 

				sid = vzm::NewScene("my scene");

				vzm::VmWeather* vWeather = vzm::GetSceneActivatedWeather(sid);
				//vzm::NewSceneComponent(vzm::COMPONENT_TYPE::WEATHER, sid, "weather", 0, CMPP(vWeather));
				if (vWeather)
				{
					vWeather->SetRealisticSky(true);
					vWeather->SetVolumetricClouds(true);
					vWeather->SetVolumetricCloudsCastShadow(true);
					vWeather->SetVolumetricCloudsReceiveShadow(true);
				}

				vzm::VmLight* vLight;
				lid = vzm::NewSceneComponent(vzm::COMPONENT_TYPE::LIGHT, sid, "global directional light", 0, CMPP(vLight));
				glm::fvec3 lightPos(0, 10, 0);
				vLight->SetTranslate(__FP lightPos);
				vLight->SetIntensity(10.f);
				vLight->SetType(vzm::VmLight::DIRECTIONAL);

				ImVec2 canvas_size = ImGui::GetContentRegionAvail();
				vzm::VmCamera* vCam;
				cid = vzm::NewSceneComponent(vzm::COMPONENT_TYPE::CAMERA, sid, "my camera", 0, CMPP(vCam));
				vCam->SetCanvasSize(wh.x, wh.y, 96.f);
				glm::fvec3 pos(0, 2, 2), up(0, 1, 0), at(0, 0, 0);
				glm::fvec3 view = at - pos;
				vCam->SetPose(__FP pos, __FP view, __FP up);
				vCam->SetPerspectiveProjection(0.1f, 5000.f, glm::pi<float>() * 0.25f, 1.f);
				vCam->SetToDrawGridHelper(true);

				auto callbackLoadModel = [&](VID sceneVid, VID rootVid)
					{
						sid_file = sceneVid;
						callbackLoadModel_Finished = true;
					};

				//callbackLoadModel(vzm::LoadMeshModel("D:\\data\\car_gltf\\ioniq.gltf", "my file scene", "my gltf root"));
				vzm::LoadFileIntoNewSceneAsync("D:\\data\\car_gltf\\ioniq.gltf", "my file scene", "my gltf root", callbackLoadModel);
				//vzm::LoadMeshModel("D:\\VisMotive\\data\\obj files\\skull\\12140_Skull_v3_L2.obj", "my file scene", "my obj");
				//vzm::LoadFileIntoNewSceneAsync("D:\\data\\showroom1\\car_action_08.gltf", "my file scene", "my gltf root", callbackLoadModel);

			}

			if (sid_file != INVALID_VID)
			{
				vzm::MergeScenes(sid_file, sid);
				sid_file = INVALID_VID;

				std::vector<VID> camIds;
				vzm::GetSceneCompoenentVids(vzm::COMPONENT_TYPE::CAMERA, sid, camIds);
				for (VID id : camIds)
				{
					if (id != cid)
					{
						vzm::VmCamera* vCam = (vzm::VmCamera*)vzm::GetComponent(vzm::COMPONENT_TYPE::CAMERA, cid);
						vzm::VmCamera* vCamNew = (vzm::VmCamera*)vzm::GetComponent(vzm::COMPONENT_TYPE::CAMERA, id);
						glm::fvec3 pos, up, view;
						vCamNew->GetPose(__FP pos, __FP view, __FP up);
						vCam->SetPose(__FP pos, __FP view, __FP up);
						vCamNew->SetCanvasSize(wh.x, wh.y, 96.f);

						float zNearP, zFarP, fovY, aspectRatio;
						vCamNew->GetPerspectiveProjection(&zNearP, &zFarP, &fovY, &aspectRatio);
						vCam->SetPerspectiveProjection(zNearP, zFarP, fovY, aspectRatio);
						cid_ani = id;
						break;
					}
				}

				VID aid_backTire = vzm::GetFirstVidByName("back_tire_d");
				if (aid_backTire != INVALID_VID)
				{
					eid = vzm::NewSceneComponent(vzm::COMPONENT_TYPE::EMITTER, sid, "grapicar emitter", aid_backTire);
					vzm::VmEmitter* vEmitter = (vzm::VmEmitter*)vzm::GetComponent(vzm::COMPONENT_TYPE::EMITTER, eid);
					VID mid = vzm::GetFirstVidByName("back_tire");
					vEmitter->SetMeshVid(mid);

					// add plane collider
					vzm::VmCollider* collider = nullptr;
					VID colliderId = vzm::NewSceneComponent(vzm::COMPONENT_TYPE::COLLIDER, sid, "road collider", 0, CMPP(collider));
					collider->SetGPUEnabled(true);
					collider->SetRadius(1000.f);
					collider->SetShape(vzm::VmCollider::Shape::Plane);
				}


				std::vector<VID> aniComponentes;
				if (vzm::GetSceneCompoenentVids(vzm::COMPONENT_TYPE::ANIMATION, sid, aniComponentes) > 0u)
				{
					ani_id = aniComponentes[0];
				}

				aid_body = vzm::GetFirstVidByName("body_m");
			}

			auto aniComp = (vzm::VmAnimation*)vzm::GetComponent(vzm::COMPONENT_TYPE::ANIMATION, ani_id);
			glm::fvec3 at(0);
			if (aniComp && callbackLoadModel_Finished)
			{
				auto camComp = vzm::GetComponent(vzm::COMPONENT_TYPE::BASE, cid);
				if (camComp->GetParentVid() != aid_body)
				{
					vzm::AppendComponentTo(cid, aid_body);
				}

				vzm::VmBaseComponent* bodyBase = vzm::GetComponent(vzm::COMPONENT_TYPE::BASE, aid_body);
				bodyBase->GetWorldPosition(__FP at);
			}

			ImGui::Begin("Arcball Viewer");
			{
				static ImVec2 prevWindowSize = ImVec2(0, 0);
				ImVec2 curWindowSize = ImGui::GetWindowSize();

				if (prevWindowSize.x * prevWindowSize.y == 0)
					ImGui::SetWindowSize(ImVec2(0, 0));

				bool resized = prevWindowSize.x != curWindowSize.x || prevWindowSize.y != curWindowSize.y;
				prevWindowSize = curWindowSize;

				if (resized)
				{
					vzm::VmCamera* vCam = (vzm::VmCamera*)vzm::GetComponent(vzm::COMPONENT_TYPE::CAMERA, cid);
					ImVec2 canvas_size = ImGui::GetContentRegionAvail();
					canvas_size.y = std::max(canvas_size.y, 1.f);
					vCam->SetCanvasSize(canvas_size.x, canvas_size.y, 96.f);
					wh = canvas_size;
				}
				ImVec2 winPos = ImGui::GetWindowPos();
				ImVec2 curItemPos = ImGui::GetCursorPos();
				ImGui::InvisibleButton("render window", wh, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
				ImGui::SetItemAllowOverlap();

				bool is_hovered = ImGui::IsItemHovered(); // Hovered
				bool is_active = ImGui::IsItemActive();   // Held

				vzm::VmCamera* vCam = (vzm::VmCamera*)vzm::GetComponent(vzm::COMPONENT_TYPE::CAMERA, cid);
				if (aniComp) is_hovered = is_hovered && !aniComp->IsPlaying();
				if (is_hovered && !resized)
				{
					static glm::fvec2 __prevMousePos(0);
					glm::fvec2 ioPos = *(glm::fvec2*)&io.MousePos;
					glm::fvec2 s_pos = *(glm::fvec2*)&curItemPos;
					glm::fvec2 w_pos = *(glm::fvec2*)&winPos;
					glm::fvec2 m_pos = ioPos - s_pos - w_pos;
					glm::fvec2 pos_ss = m_pos;

					orbitControl.SetTargetCam(cid, __FP at, 2.f);

					if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right))
					{
						float np, fp;
						vCam->GetPerspectiveProjection(&np, &fp, NULL, NULL);
						orbitControl.Start(__FP pos_ss);
					}
					else if ((ImGui::IsMouseDragging(ImGuiMouseButton_Left, 1.f) || ImGui::IsMouseDragging(ImGuiMouseButton_Right, 1.f))
						&& glm::length2(__prevMousePos - m_pos) > 0)
					{
						if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
							orbitControl.PanMove(__FP pos_ss);
						else
							orbitControl.Move(__FP pos_ss);
					}
					else if (io.MouseWheel != 0)
					{
						glm::fvec3 pos, view, up;
						vCam->GetPose(__FP pos, __FP view, __FP up);
						if (io.MouseWheel > 0)
							pos += 0.2f * view;
						else
							pos -= 0.2f * view;
						vCam->SetPose(__FP pos, __FP view, __FP up);
					}
					__prevMousePos = pos_ss;
				}
				
				ImGui::SetCursorPos(curItemPos);

				vzm::Render(cid, true);

				uint32_t w, h;
				ImTextureID texId = vzm::GetGraphicsSharedRenderTarget(cid, g_pd3dDevice, g_pd3dSrvDescHeap, 1, &w, &h);
				// https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
				ImGui::Image(texId, ImVec2((float)w, (float)h));
			}
			ImGui::End();


			ImGui::Begin("Animation");
			if (callbackLoadModel_Finished && cid_ani != INVALID_VID)
			{
				static ImVec2 prevWindowSize2 = ImVec2(0, 0);
				ImVec2 curWindowSize2 = ImGui::GetWindowSize();

				if (prevWindowSize2.x * prevWindowSize2.y == 0)
					ImGui::SetWindowSize(ImVec2(0, 0));

				bool resized = prevWindowSize2.x != curWindowSize2.x || prevWindowSize2.y != curWindowSize2.y;
				prevWindowSize2 = curWindowSize2;
				if (resized)
				{
					vzm::VmCamera* vCamAni = (vzm::VmCamera*)vzm::GetComponent(vzm::COMPONENT_TYPE::CAMERA, cid_ani);
					ImVec2 canvas_size = ImGui::GetContentRegionAvail();
					vCamAni->SetCanvasSize(canvas_size.x, std::max(canvas_size.y, 1.f), 96.f);
				}
				ImVec2 winPos = ImGui::GetWindowPos();
				ImVec2 curItemPos = ImGui::GetCursorPos();
				ImGui::InvisibleButton("render window", wh, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
				ImGui::SetItemAllowOverlap();

				orbitControlAni.SetTargetCam(cid_ani, __FP at, 2.f);

				vzm::VmCamera* vCamAni = (vzm::VmCamera*)vzm::GetComponent(vzm::COMPONENT_TYPE::CAMERA, cid_ani);
				bool is_hovered = ImGui::IsItemHovered(); // Hovered
				bool is_active = ImGui::IsItemActive();   // Held
				if (aniComp) is_hovered = is_hovered && !aniComp->IsPlaying();
				if (is_hovered && !resized)
				{
					static glm::fvec2 __prevMousePos(0);
					glm::fvec2 ioPos = *(glm::fvec2*)&io.MousePos;
					glm::fvec2 s_pos = *(glm::fvec2*)&curItemPos;
					glm::fvec2 w_pos = *(glm::fvec2*)&winPos;
					glm::fvec2 m_pos = ioPos - s_pos - w_pos;
					glm::fvec2 pos_ss = m_pos;

					if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right))
					{
						float np, fp;
						vCamAni->GetPerspectiveProjection(&np, &fp, NULL, NULL);
						orbitControlAni.Start(__FP pos_ss);
					}
					else if ((ImGui::IsMouseDragging(ImGuiMouseButton_Left, 1.f) || ImGui::IsMouseDragging(ImGuiMouseButton_Right, 1.f))
						&& glm::length2(__prevMousePos - m_pos) > 0)
					{
						if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
							orbitControlAni.PanMove(__FP pos_ss);
						else
							orbitControlAni.Move(__FP pos_ss);
					}
					else if (io.MouseWheel != 0)
					{
						glm::fvec3 pos, view, up;
						vCamAni->GetPose(__FP pos, __FP view, __FP up);
						if (io.MouseWheel > 0)
							pos += 0.2f * view;
						else
							pos -= 0.2f * view;
						vCamAni->SetPose(__FP pos, __FP view, __FP up);
					}
					__prevMousePos = pos_ss;
				}
				ImGui::SetCursorPos(curItemPos);

				vzm::Render(cid_ani, true);

				// Note that we pass the GPU SRV handle here, *not* the CPU handle. We're passing the internal pointer value, cast to an ImTextureID
				uint32_t w, h;
				ImTextureID texId = vzm::GetGraphicsSharedRenderTarget(cid_ani, g_pd3dDevice, g_pd3dSrvDescHeap, 2, &w, &h);
				// https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
				ImGui::Image(texId, ImVec2((float)w, (float)h));
			}
			ImGui::End();

			ImGui::Begin("Controls");
			{
				if (ImGui::Button("Shader Reload"))
				{
					vzm::ReloadShader();
				}
				if (aniComp)
				{
					if (ImGui::Button("PLAY"))
					{
						aniComp->Play();
					}
					ImGui::SameLine();
					if (ImGui::Button("PAUSE"))
					{
						aniComp->Pause();
					}
					ImGui::SameLine();
					if (ImGui::Button("STOP"))
					{
						aniComp->Stop();
					}
				}

				if (eid != INVALID_VID)
				{
					vzm::VmEmitter* vEmitter = (vzm::VmEmitter*)vzm::GetComponent(vzm::COMPONENT_TYPE::EMITTER, eid);

					ImGui::Separator();
					ImGui::Text("Emitter System");
					static bool initialized = false;

					static float LifeSpan = vEmitter->GetLife();
					ImGui::SliderFloat("LifeSpan", &LifeSpan, 0.f, 100.f);
					vEmitter->SetLife(LifeSpan);
					static float LifeSpanRandomness = vEmitter->GetRandomLife();
					ImGui::SliderFloat("LifeSpanRandomness", &LifeSpanRandomness, 0.f, 1.f);
					vEmitter->SetRandomLife(LifeSpanRandomness);
					static float NormFactor = 0;// vEmitter->GetNormalFactor();
					ImGui::SliderFloat("NormFactor", &NormFactor, 0.f, 1.f);
					vEmitter->SetNormalFactor(NormFactor);
					static float RandomFactor = vEmitter->GetRandomFactor();
					ImGui::SliderFloat("RandomFactor", &RandomFactor, 0.f, 1.f);
					vEmitter->SetRandomFactor(RandomFactor);
					static float ScaleX = -1.f, ScaleY = -1.f;
					if (!initialized)
						vEmitter->GetScaleXY(&ScaleX, &ScaleY);
					ImGui::SliderFloat("ScaleX", &ScaleX, 0.00001f, 1.f, "%.4f");
					ImGui::SliderFloat("ScaleY", &ScaleY, 0.00001f, 1.f, "%.4f");
					vEmitter->SetScaleXY(ScaleX, ScaleY);
					static float Size = 0.002f;// vEmitter->GetSize();
					ImGui::SliderFloat("Size", &Size, 0.00001f, 1.f, "%.4f");
					vEmitter->SetSize(Size);
					static float MotionBlurAmount = vEmitter->GetMotionBlurAmount();
					ImGui::SliderFloat("MotionBlurAmount", &MotionBlurAmount, 0.0f, 100.f);
					vEmitter->SetMotionBlurAmount(MotionBlurAmount);
					static float Rotation = vEmitter->GetRotation();
					ImGui::SliderFloat("Rotation", &Rotation, 0.0f, 1.f);
					vEmitter->SetRotation(Rotation);
					static float Mass = vEmitter->GetMass();
					ImGui::SliderFloat("Mass", &Mass, 0.0f, 100.f);
					vEmitter->SetMass(Mass);
					static float Restitution = vEmitter->GetRestitution();
					ImGui::SliderFloat("Restitution", &Restitution, -1.0f, 10.f);
					vEmitter->SetRestitution(Restitution);
					static float ParticleDrag = vEmitter->GetDrag();
					ImGui::SliderFloat("ParticleDrag", &ParticleDrag, 0.0f, 10.f);
					vEmitter->SetDrag(ParticleDrag);
					static float Gravity[3] = { 0, -0.9f, 0 };
					//if (!initialized)
					//	vEmitter->GetGravity(Gravity);
					ImGui::SliderFloat3("Gravity", Gravity, -1.0f, 1.f);
					vEmitter->SetGravity(Gravity);
					static float Velocity[3] = { 0, 0, 0.0f};
					//if (!initialized)
					//	vEmitter->GetVelocity(Velocity);
					ImGui::SliderFloat3("Velocity", Velocity, -1.0f, 1.f);
					vEmitter->SetVelocity(Velocity);
					static float ParticleCount = 300.f;// vEmitter->GetCount();
					ImGui::SliderFloat("ParticleCount", &ParticleCount, 0.0f, (float)vEmitter->GetMaxParticleCount());
					vEmitter->SetCount(ParticleCount);
					static int ParticleBurst = 100;
					ImGui::SliderInt("ParticleBurst", &ParticleBurst, 0, 1000);
					if (ImGui::Button("Particle Burst")) {
						vEmitter->Burst(ParticleBurst);
					}

					static bool isDebug = vEmitter->IsDebug();
					ImGui::Checkbox("Debug", &isDebug);
					vEmitter->SetDebug(isDebug);
					static bool isPaused = vEmitter->IsPaused();
					ImGui::Checkbox("Paused", &isPaused);
					vEmitter->SetPaused(isPaused);
					static bool isSort = vEmitter->IsSorted();
					ImGui::Checkbox("Sort", &isSort);
					vEmitter->SetSorted(isSort);
					static bool isDepthCollisionEnabled = vEmitter->IsDepthCollisionEnabled();
					ImGui::Checkbox("DepthCollisionEnabled", &isDepthCollisionEnabled);
					vEmitter->SetDepthCollisionEnabled(isDepthCollisionEnabled);
					static bool isSPHEnabled = vEmitter->IsSPHEnabled();
					ImGui::Checkbox("SPHEnabled", &isSPHEnabled);
					vEmitter->SetSPHEnabled(isSPHEnabled);
					static bool isVolumeEnabled = vEmitter->IsVolumeEnabled();
					ImGui::Checkbox("VolumeEnabled", &isVolumeEnabled);
					vEmitter->SetVolumeEnabled(isVolumeEnabled);
					static bool isFrameBlendingEnabled = vEmitter->IsFrameBlendingEnabled();
					ImGui::Checkbox("FrameBlendingEnabled", &isFrameBlendingEnabled);
					vEmitter->SetFrameBlendingEnabled(isFrameBlendingEnabled);
					static bool isCollidersDisabled = vEmitter->IsCollidersDisabled();
					ImGui::Checkbox("CollidersDisabled", &isCollidersDisabled);
					vEmitter->SetCollidersDisabled(isCollidersDisabled);
					static bool isTakeColorFromMesh = vEmitter->IsTakeColorFromMesh();
					ImGui::Checkbox("TakeColorFromMesh", &isTakeColorFromMesh);
					vEmitter->SetSorted(isTakeColorFromMesh);

					initialized = true;

					vzm::VmEmitter::ParticleCounters pc;
					vEmitter->GetStatistics(pc);
					ImGui::Text("aliveCount %d", pc.aliveCount);
					ImGui::Text("deadCount %d", pc.deadCount);
					ImGui::Text("realEmitCount %d", pc.realEmitCount);
					ImGui::Text("aliveCount_afterSimulation %d", pc.aliveCount_afterSimulation);
					ImGui::Text("culledCount %d", pc.culledCount);
					ImGui::Text("cellAllocator %d", pc.cellAllocator);
				}
				ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			}
			ImGui::End();

			ImGui::Begin("Info");
			ImVec2 curWindowSize3 = ImGui::GetWindowSize();
			if (curWindowSize3.x * curWindowSize3.y == 0)
			{
				ImGui::SetWindowSize(ImVec2(0, 0));
				curWindowSize3 = ImGui::GetWindowSize();
			}
			VID canvasVis = vzm::DisplayEngineProfiling(curWindowSize3.x, curWindowSize3.y);
			// Note that we pass the GPU SRV handle here, *not* the CPU handle. We're passing the internal pointer value, cast to an ImTextureID
			uint32_t w, h;
			ImTextureID texId = vzm::GetGraphicsSharedRenderTarget(canvasVis, g_pd3dDevice, g_pd3dSrvDescHeap, 3, &w, &h);
			// https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
			ImGui::Image(texId, ImVec2((float)w, (float)h));
			ImGui::End();
		}

		// Rendering
		ImGui::Render();

		FrameContext* frameCtx = WaitForNextFrameResources();
		UINT backBufferIdx = g_pSwapChain->GetCurrentBackBufferIndex();
		frameCtx->CommandAllocator->Reset();

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = g_mainRenderTargetResource[backBufferIdx];
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		g_pd3dCommandList->Reset(frameCtx->CommandAllocator, nullptr);
		g_pd3dCommandList->ResourceBarrier(1, &barrier);

		// Render Dear ImGui graphics
		const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		g_pd3dCommandList->ClearRenderTargetView(g_mainRenderTargetDescriptor[backBufferIdx], clear_color_with_alpha, 0, nullptr);
		g_pd3dCommandList->OMSetRenderTargets(1, &g_mainRenderTargetDescriptor[backBufferIdx], FALSE, nullptr);
		g_pd3dCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_pd3dCommandList);
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		g_pd3dCommandList->ResourceBarrier(1, &barrier);
		g_pd3dCommandList->Close();

		g_pd3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&g_pd3dCommandList);

		g_pSwapChain->Present(1, 0); // Present with vsync
		//g_pSwapChain->Present(0, 0); // Present without vsync

		UINT64 fenceValue = g_fenceLastSignaledValue + 1;
		g_pd3dCommandQueue->Signal(g_fence, fenceValue);
		g_fenceLastSignaledValue = fenceValue;
		frameCtx->FenceValue = fenceValue;
	}

	vzm::DeinitEngineLib();

	WaitForLastSubmittedFrame();

	// Cleanup
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC1 sd;
	{
		ZeroMemory(&sd, sizeof(sd));
		sd.BufferCount = NUM_BACK_BUFFERS;
		sd.Width = 0;
		sd.Height = 0;
		sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		sd.Scaling = DXGI_SCALING_STRETCH;
		sd.Stereo = FALSE;
	}

	// [DEBUG] Enable debug interface
#ifdef DX12_ENABLE_DEBUG_LAYER
	ID3D12Debug* pdx12Debug = nullptr;
	// note : only one debug_layer is available
	//if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
	//	pdx12Debug->EnableDebugLayer();
#endif

	// Create device
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_12_2;
	if (D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&g_pd3dDevice)) != S_OK)
		return false;

	// [DEBUG] Setup debug interface to break on any warnings/errors
#ifdef DX12_ENABLE_DEBUG_LAYER
	if (pdx12Debug != nullptr)
	{
		ID3D12InfoQueue* pInfoQueue = nullptr;
		g_pd3dDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		pInfoQueue->Release();
		pdx12Debug->Release();
	}
#endif

	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		desc.NumDescriptors = NUM_BACK_BUFFERS;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NodeMask = 1;
		if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dRtvDescHeap)) != S_OK)
			return false;

		SIZE_T rtvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
		for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
		{
			g_mainRenderTargetDescriptor[i] = rtvHandle;
			rtvHandle.ptr += rtvDescriptorSize;
		}
	}

	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 4;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
			return false;
	}

	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 1;
		if (g_pd3dDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&g_pd3dCommandQueue)) != S_OK)
			return false;
	}

	for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
		if (g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_frameContext[i].CommandAllocator)) != S_OK)
			return false;

	if (g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frameContext[0].CommandAllocator, nullptr, IID_PPV_ARGS(&g_pd3dCommandList)) != S_OK ||
		g_pd3dCommandList->Close() != S_OK)
		return false;

	if (g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)) != S_OK)
		return false;

	g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (g_fenceEvent == nullptr)
		return false;

	{
		IDXGIFactory4* dxgiFactory = nullptr;
		IDXGISwapChain1* swapChain1 = nullptr;
		if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK)
			return false;
		if (dxgiFactory->CreateSwapChainForHwnd(g_pd3dCommandQueue, hWnd, &sd, nullptr, nullptr, &swapChain1) != S_OK)
			return false;
		if (swapChain1->QueryInterface(IID_PPV_ARGS(&g_pSwapChain)) != S_OK)
			return false;
		swapChain1->Release();
		dxgiFactory->Release();
		g_pSwapChain->SetMaximumFrameLatency(NUM_BACK_BUFFERS);
		g_hSwapChainWaitableObject = g_pSwapChain->GetFrameLatencyWaitableObject();
	}

	CreateRenderTarget();
	return true;
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain) { g_pSwapChain->SetFullscreenState(false, nullptr); g_pSwapChain->Release(); g_pSwapChain = nullptr; }
	if (g_hSwapChainWaitableObject != nullptr) { CloseHandle(g_hSwapChainWaitableObject); }
	for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
		if (g_frameContext[i].CommandAllocator) { g_frameContext[i].CommandAllocator->Release(); g_frameContext[i].CommandAllocator = nullptr; }
	if (g_pd3dCommandQueue) { g_pd3dCommandQueue->Release(); g_pd3dCommandQueue = nullptr; }
	if (g_pd3dCommandList) { g_pd3dCommandList->Release(); g_pd3dCommandList = nullptr; }
	if (g_pd3dRtvDescHeap) { g_pd3dRtvDescHeap->Release(); g_pd3dRtvDescHeap = nullptr; }
	if (g_pd3dSrvDescHeap) { g_pd3dSrvDescHeap->Release(); g_pd3dSrvDescHeap = nullptr; }
	if (g_fence) { g_fence->Release(); g_fence = nullptr; }
	if (g_fenceEvent) { CloseHandle(g_fenceEvent); g_fenceEvent = nullptr; }
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }

#ifdef DX12_ENABLE_DEBUG_LAYER
	IDXGIDebug1* pDebug = nullptr;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
	{
		pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
		pDebug->Release();
	}
#endif
}

void CreateRenderTarget()
{
	for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
	{
		ID3D12Resource* pBackBuffer = nullptr;
		g_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
		g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, g_mainRenderTargetDescriptor[i]);
		g_mainRenderTargetResource[i] = pBackBuffer;
	}
}

void CleanupRenderTarget()
{
	WaitForLastSubmittedFrame();

	for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
		if (g_mainRenderTargetResource[i]) { g_mainRenderTargetResource[i]->Release(); g_mainRenderTargetResource[i] = nullptr; }
}

void WaitForLastSubmittedFrame()
{
	FrameContext* frameCtx = &g_frameContext[g_frameIndex % NUM_FRAMES_IN_FLIGHT];

	UINT64 fenceValue = frameCtx->FenceValue;
	if (fenceValue == 0)
		return; // No fence was signaled

	frameCtx->FenceValue = 0;
	if (g_fence->GetCompletedValue() >= fenceValue)
		return;

	g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
	WaitForSingleObject(g_fenceEvent, INFINITE);
}

FrameContext* WaitForNextFrameResources()
{
	UINT nextFrameIndex = g_frameIndex + 1;
	g_frameIndex = nextFrameIndex;

	HANDLE waitableObjects[] = { g_hSwapChainWaitableObject, nullptr };
	DWORD numWaitableObjects = 1;

	FrameContext* frameCtx = &g_frameContext[nextFrameIndex % NUM_FRAMES_IN_FLIGHT];
	UINT64 fenceValue = frameCtx->FenceValue;
	if (fenceValue != 0) // means no fence was signaled
	{
		frameCtx->FenceValue = 0;
		g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
		waitableObjects[1] = g_fenceEvent;
		numWaitableObjects = 2;
	}

	WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);

	return frameCtx;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
		{
			WaitForLastSubmittedFrame();
			CleanupRenderTarget();
			HRESULT result = g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
			assert(SUCCEEDED(result) && "Failed to resize swapchain.");
			CreateRenderTarget();
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
