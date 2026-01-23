#pragma once

#include <memory>

class Assets;
class AssetsLoader;
class Engine;
class Content;

class AssetsManagement {
public:
    AssetsManagement(Engine& engine);

    void loadAssets(Content* content);
    
    Assets* getStorage();
    const Assets* getStorage() const;
private:
    Engine& engine;
    std::unique_ptr<Assets> assets;
    std::unique_ptr<AssetsLoader> backgroundLoader;
};
