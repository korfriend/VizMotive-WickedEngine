#pragma once
#include "VizComponentAPIs.h"

#define CMPP(P) (vzm::VmBaseComponent**)&P

namespace vzm 
{
	// This must be called before using engine APIs
	//  - paired with DeinitEngineLib()
	__dojostatic VZRESULT InitEngineLib(const std::string& coreName = "VzmEngine", const std::string& logFileName = "EngineApi.log");
	// Get Entity ID 
	//  - return zero in case of failure 
	__dojostatic VID GetFirstVidByName(const std::string& name);
	// Get Entity IDs whose name is the input name (VID is allowed for redundant name)
	//  - return # of entities
	__dojostatic uint32_t GetVidsByName(const std::string& name, std::vector<VID>& vids);
	// Get Entity's name if possible
	//  - return name string if entity's name exists, if not, return "" 
	__dojostatic bool GetNameByVid(const VID vid, std::string& name);
	// Remove an entity (scene, scene components, renderer) 
	__dojostatic void RemoveComponent(const VID vid);
	// Create new scene and return scene (NOT a scene item) ID, a scene 
	//  - return zero in case of failure (the name is already registered or overflow VID)
	__dojostatic VID NewScene(const std::string& sceneName);
	// Create new scene component and return its component (scene item) ID (global entity)
	//  - Must belong to a scene
	//  - return zero in case of failure (invalid sceneID, the name is already registered, or overflow VID)
	__dojostatic VID NewSceneComponent(const COMPONENT_TYPE compType, const VID sceneVid, const std::string& compName, const VID parentVid = 0u, VmBaseComponent** baseComp = nullptr);
	// Append Component to the parent component
	//  - return sceneId containing the parent component 
	__dojostatic VID AppendComponentTo(const VID vid, const VID parentVid);
	// Get Component and return its pointer registered in renderer
	//  - return nullptr in case of failure
	__dojostatic VmBaseComponent* GetComponent(const COMPONENT_TYPE compType, const VID vid);
	// Get Component IDs in a scene
	//  - return # of scene Components 
	__dojostatic uint32_t GetSceneCompoenentVids(const COMPONENT_TYPE compType, const VID sceneVid, std::vector<VID>& vids);	// Get CameraParams and return its pointer registered in renderer
	// Get VmWeather and return its pointer 
	//  - return nullptr in case of failure 
	__dojostatic VmWeather* GetSceneActivatedWeather(const VID sceneVid);
	// Load scene components into a new scene and return the scene ID
	//  - Must belong to the internal scene
	//  - return zero in case of failure
	__dojostatic VID LoadFileIntoNewScene(const std::string& file, const std::string& rootName, const std::string& sceneName = "", VID* rootVid = nullptr);
	// Async version of LoadFileIntoNewScene
	__dojostatic void LoadFileIntoNewSceneAsync(const std::string& file, const std::string& rootName, const std::string& sceneName = "", const std::function<void(VID sceneVid, VID rootVid)>& callback = nullptr);
	// Merge src scene to dest scene 
	//  - This is not THREAD-SAFE 
	__dojostatic VZRESULT MergeScenes(const VID srcSceneVid, const VID dstSceneVid);
	// Render a scene on camera (camVid)
	//  - Must belong to the internal scene
	//  - if updateScene is true, uses the camera for camera-dependent scene updates
	//  - strongly recommend a single camera-dependent update per a scene 
	__dojostatic VZRESULT Render(const VID camVid, const bool updateScene = true);
	// Get a graphics render target view 
	//  - Must belong to the internal scene
	__dojostatic void* GetGraphicsSharedRenderTarget(const int camVid, const void* device2, const void* srv_desc_heap2, const int descriptor_index, uint32_t* w = nullptr, uint32_t* h = nullptr);
	// Reload shaders
	__dojostatic void ReloadShader();

	__dojostatic VZRESULT DeinitEngineLib();
}

namespace vzm // helper APIs
{
	__dojoclass OrbitalControl
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
}
