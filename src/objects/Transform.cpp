#include "Transform.hpp"

#include "data/dv_util.hpp"
#include "maths/util.hpp"

#include <glm/gtc/matrix_transform.hpp>

void Transform::refresh() {
    combined = glm::mat4(1.0f);
    combined = glm::translate(combined, pos);
    combined = combined * glm::mat4(rot);
    combined = glm::scale(combined, size);
    displayPos = pos;
    displaySize = size;
    dirty = false;
}

dv::value Transform::serialize() const {
    auto tsfmap = dv::object();
    tsfmap["pos"] = dv::to_value(pos);
    if (size != glm::vec3(1.0f)) {
        tsfmap["size"] = dv::to_value(size);
    }
    if (rot != glm::mat3(1.0f)) {
        tsfmap["rot"] = dv::to_value(rot);
    }
    return tsfmap;
}

void Transform::deserialize(const dv::value& root) {
    dv::get_vec(root, "pos", pos);
    dv::get_vec(root, "size", size);
    dv::get_mat(root, "rot", rot);
}

bool Transform::checkValue(const glm::vec3& vector) {
    if (util::is_nan_or_inf(vector)) {
#ifndef NDEBUG
        throw std::invalid_argument(
            "invalid vector: " + std::to_string(vector.x) + ", " +
            std::to_string(vector.y) + ", " + std::to_string(vector.z)
        );
#else
        return false;
#endif
    }
    return true;
}

bool Transform::checkValue(const glm::mat3& matrix) {
    if (util::is_nan_or_inf(matrix)) {
#ifndef NDEBUG
        throw std::invalid_argument("invalid matrix (contains nan or inf)");
#else
        return false;
#endif
    }
    return true;
}
