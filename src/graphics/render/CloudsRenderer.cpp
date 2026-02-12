#include "CloudsRenderer.hpp"

#include "assets/Assets.hpp"
#include "coders/imageio.hpp"
#include "debug/Logger.hpp"
#include "graphics/core/ImageData.hpp"
#include "graphics/core/Mesh.hpp"
#include "graphics/core/Shader.hpp"
#include "graphics/core/Texture.hpp"
#include "io/io.hpp"
#include "lighting/Lightmap.hpp"
#include "maths/FastNoiseLite.h"
#include "util/timeutil.hpp"
#include "window/Camera.hpp"
#include "world/Weather.hpp"

#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>

static debug::Logger logger("clouds-render");

class CloudsMap {
public:
    glm::ivec3 size;
    const bool* voxels;

    CloudsMap() = default;

    CloudsMap(const glm::ivec3& size, const bool* voxels)
        : size(size), voxels(voxels) {
    }

    bool isOpen(int x, int y, int z) const {
        if (x >= 0 && x < size.x && y >= 0 && y < size.y && z >= 0 && z < size.z) {
            return !voxels[vox_index(x, y, z, size.x, size.z)];
        }
        return true;
    }
};


class VolumeRenderer final {
public:
    VolumeRenderer(size_t capacity)
        : vertices(std::make_unique<ChunkVertex[]>(capacity)),
          indexBuffer(std::make_unique<uint32_t[]>(capacity * 6)),
          capacity(capacity) {
    }

    ~VolumeRenderer() = default;

    void build(CloudsMap volumeMap) {
        this->map = std::move(volumeMap);
        overflow = false;
        offset = 0;
        indexOffset = 0;
        indexCount = 0;

        int end = map.size.x * map.size.y * map.size.z;
        for (int i = 0; i < end; i++) {
            int x = i % map.size.x;
            int y = i / (map.size.z * map.size.x);
            int z = (i / map.size.x) % map.size.z;
            const auto& vox = map.voxels[i];
            if (!vox) {
                continue;
            }
            cube({x, y, z});
            if (overflow) {
                return;
            }
        }
        logger.info() << "layer vertices: " << offset;
    }

    MeshData<ChunkVertex> createMesh() const {
        return MeshData(
            util::Buffer(vertices.get(), offset),
            std::vector<util::Buffer<uint32_t>> {
                util::Buffer(indexBuffer.get(), indexCount),
            },
            util::Buffer(
                ChunkVertex::ATTRIBUTES,
                sizeof(ChunkVertex::ATTRIBUTES) / sizeof(VertexAttribute)
            )
        );
    }

private:
    std::unique_ptr<ChunkVertex[]> vertices;
    std::unique_ptr<uint32_t[]> indexBuffer;
    size_t capacity;
    uint32_t indexOffset = 0;
    uint32_t indexCount = 0;
    uint32_t offset = 0;
    bool overflow = false;
    CloudsMap map {};

    void vertex(
        const glm::vec3& coord,
        const glm::vec3& normal
    ) {
        auto& vert = vertices[offset++];
        vert.position = coord;
        vert.uv = {};
        vert.normal = {
            static_cast<uint8_t>(normal.r * 127 + 128),
            static_cast<uint8_t>(normal.g * 127 + 128),
            static_cast<uint8_t>(normal.b * 127 + 128),
            255
        };
        vert.color = {
            0, 0, 0, static_cast<uint8_t>((coord.y / 8.0f * 0.25f + 0.75f) * 255)
        };
    }

    void face(
        const glm::vec3& coord,
        const glm::vec3& X,
        const glm::vec3& Y,
        const glm::vec3& Z
    ) {
        if (!isOpen(coord + Z)) {
            return;
        }
        if (offset + 4 >= capacity) {
            overflow = true;
            return;
        }

        float s = 0.5f;
        vertex(coord + (-X - Y + Z) * s, Z);
        vertex(coord + ( X - Y + Z) * s, Z);
        vertex(coord + ( X + Y + Z) * s, Z);
        vertex(coord + (-X + Y + Z) * s, Z);

        const uint32_t indices[] {0, 1, 2, 0, 2, 3};
        for (size_t i = 0; i < 6; i++) {
            indexBuffer[indexCount++] = indexOffset + indices[i];
        }
        indexOffset += 4;
    }

    bool isOpen(const glm::ivec3& pos) const {
        return pos.y < 0 || pos.y >= map.size.y ||
               map.isOpen(pos.x, pos.y, pos.z);
    }

    void cube(const glm::ivec3& coord) {
        const glm::ivec3 X(1, 0, 0);
        const glm::ivec3 Y(0, 1, 0);
        const glm::ivec3 Z(0, 0, 1);

        face(coord, X, Y, Z);
        face(coord, -X, Y, -Z);
        face(coord, X, -Z, Y);
        face(coord, X, Z, -Y);
        face(coord, -Z, Y, X);
        face(coord, Z, Y, -X);
    }
};

static inline constexpr int WIDTH = 256;
static inline constexpr int DEPTH = 256;


CloudsRenderer::CloudsRenderer() {
    VolumeRenderer volumeRenderer(1024 * 1024);

    for (int layer = 0; layer < 2; layer++) {
        fnl_state state = fnlCreateState();
        const int w = WIDTH;
        const int h = 8;
        const int d = DEPTH;
        const int dd = d * 1.5;

        state.seed = 5265 + layer * 3521;

        float pi2 = glm::two_pi<float>();

        float heightmap[w * dd];
        for (int lz = 0; lz < dd; lz++) {
            for (int lx = 0; lx < w; lx++) {
                float x = glm::sin(lx / static_cast<float>(w) * pi2) * w / pi2;
                float y = -glm::cos(lx / static_cast<float>(w) * pi2) * w / pi2;
                float z = lz;
                float s = 1.5f;
                auto n = fnlGetNoise3D(&state, x * s, y * s, z * s);
                n += fnlGetNoise3D(&state, x * s + fnlGetNoise2D(&state, x * s * 4 + 2, z * s * 4) * 0.25f, y * s, z * 3.0f) * 0.5f;
                n += fnlGetNoise3D(&state, x * s * 2, y * s * 2, z * s * 2) * 0.25f;
                n += fnlGetNoise3D(&state, x * s * 4, y * s * 4, z * s * 4) * 0.125f * 2;
                n += fnlGetNoise3D(&state, x * s * 8, y * s * 8, z * s * 8) * 0.125f * 0.5f * 2;
                n += fnlGetNoise3D(&state, x * s * 16, y * s * 16, z * s * 16) * 0.125f * 0.25f * 3;
                n = glm::max(0.0f, n);
                // n = n * n * 2.0f;
                n -= 0.2f + layer * 0.5f;
                // n -= 0.5f;

                heightmap[lz * w + lx] = n;
            }
        }

        bool voxels[w * h * d];
        for (int y = 0; y < h; y++) {
            for (int z = 0; z < d; z++) {
                for (int x = 0; x < w; x++) {
                    float n = heightmap[z * w + x];
                    if (z < d / 2) {
                        float t = z / static_cast<float>(d / 2);
                        n = n * t + heightmap[(d + z) * w + x] * (1.0f - t);
                    }
                    bool solid = y <= n * h && y >= (0.5f - n * 0.5f) * h;
                    voxels[vox_index(x, y, z, w, d)] = solid;
                }
            }
        }
        
        CloudsMap map({w, h, d}, voxels);

        volumeRenderer.build(map);
        testMeshes[layer] = std::make_unique<Mesh<ChunkVertex>>(volumeRenderer.createMesh());
    }
}

CloudsRenderer::~CloudsRenderer() = default;

void CloudsRenderer::draw(
    Shader& shader,
    const Weather& weather,
    float timer,
    float fogFactor,
    const Camera& camera
) {
    float clouds = weather.clouds();
    shader.uniform4f(
        "u_tint",
        glm::vec4(
            1.0f - clouds * 0.5, 1.0f - clouds * 0.45, 1.0f - clouds * 0.4, 1.0f
        )
    );

    float scale = 8;
    float totalWidth = WIDTH * scale;
    float totalDepth = DEPTH * scale;

    int cellX = glm::floor(camera.position.x / totalWidth + 0.5f);
    int cellZ = glm::floor(camera.position.z / totalDepth + 0.5f);

    float speed = 4.0f;
    for (int i = 0; i < 2; i++) {
        float speedX = glm::sin(i * 0.3f + 0.4f) * speed / (i + 1);
        float speedZ = -glm::cos(i * 0.3f + 0.4f) * speed / (i + 1);
        for (int x = -2; x <= 2; x++) {
            for (int z = -2; z <= 2; z++) {
                auto matrix = glm::mat4(1.0f);
                matrix = glm::translate(
                    matrix,
                    glm::vec3(
                        -128 * scale + (x + cellX) * WIDTH * scale +
                            glm::mod(timer * speedX, totalWidth),
                        250 + i * 200,
                        -128 * scale + (z + cellZ) * DEPTH * scale +
                            glm::mod(timer * speedZ, totalDepth)
                    )
                );
                matrix = glm::scale(matrix, glm::vec3(scale, scale, scale));
                shader.uniform1f("u_fogFactor", fogFactor * 0.03f);
                shader.uniform1f("u_fogCurve", 0.7f - 0.3f);
                shader.uniformMatrix("u_model", matrix);
                testMeshes[i]->draw();
            }
        }
    }
}
