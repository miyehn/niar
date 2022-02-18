# niar

A playground to test my patience.

Roadmap (as of 2/19/2022)
* keep building the scene in Blender, write more tools, and get used to the content creation pipeline in general
* batching draw calls
* figure out foliage? even animate them w compute?
* area lights
* RTX shadow
* volumetrics (real-time and path tracing)
* global illumination that takes light leakage into account

---

**For real-time rendering**, here's a selected list of its features:
* common Vulkan utilities to save my sanity
* Dear ImGui and RenderDoc integration
* loading gLTF scenes and basic scene management
* rasterization and compute pipelines, and utilities to easily modify/extend them into new variants
  * For rasterization, it uses PBR deferred pipeline by default
* RTX pipeline using the `VK_KHR_ray_tracing_pipeline` extension

And some now-deleted features back when it still used OpenGL:
* shadow mapping for point and directional lights with percentage closer filtering
* automatic adjustments of light camera and its frustum based on view frustum and scene AABB

<img src="img/fchouse-export-test.png" width=640></img>
(WIP model of our free company house in FF14)

<img src="img/water_tower_10_24.jpg" width=640></img>

<img src="img/water_tower_detail_10_24.jpg" width=640></img>  
(The above water tower model and its textures are from www.animatedheaven.weebly.com)

**For offline rendering**, it has 2 multi-threaded CPU path tracer implementations:
* One is a regular C++ path tracer - sort of rebuilt [Scotty3D](https://github.com/cmu462/Scotty3D)'s pathtracer part from scratch, plus depth of field.
* Another is a SIMD path tracer implemented with [Intel ISPC](https://ispc.github.io/), which has mostly the same features as above but renders ~5x faster. This implementation is part of our CMU 15-418F20 final project. Project members include Michelle Ma and I (Rain Du). See [pathtracer-standalone](https://github.com/miyehn/niar/tree/pathtracer-standalone) branch or our [project site](https://miyehn.me/SIMD-pathtracer/) for more information.

<img src="img/dof.jpg" width=480></img>


There're also some **utilities** for testing (see implementation notes for details)
* load input from disk for initial configuration
* command line configuration of properties (CVars)
* debug draw overlays

Also check out the [grass-sim](https://github.com/miyehn/glFiddle/tree/grass-sim) branch which is not integrated into this branch yet

## Known issues about the ISPC path tracer

* ISPC: `gather_rays` doesn't seem buggy but the two calls to it inside `trace_rays` seem to mess up the ray tasks buffers. That function currently falls back to calling `trace_ray`.
* ISPC path tracer is slightly buggy when multithreaded is enabled and crashes under some very specific settings.
* TODO: optimize BVH traversal in ISPC
* TODO: avoid allocating and deallocating buffers every time the ISPC function is called

## Implementation notes (code tour)

(Since I work on this on and off, I better write this down to help myself remember what I did when I pick it up)

### Run

```
./niar
```

Press `ESC` to quit.

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

### Render to file

Something like:
```
./niar -w 200 -h 150 -o output.ppm
```

Note that currently rendering to file automatically loads the cornell box scene.

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
