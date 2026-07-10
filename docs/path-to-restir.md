# Path to ReSTIR GI

This roadmap keeps `niar` moving toward ReSTIR GI without turning every adjacent
improvement into a renderer rewrite. The target is not production-quality path
tracing. The target is a debuggable, incremental ray-traced indirect lighting
path that grows from the current deferred renderer, RTX shadows, shader hot
reload, and bindless glTF material infrastructure.

Verdict of the latest audit (2026-07-10): **ReSTIR GI is still the right north
star, and nothing built so far is wasted** — the bindless material/geometry
tables, TLAS ray queries, secondary-hit direct lighting, motion-vector
reprojection, and history rejection are exactly the foundation ReSTIR GI needs.
The remaining plan is reordered to be **reservoir-first**: the old "bilateral
spatial filter next" milestone is replaced, because the current per-pixel signal
is a radiance-space running average that ReSTIR has to discard anyway. See
[Reassessment](#reassessment-2026-07-10) below.

## Current State

The RTGI foundation through temporal accumulation is in place, plus TAA:

- deferred is the current primary visibility path; RTGI is attached to it but
  kept renderer-agnostic on purpose
- RTGI runs as a compute pass (`shaders/rtgi_generate.comp`) that owns an
  indirect-lighting texture, ping-pong history textures, and a per-pixel sample
  count texture (`src/Render/RendererComponents/GI.*`)
- one cosine-weighted diffuse ray per visible G-buffer surface, through the
  shared TLAS; misses sample the sky/environment path
- committed hits resolve scene instance -> bindless material + geometry record,
  reconstruct UV/position/normal from primitive index + barycentrics, sample
  albedo/emissive/ORM, and return emissive + direct-lit outgoing radiance toward
  the primary surface (shared BRDF eval with deferred/translucent lighting)
- secondary-hit direct lighting uses the shared point/directional light buffers
  and ray-query shadow visibility against the current TLAS
- **temporal accumulation is a same-pixel running average in radiance space**,
  capped at `maxSampleCount`, reprojected via a dedicated motion-vector G-buffer
  attachment (`currentUV - prevUV`), with **depth-based** history rejection
  (linear view-space depth stored in history alpha vs. expected reprojected
  depth, threshold `historyDepthRejectionThreshold`)
- all GI/TAA tuning lives in `config/deferred.ini` (`gi:` and `taa:` sections,
  hot-reloadable). Setting `gi.maxSampleCount: 0` collapses accumulation to the
  raw one-sample result for A/B comparison
- TAA resolves the final image (`shaders/taa_resolve.comp`): 3x3 neighborhood
  color-clamp anti-ghosting, motion-vector reprojection, jitter baked into the
  projection matrix upstream
- raster glTF paths (deferred opaque, deferred translucent, simple forward) and
  RTGI hit shading all read the shared bindless material/texture/geometry tables;
  no per-material descriptor sets or string-keyed material lookups remain

### Known gaps / audit notes (inputs to the plan below)

- **Per-pixel GI state is radiance-space, not a reservoir.** History stores
  `(accumulated RGB, linear depth)` + a `uint` count. There is no stored *sample
  point* (secondary-hit position/normal/radiance), which ReSTIR requires. This is
  why the next milestone is a data-model change, not a denoiser.
- **History rejection is depth-only.** No normal/material/roughness validation
  yet — needed for both robust reprojection and reservoir reuse.
- **Sampling is per-pixel white noise** (`white_noise01`), not blue-noise or a
  low-discrepancy sequence. ReSTIR quality improves markedly with decorrelated
  samples; cheap to upgrade.
- **Single indirect bounce**; secondary hit gets direct lighting only. Adequate
  for ReSTIR initial candidates; multi-bounce is a later fidelity item.
- Carried-forward backlog from earlier milestones (still open): tangent-space
  normal maps at secondary hits, MIS / explicit light sampling at the primary
  surface, and dedicated ray-hit debug visualizations (instance/material/
  primitive/bary/UV, history age, rejected pixels, sample-count heatmap). These
  are folded into the milestones below rather than tracked separately.

## Velocity and Estimation Basis

The original roadmap assumed evenings-and-weekends hand-coding with rabbit holes,
and sized milestones in **weeks to months**. Measured reality since the roadmap
was authored (2026-06-11) is far faster because most work is now agentic coding:

- In ~4 weeks (2026-06-11 -> 2026-07-07) the following all landed: Milestones 0,
  1, 2, 2.5, 3, **plus** the entire bindless material/geometry infrastructure
  (a prerequisite side quest), **plus** TAA (unplanned), **plus** stochastic
  alpha-translucent shadows.
- Milestones the old roadmap sized at **3-8 weeks each** were each landed in
  **~1-4 focused days**: M2 hit shading in ~1 day (2026-06-29), M2.5 direct-lit
  secondary hit in ~1 day (2026-06-30), M3 temporal accumulation core in ~1 day
  (2026-07-01) with cleanup through 07-04.
- The old "temporal/spatial denoised RTGI" line item was estimated at 3-4 months
  on the normal path; temporal accumulation was reached ~3 weeks after the
  roadmap started.

**Recalibration:** focused work is running roughly **5-10x faster** than the
original estimates. New milestone estimates below are given in **focused-work
days**, with a wider calendar range to absorb debugging and side quests. Rule of
thumb: a milestone that "feels like a couple weeks" is usually a few days of
focused agentic work here; reservoir *correctness* (bias, the spatial Jacobian)
is the most likely place to lose a week.

## Direction

Use the existing deferred renderer as the current primary visibility path:

- rasterize the G-buffer as usual and use position/normal/albedo/roughness/
  metallic/depth as RTGI inputs
- use the shared TLAS/ray-query infrastructure for traversal
- use the shared bindless material/geometry tables for hit data
- write indirect lighting into a separate texture and composite into deferred
  lighting
- evolve the per-pixel signal from running-average accumulation into ReSTIR
  reservoirs, then denoise

Deferred is the *current* primary path, not a permanent commitment. Keep RTGI
attached to deferred but not fused into it: the durable investment is
renderer-agnostic shared data — camera/view buffers, light buffers, TLAS access,
material/geometry tables, history textures, reservoir buffers, and the indirect
output — so a future non-deferred primary path could consume the same GI stack
without a rewrite.

## Non-Goals For This Roadmap

- Do not build a general render graph yet.
- Do not broaden bindless beyond current glTF material/geometry needs unless
  RTGI proves a concrete need.
- Do not redesign descriptor allocation unless the current code becomes painful.
- Do not add async compute or multi-queue scheduling.
- Do not implement ReSTIR PT or ReSTIR PT Enhanced (whole-path reuse); the target
  is ReSTIR **GI** (sample/path-vertex reuse).
- Do not chase full material fidelity before the reservoir path is correct.
- Do not start a Forward+ (or other) renderer swap unless a concrete pain point
  during the GI work demands it. Deferred is not assumed permanent, but there is
  no current reason to move off it.

---

## Completed Milestones (M0-M3 + TAA)

Condensed; these are done. Detail lives in git history and the code.

**M0 - RTGI Workspace** (complete). RTGI-owned indirect-lighting texture, compute
path, composite into deferred lighting, toggle/debug plumbing, shader hot reload
smoke-tested. Full-resolution v0 to avoid upsampling distractions.

**M1 - Naive One-Bounce Diffuse RTGI** (complete). Stable per-pixel randoms,
cosine-weighted hemisphere sampling around the G-buffer normal, one diffuse ray
through the TLAS, sky/environment on miss.

**M2 - Minimal Hit Shading** (complete). Committed hit -> scene instance ->
bindless material + geometry record; `instanceCustomIndex` maps to the instance
table; UV reconstruction from primitive index + barycentrics; albedo (LOD 0) x
base-color factor; emissive contribution. Geometry records use buffer-device-
address vertex/index lookups; GLSL and CPU vertex/index layouts must stay in
lockstep (incl. 16-bit index decoding).

**M2.5 - Direct-Lit Secondary Hit Shading** (complete). Secondary-hit world
position/normal reconstructed from the ray + barycentrics; shared point/
directional light buffers; shared direct-light BRDF eval across deferred/
translucent/RTGI; secondary-hit contribution = emissive + shadowed direct-lit
outgoing radiance toward the primary surface (ray-query shadow visibility). The
deferred composite treats the RTGI buffer as an incoming indirect radiance signal
at the primary surface.

**M3 - Temporal Accumulation** (complete). Ping-pong history + per-pixel sample
count; same-pixel running-average accumulation capped at `gi.maxSampleCount`
(`config/deferred.ini`); reprojection via the motion-vector G-buffer attachment;
depth-based history rejection via linear depth in history alpha vs.
`gi.historyDepthRejectionThreshold`; `gi.maxSampleCount: 0` gives raw-vs-converged
A/B. (Normal/material rejection intentionally left for the reservoir milestones.)

**TAA** (complete, unplanned bonus). Full-image temporal AA
(`shaders/taa_resolve.comp`): jitter baked into the projection matrix, 3x3
neighborhood color-clamp anti-ghosting (`taa.clampExpansion`), history blend
(`taa.historyWeight`), motion-vector reprojection. Confirms the motion-vector and
ping-pong plumbing that ReSTIR temporal reuse will share.

---

## Reassessment (2026-07-10)

**Should the remaining path change?** Keep ReSTIR GI as the target; reorder the
remaining milestones.

The original plan put a bilateral/à-trous **spatial denoiser** next (old M4),
before any reservoir work. Given the code as it stands, that ordering is weaker
than it was when written:

1. The current per-pixel signal is a **running average of radiance**. ReSTIR
   replaces that representation entirely with per-pixel **reservoirs** carrying a
   *sample point*. A denoiser built on the radiance buffer now does not advance
   the reservoir work and risks being reworked.
2. **TAA + temporal accumulation already give acceptable interim stability** for
   static/slow scenes, so the immediate visual payoff of a spatial denoiser
   *right now* is lower than the roadmap assumed.
3. A classical edge-stopping denoiser is genuinely needed **at the end**, over
   the 1-spp resolved ReSTIR output. Placed there, it is not throwaway and it
   reuses the same depth/normal edge-stopping weights as spatial reservoir
   validation.

So the remaining milestones become: **reservoir data model -> temporal
reservoir reuse -> spatial reservoir reuse -> denoise/resolve -> polish.** The
denoiser moves from the front of the line to the back, where the signal actually
needs it.

**Optional de-risking spike (recommended if reservoir math is unfamiliar):** a
2-3 day ReSTIR **DI** prototype on the existing point/directional lights. DI
reservoirs exercise RIS, temporal reuse, and bias control with *no* Jacobian and
no reconnection, which is the cheapest way to validate the reservoir machinery
before applying it to the harder GI domain (M5-M6). It also directly improves the
current direct lighting. Skip it if the RIS/temporal-reuse math is already
comfortable.

---

## Milestone 4: Reservoir Data Model and Sample Buffer

Estimated focused work: **2-4 days** (calendar ~3-6 days). Comparable in size to
M3 core; mostly plumbing plus one equivalence check.

Goal: change the per-pixel representation from radiance-accumulation to a
single-candidate **reservoir** carrying a reusable **sample**, without changing
the visible result yet. This is the enabling refactor for everything after.

Tasks:

- Define, in `shaders/cshared/`, a compact `GiReservoir` and `GiSample`:
  - sample point `x_s` (secondary-hit world position + normal)
  - sample outgoing radiance `L_o` toward the visible point (the current
    per-ray result: emissive + direct-lit, or background on miss)
  - reservoir state: `w_sum`, `M`, `W` (unbiased contribution weight)
  - keep the visible point `x_v` implicit from the G-buffer for now
  - enforce the 16-byte-aligned CPU/GLSL ABI with `static_assert`s, as with the
    existing shared structs
- Have the RTGI initial pass emit a `GiSample` per pixel and insert it into a
  fresh 1-candidate reservoir via RIS, with target function `p_hat` = luminance
  of the sample's contribution at the visible point.
- Add a resolve step that shades the primary surface from the reservoir's
  selected sample using `W` (i.e. `contribution(x_v, z) * W`).
- Keep the existing running-average accumulation available behind a config
  toggle for A/B during the transition.
- Add debug views: sample position, sample radiance, `W`, reservoir validity.

Done when:

- A single-candidate reservoir with correct RIS weights **reproduces the current
  naive 1-spp signal** (the sanity check: RIS over one candidate equals the raw
  estimate).
- The sample buffer round-trips through history (write/read) with the ABI intact.

Likely pain points:

- getting `W` and `p_hat` consistent so the 1-candidate case is unbiased
- history texture format/packing for the wider per-pixel payload

## Milestone 5: Temporal Reservoir Reuse (ReSTIR Temporal)

Estimated focused work: **3-6 days** (calendar ~1-2 weeks). Add the optional
ReSTIR DI spike (2-3 days) here if de-risking.

Goal: replace the naive running-average with reservoir-based temporal
resampling. This supersedes M3 as the primary temporal denoiser for GI.

Tasks:

- Store previous-frame reservoirs (extend the existing ping-pong history to carry
  reservoir payload instead of accumulated radiance).
- Reproject the visible point via the existing motion vectors.
- Combine current + temporal reservoirs with RIS, **capping temporal `M`** (e.g.
  `M <= 20x` the current-frame `M`) for bias control.
- Validate the temporal neighbor with the existing depth rejection **plus** new
  normal/material/roughness checks (closes the depth-only gap noted above).
- Upgrade sampling to a decorrelated/blue-noise sequence while here (cheap, and
  temporal reuse amplifies its benefit).

Done when:

- Static-camera convergence is at least as fast as the naive accumulator, ideally
  faster.
- Camera motion does not create severe trails/ghosting.
- Temporal reuse can be toggled and visualized independently of the initial
  sampling.

Likely pain points:

- temporal bias if `M` is not capped or `W` is mishandled
- disocclusion handling at reprojection edges
- keeping the sample point valid across reprojection (it is a world-space point,
  so it survives camera motion, unlike screen-space radiance)

## Milestone 6: Spatial Reservoir Reuse (ReSTIR Spatial)

Estimated focused work: **4-8 days** (calendar ~1.5-2.5 weeks). The hardest
correctness milestone.

Goal: resample nearby pixels' reservoirs so neighbors share useful indirect
paths.

Tasks:

- Pick a small neighborhood (a few taps, low-discrepancy/rotated offsets).
- Validate neighbors by depth/normal/material/roughness before combining.
- Apply the **ReSTIR GI Jacobian** for the reconnection shift (the solid-angle /
  geometry-term ratio between the neighbor's visible point and this pixel's
  visible point through the shared sample point `x_s`). This is the subtle part;
  getting it wrong shows up as darkening/brightening near depth or normal edges.
- Optionally add a visibility check from the visible point to the reused sample
  point to suppress light leaking (bias/perf tradeoff; can start without it).
- Allow 1-2 spatial iterations; debug mode for accepted/rejected neighbors.

Done when:

- Adjacent pixels share useful samples; noise drops faster than temporal-only.
- No obvious darkening/brightening bias at edges (Jacobian sanity).
- Light leaking is controlled enough for the test scenes.

Likely pain points:

- the Jacobian (most common source of ReSTIR GI bias)
- leaking through thin geometry without the visibility check
- balancing neighbor count / iterations against cost

## Milestone 7: Denoise and Resolve

Estimated focused work: **3-6 days** (calendar ~1-2 weeks). This is the old
"spatial filtering" milestone, now correctly placed **after** the reservoir path
where the 1-spp output actually needs it.

Goal: turn the resolved 1-spp ReSTIR GI into a clean image.

Tasks:

- Add an à-trous / edge-stopping (SVGF-style) spatial filter over the *resolved*
  GI, reusing the depth/normal edge-stopping weights from M6 validation.
- Drive filter width by temporal history length (blur more where history is
  short / just disoccluded).
- Add firefly clamping for the initial candidates.
- Debug modes: raw GI, reservoir-resolved GI, filtered GI, filter weights.

Done when:

- 1-spp ReSTIR GI is visibly smooth without destroying edges.
- The filter can be disabled to compare raw vs. filtered.
- Corners are not obviously over-blurred; no gross leaking through thin geometry.

Likely pain points:

- filter order relative to temporal/spatial reuse
- over-blurring vs. residual noise tradeoff

## Milestone 8: Polish, Presets, and Performance

Estimated focused work: **1-2 weeks** (calendar ~2-4 weeks).

Goal: turn the prototype into a renderer feature that is pleasant to iterate on.

Tasks:

- Half-resolution GI + upsampling, with quality/performance presets.
- Improve hit-shading fidelity: tangent-space normal maps at secondary hits,
  optional MIS / explicit light sampling at the primary surface, and (if it earns
  its cost) a second indirect bounce.
- Improve history rejection and denoising robustness.
- Add GPU timing markers around the RTGI/reservoir/denoise passes.
- Add scene/material stress tests and document debug modes + known limitations.

Done when:

- RTGI is a believable renderer mode rather than a fragile experiment.
- It survives camera movement and common scenes.
- The debug tooling is good enough to diagnose bad frames.

---

## Overall Timeline From Here

Recalibrated to measured velocity (focused-work days; calendar spans absorb side
quests and debugging). Compare with the original estimate of 8-12 months to a
recognizable ReSTIR GI.

| Milestone | Focused work | Calendar |
|---|---|---|
| M4 Reservoir data model | 2-4 days | 3-6 days |
| M5 Temporal reservoir reuse | 3-6 days | 1-2 weeks |
| M6 Spatial reservoir reuse | 4-8 days | 1.5-2.5 weeks |
| M7 Denoise and resolve | 3-6 days | 1-2 weeks |
| **Subtotal to recognizable ReSTIR GI (M4-M7)** | **~2-4 weeks** | **~4-7 weeks** |
| M8 Polish / presets / perf | 1-2 weeks | 2-4 weeks |

- Optimistic (few rabbit holes): recognizable ReSTIR GI in **~3-4 weeks**.
- Normal: recognizable ReSTIR GI in **~5-7 weeks**, polished in **~2-3 months**.
- Rabbit-hole (most likely in M6's Jacobian / M5 bias): add 1-3 weeks.

The optional ReSTIR DI de-risking spike adds ~2-3 days but can save more than
that in M5/M6 if reservoir math is new.

## Recommended Next Commit

Start Milestone 4 with the smallest reservoir refactor that keeps the image
identical:

- add `GiReservoir` / `GiSample` structs to `shaders/cshared/` with the CPU/GLSL
  ABI static_asserts
- have `rtgi_generate.comp` emit the per-pixel sample and wrap it in a
  1-candidate reservoir (RIS, `p_hat` = contribution luminance)
- shade from the reservoir via `W`, and verify it matches the current naive
  1-spp output (`gi.maxSampleCount: 0`) before touching temporal reuse
- keep the running-average path behind a toggle until M5 lands

That converts the signal into the representation ReSTIR needs, with a built-in
equivalence check, while leaving temporal/spatial reservoir reuse as the next
slices.

## Reading Order

Suggested order, from most immediately useful to most ambitious:

1. ReSTIR GI: path resampling for real-time path tracing (Ouyang et al. 2021) -
   the direct target.
2. ReSTIR DI / RTXDI: direct-light reservoir sampling - the simplest reservoir
   implementation, and the basis for the optional M5 de-risking spike.
3. GRIS: the generalized resampling math behind ReSTIR (bias, MIS weights,
   the shift-mapping Jacobian used in M6).
4. ReSTIR PT: whole-path reuse - context only; not an implementation target here.
5. ReSTIR PT Enhanced: future reference, not a target for this roadmap.
