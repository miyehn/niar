# niar

A playground to test my patience.

9/7/22 update: Oh man this information is outdated again... Documentation updates coming in the next few days.

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

## Usage

### Run

```
./ellyn
```

WASD to move the camera, E/Q to move up/down. LMB drag to rotate camera. ESC to quit.

When pathtracer is set as the active renderer, camera control is disabled, but instead some pathtracer-specific controls become effective. Look for `PathtracerController` in the scene hierarchy for details.

### Render to file

Something like:
```
./asz -w 200 -h 150 -o output.png
```

### Configuration

See `config/global.ini` for properties that get loaded on program start. It gets loaded once and stays effective for the duration of the program.

There's also `config/pathtracer.ini` that gets loaded when the path tracer initializes. It automatically hot reloads, so I use it for tweaking path tracer settings.
