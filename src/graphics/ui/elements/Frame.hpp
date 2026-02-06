#pragma once

#include "Container.hpp"

class Framebuffer;
class UiDocument;

namespace gui {
    class Frame final : public Container {
    public:
        struct State {
        };

        Frame(GUI& gui, std::string id, std::string outputTexture);
        virtual ~Frame();

        void draw(const DrawContext& pctx, const Assets& assets) override;

        void updateOutput(Assets& assets);

        const std::string& getOutputTexture() const;

        const std::string& getFrameId() const;

        /// @brief Frame state is controlled in gui::GUI
        State& getState();
    private:
        std::string frameId;
        State state {};
        std::unique_ptr<Framebuffer> fbo;
        std::string outputTexture;
    };
}
