#pragma once

#include "commons.hpp"
#include "ImageData.hpp"
#include "MeshData.hpp"

#include <GL/glew.h>

namespace gl {
    inline GLenum to_glenum(ImageFormat imageFormat) {
        switch (imageFormat) {
            case ImageFormat::rgb888: return GL_RGB;
            case ImageFormat::rgba8888: return GL_RGBA;
            default: 
                return 0;
        }
    }

    inline GLenum to_glenum(DrawPrimitive primitive) {
        static const GLenum primitives[]{
            GL_POINTS,
            GL_LINES,
            GL_TRIANGLES
        };
        return primitives[static_cast<int>(primitive)];
    }

    inline GLenum to_glenum(VertexAttribute::Type type) {
        using Type = VertexAttribute::Type;
        switch (type) {
            case Type::FLOAT:
                return GL_FLOAT;
            case Type::UNSIGNED_INT:
                return GL_UNSIGNED_INT;
            case Type::INT:
                return GL_INT;
            case Type::UNSIGNED_SHORT:
                return GL_UNSIGNED_SHORT;
            case Type::SHORT:
                return GL_SHORT;
            case Type::UNSIGNED_BYTE:
                return GL_UNSIGNED_BYTE;
            case Type::BYTE:
                return GL_BYTE;
        }
        return 0;
    }

    /// TODO: extend
    inline const char* to_string(GLenum item) {
        switch (item) {
            case GL_FRAMEBUFFER_UNDEFINED:
                return "framebuffer undefined";
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                return "framebuffer incomplete attachment";
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                return "framebuffer incomplete missing attachment";
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                return "framebuffer incomplete draw buffer";
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
                return "framebuffer incomplete read buffer";
            case GL_FRAMEBUFFER_UNSUPPORTED:
                return "framebuffer unsupported";
            case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
                return "framebuffer incomplete multisample";
            default:
                return "unknown";
        }
    }
}
