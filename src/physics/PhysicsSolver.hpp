#pragma once

#include "Hitbox.hpp"

#include "typedefs.hpp"
#include "voxels/voxel.hpp"

#include <vector>
#include <glm/glm.hpp>

class Block;
class Entities;
class GlobalChunks;
struct Sensor;

class PhysicsSolver {
public:
    PhysicsSolver(glm::vec3 gravity);
    void step(
        const GlobalChunks& chunks,
        Hitbox& hitbox,
        float delta,
        uint substeps,
        entityid_t entity
    );

    auto& getSensorsWriteable() {
        return sensors;
    }

    auto& getSolidHitboxesWriteable() {
        return solidHitboxes;
    }

    void removeSensor(Sensor* sensor);
private:
    glm::vec3 gravity;
    std::vector<Sensor*> sensors;
    std::vector<Hitbox*> solidHitboxes;

    void calcCollisions(
        const GlobalChunks& chunks,
        Hitbox& hitbox,
        glm::vec3& vel,
        glm::vec3& pos,
        const glm::vec3 half,
        float stepHeight
    );

    void calcSubstep(
        const GlobalChunks& chunks,
        Hitbox& hitbox,
        glm::vec3& vel,
        glm::vec3& pos,
        bool prevGrounded,
        float dt,
        int substeps
    );
};
