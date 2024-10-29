# High-level Engine APIs with a combination of Wicked Engine and Customized Rendering Frameworks
- This project starts with the compact version of Wicked Engine (first milestone)
- And adding my previous rendering engine features to this project while refactoring/compacting the underlying datastructures
- High-level (COM-based) APIs (not exposing the core datastructures)
- Refer to ["Viewer_SeparatedEngine" project](https://github.com/korfriend/VizMotive/tree/master/Viewer_SeparatedEngine) for the usage of my high-level APIs
- Last update : 2024.08.24

## Our Engine (based on Wicked Engine)

VizMotive Engine is an open-source 3D engine for scientific data visualizations based on <a href="https://github.com/turanszkij/WickedEngine/">Wicked Engine</a>.


<br/>
<img align="right" src="https://github.com/turanszkij/wickedengine-gifs/raw/main/videoprojectors.gif" width="320px"/>
Wicked Engine is an open-source 3D engine with modern graphics. Use this as a C++ framework for your graphics projects, a standalone 3D editor, LUA scripting or just for learning.<br/>

- [Website](https://wickedengine.net/)<br/>
- [Forum](https://wickedengine.net/forum/)<br/>
- [Features](features.txt)<br/>
- [Videos](https://www.youtube.com/playlist?list=PLLN-1FTGyLU_HJoC5zx6hJkB3D2XLiaxS)<br/>
- [Editor Documentation](https://wickedengine.net/wp-content/uploads/2024/09/Wicked-Engine-Editor-Documentation.pdf)<br/>
- [C++ Documentation](Content/Documentation/WickedEngine-Documentation.md)<br/>
- [Lua Documentation](Content/Documentation/ScriptingAPI-Documentation.md)<br/>

You can get the full source code by using Git version control and cloning https://github.com/turanszkij/WickedEngine.git, or downloading it as zip. You can also download nightly packaged builds of the Editor here (requires Github sign in): [![Github Build Status](https://github.com/turanszkij/WickedEngine/workflows/Build/badge.svg)](https://github.com/turanszkij/WickedEngine/actions)
<br/>
<br/>

<img align="right" src="https://github.com/turanszkij/wickedengine-gifs/raw/main/fps.gif" width="320px"/>

### Platforms:
- Windows 10 or newer
- Linux
- UWP

### How to build: 

#### Windows
To build Wicked Engine for Windows (10 or later), use the latest version of Visual Studio and the provided `WickedEngine.sln` solution file. By simply pressing F5, the Editor application will be built. There are other example projects that you can build as well within the solution.

<img align="right" src="https://github.com/turanszkij/wickedengine-gifs/raw/main/fighting_game.gif" width="320px"/>

If you want to develop a C++ application that uses Wicked Engine, you can build the WickedEngine static library project for the appropriate platform, such as `WickedEngine_Windows` and link against it. Including the `"WickedEngine.h"` header will attempt to link the binaries for the appropriate platform, but search directories should be set up beforehand. For example, you can set additional library directories to `$(SolutionDir)BUILD\$(Platform)\$(Configuration)` by default. For examples, see the `Template`, `Tests`, and `Editor` projects. 

If you have questions or stuck, please use the `windows` communication channel on Discord: [![Discord chat](https://img.shields.io/discord/602811659224088577?logo=discord)](https://discord.gg/CFjRYmE)

<img align="right" src="https://github.com/turanszkij/wickedengine-gifs/raw/main/character_grass.gif" width="320px"/>

#### Linux
To build the engine for Linux, use Cmake. You can find a sample build script for Ubuntu [here](.github/workflows/build.yml) (in the linux section). On the Linux operating system, you will need to ensure some additional dependencies are installed, such as Cmake (3.7 or newer), g++ compiler (C++ 17 compliant version) and SDL2. For Ubuntu 20.04, you can use the following commands to install dependencies:

<img align="right" src="https://github.com/turanszkij/wickedengine-gifs/raw/main/inverse_kinematics.gif" width="320px"/>

```bash
sudo apt update
sudo apt install libsdl2-dev
sudo apt install build-essential
```
To build the engine, editor and tests, use `cmake` and then `make`:
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

If you want to develop an application that uses Wicked Engine, you will have to link to libWickedEngine.a and `#include "WickedEngine.h"` into the source code. For examples, look at the Cmake files, or the Tests and the Editor applications.

You can also download prebuilt and packaged versions of the Editor and Tests here (requires Github sign in): [![Github Build Status](https://github.com/turanszkij/WickedEngine/workflows/Build/badge.svg)](https://github.com/turanszkij/WickedEngine/actions)

If you have questions or stuck, please use the `linux` communication channel on Discord: [![Discord chat](https://img.shields.io/discord/602811659224088577?logo=discord)](https://discord.gg/CFjRYmE)

#### Xbox Series X|S
To build for Xbox Series natively, download and install the Xbox SDK from your Xbox developer account. Using the latest version of Visual Studio, create a new static library project for the Xbox Series platform and reference the WickedEngine_SOURCE shared project. Xbox specific extension files required for building, or sample projects will be provided for registered Xbox developers on request.

#### PlayStation 5
To build for PlayStation 5, download and install the PlayStation 5 SDK from your PlayStation developer account. Using the latest Visual Studio, create a new PlayStation 5 static library project and reference the WickedEngine_SOURCE shared project. PlayStation 5 specific extension files requierd for building, or sample projects will be provided for registered PlayStation developers on request.


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
