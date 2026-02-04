#pragma once

#include "Container.hpp"

class Framebuffer;
class UiDocument;

namespace gui {
    class Frame final : public Container {
    public:
        Frame(GUI& gui, std::string outputTexture);
        virtual ~Frame();

        void draw(const DrawContext& pctx, const Assets& assets) override;

        void updateOutput(Assets& assets);

        const std::string& getOutputTexture() const;
    private:
        std::unique_ptr<Framebuffer> fbo;
        std::string outputTexture;
    };
}
