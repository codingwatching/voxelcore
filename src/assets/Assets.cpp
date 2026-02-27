#include "Assets.hpp"

Assets::Assets(util::ObjectsKeeper& vault) : vault(vault) {
}

Assets::~Assets() {
    for (auto& [_, map] : assets) {
        for (auto& [__, asset] : map) {
            vault.keepAlive(std::move(asset));
        }
    }
}

const std::vector<TextureAnimation>& Assets::getAnimations() {
    return animations;
}

void Assets::store(const TextureAnimation& animation) {
    animations.emplace_back(animation);
}
