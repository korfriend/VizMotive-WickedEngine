# New Vizualization Framework Engine
- This project starts with the compact version of Wicked Engine (first milestone)
- And adding my previous rendering engine features to this project while refactoring/compacting the underlying datastructures
- Jobsystem as Thread manager will be modified for supporting MD project
- High-level (COM-based) APIs (not exposing the core datastructures, auto-generation for a unified header incluing underlying components)

## Our Engine (based on Wicked Engine)

VizMotive Engine is an open-source 3D engine for scientific data visualizations. Use this as a C++ framework for your own graphics projects.<br/>
This project is based on <a href="https://github.com/turanszkij/WickedEngine/">Wicked Engine</a>.

### Platforms:
- Windows 10 or newer
- Linux
- UWP
- Xbox Series X|S
- PlayStation 5 [in progress]

### Examples:

#### Initialization (C++):
```cpp
// Include engine headers:
#include "WickedEngine.h"

// Create the Wicked Engine application:
wi::Application application;

// Assign window that you will render to:
application.SetWindow(hWnd);

// Run the application:
while(true) {
   application.Run(); 
}
```

#### Basics (C++):
```cpp
application.Initialize(); // application will start initializing at this point (asynchronously). If you start calling engine functionality immediately before application.Run() gets called, then you must first initialize the application yourself.

wi::initializer::InitializeComponentsImmediate(); // (Optional) allows to initialize all components immediately and block the application until finished. Otherwise the initialization will take place at the first application.Run() asynchronously. This is useful if you want to start using other parts of the engine before application.Run() is called.

wi::RenderPath3D myGame; // Declare a game screen component, aka "RenderPath" (you could also override its Update(), Render() etc. functions). 
application.ActivatePath(&myGame); // Register your game to the application. It will call Start(), Update(), Render(), etc. from now on...

wi::scene::LoadModel("myModel.wiscene"); // Simply load a model into the current global scene
wi::scene::GetScene(); // Get the current global scene

wi::scene::Scene scene2; // create a separate scene
wi::scene::LoadModel(scene2, "myModel2.wiscene"); // Load model into a separate scene
wi::scene::GetScene().Merge(scene2); // Combine separate scene with global scene

myGame.setFXAAEnabled(true); // You can enable post process effects this way...

wi::RenderPath2D myMenuScreen; // This is an other render path, but now a simple 2D one. It can only render 2D graphics by default (like a menu for example)
application.ActivatePath(&myMenuScreen); // activate the menu, the previous path (myGame) will be stopped

wi::Sprite mySprite("image.png"); // There are many utilities, such as a "sprite" helper class
myMenuScreen.AddSprite(&mySprite); // The 2D render path is ready to handle sprite and font rendering for you

wi::audio::Sound mySound;
wi::audio::CreateSound("explosion.wav", &mySound); // Loads a sound file
wi::audio::SoundInstance mySoundInstance;
wi::audio::CreateSoundInstance(&mySound, &mySoundInstance); // Instances the sound file, it can be played now
wi::audio::Play(&mySoundInstance); // Play the sound instance
wi::audio::SetVolume(0.6, &mySoundInstance); // Set the volume of this soundinstance
wi::audio::SetVolume(0.2); // Set the master volume

if (wi::input::Press(wi::input::KEYBOARD_BUTTON_SPACE)) { wi::audio::Stop(&mySoundInstance); } // You can check if a button is pressed or not (this only triggers once)
if (wi::input::Down(wi::input::KEYBOARD_BUTTON_SPACE)) { wi::audio::Play(&mySoundInstance); } // You can check if a button is pushed down or not (this triggers repeatedly)
```


### Graphics API:
The default renderer is `DirectX 12` on Windows and `Vulkan` on Linux. The `DirectX 11` renderer is no longer available starting from version 0.57.0, but it can be found on the <a href="https://github.com/turanszkij/WickedEngine/tree/dx11-backup">dx11-backup branch</a>.
You can specify command line arguments (without any prefix) to switch between render devices or other settings. Currently the list of options:
<table>
  <tr>
	<th>Argument</th>
	<th>Description</th>
  </tr>
  <tr>
	<td>dx12</td>
	<td>Use DirectX 12 rendering device</td>
  </tr>
  <tr>
	<td>vulkan</td>
	<td>Use Vulkan rendering device</td>
  </tr>
  <tr>
	<td>debugdevice</td>
	<td>Use debug layer for graphics API validation. Performance will be degraded, but graphics warnings and errors will be written to "Output" window</td>
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

### Other software using Wicked Engine
- <a href="https://www.game-guru.com/max">Game Guru MAX</a>: Easy to use game creator software
- <a href="https://www.youtube.com/watch?v=0SxXmnSQ6Q4">Flytrap</a>: Demoscene production by qop
- <a href="https://youtu.be/mbmNU5QVM8A?si=9sDMS1LrMsz03f5r">doddering</a>: Demoscene production by qop
- Your project: add your project to this readme and open a pull request

<br/>

### Troubleshooting
(At this moment, no other my own troubleshooting policy. Just refer to the following, which is decribed in the original Wicked Engine github)
If you are having trouble getting the applications to run, make sure that you satisfy the following conditions:
- If you built the application with Visual Studio, run it from the Visual Studio environment, where the executable working directory is set up to be the Project directory (not the build directory where the exe will be found)
- If you want to run an application without Visual Studio, either copy the executable from the BUILD directory to the correct project directory, or set the working directory appropriately. You can also check the Working directory setting in Visual Studio to find out the right working directory of every project. 
- If you want to build UWP application, then you will first need to build the shaders into a shader dump. For that, build and run the `offlineshadercompiler` project with the `hlsl6 shaderdump` command line arguments. If the `wiShaderDump.h` file is successfully generated, rebuilding the engine will embed all the shader files so they are not loaded separately. But embedded shaders also cannot be recompiled during runtime.

- If you experience crashes, follow these steps to find out the problem:
	- make sure your environment is up to date, with latest graphics drivers and operating system updates.
	- see if there is a wiBackLog.txt in your user temp folder (for example: C:\Users\username\AppData\Local\Temp), and request help on Discord or Github issue
	- build the engine in Debug mode and try to run it, see where it crashes, provide call stack on Discord or Github issue
	- run the engine with the `debugdevice` command argument and post the text from your console output window when the crash happens
		- for very advanced users, using `gpuvalidation` with `debugdevice` will print additional graphics debug information
