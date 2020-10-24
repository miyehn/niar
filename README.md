# niar

A playground to test my patience.

**For offline stuff**, it has a multi-threaded CPU pathtracer - sort of rebuilt [Scotty3D](https://github.com/cmu462/Scotty3D)'s pathtracer part from scratch. 

<img src="img/dof.jpeg" width=400></img>

**For real-time rendering**, so far it has a very basic deferred PBR pipeline and can load metallic-roughness materials. It supports point and directional lights, both can cast shadows, and has some very basic optimizations in addition to plain shadow mapping (automatic adjustments of light camera and its frustrum based on scene AABB and view frustrum, percentage closer filtering for soft shadow edges / anti aliasing).

<img src="img/water_tower_10_24.jpg" width=480></img>

<img src="img/water_tower_detail_10_24.jpg" width=480></img>  
(The above water tower model and its textures are from cgtrader.com)

Also check out the [grass-sim](https://github.com/miyehn/glFiddle/tree/grass-sim) branch which is not integrated into master yet (because I mainly develop on macOS and OpenGL compute shaders are not supported)

## Some notes

(Since I probably won't be able to work on this for quite another while, I better write this down to help myself remember what I did when I pick it up next time)

### Basic usage

Build and run the `niar` binary. Works on both mac and windows. `ESC` to quit.

There's a rasterizer and a pathtracer. Pathtracer is disabled by default, `TAB` to toggle between enabled and disabled. It initializes itself when it's first enabled, then keeps the setting, buffers, etc. even when it's later disabled.

When pathtracer is disabled:
* WASD to move camera, E/Q to move up/down.
* LMB drag to rotate camera.

When pathtracer is enabled:
* Space bar to start/pause render. Note that when multitreaded rendering is enabled, it might take a little longer for the threads to finish the already-started tile when pausing command is sent.
* `0` to clear the buffer.
* `SHIFT` + LMB click to trace a debug ray through the clicked pixel and draw it as yellow line segments. There's also some (perhaps no longer helpful) console output.
* RMB click to dismiss the debug ray overlay.
* `Alt` + LMB click to set camera focal length to the scene depth at the selected pixel. Focal length is only effective when DOF (depth of field) is on.

### Configuration

See `config.ini` for properties that get loaded on program start. All settings have detailed comments there.

`config.ini` also specifies resource paths, including scene file (fbx), shaders and textures. Create materials in the `Materials` section, and assign materials to meshes in the `MaterialAssignments` section.

To configure properties at runtime, use the console as such (type into the window, not the console itself):
* `/` to start a command
* `ls` to list all properties (CVars) and their current values
* `set (property name) (new value)` to configure. `set` can be abbreviated as `s`. Ex: `s showdebugtex 1` to render the currently configured debug texture as overlay. Property names are case insensitive.
* `ls textures` or `ls t` lists all the textures that can be drawn as overlay.

<img src="img/cvars.png" height=400></img>

Too add a property in `config.ini`, first define that property in the config file, then in `Input.hpp` add it to the struct called `Cfg`, and assign the property in `Input.cpp`. Then use it anywhere as `Cfg.PropertyName`.

CVars can be defined / used anywhere. For the ones local to a file, define them with global scope inside some `.cpp` file. For the ones that need to be accessed everywhere, define in `Input.hpp` or even as part of the `Cfg` struct.

### Rendering pipeline

See `Scene::draw()`. It depends on the material set used (configure this in `config.ini`). `0` is basic; everything gets rendered in a single pass, no lighting is applied. `1` is basic lambertian without textures, and `2` is whatever material that gets assigned to each mesh. If a mesh has no material assigned to it, it falls back to default lambertian.

#### Deferred pipeline

Geometry pass (MRT) (3 G buffers: world position, normal, base color)  
↓  
Shadow maps are rendered for lights that cast shadows.  
↓  
Passes that uses shadow maps and the position & normal G buffers to produce shadow masks (basically stencils for light contribution).  
↓  
Lighting pass(es) for directional lights, with G buffers and shadow masks as input  
↓  
Lighting pass(es) for point lights done similarly, using additive blending
↓  
Exposure adjustment, as well as extracting bright pixels into a separate texture  
↓  
Horizontal and vertical gaussian blur passes to the bright pixels texture (bloom)  
↓  
Apply bloom result with rendered image, then do tone mapping and gamma correction

### Shader, texture and material resources

Currently all shaders, textures and materials are managed by the classes themselves, either in a private pool (accessable from outside by name using the `get` method), or as static read-only constants.

Shaders and materials are compiled and added to the pool on program start; textures are loadeded on first use.

### Materials and blit

Basically a material := a reference to a shader + a set of properties. There're two types of materials: generic and standard.

A generic material is one that can be used with any shader. It attempts to set all transformation matrices to its shader when used.
* Usage: create the material by giving its shader's name, define `set_parameters` if its shader requires properties that are not transformation matrices, then `use` just before drawing stuff.

A standard material is one that's associated with a specific shader.
* Creation: inherit from `Material` to create a new material class with all properties that should be stored with it. Define its `use` function where it should set all its properties to its shader, including the transformation matrices.
* Usage: create an instance of this material somewhere else, assign the properties, optionally define `set_parameters`, and `use`.
* To create a material that uses an existing shader, just edit `config.ini` to specify what shader to use and some properties, and assign it to meshes.

`Blit` is a special type of shader: instances all share the same vertex shader and are used to draw screen-space quads.
* Creation: create a `CONST_PTR` in `Blit` class, and `IMPLEMENT_BLIT` by specifing which shader to use (see the macro definitions).
* Usage: `Blit::name()->begin_pass()`, set properties depending on the shader, `Blit::name()->end_pass()`.
