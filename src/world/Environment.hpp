#pragma once

#include "interfaces/Serializable.hpp"
#include "util/EnumMetadata.hpp"

enum class SkyMode {
    NONE, SOLID, BOX
};

VC_ENUM_METADATA(SkyMode)
    {"none", SkyMode::NONE},
    {"solid", SkyMode::SOLID},
    {"box", SkyMode::BOX},
VC_ENUM_END

class Environment : public Serializable {
public:
    struct {
        SkyMode mode = SkyMode::BOX;
        bool stars = true;
        bool clouds = true;
        bool sprites = true;
    } sky;

    Environment() = default;

    dv::value serialize() const override;
    void deserialize(const dv::value& src) override;
private:
};
