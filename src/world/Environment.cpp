#define VC_ENABLE_REFLECTION
#include "Environment.hpp"

#include "data/dv.hpp"
#include "data/dv_util.hpp"

dv::value Environment::serialize() const {
    return dv::object({
        {"sky", dv::object({
            {"mode", SkyModeMeta.getNameString(sky.mode)},
            {"stars", sky.stars},
            {"clouds", sky.clouds},
            {"sprites", sky.sprites},
        })},
    });
}

void Environment::deserialize(const dv::value& src) {
    if (auto skyItem = src.at("sky")) {
        skyItem->at("mode").get(sky.mode, SkyModeMeta);
        skyItem->at("stars").get(sky.stars);
        skyItem->at("clouds").get(sky.clouds);
        skyItem->at("sprites").get(sky.sprites);
    }
}
