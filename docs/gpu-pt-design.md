Thinking about how the GPU path tracer should be organized.

To start simple, let's treat all scene objects as opaque or alpha clip, 
and assume they are all perfect diffuse materials (potentially tinted by tint 
and albedo texture)

Would like to support multiple bounces, say, 4 bounces for now.

Path tracing loop might look something like this in the shader?

```glsl
// pseudo code, inside gpu_path_tracer.comp for now

// helpers for sampling

//// Cosine-weighted
void CosineWeightedSampler_Sample(vec3 surfaceNormal, out vec3 wi, out float pdf) {
    wi = cosine-weighted sampled direction wi;
    pdf = pdf of the sampled direction wi;
}

vec3 CosineWeightedSampler_Evaluate(vec3 surfaceNormal, vec3 albedo, vec3 wo, vec3 wi) {
    return ... // forgot math here
}

//// Mirror (just for example; don't have to support it now)
void MirrorSampler_Sample(vec3 surfaceNormal, vec3 wo, out vec3 wi, out float pdf) {
    wi = wo mirrored against surfaceNormal;
    pdf = 1;
}

vec3 MirrorSampler_Evaluate(vec3 surfaceNormal, vec3 wo, vec3 wi) {
    return vec3(1);
}

// the actual tracing function, name is tentative
vec3 tracePath(...)
{
    vec3 radiance = vec3(0);
    uint remainingBounces = maxRayDepth; // configurable
    vec3 contrib = vec3(1.0f);
    while (remainingBounces > 0)
    {
        remainingBounces -= 1;
        RayHitResult hit = traceClosestHit(...);
        if (hit a triangle)
        {
            HitSurface surface;
            if (!reconstructHitSurface(hit, ...)) break;
            
            // now able to get material information at hit
            vec3 albedo = ...;
            vec3 emission = ...;
            vec3 orm = ...; // unused for this simple example
                
            radiance += emission * contrib;
            
            vec3 wi;
            float pdf;
            CosineWeightedSampler_Sample(surface.worldNormal, wi, pdf);
                
            vec3 f = CosineWeightedSampler_Evaluate(surface.worldNormal, albedo, surface.outgoingDirection, wi);
            contrib *= f * dot(surface.worldNormal, wi) / pdf; // don't simplify even for just diffuse, to keep it generalized
        }
        else // miss
        {
            radiance += contrib * (sampled background color (sky or envmap), or black);
            break;
        }
    }
    
    return radiance;
}


```

`tracePath` would probably function similar to `traceRadiance` except it traces multiple bounces (equiv. to recursion), 
and currently doesn't evaluate delta lights (point and directional lights). So light can only come from emissive surfaces 
and sky / envmap.

Then call `tracePath` instead of `traceRadiance` from the gpu path tracer, to get multiple bounces.

Goal is to later extend it toward supporting direct light contribution with MIS, PBR and better sampling, etc.

### Notes

Don't mind MIS just yet, the API can grow later.

One sample per pixel is fine just for now.

Sun and point light would not be able to contribute anything which is fine for now.

Sky is to be fixed later so just do whatever deferred renderer is doing is fine for now.

ray ID should vary for each bounce. Which means we should probably remove constant `NON_SHADOW_RAY_ID` and 
just pass in number directly, when needed.

all surfaces should be treated as double-sided for now (if a ray hits the back of a face, flip its normal for this ray)

