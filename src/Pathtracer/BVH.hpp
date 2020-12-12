#pragma once
#include "lib.h"
#include "Primitive.hpp"

struct BVH
{
    BVH() {
        min = vec3(INF);
        max = vec3(-INF);
        primitives = std::vector<Primitive*>();
        left = nullptr;
        right = nullptr;
    }
    ~BVH() {
        if (left) delete left;
        if (right) delete right;
    }

    vec3 min;
    vec3 max;
    std::vector<Primitive*> primitives;
    BVH* left;
    BVH* right;

    void add_primitive(Primitive* prim);
    
    void build_bvh();

    // TODO: when (if) to modify ray?
    float surface_area();
    bool intersect_aabb(Ray& ray, float& distance);
    Primitive* intersect_primitives(Ray& ray, double& t, vec3& n);
};