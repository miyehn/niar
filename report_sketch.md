8 slides
5 mins of recorded video (voice over slides maybe)
report (see [here](http://www.cs.cmu.edu/afs/cs/academic/class/15418-f20/public/projects/project-proposal.pdf))

## Summary

We modified a sequential path tracer to utilize SIMD parallelization using ISPC and optimized it, and achieved a () speedup compared to its original C++ implementation. As shown in our demo app, this modified implementation makes it possible to view 3D scenes interactively in real time.

## Background

(can pretty much just copy the background section from our proposal)

## Approach

We started with a sequential C++ implementation which is part of a side project of one of us.

We used ISPC because (1) both of us work on mac laptops which supports it (2) staying on the cpu makes debugging easier while still benefiting from SIMD parallelization model

"how we mapped problem to target machine": two main focuses:
 - SIMD -> worries about instruction stream branching -> break tasks down into small chunks and remove dependencies, move tasks around and put coherent tasks together as much as possible (get rid of recursion in trace_ray and perform path tracing one bounce at a time -> pack unfinished rays together, sort rays hitting the same material together)
 - Memory -> the process of optimizing in favor of reducing instruction stream branching introduced a lot of data movement
 
Adapted the "ray packet" idea from lecture for sorting unfinished rays and material grouping
 - give speedups achieved by each

The actual sorting/grouping process depends on parallel scan

Data movement (1): many operations are encapsulated as in-place (scan, ray sorting, etc.) but hard to actually implement as in-place --> workaround: ping-pong buffers, pointer swapping

Data movement (2): pass around / move indices to objects instead of pointers or objects

Other optimizations:
 - pre-allocate large arrays instead of dynamically allocating and freeing small arrays
 - pre-compute things as much as possible (triangle info, bvh)

BVH was a last-minute challenge. Outcome: works, gives significant speedup (how much), there's definitely room for even more speedup. (probably) unfinished by the time this project ends, but we have ideas (similar to packing unfinished rays) and will continue as side project.
 - measure how much time is spent on ray-scene intersection tests? (how)

## Results

Measure(d) and compared runtime of our path tracer using a fixed setting with some controlled variables (describe it)

Speedups
 - ispc 4, 8, 16-wide vectors | diff. num threads
 - cpp diff. num threads
 - graph: ispc(s) vs. single core ispc 4-wide vector
 - graph: ispc(s) vs. sequential cpp (include notes about they're essentially two implementations)
 - graph: K-thread ispc vs. K-thread cpp (include notes about they're essentially two implementations)
 - all the above for: simple scene & medium-large scene

include rendered images (of course)
 
Explanations of speedups (what limited speedups, etc.) (make measurements to support claims)

Breakdown of runtime (how?)

Comment about target machine choice?

## References
