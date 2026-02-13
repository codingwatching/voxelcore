#include "vector_fonts.hpp"

#include "io/io.hpp"

#include <stdexcept>
#include <ft2build.h>
#include FT_FREETYPE_H

static FT_Library library;

void vector_fonts::initialize() {
    if (FT_Error error = FT_Init_FreeType(&library)) {
        throw std::runtime_error(
            "could not initialize freetype: " +
            std::string(FT_Error_String(error))
        );
    }
}

static FT_Face load_ft_face(const io::path& path) {
    auto bytes = io::read_bytes_buffer(path);

    FT_Face face;

    if (FT_Error error =
            FT_New_Memory_Face(library, bytes.data(), bytes.size(), 0, &face)) {
        throw std::runtime_error(
            "could not load font '" + path.string() +
            "': " + std::string(FT_Error_String(error))
        );
    }

    return face;
}
