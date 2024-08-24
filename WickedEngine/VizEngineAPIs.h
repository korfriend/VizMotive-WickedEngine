#pragma once
#include "VizComponentAPIs.h"

#define CMPP(P) (vzm::VmBaseComponent**)&P

namespace vzm 
{
	// This must be called before using engine APIs
	//  - paired with DeinitEngineLib()
	extern "C" API_EXPORT VZRESULT InitEngineLib(const std::string& coreName = "VzmEngine", const std::string& logFileName = "EngineApi.log");
	// Get Entity ID 
	//  - return zero in case of failure 
	extern "C" API_EXPORT VID GetFirstVidByName(const std::string& name);
	// Get Entity IDs whose name is the input name (VID is allowed for redundant name)
	//  - return # of entities
	extern "C" API_EXPORT size_t GetVidsByName(const std::string& name, std::vector<VID>& vids);
	// Get Entity's name if possible
	//  - return name string if entity's name exists, if not, return "" 
	extern "C" API_EXPORT bool GetNameByVid(const VID vid, std::string& name);
	// Remove an entity (scene, scene components, renderer) 
	extern "C" API_EXPORT void RemoveComponent(const VID vid);
	// Create new scene and return scene (NOT a scene item) ID, a scene 
	//  - return zero in case of failure (the name is already registered or overflow VID)
	extern "C" API_EXPORT VID NewScene(const std::string& sceneName);
	// Create new scene component and return its component (scene item) ID (global entity)
	//  - Must belong to a scene
	//  - return zero in case of failure (invalid sceneID, the name is already registered, or overflow VID)
	extern "C" API_EXPORT VID NewSceneComponent(const COMPONENT_TYPE compType, const VID sceneVid, const std::string& compName, const VID parentVid = 0u, VmBaseComponent** baseComp = nullptr);
	// Append Component to the parent component
	//  - return sceneId containing the parent component 
	extern "C" API_EXPORT VID AppendComponentTo(const VID vid, const VID parentVid);
	// Get Component and return its pointer registered in renderer
	//  - return nullptr in case of failure
	extern "C" API_EXPORT VmBaseComponent* GetComponent(const COMPONENT_TYPE compType, const VID vid);
	// Get Component IDs in a scene
	//  - return # of scene Components 
	extern "C" API_EXPORT uint32_t GetSceneCompoenentVids(const COMPONENT_TYPE compType, const VID sceneVid, std::vector<VID>& vids);	// Get CameraParams and return its pointer registered in renderer
	// Get VmWeather and return its pointer 
	//  - return nullptr in case of failure 
	extern "C" API_EXPORT VmWeather* GetSceneActivatedWeather(const VID sceneVid);
	// Load scene components into a new scene and return the scene ID
	//  - Must belong to the internal scene
	//  - return zero in case of failure
	extern "C" API_EXPORT VID LoadFileIntoNewScene(const std::string& file, const std::string& rootName, const std::string& sceneName = "", VID* rootVid = nullptr);
	// Async version of LoadFileIntoNewScene
	extern "C" API_EXPORT void LoadFileIntoNewSceneAsync(const std::string& file, const std::string& rootName, const std::string& sceneName = "", const std::function<void(VID sceneVid, VID rootVid)>& callback = nullptr);
	// Merge src scene to dest scene 
	//  - This is not THREAD-SAFE 
	extern "C" API_EXPORT VZRESULT MergeScenes(const VID srcSceneVid, const VID dstSceneVid);
	// Render a scene on camera (camVid)
	//  - Must belong to the internal scene
	//  - if updateScene is true, uses the camera for camera-dependent scene updates
	//  - strongly recommend a single camera-dependent update per a scene 
	extern "C" API_EXPORT VZRESULT Render(const VID camVid, const bool updateScene = true);
	// Get a graphics render target view 
	//  - Must belong to the internal scene
	extern "C" API_EXPORT void* GetGraphicsSharedRenderTarget(const int camVid, const void* device2, const void* srv_desc_heap2, const int descriptor_index, uint32_t* w = nullptr, uint32_t* h = nullptr);
	// Reload shaders
	extern "C" API_EXPORT void ReloadShader();

	// Display Engine's states and profiling information
	//  - return canvas VID (use this as a camVid)
	extern "C" API_EXPORT VID DisplayEngineProfiling(const int w, const int h, const bool displayProfile = true, const bool displayEngineStates = true);

	extern "C" API_EXPORT VZRESULT DeinitEngineLib();
}

namespace vzm // helper APIs
{
	class API_EXPORT OrbitalControl
	{
		VID camVid = INVALID_VID;
	public:
		OrbitalControl();
		~OrbitalControl();
		void SetTargetCam(const VID camVid, const float stage_center[3], const float stage_radius);
		bool Start(const float pos_xy[2], const float sensitivity = 1.0f);
		bool Move(const float pos_xy[2]);	// target is camera
		bool PanMove(const float pos_xy[2]);
	};

	struct InfoDisplayer
	{
		// activate the whole display
		bool active = false;
		// display engine version number
		bool watermark = false;
		// display framerate
		bool fpsinfo = false;
		// display graphics device name
		bool device_name = false;
		// display resolution info
		bool resolution = false;
		// window's size in logical (DPI scaled) units
		bool logical_size = false;
		// HDR status and color space
		bool colorspace = false;
		// display number of heap allocations per frame
		bool heap_allocation_counter = false;
		// display the active graphics pipeline count
		bool pipeline_count = false;
		// display video memory usage and budget
		bool vram_usage = false;
		// text size
		int size = 10;
		// display default color grading helper texture in top left corner of the screen
		bool colorgrading_helper = false;
	};
	// Set Info Display on Canvas
	extern "C" API_EXPORT void SetInfoDisplay(const VID camVid, const InfoDisplayer& infoDisplay);
	// Set Info Display on Canvas
	extern "C" API_EXPORT void SetRendererOptions(const VID camVid, const InfoDisplayer& infoDisplay);
}
