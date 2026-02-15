#include "vector_fonts.hpp"

#include "io/io.hpp"
#include "debug/Logger.hpp"
#include "graphics/core/ImageData.hpp"
#include "graphics/core/Texture.hpp"
#include "graphics/core/Font.hpp"

#include <stdexcept>
#include <ft2build.h>
#include FT_FREETYPE_H

static debug::Logger logger("vector_fonts");

static FT_Library library;

static const char* get_ft_error_message(FT_Error error) {
    #undef FTERRORS_H_
    #define FT_ERRORDEF( e, v, s )  case v: return s;
    #define FT_ERROR_START_LIST     switch (error) {
    #define FT_ERROR_END_LIST       }
    #include FT_ERRORS_H
    return "unknown error";
}

namespace {
    class FTFontFile : public vector_fonts::FontFile {
    public:
        FTFontFile(FT_Face face, util::Buffer<ubyte> buffer)
            : face(std::move(face)), buffer(std::move(buffer)) {
        }

        ~FTFontFile() {
            if (FT_Error error = FT_Done_Face(face)) {
                logger.error()
                    << "error on FT_Done_Face: " << get_ft_error_message(error);
            }
        }

        std::unique_ptr<Font> createInstance(int size) override {
            int pagesCount = 5;

            std::vector<std::unique_ptr<Texture>> pages;
            std::vector<Font::Glyph> glyphs;

            if (FT_Error error = FT_Set_Pixel_Sizes(face, 0, size)) {
                throw std::runtime_error(
                    "FT_Set_Pixel_Sizes error: " +
                    std::string(get_ft_error_message(error))
                );
            }

            int dstSize = size * 16;
            ImageData canvas(ImageFormat::RGBA8888, dstSize, dstSize);

            int srcWidth = size;
            int srcHeight = size;
            ImageData bitmapDst(ImageFormat::RGBA8888, srcWidth, srcHeight);

            for (int pageid = 0; pageid < pagesCount; pageid++) {
                for (int c = 0; c < 256; c++) {
                    if (!renderGlyph(pageid << 8 | c, bitmapDst)) {
                        continue;
                    }

                    canvas.blit(bitmapDst, (c % 16) * size, (c / 16) * size);
                    glyphs.push_back(Font::Glyph {
                        face->glyph->bitmap_top - size,
                        static_cast<int>(face->glyph->advance.x >> 6)
                    });
                }
                canvas.flipY();
                pages.push_back(Texture::from(&canvas));
            }
            return std::make_unique<Font>(
                std::move(pages), std::move(glyphs), size, size / 2
            );
        }
    private:
        FT_Face face;
        util::Buffer<ubyte> buffer;

        bool renderGlyph(int codepoint, ImageData& bitmapDst) {
            int width = bitmapDst.getWidth();
            if (FT_Error error = FT_Load_Char(face, codepoint, FT_LOAD_RENDER)) {
                logger.warning() << get_ft_error_message(error);
                return false;
            }
            const FT_Bitmap& bitmap = face->glyph->bitmap;
            auto dstData = bitmapDst.getData();
            std::memset(dstData, 0, bitmapDst.getDataSize());
            for (int row = 0; row < bitmap.rows; row++) {
                for (int col = 0; col < bitmap.width; col++) {
                    uint8_t value = bitmap.buffer[row * bitmap.pitch + col];
                    dstData[(row * width + col) * 4 + 0] = 255;
                    dstData[(row * width + col) * 4 + 1] = 255;
                    dstData[(row * width + col) * 4 + 2] = 255;
                    dstData[(row * width + col) * 4 + 3] = value;
                }
            }
            return true;
        }
    };
}

void vector_fonts::initialize() {
    if (FT_Error error = FT_Init_FreeType(&library)) {
        throw std::runtime_error(
            "could not initialize freetype: " +
            std::string(get_ft_error_message(error))
        );
    }
}

void vector_fonts::finalize() {
    if (FT_Error error = FT_Done_FreeType(library)) {
        logger.error() << "error on FT_Done_FreeType: " << get_ft_error_message(error);
    }
}

std::unique_ptr<vector_fonts::FontFile> vector_fonts::load_font(const std::string& filename) {
    auto bytes = io::read_bytes_buffer(io::path(filename));

    FT_Face face;

    if (FT_Error error =
            FT_New_Memory_Face(library, bytes.data(), bytes.size(), 0, &face)) {
        throw std::runtime_error(
            "could not load font '" + filename +
            "': " + std::string(get_ft_error_message(error))
        );
    }

    return std::make_unique<FTFontFile>(std::move(face), std::move(bytes));
}
