#include "Frame.hpp"

#include "assets/Assets.hpp"
#include "graphics/core/Batch2D.hpp"
#include "graphics/core/Framebuffer.hpp"
#include "graphics/core/DrawContext.hpp"
#include "graphics/core/Texture.hpp"
#include "window/Window.hpp"
#include "../GUI.hpp"

using namespace gui;

static inline constexpr int MAX_TEXTURE_SIZE = 2048;

gui::Frame::Frame(GUI& gui) : Container(gui, {}), fbo(nullptr) {}

gui::Frame::~Frame() = default;

void gui::Frame::draw(const DrawContext& pctx, const Assets& assets) {
    glm::ivec2 size = getSize();
    if (size.x <= 0 || size.y <= 0 ||
        size.x > MAX_TEXTURE_SIZE || size.y > MAX_TEXTURE_SIZE) {
        return;
    }
    auto& nonConstASsets = const_cast<Assets&>(assets);
    if (fbo && (fbo->getWidth() != size.x || fbo->getHeight() != size.y)) {
        fbo->resize(size.x, size.y);
        nonConstASsets.store(fbo->getSharedTexture(), texture);
    } else if (fbo == nullptr) {
        fbo = std::make_unique<Framebuffer>(size.x, size.y, true);
        nonConstASsets.store(fbo->getSharedTexture(), texture);
    }

    // ui uses flipped camera with matrix based on main viewport
    setPos({0, pctx.getViewport().y - size.y});
    
    auto ctx = pctx.sub();
    ctx.setFramebuffer(fbo.get());
    display::clear();
    Container::draw(ctx, assets);
    ctx.getBatch2D()->flush();
}
