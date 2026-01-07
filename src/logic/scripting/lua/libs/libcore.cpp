#include "api_lua.hpp"
#include "constants.hpp"
#include "assets/Assets.hpp"
#include "content/Content.hpp"
#include "content/ContentControl.hpp"
#include "engine/Engine.hpp"
#include "engine/EnginePaths.hpp"
#include "io/io.hpp"
#include "frontend/menu.hpp"
#include "frontend/screens/MenuScreen.hpp"
#include "graphics/core/Texture.hpp"
#include "logic/EngineController.hpp"
#include "logic/LevelController.hpp"
#include "util/listutil.hpp"
#include "util/platform.hpp"
#include "world/Level.hpp"
#include "world/generator/WorldGenerator.hpp"
#include "window/Window.hpp"

#include <memory>
#include <vector>
#include <sstream>

using namespace scripting;

/// @brief Creating new world
/// @param name Name world
/// @param seed Seed world
/// @param generator Type of generation
static int l_new_world(lua::State* L) {
    auto name = lua::require_string(L, 1);
    auto seed = lua::require_string(L, 2);
    auto generator = lua::require_string(L, 3);
    int64_t localPlayer = 0;
    if (lua::gettop(L) >= 4) {
        localPlayer = lua::tointeger(L, 4);
    }
    if (level != nullptr) {
        throw std::runtime_error("world must be closed before");
    }
    auto controller = engine->getController();
    controller->setLocalPlayer(localPlayer);
    controller->createWorld(name, seed, generator);
    return 0;
}

/// @brief Open world
/// @param name Name world
static int l_open_world(lua::State* L) {
    auto name = lua::require_string(L, 1);
    if (level != nullptr) {
        throw std::runtime_error("world must be closed before");
    }
    auto controller = engine->getController();
    controller->setLocalPlayer(0);
    controller->openWorld(name, false);
    return 0;
}

/// @brief Reopen world
static int l_reopen_world(lua::State*) {
    auto controller = engine->getController();
    if (level == nullptr) {
        throw std::runtime_error("no world open");
    }
    controller->reopenWorld(level->getWorld());
    return 0;
}

/// @brief Save world
static int l_save_world(lua::State* L) {
    if (controller == nullptr) {
        throw std::runtime_error("no world open");
    }
    controller->saveWorld();
    return 0;
}

/// @brief Close world
/// @param flag Save world (bool)
static int l_close_world(lua::State* L) {
    if (controller == nullptr) {
        throw std::runtime_error("no world open");
    }
    controller->processBeforeQuit();
    bool save_world = lua::toboolean(L, 1);
    if (save_world) {
        controller->saveWorld();
    }
    engine->onWorldClosed();
    return 0;
}

/// @brief Delete world
/// @param name Name world
static int l_delete_world(lua::State* L) {
    auto name = lua::require_string(L, 1);
    auto controller = engine->getController();
    controller->deleteWorld(name);
    return 0;
}

static int l_blank(lua::State*) {
    return 0;
}

static int l_capture_output(lua::State* L) {
    int argc = lua::gettop(L) - 1;
    if (!lua::isfunction(L, 1)) {
        throw std::runtime_error("function expected as argument 1");
    }
    for (int i = 0; i < argc; i++) {
        lua::pushvalue(L, i + 2);
    }
    lua::pushvalue(L, 1);

    auto prev_output = output_stream;
    auto prev_error = error_stream;

    std::stringstream captured_output;

    output_stream = &captured_output;
    error_stream = &captured_output;
    
    lua::call_nothrow(L, argc, 0);

    output_stream = prev_output;
    error_stream = prev_error;
    
    lua::pushstring(L, captured_output.str());
    return 1;
}

const luaL_Reg corelib[] = {
    {"blank", lua::wrap<l_blank>},
    {"new_world", lua::wrap<l_new_world>},
    {"open_world", lua::wrap<l_open_world>},
    {"reopen_world", lua::wrap<l_reopen_world>},
    {"save_world", lua::wrap<l_save_world>},
    {"close_world", lua::wrap<l_close_world>},
    {"delete_world", lua::wrap<l_delete_world>},
    {"capture_output", lua::wrap<l_capture_output>},
    {nullptr, nullptr}
};
