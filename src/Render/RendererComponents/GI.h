//
// Created by miyehn on 6/11/2026.
//
#pragma once
#include "Assets/ConfigAsset.hpp"


class Texture2D;

struct GI
{
    GI() = default;
    GI(const GI&) = delete;
    GI& operator=(const GI&) = delete;

    void init();
    void release();

    void render(Texture2D* sceneColor);

private:
    // can use a config asset for options
    // or can use on-screen sliders
    static ConfigAsset* config;

};

