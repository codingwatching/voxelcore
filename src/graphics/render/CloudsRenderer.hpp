#pragma once

#include "commons.hpp"

#include <memory>

class Assets;
class Camera;

class CloudsRenderer final {
public:
    CloudsRenderer(const Assets& assets);
    ~CloudsRenderer();

    void draw(float timer, float fogFactor, const Camera& camera);
private:
    const Assets& assets;
    std::array<std::unique_ptr<Mesh<ChunkVertex>>, 2> testMeshes;
};
