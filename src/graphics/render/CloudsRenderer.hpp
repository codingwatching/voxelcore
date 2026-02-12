#pragma once

#include "commons.hpp"

#include <memory>

class Camera;
class Shader;
struct Weather;

class CloudsRenderer final {
public:
    CloudsRenderer();
    ~CloudsRenderer();

    void draw(
        Shader& shader,
        const Weather& weather,
        float timer,
        float fogFactor,
        const Camera& camera,
        int quality
    );
private:
    std::array<std::unique_ptr<Mesh<ChunkVertex>>, 2> testMeshes;
};
