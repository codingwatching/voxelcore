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

static int l_load_content(lua::State* L) {
    content_control->loadContent();
    return 0;
}

static int l_reset_content(lua::State* L) {
    if (level != nullptr) {
        throw std::runtime_error("world must be closed before");
    }
    std::vector<std::string> nonResetPacks;
    if (lua::istable(L, 1)) {
        int len = lua::objlen(L, 1);
        for (int i = 0; i < len; i++) {
            lua::rawgeti(L, i + 1, 1);
            nonResetPacks.emplace_back(lua::require_lstring(L, -1));
            lua::pop(L);
        }
    }
    content_control->resetContent(std::move(nonResetPacks));
    return 0;
}

static int l_is_content_loaded(lua::State* L) {
    return lua::pushboolean(L, content != nullptr);
}

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

/// @brief Reconfigure packs
/// @param addPacks An array of packs to add
/// @param remPacks An array of packs to remove
static int l_reconfig_packs(lua::State* L) {
    if (!lua::istable(L, 1)) {
        throw std::runtime_error("strings array expected as the first argument");
    }
    if (!lua::istable(L, 2)) {
        throw std::runtime_error("strings array expected as the second argument");
    }
    std::vector<std::string> addPacks;
    int addLen = lua::objlen(L, 1);
    for (int i = 0; i < addLen; i++) {
        lua::rawgeti(L, i + 1, 1);
        addPacks.emplace_back(lua::require_lstring(L, -1));
        lua::pop(L);
    }
    std::vector<std::string> remPacks;
    int remLen = lua::objlen(L, 2);
    for (int i = 0; i < remLen; i++) {
        lua::rawgeti(L, i + 1, 2);
        remPacks.emplace_back(lua::require_lstring(L, -1));
        lua::pop(L);
    }
    auto engineController = engine->getController();
    try {
        engineController->reconfigPacks(controller, addPacks, remPacks);
    } catch (const contentpack_error& err) {
        throw std::runtime_error(
            std::string(err.what()) + " [" + err.getPackId() + " ]"
        );
    }
    return 0;
}

/// @brief Quit the game
static int l_quit(lua::State*) {
    engine->quit();
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
    {"load_content", lua::wrap<l_load_content>},
    {"reset_content", lua::wrap<l_reset_content>},
    {"is_content_loaded", lua::wrap<l_is_content_loaded>},
    {"new_world", lua::wrap<l_new_world>},
    {"open_world", lua::wrap<l_open_world>},
    {"reopen_world", lua::wrap<l_reopen_world>},
    {"save_world", lua::wrap<l_save_world>},
    {"close_world", lua::wrap<l_close_world>},
    {"delete_world", lua::wrap<l_delete_world>},
    {"reconfig_packs", lua::wrap<l_reconfig_packs>},
    {"quit", lua::wrap<l_quit>},
    {"capture_output", lua::wrap<l_capture_output>},
    {nullptr, nullptr}
};
