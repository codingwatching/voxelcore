#include "api_lua.hpp"

#include "io/io.hpp"
#include "io/settings_io.hpp"
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

static int l_get_version(lua::State* L) {
    return lua::pushvec_stack(
        L, glm::vec2(ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR)
    );
}

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

/// @brief Get a setting value
/// @param name The name of the setting
/// @return The value of the setting
static int l_get_setting(lua::State* L) {
    auto name = lua::require_string(L, 1);
    const auto value = engine->getSettingsHandler().getValue(name);
    return lua::pushvalue(L, value);
}

/// @brief Set a setting value
/// @param name The name of the setting
/// @param value The new value for the setting
static int l_set_setting(lua::State* L) {
    auto name = lua::require_string(L, 1);
    const auto value = lua::tovalue(L, 2);
    engine->getSettingsHandler().setValue(name, value);
    return 0;
}

/// @brief Convert a setting value to a string
/// @param name The name of the setting
/// @return The string representation of the setting value
static int l_str_setting(lua::State* L) {
    auto name = lua::require_string(L, 1);
    const auto string = engine->getSettingsHandler().toString(name);
    return lua::pushstring(L, string);
}

/// @brief Get information about a setting
/// @param name The name of the setting
/// @return A table with information about the setting
static int l_get_setting_info(lua::State* L) {
    auto name = lua::require_string(L, 1);
    auto setting = engine->getSettingsHandler().getSetting(name);
    lua::createtable(L, 0, 1);
    if (auto number = dynamic_cast<NumberSetting*>(setting)) {
        lua::pushnumber(L, number->getMin());
        lua::setfield(L, "min");
        lua::pushnumber(L, number->getMax());
        lua::setfield(L, "max");
        lua::pushnumber(L, number->getDefault());
        lua::setfield(L, "def");
        return 1;
    }
    if (auto integer = dynamic_cast<IntegerSetting*>(setting)) {
        lua::pushinteger(L, integer->getMin());
        lua::setfield(L, "min");
        lua::pushinteger(L, integer->getMax());
        lua::setfield(L, "max");
        lua::pushinteger(L, integer->getDefault());
        lua::setfield(L, "def");
        return 1;
    }
    if (auto boolean = dynamic_cast<FlagSetting*>(setting)) {
        lua::pushboolean(L, boolean->getDefault());
        lua::setfield(L, "def");
        return 1;
    }
    if (auto string = dynamic_cast<StringSetting*>(setting)) {
        lua::pushstring(L, string->getDefault());
        lua::setfield(L, "def");
        return 1;
    }
    lua::pop(L);
    throw std::runtime_error("unsupported setting type");
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
    {"get_version", lua::wrap<l_get_version>},
    {"start_debug_instance", lua::wrap<l_start_debug_instance>},
    {"focus", lua::wrap<l_focus>},
    {"create_memory_device", lua::wrap<l_create_memory_device>},
    {"get_content_sources", lua::wrap<l_get_content_sources>},
    {"set_content_sources", lua::wrap<l_set_content_sources>},
    {"reset_content_sources", lua::wrap<l_reset_content_sources>},
    {"set_title", lua::wrap<l_set_title>},
    {"open_folder", lua::wrap<l_open_folder>},
    {"open_url", lua::wrap<l_open_url>},
    {"get_setting", lua::wrap<l_get_setting>},
    {"set_setting", lua::wrap<l_set_setting>},
    {"str_setting", lua::wrap<l_str_setting>},
    {"get_setting_info", lua::wrap<l_get_setting_info>},
    // for other functions see libcore.cpp and stdlib.lua
    {nullptr, nullptr}
};
