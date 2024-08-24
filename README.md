# High-level Engine APIs with a combination of Wicked Engine and Customized Rendering Frameworks
- This project starts with the compact version of Wicked Engine (first milestone)
- And adding my previous rendering engine features to this project while refactoring/compacting the underlying datastructures
- High-level (COM-based) APIs (not exposing the core datastructures)
- Refer to ["Viewer_SeparatedEngine" project](https://github.com/korfriend/VizMotive/tree/master/Viewer_SeparatedEngine) for the usage of my high-level APIs
- Last update : 2024.08.24

## Our Engine (based on Wicked Engine)

VizMotive Engine is an open-source 3D engine for scientific data visualizations based on <a href="https://github.com/turanszkij/WickedEngine/">Wicked Engine</a>.

### Main Contribution Features
- High-level (COM-based) APIs
	- ["WickedEngine/VizEngineAPIs.h"](https://github.com/korfriend/VizMotive/blob/master/WickedEngine/VizEngineAPIs.h) for main COM-based high-level APIs
 	- ["WickedEngine/VizComponentAPIs.h"](https://github.com/korfriend/VizMotive/blob/master/WickedEngine/VizComponentAPIs.h) for data structures referring to Wicked Engine components
  	- Support off-screen rendering to other device's canvas/texture using shared resource graphics
  	- Multi-canvas and Multi-scene visualization
- Modified scene structure based on actors and their components and systems
	- Camera having renderer (as a canvas) with transform system
 	- Mesh actor having mesh component (w/ external material component) with transform system
	- Particle actor having emitter component (w/ internal material component) with transform system
	- (TO DO) Volume actor having volume component (w/ external material having OTF) with transform system
 	- (TO DO) Allowing to assign different MaterialComponent to ObjectComponent (by default, using the MaterialComponent intrinsically assigned in MeshComponent). In other words, use the MeshComponent as GeometryComponent that can be used along with different MaterialComponents)
 	- (TO DO) Separate Scene and Resource Pool to share resources across different scenes
 
### Platforms:
- Windows 10 or newer
- Linux
- UWP

### Examples:

#### Basics (C++):
- Refer to vzm-namespace functions in [main.cpp](https://github.com/korfriend/VizMotive/blob/master/Viewer_SeparatedEngine/main.cpp) (based on dear imgui) ["Viewer_SeparatedEngine" project](https://github.com/korfriend/VizMotive/tree/master/Viewer_SeparatedEngine)
```cpp
// Include engine headers:
#include "VizEngineAPIs.h"	// this requires "VizComponentAPIs.h", none external dependencies

// Start the Engine APIs:
// Note: the engine uses internally-assigned GPU device handler 
vzm::InitEngineLib();

// Create a scene
VID sid = vzm::NewScene("my scene");

// Load local scene structure
vzm::LoadMeshModel(sid, [filename], "my obj");

// Run the application:
while(true)
{
	...

	vzm::VmCamera* vCam = nullptr;
	static int cid = vzm::NewSceneComponent(vzm::COMPONENT_TYPE::CAMERA, sid, "my camera", 0, CMPP(vCam));

	// wh is float2 type for rendertarget width and height 
	vCam->SetCanvasSize(wh.x, wh.y, 96.f);

	float3 pos(0, 2, 2), up(0, 1, 0), at(0, 0, 0);
	float3 view = at - pos;
	vCam->SetPose(__FP pos, __FP view, __FP up);
	vCam->SetPerspectiveProjection(0.1f, 5000.f, glm::pi<float>() * 0.4f, 1.f);

	vzm::Render(cid);
	uint32_t w, h;
	// ImGui (DX12 version) example 
	ImTextureID texId = vzm::GetGraphicsSharedRenderTarget(cid, g_pd3dDevice, g_pd3dSrvDescHeap, 1, &w, &h);
	// https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
	ImGui::Image(texId, ImVec2((float)w, (float)h));			

	...
}

// Finish the Engine APIs:
vzm::DeinitEngineLib();
```

### Graphics API:
Graphics APIs are tightly dependent on the core engine source, <a href="https://github.com/turanszkij/WickedEngine/">Wicked Engine</a>. The default renderer is `DirectX 12` on Windows and `Vulkan` on Linux. I will support `Mobile` Vulkan, which is allowed on Android running in mobile devices. 
You can specify command line arguments (without any prefix) to switch between render devices or other settings. Currently the list of options:
<table>
  <tr>
	<th>Argument</th>
	<th>Description</th>
  </tr>
  <tr>
	<td>vulkan</td>
	<td>Use the Vulkan rendering device on Windows</td>
  </tr>
  <tr>
	<td>debugdevice</td>
	<td>Use debug layer for graphics API validation. Performance will be degraded, but graphics warnings and errors will be written to the "Output" window</td>
  </tr>
  <tr>
	<td>gpuvalidation</td>
	<td>Use GPU Based Validation for graphics. This must be used together with the debugdevice argument. Currently DX12 only.</td>
  </tr>
  <tr>
	<td>gpu_verbose</td>
	<td>Enable verbose GPU validation mode.</td>
  </tr>
  <tr>
	<td>igpu</td>
	<td>Prefer integrated GPU selection for graphics. By default, dedicated GPU selection will be preferred.</td>
  </tr>
  <tr>
	<td>alwaysactive</td>
	<td>The application will not be paused when the window is in the background.</td>
  </tr>
</table>

<br/>

### Troubleshooting
(At this moment, no other my own troubleshooting policy. Just refer to the following, which is decribed in the original Wicked Engine github)
If you are having trouble getting the applications to run, make sure that you satisfy the following conditions:
- If you built the application with Visual Studio, run it from the Visual Studio environment, where the executable working directory is set up to be the Project directory (not the build directory where the exe will be found)
- If you want to run an application without Visual Studio, either copy the executable from the BUILD directory to the correct project directory, or set the working directory appropriately. You can also check the Working directory setting in Visual Studio to find out the right working directory of every project. 

- If you experience crashes, follow these steps to find out the problem:
	- make sure your environment is up to date, with latest graphics drivers and operating system updates.
	- see if there is a wiBackLog.txt in your user temp folder (for example: C:\Users\username\AppData\Local\Temp)
	- request help on the [Forum](https://wickedengine.net/forum/), [Discord](https://discord.gg/CFjRYmE) or [Github issue](https://github.com/turanszkij/WickedEngine/issues)
	- build the engine in Debug mode and try to run it, see where it crashes
	- run the engine with the `debugdevice` command argument and post the text from your console output window when the crash happens
		- for very advanced users, using `gpuvalidation` with `debugdevice` will print additional graphics debug information
