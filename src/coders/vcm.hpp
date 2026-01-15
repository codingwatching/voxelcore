#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "graphics/commons/Model.hpp"

namespace rigging {
    class SkeletonConfig;
}

namespace vcm {
    struct VcmModel {
        std::unordered_map<std::string, model::Model> parts;
        std::unique_ptr<rigging::SkeletonConfig> skeleton;
    };

    VcmModel parseFull(
        std::string_view file, std::string_view src, bool usexml
    );

    std::unique_ptr<model::Model> parse(
        std::string_view file, std::string_view src, bool usexml
    );
}
