#include "api_lua.hpp"

#include "io/io.hpp"
#include "io/devices/MemoryDevice.hpp"
#include "logic/scripting/scripting.hpp"
#include "content/ContentControl.hpp"
#include "engine/Engine.hpp"
#include "engine/EnginePaths.hpp"
#include "devtools/Project.hpp"
#include "network/Network.hpp"
#include "util/platform.hpp"
#include "util/stringutil.hpp"
#include "window/Window.hpp"
#include "frontend/locale.hpp"
#include "graphics/ui/GUI.hpp"
#include "graphics/ui/gui_util.hpp"
#include "graphics/ui/elements/Menu.hpp"

using namespace scripting;

static int l_start_debug_instance(lua::State* L) {
    if (!engine->getProject().permissions.has(Permissions::DEBUGGING)) {
        throw std::runtime_error("project has no debugging permission");
    }

    int port = lua::tointeger(L, 1);
    if (port == 0) {
        auto network = engine->getNetwork();
        if (network == nullptr) {
            throw std::runtime_error("project has no network permission");
        }
        port = network->findFreePort();
        if (port == -1) {
            throw std::runtime_error("could not find free port");
        }
    }
    auto projectPath = lua::isstring(L, 2) ? lua::require_lstring(L, 2) : "";
    const auto& paths = engine->getPaths();

    std::vector<std::string> args {
        "--res", paths.getResourcesFolder().u8string(),
        "--dir", paths.getUserFilesFolder().u8string(),
        "--dbg-server",  "tcp:" + std::to_string(port),
    };
    if (!projectPath.empty()) {
        args.emplace_back("--project");
        args.emplace_back(io::resolve(std::string(projectPath)).string());
    }

    platform::new_engine_instance(std::move(args));
    return lua::pushinteger(L, port);
}

static int l_focus(lua::State* L) {
    engine->getWindow().focus();
    return 0;
}

static int l_create_memory_device(lua::State* L) {
    std::string name = lua::require_string(L, 1);
    if (io::get_device(name)) {
        throw std::runtime_error(
            "entry-point '" + name + "' is already used"
        );
    }
    if (name.find(':') != std::string::npos) {
        throw std::runtime_error("invalid entry point name");
    }
    
    io::set_device(name, std::make_unique<io::MemoryDevice>());
    return 0;
}

static int l_get_content_sources(lua::State* L) {
    const auto& sources = engine->getContentControl().getContentSources();
    lua::createtable(L, static_cast<int>(sources.size()), 0);
    for (size_t i = 0; i < sources.size(); i++) {
        lua::pushlstring(L, sources[i].string());
        lua::rawseti(L, static_cast<int>(i + 1));
    }
    return 1;
}

static int l_set_content_sources(lua::State* L) {
    if (!lua::istable(L, 1)) {
        throw std::runtime_error("table expected as argument 1");
    }
    int len = lua::objlen(L, 1);
    std::vector<io::path> sources;
    for (int i = 0; i < len; i++) {
        lua::rawgeti(L, i + 1);
        sources.emplace_back(std::string(lua::require_lstring(L, -1)));
        lua::pop(L);
    }
    engine->getContentControl().setContentSources(std::move(sources));
    return 0;
}

static int l_reset_content_sources(lua::State* L) {
    engine->getContentControl().resetContentSources();
    return 0;
}

static int l_set_title(lua::State* L) {
    auto title = lua::require_string(L, 1);
    engine->getWindow().setTitle(title);
    return 0;
}

static int l_open_folder(lua::State* L) {
    platform::open_folder(io::resolve(lua::require_string(L, 1)));
    return 0;
}

static int l_open_url(lua::State* L) {
    auto url = lua::require_string(L, 1);

    std::wstring msg = langs::get(L"Are you sure you want to open the link:") +
                       L"\n" + util::str2wstr_utf8(url) +
                       std::wstring(L"?");

    auto menu = engine->getGUI().getMenu();

    guiutil::confirm(*engine, msg, [url, menu]() {
        platform::open_url(url);
        if (!menu->back()) {
            menu->reset();
        }
    });
    return 0;
}

const luaL_Reg applib[] = {
    {"start_debug_instance", lua::wrap<l_start_debug_instance>},
    {"focus", lua::wrap<l_focus>},
    {"create_memory_device", lua::wrap<l_create_memory_device>},
    {"get_content_sources", lua::wrap<l_get_content_sources>},
    {"set_content_sources", lua::wrap<l_set_content_sources>},
    {"reset_content_sources", lua::wrap<l_reset_content_sources>},
    {"set_title", lua::wrap<l_set_title>},
    {"open_folder", lua::wrap<l_open_folder>},
    {"open_url", lua::wrap<l_open_url>},
    // for other functions see libcore.cpp and stdlib.lua
    {nullptr, nullptr}
};
