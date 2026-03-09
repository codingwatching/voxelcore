#include "PhysicsSolver.hpp"
#include "Hitbox.hpp"

#include "maths/aabb.hpp"
#include "voxels/Block.hpp"
#include "voxels/GlobalChunks.hpp"
#include "voxels/voxel.hpp"
#include "objects/Entities.hpp"
#include "debug/Logger.hpp"

#include <algorithm>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

inline constexpr float E = 0.03f;
inline constexpr float MAX_FIX = 0.15f;

static debug::Logger logger("physics-solver");

PhysicsSolver::PhysicsSolver(glm::vec3 gravity) : gravity(std::move(gravity)) {}

static float calc_step_height(
    const GlobalChunks& chunks, 
    const glm::vec3& pos, 
    const glm::vec3& half,
    float stepHeight
) {
    AABB aabb(-half, +half);
    aabb.scale(glm::vec3(1.0f - E * 2, 1.0f, 1.0f - E * 2));
    aabb = aabb + pos + glm::vec3(0.0f, stepHeight, 0.0f);
    if (stepHeight > 0.0f) {
        for (int ix = 0; ix <= glm::ceil((half.x - E) * 2); ix++) {
            float x = (pos.x - half.x) + ix;
            for (int iz = 0; iz <= glm::ceil((half.z - E) * 2); iz++) {
                float z = (pos.z - half.z) + iz;
                if (chunks.isObstacleAt(x, pos.y + half.y + stepHeight, z, aabb)) {
                    return 0.0f;
                }
            }
        }
    }
    return stepHeight;
}

template <int nx, int ny, int nz, int sign>
static void calc_collision(
    Hitbox& hitbox,
    const GlobalChunks& chunks,
    const std::vector<Hitbox*>& solidHitboxes,
    const glm::vec3& half,
    float stepHeight
) {
    auto& pos = hitbox.position;
    auto& vel = hitbox.velocity;

    glm::vec3 offset(0.0f, stepHeight + E, 0.0f);
    for (auto box : solidHitboxes) {
        if (glm::distance2(box->position, pos) < E) {
            continue;
        }
        auto boxhalf = box->getHalfSize();
        auto aabb = AABB(pos - half, pos + half);
        glm::vec3 scale(1.0f);
        scale[nz] = 1.0f - E * 8.0f;
        scale[ny] = 1.0f - E * 2.0f;
        aabb.scale(scale);
        aabb = aabb + offset;
        aabb.b.y -= stepHeight + E * 2;

        if ((box->position[nx] - pos[nx]) * sign <= 0.0f || !box->getAABB().intersects(aabb)) {
            continue;
        }

        float newnegx = box->position[nx] - boxhalf[nx] - half[nx];
        float newposx = box->position[nx] + boxhalf[nx] + half[nx];
        float newx;
        if ((glm::abs(newnegx - pos[nx]) - glm::abs(newposx - pos[nx]))*sign < 0.0f) {
            newx = sign > 0 ? newnegx : newposx;
        } else {
            continue;
        }
        if ((pos[nx] - newx) * sign > 0.0f && glm::abs(pos[nx] - newx) < MAX_FIX) {
            if (vel[nx] * sign > 0.0f) {
                vel[nx] = 0.0f;
            }
            pos[nx] = newx;
        }
    }
    if (vel[nx] * sign <= 0.0f && hitbox.groundVelocity[nx] * sign <= 0.0f) {
        return;
    }
    for (int iy = 0; iy <= glm::ceil(((half - offset * 0.5f)[ny] - E) * 2); iy++) {
        glm::vec3 coord;
        coord[ny] = ((pos + offset)[ny] - half[ny] + E) + iy;
        for (int iz = 0; iz <= glm::ceil((half[nz] - E) * 2); iz++) {
            coord[nz] = (pos[nz] - half[nz] + E) + iz;
            coord[nx] = pos[nx] + (half[nx] + E * 2) * sign;

            auto aabb = AABB(pos - half, pos + half);
            glm::vec3 scale(1.0f);
            scale[nz] = 1.0f - E * 8.0f;
            aabb.scale(scale);
            aabb = aabb + offset;
            aabb.b.y -= stepHeight + E * 2;

            if (const auto obstacle = chunks.isObstacleAt(coord.x, coord.y, coord.z, aabb)) {
                float newx = std::floor(coord[nx]) - half[nx] * sign +
                             (sign > 0 ? obstacle->min() : obstacle->max())[nx];
                if ((pos[nx] - newx) * sign > 0.0f && glm::abs(pos[nx] - newx) < MAX_FIX) {
                    vel[nx] = 0.0f;
                    pos[nx] = newx;
                }
                return;
            }
        }
    }
}

static bool calc_collision_neg_y(
    Hitbox& hitbox,
    const GlobalChunks& chunks,
    const std::vector<Hitbox*>& solidHitboxes,
    const glm::vec3& half
) {
    auto& pos = hitbox.position;
    auto& vel = hitbox.velocity;

    for (auto box : solidHitboxes) {
        if (glm::distance2(box->position, pos) < E) {
            continue;
        }
        auto aabb = AABB(pos - half, pos + half);
        glm::vec3 scale(1.0f);
        scale.x = 1.0f - E * 4.0f;
        scale.z = 1.0f - E * 4.0f;
        aabb.scale(scale);

        auto boxhalf = box->getHalfSize();
        if (box->position.y < pos.y && box->getAABB().intersects(aabb)) {
            float newy = box->position.y + boxhalf.y + half.y;
            if (pos.y < newy && glm::abs(pos.y - newy) < boxhalf.y) {
                pos.y = newy;
            }
            if (glm::abs(box->delta) > 1e-5f) {
                hitbox.groundVelocity =
                    (box->position - box->prevPosition) / box->delta;
            }
            if (vel.y < 0.0f) {
                vel.y = 0.0f;
                if (hitbox.groundMaterial.empty()) {
                    hitbox.groundMaterial = box->material;
                }
                return true;
            }
        }
    }
    if (vel.y >= 0.0f) {
        return false;
    }
    hitbox.groundVelocity = {};
    for (int ix = 0; ix <= glm::ceil((half.x - E) * 2); ix++) {
        glm::vec3 coord;
        coord.x = (pos.x - half.x + E) + ix;
        for (int iz = 0; iz <= glm::ceil((half.z - E) * 2); iz++) {
            coord.z = (pos.z - half.z + E) + iz;
            coord.y = pos.y - half.y - E * 2;

            auto aabb = AABB(pos - half, pos + half);
            glm::vec3 scale(1.0f);
            scale.x = 1.0f - E * 4.0f;
            scale.z = 1.0f - E * 4.0f;
            aabb.scale(scale);

            if (const auto obstacle =
                    chunks.isObstacleAt(coord.x, coord.y, coord.z, aabb)) {
                float newy = std::floor(coord.y) + half.y + obstacle->max().y;
                if (newy >= pos.y) {
                    vel.y = 0.0f;
                    pos.y = newy;
                }
                return true;
            }
        }
    }
    return false;
}

void PhysicsSolver::calcCollisions(
    const GlobalChunks& chunks,
    Hitbox& hitbox, 
    glm::vec3& vel, 
    glm::vec3& pos, 
    const glm::vec3& half,
    float stepHeight
) {
    hitbox.yCollided = false;
    stepHeight = calc_step_height(chunks, pos, half, stepHeight);

    auto prevPos = pos;

    calc_collision<0, 1, 2, -1>(hitbox, chunks, solidHitboxes, half, stepHeight);
    calc_collision<0, 1, 2, 1>(hitbox, chunks, solidHitboxes, half, stepHeight);

    float xpos = pos.x;
    pos.x = prevPos.x;

    calc_collision<2, 1, 0, -1>(hitbox, chunks, solidHitboxes, half, stepHeight);
    calc_collision<2, 1, 0, 1>(hitbox, chunks, solidHitboxes, half, stepHeight);
    pos.x = xpos;

    if (calc_collision_neg_y(hitbox, chunks, solidHitboxes, half)) {
        hitbox.grounded = true;
    }

    if (vel.y > 0.0f){
        AABB boxAABB = AABB(-half, +half);
        boxAABB.scale(glm::vec3(1.0f - E * 4, 1.0f + E * 2, 1.0f - E * 4));
        boxAABB = boxAABB.translated(pos);

        for (int ix = 0; ix <= glm::ceil((half.x - E) * 2); ix++) {
            float x = (pos.x - half.x + E) + ix;
            for (int iz = 0; iz <= glm::ceil((half.z - E) * 2); iz++) {
                float z = (pos.z - half.z + E) + iz;
                float y = (pos.y + half.y + E) + 0.5f;
                if (auto aabb = chunks.isObstacleAt(x, y, z, boxAABB)) {
                    float newy = std::floor(y) - half.y + aabb->min().y - E;
                    if (pos.y >= newy) {
                        vel.y = 0.0f;
                        pos.y = newy;
                        hitbox.yCollided = true;
                    }
                    break;
                }
            }
        }
        for (auto box : solidHitboxes) {
            if (glm::distance2(box->position, pos) < E) {
                continue;
            }
            auto boxhalf = box->getHalfSize();
            if (box->position.y > pos.y && box->getAABB().intersects(boxAABB)) {
                float newy = box->position.y - boxhalf.y - half.y;
                if (pos.y > newy && glm::abs(pos.y - newy) < 0.5f) {
                    pos.y = newy;
                }
                if (vel.y > 0.0f) {
                    vel.y = 0.0f;
                    break;
                }
            }
        }
    }

    if (hitbox.yCollided) {
        pos.y = prevPos.y;
    }

    // step on
    if (stepHeight > 0.0 && vel.y <= 0.0f) {
        AABB boxAABB = AABB(pos - half, pos + half);
        boxAABB.scale(glm::vec3(1.0f - E * 2, 1.0f - E * 2, 1.0f - E * 2));
        for (int ix = 0; ix <= glm::ceil((half.x - E) * 2); ix++) {
            float x = (pos.x - half.x) + ix;
            for (int iz = 0; iz <= glm::ceil((half.z - E) * 2); iz++) {
                float z = (pos.z - half.z) + iz;
                float y = (pos.y - half.y + E) + E;
                if (auto aabb = chunks.isObstacleAt(x, y, z, boxAABB)) {
                    if (vel.y < 0.0f) {
                        vel.y = 0.0f;
                    }
                    float newy = std::floor(y) + aabb->max().y + half.y;
                    if (std::abs(newy - pos.y) <= stepHeight) {
                        pos.y = newy;
                    }
                    break;
                }
            }
        }
        for (auto box : solidHitboxes) {
            if (glm::distance2(box->position, pos) < E) {
                continue;
            }
            auto boxhalf = box->getHalfSize();
            if (box->getAABB().intersects(boxAABB)) {
                vel.y = 0.0f;
                float newy = box->position.y + boxhalf.y + half.y;
                if (std::abs(newy - pos.y) <= stepHeight + E * 4) {
                    pos.y = newy;
                }
            }
        }
    }
}

void PhysicsSolver::calcSubstep(
    const GlobalChunks& chunks,
    Hitbox& hitbox,
    glm::vec3& vel,
    glm::vec3& pos,
    bool prevGrounded,
    float dt,
    int substeps
) {
    if (hitbox.grounded) {
        pos.x += hitbox.groundVelocity.x * dt * substeps;
        if (hitbox.groundVelocity.y < 0.0f) {
            pos.y += hitbox.groundVelocity.y * dt * substeps;
        }
        pos.z += hitbox.groundVelocity.z * dt * substeps;
    }

    auto initpos = pos;
    auto half = hitbox.getHalfSize();
    float gravityScale = hitbox.gravityScale;
        
    if (hitbox.type == BodyType::DYNAMIC) {
        calcCollisions(
            chunks,
            hitbox,
            vel,
            pos,
            half,
            (prevGrounded && gravityScale > 0.0f) ? hitbox.stepHeight : 0.0f
        );
    }

    vel += gravity * dt * gravityScale;
    pos += vel * dt + gravity * gravityScale * dt * dt * 0.5f;

    if (hitbox.grounded && pos.y < initpos.y) {
        pos.y = initpos.y;
    }

    // crouching
    if (!hitbox.crouching || !hitbox.grounded) {
        return;
    }
    float y = (pos.y - half.y - E);
    for (int axis = 0; axis <= 2; axis += 2) {
        hitbox.grounded = false;

        auto checkPos = pos;
        checkPos.x = axis != 0 ? initpos.x : pos.x;
        checkPos.z = axis == 0 ? initpos.z : pos.z;

        AABB boxAABB(checkPos - half, checkPos + half);
        boxAABB.scale(glm::vec3(1.0f - E * 4, 1.0f, 1.0f - E * 4));
        for (int ix = 0; ix <= glm::ceil((half.x - E) * 2); ix++) {
            float x = (pos.x - half.x + E) + ix;
            for (int iz = 0; iz <= glm::ceil((half.z - E) * 2); iz++){
                float z = (pos.z - half.z + E) + iz;
                if (chunks.isObstacleAt(x, y, z, boxAABB)){
                    hitbox.grounded = true;
                    break;
                }
            }
        }
        for (auto box : solidHitboxes) {
            if (glm::distance2(box->position, pos) < E) {
                continue;
            }
            if (box->getAABB().intersects(boxAABB)) {
                hitbox.grounded = true;
                break;
            }
        }
        if (!hitbox.grounded) {
            pos[axis] = initpos[axis];
            vel[axis] = 0.0f;
        }
    }
    hitbox.grounded = true;
}

void PhysicsSolver::step(
    const GlobalChunks& chunks,
    Hitbox& hitbox,
    float delta,
    uint substeps,
    entityid_t entity
) {
    hitbox.prevPosition = hitbox.position;
    hitbox.delta = delta;
    hitbox.groundMaterial.clear();
    
    float dt = delta / static_cast<float>(substeps);
    float linearDamping = hitbox.linearDamping * hitbox.friction;

    glm::vec3& pos = hitbox.position;
    glm::vec3& vel = hitbox.velocity;
   
    bool prevGrounded = hitbox.grounded;
    hitbox.grounded = false;
    for (uint i = 0; i < substeps; i++) {
        calcSubstep(chunks, hitbox, vel, pos, prevGrounded, dt, substeps);
    }

    vel.x /= 1.0f + delta * linearDamping;
    vel.z /= 1.0f + delta * linearDamping;
    if (hitbox.verticalDamping > 0.0f) {
        vel.y /= 1.0f + delta * linearDamping * hitbox.verticalDamping;
    }
    if (prevGrounded && !hitbox.grounded) {
        auto appliedVelocity = hitbox.groundVelocity;
        vel += appliedVelocity;
        hitbox.groundVelocity = {};
    }

    AABB aabb;
    aabb.a = pos - hitbox.getHalfSize();
    aabb.b = pos + hitbox.getHalfSize();
    for (size_t i = 0; i < sensors.size(); i++) {
        auto& sensor = *sensors[i];
        if (sensor.entity == entity) {
            continue;
        }

        bool triggered = false;
        switch (sensor.type) {
            case SensorType::AABB:
                triggered = aabb.intersects(sensor.calculated.aabb);
                break;
            case SensorType::RADIUS:
                triggered = glm::distance2(
                    pos, glm::vec3(sensor.calculated.radial))
                     < sensor.calculated.radial.w;
                break;
        }
        if (triggered) {
            if (sensor.prevEntered.find(entity) == sensor.prevEntered.end()) {
                sensor.enterCallback(sensor.entity, sensor.index, entity);
            }
            sensor.nextEntered.insert(entity);
        }
    }
}

void PhysicsSolver::removeSensor(Sensor* sensor) {
    sensors.erase(
        std::remove(sensors.begin(), sensors.end(), sensor), sensors.end()
    );
}
