//
// Created by miyehn on 6/11/2026.
//

#include "GI.h"

ConfigAsset* GI::config = nullptr;

void GI::init()
{
    if (!config) {
        config = new ConfigAsset("config/gi.ini", true, [](const ConfigAsset* cfg)
        {
            LOG("GI enabled: %i", cfg->lookup<int>("enabled"));
        });
    }

}

void GI::release()
{

}
