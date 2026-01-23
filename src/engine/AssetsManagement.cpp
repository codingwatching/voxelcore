#include "AssetsManagement.hpp"

#include "assets/Assets.hpp"
#include "assets/AssetsLoader.hpp"
#include "engine/Engine.hpp"
#include "debug/Logger.hpp"
#include "content/Content.hpp"
#include "graphics/core/Shader.hpp"
#include "EnginePaths.hpp"
#include "coders/GLSLExtension.hpp"
#include "graphics/render/ModelsGenerator.hpp"
#include "graphics/ui/GUI.hpp"

static debug::Logger logger("assets-management");

AssetsManagement::AssetsManagement(Engine& engine) : engine(engine) {}

const Assets* AssetsManagement::getStorage() const {
    return assets.get();
}

Assets* AssetsManagement::getStorage() {
    return assets.get();
}

void AssetsManagement::loadAssets(Content* content) {
    const auto& paths = engine.getPaths();
    logger.info() << "loading assets";
    Shader::preprocessor->setPaths(&paths.resPaths);

    auto new_assets = std::make_unique<Assets>();
    AssetsLoader loader(engine, *new_assets, paths.resPaths);
    AssetsLoader::addDefaults(loader, content);

    // no need
    // correct log messages order is more useful
    // todo: before setting to true, check if GLSLExtension thread safe
    bool threading = false; // look at three upper lines
    if (threading) {
        auto task = loader.startTask([=](){});
        task->waitForEnd();
    } else {
        while (loader.hasNext()) {
            loader.loadNext();
        }
    }
    assets = std::move(new_assets);
    if (content) {
        ModelsGenerator::prepare(*content, *assets);
    }
    assets->setup();
    engine.getGUI().onAssetsLoad(assets.get());
}
