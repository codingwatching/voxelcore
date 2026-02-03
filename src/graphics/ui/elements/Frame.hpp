#pragma once

#include "Container.hpp"

class Framebuffer;

namespace gui {
    class Frame final : public Container {
    public:
        explicit Frame(GUI& gui);
        virtual ~Frame();

        void draw(const DrawContext& pctx, const Assets& assets) override;
    private:
        std::unique_ptr<Framebuffer> fbo;
        std::string texture = "gui/no_icon";
    };
}
