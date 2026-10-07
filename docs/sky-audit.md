# Sky atmosphere radiance audit

Written 2026-10-06. Notes for investigating later; nothing here has been changed in the code.

The question that started this: the sky looks dim, in the deferred renderer as well as in the GPU path
tracer. Is the sky radiance actually wrong, or just displayed dim?

Short version: the deferred renderer and the GPU path tracer show the same sky values, so the path tracer adds no
dimness. There are several real discrepancies in the sky code (below), but none is clearly *the* reason it looks
dim. Part of the dimness comes from exposure and display choices.

## How the sky reaches the screen today

- `shaders/sky_view_lut.comp` calls `computeSkyAtmosphere(...)` with `sunLuminance = vec3(1, 1, 1)`
  (line 29). `sky_common.glsl` multiplies that by the phase function and the scattering coefficients, so the
  result L is in units where the sun's **illuminance** is 1.
- `sampleSkyAtmosphere(viewDir)` (`sky_common.glsl:478`) reads that LUT, adds the sun disc, then applies
  `L = 1 - exp(-L / white_point * params.exposure)` (line 500). `params.exposure` comes from `exposure:` in
  `config/skyAtmosphere.ini` (currently 1.0).
- The deferred renderer adds that to the lit scene color for sky pixels
  (`shaders/deferred_lighting.frag:72`). Post-processing then applies the camera exposure (`cfgExposure`
  defaults to 3.0 stops, so x8, at `DeferredRenderer.cpp:808`) and Reinhard with a white point of 2
  (`post_processing.frag`).
- GI uses the same function as its miss radiance, so the sky-lit indirect light carries the same limits.
- The GPU path tracer samples it the same way on primary-ray miss (`traceRadiance` in
  `lighting_common.glsl`), then applies the same exposure and Reinhard curve.

## What I measured

I inverted the display pipeline on screenshot pixels (sRGB decode, inverse Reinhard with white point 2, divide
by 2^3, then inverse of the `1 - exp(...)` curve). Sampled in a column on the right side of the window, where
only sky is visible, from the deferred renderer and from the GPU path tracer:

| Position (y in window) | sRGB pixel | sky function output | recovered L (sun = 1 units) |
|---|---|---|---|
| top (45) | (77, 88, 90) | 0.010, 0.013, 0.014 | 0.011, 0.013, 0.013 |
| mid (150) | (88, 98, 91) | 0.013, 0.017, 0.014 | 0.014, 0.016, 0.014 |
| lower (260) | (105, 108, 85) | 0.020, 0.021, 0.012 | 0.022, 0.021, 0.012 |
| near horizon (340) | (121, 110, 64) | 0.028, 0.022, 0.007 | 0.030, 0.022, 0.006 |
| near horizon (385) | (122, 99, 50) | 0.028, 0.017, 0.004 | 0.031, 0.017, 0.004 |

Deferred and GPU path tracer agree to within rounding (the largest difference I saw is 1 level of sRGB).

## Discrepancies found

1. **Sun disc radiance is wrong** (`sky_common.glsl:495`). The disc is added as `transmittanceToSun * sunVisibility`
   with the comment "assumes sun luminance is 1". The LUT is computed with sun *illuminance* 1, so a consistent
   disc radiance is 1 / solid angle. With `sunAngularRadius = 0.004675`, that is 1 / (pi * r^2) ~ 1 / 6.9e-5
   ~ 14,500. The disc is roughly 14,500x too dim in these units. It saturates on screen, so it isn't visible as a
   problem. It matters as soon as the sky is used as light: a bounce ray that found the disc would carry about
   1/14,500 of the right energy.
2. **A display tone curve is baked into the sampling function** (`sky_common.glsl:498-500`). At L ~ 0.02 and
   `exposure: 1.0` the curve is almost linear, so it is *not* what makes the sky dim. But it caps the result
   below 1 and means the function doesn't return real radiance, which is wrong for anything using the sky as a
   light. Note `envmap_visualizer.frag:29` also calls it.
3. **Single scattering only.** `sky_common.glsl:455` has `// todo: add multiscattered light contribution here`.
   A single-scattering sky is darker (and differently colored, especially near the horizon and at twilight) than
   a full model.
4. **The sky and the scene's Sun use different units.** The sky assumes sun illuminance 1. The scene's Sun light
   reaches surfaces through `DirectionalLight::getIrradianceLx()` (`Light.hpp:41`):
   `intensity / (2 * PI) / PBR_WATTS_TO_LUMENS`. `auto_export.glb` has the Sun at intensity 683, so it lights
   surfaces with 683 / 683 / 2pi ~ 0.159. The comment in `Renderer::gatherLights` (`Renderer.cpp:105`) says
   "the last div by 2*PI is converting irradiance to radiance (???)", so the `2 pi` is already marked as
   uncertain.

## Why the sky still looks dim

- **Compared with the lit scene, it isn't off by an order of magnitude.** A sunlit white-ish surface ends up with
  radiance around `albedo / pi * 0.159 ~ 0.04` (albedo 0.8), and the sky is 0.01-0.03. The ratio is within a
  factor of a few of what a real clear day gives. Item 4 cuts the other way too: relative to the scene's actual
  Sun (0.159), the sky is about 6x *brighter* than a consistent unit choice would give.
- **Display choices make most of the difference.** The camera is 3 stops over with a Reinhard curve whose white
  point is 2, tuned for the indoor box. `exposure: 1.0` in `skyAtmosphere.ini` leaves the sky near-linear and
  low.
- **Sun position.** The horizon is a low orange band, which looks like a low sun, and a low sun means a dimmer
  sky. I could not confirm the actual elevation. The Sun node's rotation in `auto_export.glb` converted to a
  to-sun direction with a negative Y component (about -69 degrees in glTF space), which can't be right for what
  is on screen, so the engine's axis conversion is involved and I didn't chase it.

## Things to try / open questions

- Hot-reload `exposure` in `config/skyAtmosphere.ini` to 5-10 and see if the sky looks the way you expect. From
  memory, the reference demos for this model use a much higher value (around 10), but that is unverified.
- Print or inspect `params.dir2sun` for the loaded scene to get the real sun elevation.
- Decide what the `2 pi` in `getIrradianceLx()` is supposed to be. If the Sun should have the same illuminance
  the sky assumes, the correct value is 1 (or the sky should be scaled by 0.159). This is the largest knob for
  "sky versus sun" balance.
- `groundAlbedo` in `skyAtmosphere.ini` (0.3, 0.5, 0.4) is marked "not used for now" in `sky_common.glsl:25`
  and `SkyAtmosphere.h:28`, so there is no ground bounce in the sky either. That is another source of
  darkness, especially for a sun near the horizon.

## Suggested fixes (not done)

For use as a light source in the GPU path tracer:

1. Return raw radiance from `sampleSkyAtmosphere`, and apply the display exposure and curve outside it. Keep the
   current look in deferred by applying the old curve at its call site (`deferred_lighting.frag:72`) and in
   `envmap_visualizer.frag:29`.
2. Fix the sun disc radiance to illuminance / solid angle.
3. Pick one illuminance for the sun, shared by the sky and the Sun light, and fix or remove the `2 pi` in
   `getIrradianceLx()`.
4. Add multiple scattering (the usual approach is a small multi-scattering LUT).

Items 1 and 2 are self-contained. Item 3 changes how all the scene's lit surfaces look. Item 4 is new work.
