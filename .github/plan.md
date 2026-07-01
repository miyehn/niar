# Milestone 3 Review Plan: Temporal Accumulation

Source roadmap: `.github/path-to-restir.md`, Milestone 3.

Goal: convert the noisy one-ray RTGI result into a temporally accumulated
indirect signal. The signal is already physically meaningful: sky/environment
misses, emissive secondary hits, and shadowed direct-lit bounce color all work.
The missing piece is convergence. Each chunk keeps one concern isolated so each
is reviewable and testable on its own.

## Review Chunk 1: Previous-Frame View Matrices In ViewInfo

Purpose: give the GPU both the current and previous frame's view and projection
matrices so that any shader can reproject a screen-space position to its
previous-frame screen-space location.

Implementation scope:

- Add `PrevViewMatrix` and `PrevProjectionMatrix` fields to the `ViewInfo`
  struct in `shaders/cshared/cshared_common.h`.
- Ensure the layout stays 16-byte aligned. Add `static_assert`s for the new
  fields.
- On the CPU side, copy the current frame's view and projection matrices into
  the previous-frame fields at the end of each frame, before uploading the next
  frame's `ViewInfo` UBO.
- No shader reads the new fields yet; this chunk is purely infrastructure.

Review checklist:

- `ViewInfo` layout is alignment-safe and the `static_assert`s confirm offsets
  for the new fields.
- The CPU upload path writes `PrevViewMatrix` and `PrevProjectionMatrix` from
  the just-submitted frame, not from two frames ago.
- No other ViewInfo consumers are broken by the struct growth.
- No rendering changes; visual output is identical.

Done when:

- Every frame's `ViewInfo` UBO contains valid previous-frame matrices ready to
  use for reprojection.


## Review Chunk 2: Ping-Pong History Textures In GI

Purpose: give the GI component the persistent per-frame state needed for
temporal accumulation: two history textures that alternate roles each frame, and
a per-pixel sample count texture.

Implementation scope:

- Add two history textures to `GI` in the same `R16G16B16A16_SFLOAT` format as
  `indirectLighting`. The RGB channels will hold the accumulated indirect
  radiance. The alpha channel will hold the linearized depth of the primary
  surface at that pixel, so the accumulation pass can detect disocclusion
  without keeping a separate previous-depth buffer.
- Add a per-pixel sample count texture (`R32_UINT`) that tracks how many samples
  have been accumulated at each pixel.
- Track which texture is the "read history" and which is the "write history" for
  the current frame, alternating each frame.
- Update `GI::init` to allocate and transition all new textures into appropriate
  initial layouts.
- Update `GI::release` to delete all new textures.
- Update `GI::clear` and `GI::clear(VkCommandBuffer)` to zero-fill all new
  textures and reset layouts.
- Extend `GI::InitInfo` with the new textures' needs if any external input is
  required (currently none expected).
- Update the descriptor set layout and per-frame `giDescriptorSets` bindings to
  expose the read-history texture and the sample-count texture as inputs, and
  the write-history texture and sample-count texture as storage image outputs.
  Keep binding numbers consistent with the existing layout convention.

Review checklist:

- Both history textures and the sample count texture are created, transitioned,
  and destroyed cleanly.
- Ping-pong index alternates correctly between frames; the read and write targets
  are never the same texture in the same frame.
- `clear()` zeroes all three new textures, not just `indirectLighting`.
- Descriptor set bindings stay aligned between C++ layout and the slot numbers
  used in the next chunk's shader work.
- No shader reads the new bindings yet; visual output is unchanged.

Done when:

- `GI` owns a stable ping-pong pair of history textures and a sample count
  texture, all properly managed across frames and scene reloads.


## Review Chunk 3: Naive Same-Pixel Temporal Accumulation

Purpose: prove that the accumulation loop works and that a still camera produces
a converging indirect signal, before adding any reprojection.

Implementation scope:

- Change `rtgi_generate.comp` from a pure write-only pass to an
  accumulation pass:
  - Add bindings for the read-history texture (sampled), the sample count
    texture (read/write storage), and the write-history texture (write-only
    storage image). Keep the existing `IndirectLighting` output binding for the
    composited result that deferred lighting reads.
  - Each pixel: read `prevAccumulated` and `prevCount` from the history and
    sample count textures at the same screen coordinate. Clamp `prevCount` to a
    configurable maximum (default 64, driven by `config/gi.ini`).
  - Compute the new accumulated value:
    `accumulated = (prevAccumulated.rgb * prevCount + newSample) / (prevCount + 1)`
  - Write the accumulated result to the write-history texture (RGB = accumulated
    indirect, A = current pixel's linearized view-space depth from the G-buffer).
  - Write `min(prevCount + 1, maxCount)` to the sample count texture.
  - Write the accumulated indirect RGB to `IndirectLighting` so the deferred
    composite is unchanged.
- Add the maximum sample count as a config key in `config/gi.ini`.
- Update `GI::render` to insert the barriers needed around the new storage image
  reads and writes, and to advance the ping-pong index after the pass.

Review checklist:

- A still camera causes the indirect buffer to converge visibly over several
  frames; noise decreases noticeably within a second or two.
- Moving the camera produces ghosting (expected and acceptable at this stage).
- The deferred composite output is not broken; `IndirectLighting` still feeds
  the same binding in the lighting pass.
- When GI is disabled, `clear()` zeroes the history and sample count textures so
  re-enabling starts fresh.
- The max sample count cap is config-driven and hot-reloads correctly.

Done when:

- Still camera noise visibly decreases over time through accumulated samples.


## Review Chunk 4: Reprojection Using Depth And Prev View Matrices

Purpose: use the previous-frame view and projection matrices to find where the
current pixel's surface was in the previous frame, so that camera movement no
longer leaves persistent ghosts.

Implementation scope:

- In `rtgi_generate.comp`, before reading history, compute the reprojected
  previous-frame UV for the current pixel:
  - Reconstruct the current pixel's world position from screen UV and G-buffer
    depth (the same path already used to compute the ray origin).
  - Project the world position through `PrevViewMatrix * PrevProjectionMatrix`
    to get the previous-frame clip position.
  - Convert clip position to a UV in [0, 1]. If the reprojected UV is outside
    [0, 1] (the surface went offscreen), mark the history as invalid.
- Replace the same-pixel history lookup from Chunk 3 with a bilinear sample of
  the read-history texture at the reprojected UV.
- Preserve the alpha channel depth value when the history is valid; when the
  history is invalid, write depth with no previous accumulation (count = 0).
- The sample count texture lookup should also use the nearest-neighbor reprojected
  UV (or clamp to zero for out-of-bounds). Bilinear blending of integer sample
  counts is not meaningful; use point sampling or take the minimum of the
  reprojected neighborhood.

Review checklist:

- Camera rotation and translation no longer leave obvious trails; new
  geometry appears without blending with stale history from the old view.
- Reprojected UV is computed only from the current frame's G-buffer depth and
  `PrevViewMatrix` / `PrevProjectionMatrix`; no new G-buffer attachment is
  added here.
- Out-of-bounds reprojection gracefully falls back to the new sample only.
- Static scenes still converge as before; accumulated sample count grows
  correctly when the camera is still.

Done when:

- Camera movement does not leave persistent ghosting from the old camera
  position, while a still camera still converges.


## Review Chunk 5: Per-Pixel History Rejection By Depth

Purpose: detect disocclusion and newly visible geometry so that the accumulation
resets those pixels rather than blending stale history into them.

Implementation scope:

- After computing the reprojected UV, compare the stored depth in the history
  alpha channel against the expected depth of the current surface seen from the
  previous frame:
  - From the current world position and `PrevViewMatrix * PrevProjectionMatrix`,
    derive the linearized view-space depth the current surface would have had
    last frame.
  - Read the stored linearized depth from the history texture alpha at the
    reprojected UV.
  - If the absolute or relative difference exceeds a configurable threshold,
    mark the history as invalid for this pixel.
- When history is invalid (depth mismatch or out-of-bounds reprojection), reset
  the accumulated value to the new sample only and write sample count 1.
- Add `HistoryDepthRejectionThreshold` to `config/gi.ini` with a sensible
  default.
- Keep normal-based rejection out of this chunk; depth alone is sufficient to
  catch the main disocclusion cases.

Review checklist:

- Moving the camera reveals new surfaces; those pixels reset rather than
  blending in stale background history.
- Surfaces that stay visible and correctly reprojected continue to accumulate.
- The threshold default is tight enough to reject real disocclusions but does
  not cause flickering on stationary geometry.
- Config key hot-reloads without crashing.

Done when:

- Disoccluded pixels reset gracefully instead of ghosting, while correctly
  reprojected pixels continue to converge.


## Review Chunk 6: Motion Vector G-Buffer Attachment

Purpose: move per-pixel reprojection offsets into a dedicated G-buffer
attachment so that the logic is computed once, is debuggable, and can grow to
handle per-object motion vectors later.

Implementation scope:

- Add a motion vector texture to the G-buffer, `RG16F`, storing the
  per-pixel screen-space UV delta: `current_uv - prev_uv`.
- Produce motion vectors by reprojecting depth in either a dedicated compute
  pass or a deferred G-buffer resolve step:
  - Reconstruct world position from G-buffer depth.
  - Project through `PrevViewMatrix * PrevProjectionMatrix` to get previous UV.
  - Output `currentUV - prevUV` as the motion vector.
  - This covers camera-only motion. Objects with independent transforms would
    need per-object previous model matrices; leave that for a future milestone.
- Update `GI::InitInfo` to accept the motion vector texture.
- Update `GI`'s descriptor set to bind the motion vector texture as an input.
- In `rtgi_generate.comp`, replace the inline reprojection computation from
  Chunk 4 with a texture read from the motion vector attachment. History lookup
  and depth rejection from Chunk 5 continue to use the reprojected UV and
  stored depth; the only change is how the reprojected UV is computed.
- Keep the inline reprojection path available behind a shader define or config
  flag only if the G-buffer motion vector path has a known gap; otherwise remove
  it cleanly.

Review checklist:

- Visual output matches Chunks 4 and 5 exactly when the scene has only camera
  motion; no new ghosting or rejection artifacts appear.
- Motion vector texture is transitioned to the correct layout before RTGI reads
  it.
- The G-buffer pass that writes motion vectors runs before RTGI; ordering is
  explicit in the render component, not hidden in barriers inside a helper.
- The design lets a future change substitute per-object motion vectors by
  updating the G-buffer pass only, without touching `GI.cpp` or the RTGI
  shader.
- The inline reprojection code from Chunk 4 is removed or clearly gated so it
  does not silently run alongside the new path.

Done when:

- Motion vectors are produced in the G-buffer and consumed by RTGI, with the
  same reprojection and rejection quality as before.


## Review Chunk 7: Milestone 3 Cleanup And Config

Purpose: finish the milestone by tightening the accumulation contract, removing
temporary scaffolding, and making it easy to compare raw and accumulated output.

Implementation scope:

- Add an `accumulation` toggle to `config/gi.ini` that bypasses history reuse
  and writes the raw one-sample result directly, without modifying any other
  GI behavior. This makes the noise reduction visible and reversible without a
  recompile.
- Audit `GI::clear` and `GI::render` for any lingering assumptions from before
  ping-pong: no reference to a single `indirectLighting` texture where two
  history textures now exist.
- Audit `rtgi_generate.comp` for stale comments or variable names that still
  describe single-frame direct writes rather than accumulation.
- Update `.github/path-to-restir.md` to mark Milestone 3 complete.
- Leave debug views (history age, rejected pixels, sample count heatmap) as
  optional follow-up work; do not require them to close the milestone.

Review checklist:

- Toggling `accumulation` off shows the raw noisy one-sample output; toggling
  it on restores the converged result. Hot-reload works.
- No stale code remains that assumed a single non-ping-ponged indirect texture.
- The roadmap still points clearly to Milestone 4 spatial filtering as next.
- The done conditions from `path-to-restir.md` Milestone 3 are all met:
  - still camera noise decreases over time
  - moving the camera does not leave obviously broken trails everywhere
  - disocclusion behavior resets the affected pixels instead of ghosting

Done when:

- Milestone 3's done conditions are verifiable, and the accumulation path is
  clean enough to build spatial filtering on top of in Milestone 4.


## Suggested PR Grouping

If the chunks feel too small as individual PRs, group them this way:

- PR 1: Chunk 1, previous-frame matrices in ViewInfo.
- PR 2: Chunk 2, ping-pong history and sample count textures.
- PR 3: Chunks 3 and 4, naive accumulation then reprojection.
- PR 4: Chunk 5, depth-based history rejection.
- PR 5: Chunks 6 and 7, motion vector G-buffer attachment and cleanup.

Avoid grouping across the main risk boundaries:

- descriptor layout changes (ping-pong texture additions)
- accumulation formula and sample count semantics
- reprojection correctness (out-of-bounds handling)
- depth rejection threshold interaction with reprojection
- motion vector G-buffer ownership and barrier ordering
