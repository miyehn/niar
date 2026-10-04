# Milestone 4 — Reservoir Data Model and Sample Buffer (work breakdown)

Companion to [path-to-restir.md](path-to-restir.md) §"Milestone 4". This file
slices M4 into chunks that can each be **built, reviewed, and verified in
isolation**. Implement one chunk per prompt; the author code-reviews and
runtime-tests each before the next.

Goal of M4 (unchanged): convert the per-pixel GI signal from radiance-average
accumulation into a single-candidate **reservoir** carrying a reusable
**sample**, **without changing the visible image**. This is the enabling refactor
for M5 (temporal reuse) and M6 (spatial reuse). No reuse logic lands in M4.

## Invariants that hold across every chunk

Review each diff against these:

- **The non-debug image must not change.** `IndirectLighting`
  (`VK_FORMAT_R16G16B16A16_SFLOAT`) is the composite input consumed by
  `shaders/deferred_lighting.frag:61` (`indirectDiffuse = IndirectLighting * GColor`)
  and `shaders/translucency_lit.frag`. The reservoir path must resolve into that
  same texture. Any visible change before Chunk 3's A/B gate is a bug.
- **`gi.maxSampleCount: 0` is the naive-1-spp reference.** It collapses the
  running-average path (`shaders/rtgi_generate.comp:159-167`) to the raw single
  sample. Chunk 3's correctness gate is: reservoir resolve == this reference,
  pixel-for-pixel.
- **16-byte ABI discipline.** New shared structs follow the existing pattern in
  `shaders/cshared/cshared_common.h`: `namespace glm` wrap on the C++ side,
  `CSHARED_ALIGNAS_16`, and `static_assert`s on `sizeof`/`alignof`/`offsetof`.
  Update the C++ asserts and the GLSL declaration in the same commit.
- **Reservoirs are resampled, never interpolated.** All reservoir/sample storage
  reads are point-sampled at integer pixels (`imageLoad` / buffer index), never
  bilinear `texture()`. (The running-average history reads *are* bilinear; do not
  copy that pattern for reservoir buffers.)
- Keep the running-average path intact and selectable until M5 removes it. M4
  adds the reservoir path beside it behind a config toggle.

## Files in scope

- `shaders/cshared/` — new `reservoir.h` (shared ABI).
- `shaders/rtgi_generate.comp` — sample emit, RIS, resolve, debug writes.
- `src/Render/RendererComponents/GI.h` / `GI.cpp` — storage, descriptors,
  barriers, clear/release, push data.
- `config/deferred.ini` — `gi.useReservoir`, `gi.debugView` toggles.
- Possibly a small GLSL include for reservoir ops (`reservoir_common.glsl`).

---

## Chunk 1 — Reservoir ABI structs (compile-only)

**Scope.** Define the shared data model; nothing reads or writes it yet.

- Add `shaders/cshared/reservoir.h` with `GiSample` and `GiReservoir`:
  - `GiSample`: sample point position (`vec3`), sample normal (`vec3`), outgoing
    radiance `L_o` toward the visible point (`vec3`) — the current per-ray result
    (emissive + direct-lit, or background on miss). Keep fields explicit
    `vec3`/`float` for now; note octahedral-normal / packing as an M8 size
    optimization, not now.
  - `GiReservoir`: the selected `GiSample`, plus reservoir state `w_sum` (float),
    `M` (uint), `W` (float, unbiased contribution weight), and a `valid`/flags
    `uint`. Visible point `x_v` stays implicit from the G-buffer (roadmap).
  - Lay out for `std430` storage-buffer use (this is the storage decided in
    Chunk 2). Pad to 16-byte alignment explicitly.
  - C++ side: wrap in `namespace glm`, add `static_assert`s on
    `sizeof`/`alignof`/`offsetof` mirroring `cshared_common.h`.
- `#include` the header from `rtgi_generate.comp` (and the C++ TU that will use
  it) so both sides actually compile the declaration.

**Review focus.** Field layout, padding, and that the asserts pin every offset.
This is the one place silent GPU corruption starts.

**Verify.** `--target ellyn` builds (C++ static_asserts are the ABI test) and the
shader compiles at runtime (hot reload / launch). App runs visually unchanged —
the struct is declared but unused. Note: a declared-but-unused struct is
intentional staging here; Chunk 2 consumes it immediately.

---

## Chunk 2 — Reservoir GPU storage + descriptor plumbing (no visible change)

**Scope.** Allocate the per-pixel reservoir storage and wire it through
descriptors and barriers, but keep the visible output on the running-average
path.

- Add a **ping-pong pair** of device-local storage buffers
  `reservoirBuffers[2]`, each `width * height * sizeof(GiReservoir)`, in
  `GI.cpp` (`init`/`release`, plus `clear`). Ping-pong now so M5 gets the history
  slot for free; in M4 only the current buffer is written.
- Add descriptor bindings (read-prev + write-current) to `giSetLayout`, following
  the existing `Slot_*` convention and the `historyWriteSlot`/`readHistoryIndex`
  wiring already used for `history[]` in `GI::init`. Add barriers/layout handling
  consistent with the existing history buffers in `GI::render` and `GI::clear`.
- Add `gi.useReservoir` to `config/deferred.ini`, default `0`. Plumb it into
  `RtgiPushData` (like `maxSampleCount`).
- In `rtgi_generate.comp`, under `useReservoir`, write a **zero-initialized**
  `GiReservoir` to the current buffer. Do **not** resolve from it — visible
  output still comes from the running-average path regardless of the toggle.

**Review focus.** Buffer sizing/stride vs. `sizeof(GiReservoir)`, ping-pong index
math matches the existing `frameIndex % 2` history scheme, barrier
producer/consumer correctness, clear-on-alloc so frame 0 is defined.

**Verify.** Builds; image unchanged with `useReservoir` both 0 and 1. In
RenderDoc, confirm the reservoir buffers exist, are cleared, and receive writes
when `useReservoir:1`. (Behavioral no-op by design — this chunk is pure infra.)

---

## Chunk 3 — RIS emit + reservoir resolve (the equivalence gate)

**Scope.** The core representation change, behind `gi.useReservoir`.

- Factor a small `reservoir_common.glsl` (or inline) with `updateReservoir` /
  `finalizeReservoir` helpers.
- In `rtgi_generate.comp`, when `useReservoir`:
  1. Build the `GiSample` from the existing ray result: hit → `shadeCommittedHit`
     result as `L_o` plus the reconstructed secondary-hit position/normal; miss →
     background `L_o` (sample point at infinity along `rayDir`, flagged).
  2. Insert into a fresh **1-candidate** reservoir via RIS. Target function
     `p_hat` = luminance of the sample's contribution at the visible point.
     Source pdf = the cosine-hemisphere pdf actually used to draw `rayDir`.
  3. Compute `W = w_sum / (M * p_hat)` (with `M == 1` here); guard `p_hat == 0`.
  4. Store the finalized reservoir to the current buffer.
  5. Resolve: `IndirectLighting = contribution(x_v, sample) * W`, i.e. the same
     shading the running-average path writes, weighted by `W`.
- Leave the running-average path as the `useReservoir:0` branch.

**Review focus.** `p_hat`/`W` consistency so the 1-candidate case is unbiased —
this is the roadmap's #1 pain point. Confirm the resolve reconstructs the *same*
`contribution` the naive path uses (so `W` is the only difference and it cancels
to 1 in expectation for one candidate). Division-by-zero and miss-sample guards.

**Verify (primary M4 correctness gate).** With `gi.useReservoir: 1` vs.
`gi.useReservoir: 0` + `gi.maxSampleCount: 0` (naive 1-spp), the images must
match. Expect bit-identical to within float reassociation; a large systematic
brightness delta means `W`/`p_hat` are inconsistent. Toggle live via hot reload
for direct A/B.

---

## Chunk 4 — History round-trip + debug views + toggle state

**Scope.** Prove the sample buffer survives write→read across frames (M4's second
"done when"), and add inspection tooling. Still no reuse.

- Write the finalized reservoir into the **write** buffer and, next frame, read
  the **prev** buffer at the reprojected integer pixel (via existing motion
  vectors) — for *validation/debug only*, not combined into the estimate (that is
  M5).
- Add `gi.debugView` (int, default 0) to `config/deferred.ini` and `RtgiPushData`.
  Modes write directly into `IndirectLighting` (matches the ad-hoc debug style;
  there is no generic texture viewer):
  1. sample position (remapped), 2. sample radiance `L_o`, 3. `W` (heatmap),
  4. reservoir validity / miss flag, 5. **prev-frame reservoir readback** at the
  reprojected pixel (this is the round-trip proof).
- Confirm `debugView: 0` leaves the resolved image untouched.
- Finalize the toggle story: keep `useReservoir` + running-average path both
  present (M5 removes the average). Document the two config keys with comments in
  `deferred.ini` like the existing `gi:`/`taa:` entries.

**Review focus.** Debug writes are strictly gated (no cost/no effect when
`debugView:0`), reprojected reads are point-sampled at integer pixels, round-trip
read uses the correct prev/ping-pong buffer.

**Verify.** Each `debugView` mode shows the expected content; mode 5 reprojects
last frame's sample radiance stably under slow camera motion (round-trip ABI
intact). With `debugView:0` the Chunk 3 equivalence still holds.

---

## Exit criteria for M4 (from the roadmap, mapped to chunks)

- [ ] 1-candidate reservoir + correct RIS weights reproduces the naive 1-spp
      signal — **Chunk 3**.
- [ ] Sample buffer round-trips through history with the ABI intact — **Chunks 1
      + 4**.
- [ ] Running-average path still selectable for A/B through the transition —
      **Chunks 2–4** (removed in M5).

## Deliberately deferred to M5+ (do not creep into M4)

Temporal/spatial reservoir **reuse**, `M` capping, normal/material/roughness
rejection, blue-noise sampling, and the spatial Jacobian. M4 only establishes the
representation and proves equivalence.
