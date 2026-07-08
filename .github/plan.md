# TAA Review Plan: Temporal Anti-Aliasing

Side quest, not tracked in `.github/path-to-restir.md`.

Goal: add temporal anti-aliasing to the deferred renderer, resolved before
tonemapping. TAA reuses two things Milestone 3 of the RTGI roadmap already
built: `PrevViewMatrix`/`PrevUnjitteredProjectionMatrix` in `ViewInfo`, and the `GMotion`
G-buffer attachment (`RG16F`, `currentUV - prevUV`) produced in
`geometry.vert`/`geometry.frag`. No separate "add motion vectors" chunk is
needed because of this. Each chunk below still keeps one concern isolated so
each is reviewable and testable on its own, following the same shape as the
Milestone 3 review chunks.

## Review Chunk 1: Sub-Pixel Camera Jitter (Infrastructure Only)

Purpose: introduce a per-frame low-discrepancy subpixel jitter offset that TAA
needs to gather more than one sample position per pixel over time, without yet
touching how anything is rendered.

Implementation scope:

- Add a jitter sequence (e.g. Halton(2,3), 8-16 sample period) computed from
  the frame counter, producing a per-frame offset in the range [-0.5, 0.5]
  pixels.
- Add a `JitterOffset` field (`vec2`) to `ViewInfo` in
  `shaders/cshared/cshared_common.h` with a `static_assert` confirming its
  offset stays 16-byte aligned.
- Compute the jitter offset on the CPU alongside the other per-frame
  `ViewInfo` fields (`src/Render/Renderers/Renderer.cpp`).
- Add `config/taa.ini` with `enabled: 0` as the default. While disabled, the
  uploaded `JitterOffset` stays `(0, 0)` regardless of frame index.
- Do not yet apply the jitter to `Camera::camera_to_clip()`'s output used for
  rendering; this chunk only computes and uploads the value.

Review checklist:

- `ViewInfo` layout stays alignment-safe; the `static_assert` confirms the new
  field's offset.
- With `taa.ini`'s `enabled: 0` (default), `JitterOffset` is always `(0, 0)`
  and rendering is pixel-identical to before this chunk.
- With `enabled: 1`, the uploaded jitter value changes frame-to-frame (verify
  via a debug read), but nothing consumes it yet, so pixels still don't move.
- The jitter sequence is deterministic and repeats cleanly after its period.

Done when:

- Every frame's `ViewInfo` carries a valid per-frame jitter offset in pixel
  space, computed but unused by any shader or the rasterizer.


## Review Chunk 2: TAA Render Component Skeleton + Ping-Pong Color History

Purpose: give TAA the same kind of persistent per-frame state `GI` has: a
component that owns ping-pong history color textures and is wired into the
frame without changing the visible image yet.

Implementation scope:

- Add a new `TAA` render component (`src/Render/RendererComponents/TAA.h`/
  `.cpp`), modeled directly on `GI`: an `InitInfo`, and
  `init`/`release`/`clear`/`clear(VkCommandBuffer)`/`render` methods.
- Add two ping-pong history color textures in `R16G16B16A16_SFLOAT` (matching
  `sceneColor`'s format), alternating read/write roles via `frameIndex % 2`,
  following `GI::history[2]`'s pattern.
- Add a resolved-output texture that post-processing will read instead of raw
  `sceneColor`.
- `InitInfo` takes `sceneColor` and `GMotion` as external inputs (both already
  exist; no new G-buffer attachment).
- Add a `TaaResolveCS` compute shader wrapper (modeled on `RtgiGenerateCS`)
  that, for this chunk only, does a pass-through copy: reads `sceneColor`,
  writes it unchanged to both the resolved output and the write-history
  texture. No blending yet.
- Wire `TAA taa;` into `DeferredRenderer`, dispatched between the lighting
  pass and post-processing. Update post-processing to read TAA's resolved
  output instead of `sceneColor` directly when TAA is enabled, and
  `sceneColor` directly when it's disabled via `taa.ini`.

Review checklist:

- Both history textures and the resolved-output texture are created,
  transitioned, and destroyed cleanly; `clear()` zeroes all of them.
- With TAA enabled and only a pass-through copy in the shader, visual output
  is pixel-identical to before this chunk (jitter still isn't applied yet).
- With TAA disabled via config, post-processing reads `sceneColor` directly
  and no TAA resources are touched that frame.
- Descriptor bindings for `sceneColor`, `GMotion`, read-history (sampled),
  and write-history + resolved output (storage) follow the existing
  binding-number convention.

Done when:

- `TAA` owns a stable ping-pong pair of history textures and a resolved
  output, wired into the frame between lighting and post-processing, with no
  visual change yet.


## Review Chunk 3: Apply Jitter + Naive Same-Pixel Exponential Blend

Purpose: prove the temporal loop works by actually jittering the rendered
image and blending it with same-pixel history, before adding reprojection.

Implementation scope:

- Apply `JitterOffset` to the projection matrix used for rendering the base
  pass, as a clip-space offset (`clipPos.xy += JitterOffset.xy * clipPos.w`)
  in `geometry.vert`. Keep the unjittered matrices for reprojection: motion
  vectors must stay computed from unjittered current/previous clip positions,
  or they'll carry jitter noise.
- Change `TaaResolveCS` from pass-through to a real blend:
  `resolved = mix(currentColor, sameePixelHistory, historyWeight)`, with a
  configurable `historyWeight` (default e.g. 0.9) in `taa.ini`.
- Write `resolved` to both the resolved output and the write-history texture.

Review checklist:

- A still camera shows the jittered image visibly softening/converging over a
  few frames; a moving camera shows trailing/ghosting (expected and
  acceptable at this stage, same as GI Milestone 3 Chunk 3).
- `GMotion` output is unaffected by jitter — compare `GMotion` with jitter on
  vs off on a static-camera frame; values should match.
- Disabling TAA via config returns to the unjittered, unblended raw image.
- `historyWeight` is config-driven and hot-reloads.

Done when:

- Still camera visibly converges toward a smoother anti-aliased image; camera
  motion produces expected ghosting to be fixed by reprojection.


## Review Chunk 4: Reprojection Using Motion Vectors

Purpose: use the existing `GMotion` G-buffer attachment to sample history from
where the current pixel's surface was last frame, removing camera-motion
ghosting.

Implementation scope:

- In `TaaResolveCS`, read the per-pixel motion vector from `GMotion` and
  compute `prevUv = currentUv - motionVector` (same convention already used by
  `rtgi_generate.comp`).
- Replace the same-pixel history lookup from Chunk 3 with a bilinear sample of
  the read-history texture at `prevUv`.
- If `prevUv` falls outside `[0, 1]`, mark history invalid for this pixel and
  fall back to the current color only (no blend).
- No neighborhood clamping yet; this chunk isolates reprojection correctness
  only, matching GI's Chunk 4 scope.

Review checklist:

- Camera rotation/translation no longer leaves the same persistent smear as
  Chunk 3; newly revealed geometry shows only the current frame's color, not
  stale history.
- Out-of-bounds reprojection gracefully falls back to current-only.
- Static-camera convergence still behaves as in Chunk 3.
- Reuses the existing `GMotion` texture read-only; no new G-buffer attachment
  added.

Done when:

- Camera movement no longer leaves persistent ghosting from stale screen
  positions, while a still camera still converges.


## Review Chunk 5: Neighborhood Color Clamping (Anti-Ghosting)

Purpose: reject stale or incompatible history contributions that reprojection
alone can't catch (disocclusion, newly revealed geometry, fast-moving thin
objects), using the standard TAA neighborhood-clamp technique rather than
GI's depth-threshold approach.

Implementation scope:

- Sample a small neighborhood (3x3 minimum) of the current frame's
  `sceneColor` around the current pixel; compute a per-channel min/max (or
  variance clipping if min/max proves too aggressive) to build a local color
  AABB.
- Clamp the reprojected history sample into that AABB before blending, so
  history that no longer matches the local neighborhood gets pulled toward
  the current frame's plausible range instead of causing visible ghosting.
- Optionally combine with a lightweight depth-based disocclusion check (reuse
  `SceneDepth`/`PrevViewMatrix`, same derivation `rtgi_generate.comp` already
  uses) as a secondary signal, if clamping alone leaves visible artifacts.
- Add any new clamp-related tuning values (e.g. AABB expansion factor) to
  `taa.ini`.

Review checklist:

- Disocclusion (camera reveals new geometry) no longer smears old background
  color onto new surfaces.
- Fast-moving foreground objects don't leave obvious color trails.
- Clamping doesn't visibly reduce convergence quality on a still camera
  (compare against Chunk 4 still-camera output).
- New config values hot-reload without crashing.

Done when:

- Disocclusion and fast motion no longer produce visible ghosting, while a
  still camera continues to converge to an anti-aliased result.


## Review Chunk 6: Pipeline Integration Cleanup And Config

Purpose: finish the feature by confirming the pipeline wiring is clean,
tonemapping consumes the resolved TAA output (not raw `sceneColor`), and the
feature is easy to compare on/off.

Implementation scope:

- Confirm post-processing's tonemap pass reads TAA's resolved output when TAA
  is enabled, and `sceneColor` directly when disabled (wired in Chunk 2;
  re-audit after Chunks 3-5 changed the resolve shader).
- Audit `TAA::clear`/`TAA::render` for barrier correctness and remove any
  scaffolding from earlier chunks (e.g. the Chunk 2 pass-through code path, if
  still reachable).
- Confirm `taa.ini`'s `enabled` toggle fully bypasses jitter application,
  resolve dispatch, and history bookkeeping when off. Unlike GI's Milestone 3
  cleanup — where `maxSampleCount: 0` alone could disable accumulation without
  a dedicated toggle — TAA genuinely needs this toggle, because disabling TAA
  also means skipping jitter applied to the rendering projection matrix, which
  has no equivalent "set to zero" shortcut.
- Leave debug views (jittered-vs-resolved comparison, clamp AABB
  visualization) as optional follow-up; not required to close this out.

Review checklist:

- Toggling `enabled` off in `taa.ini` shows the raw, unjittered, unblended
  image; toggling on restores jitter + resolved anti-aliased output.
  Hot-reload works.
- No stale pass-through code path remains reachable.
- Tonemapping never reads stale/uninitialized TAA output right after TAA is
  re-enabled (history should be treated as invalid on the enabling frame).

Done when:

- TAA can be toggled on/off cleanly via config, tonemapping always reads the
  correct upstream texture, and the feature is clean enough to leave alone.


## Suggested PR Grouping

If the chunks feel too small as individual PRs, group them this way:

- PR 1: Chunk 1, jitter infrastructure.
- PR 2: Chunk 2, TAA component skeleton and ping-pong color history.
- PR 3: Chunk 3, apply jitter and naive same-pixel blend.
- PR 4: Chunk 4, motion-vector reprojection.
- PR 5: Chunk 5, neighborhood color clamping.
- PR 6: Chunk 6, pipeline integration cleanup and config.

Avoid grouping across the main risk boundaries:

- jitter application correctness (clip-space offset math, keeping motion
  vectors unjittered)
- descriptor layout changes (ping-pong texture additions)
- resolve blend formula and history validity semantics
- reprojection correctness (out-of-bounds handling)
- neighborhood clamping tuning (AABB expansion, clamp gamma)
- read/write ordering between lighting, TAA, and post-processing
