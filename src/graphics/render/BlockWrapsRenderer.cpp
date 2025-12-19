#include "BlockWrapsRenderer.hpp"

#include "assets/Assets.hpp"
#include "assets/assets_util.hpp"
#include "constants.hpp"
#include "content/Content.hpp"
#include "graphics/core/Atlas.hpp"
#include "graphics/core/Shader.hpp"
#include "graphics/core/DrawContext.hpp"
#include "graphics/render/MainBatch.hpp"
#include "objects/Player.hpp"
#include "voxels/Block.hpp"
#include "voxels/Chunks.hpp"
#include "world/Level.hpp"

BlockWrapsRenderer::BlockWrapsRenderer(
    const Assets& assets, const Level& level, const Chunks& chunks
)
    : assets(assets),
      level(level),
      chunks(chunks),
      batch(std::make_unique<MainBatch>(1024)) {
}

BlockWrapsRenderer::~BlockWrapsRenderer() = default;

void BlockWrapsRenderer::draw(const BlockWrapper& wrapper) {
    auto& shader = assets.require<Shader>("entity");
    shader.use();
    shader.uniform1i("u_alphaClip", false);

    util::TextureRegion texRegions[6] {};
    UVRegion uvRegions[6] {};
    for (int i = 0; i < 6; i++) {
        auto texRegion = util::get_texture_region(assets, wrapper.textureFaces[i], "");
        texRegions[i] = texRegion;
        uvRegions[i] = texRegion.region;
    }
    batch->setTexture(texRegions[0].texture);

    const voxel* vox = chunks.get(wrapper.position);
    if (vox == nullptr || vox->id == BLOCK_VOID) {
        return;
    }
    const auto& def = level.content.getIndices()->blocks.require(vox->id);
    switch (def.getModel(vox->state.userbits).type) {
        case BlockModelType::BLOCK:
            batch->cube(
                glm::vec3(wrapper.position) + glm::vec3(0.5f),
                glm::vec3(1.01f),
                uvRegions,
                glm::vec4(1, 1, 1, 0),
                false,
                wrapper.cullingBits
            );
            break;
        case BlockModelType::AABB: {
            const auto& aabb =
                (def.rotatable ? def.rt.hitboxes[vox->state.rotation]
                                : def.hitboxes).at(0);
            const auto& size = aabb.size();
            uvRegions[0].scale(size.z, size.y);
            uvRegions[1].scale(size.z, size.y);
            uvRegions[2].scale(size.x, size.z);
            uvRegions[3].scale(size.x, size.z);
            uvRegions[4].scale(size.x, size.y);
            uvRegions[5].scale(size.x, size.y);
            batch->cube(
                glm::vec3(wrapper.position) + aabb.center(),
                size * glm::vec3(1.01f),
                uvRegions,
                glm::vec4(1, 1, 1, 0),
                false,
                wrapper.cullingBits
            );
            break;
        }
        default:
            break;
    }
}

void BlockWrapsRenderer::draw(const DrawContext& pctx, const Player& player) {
    auto ctx = pctx.sub();
    for (const auto& [_, wrapper] : wrappers) {
        draw(*wrapper);
    }
    batch->flush();
}

u64id_t BlockWrapsRenderer::add(
    const glm::ivec3& position, const std::string& texture
) {
    u64id_t id = nextWrapper++;
    wrappers[id] = std::make_unique<BlockWrapper>(BlockWrapper {
        position, {texture, texture, texture, texture, texture, texture}});
    return id;
}

BlockWrapper* BlockWrapsRenderer::get(u64id_t id) const {
    const auto& found = wrappers.find(id);
    if (found == wrappers.end()) {
        return nullptr;
    }
    return found->second.get();
}

void BlockWrapsRenderer::remove(u64id_t id) {
    wrappers.erase(id);
}
