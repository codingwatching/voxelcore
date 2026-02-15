#include "Font.hpp"

#include <limits>
#include <utility>
#include "Texture.hpp"
#include "Batch2D.hpp"
#include "Batch3D.hpp"
#include "window/Camera.hpp"

inline constexpr uint GLYPH_SIZE = 16;
inline constexpr uint MAX_CODEPAGES = 10000; // idk ho many codepages unicode has
inline constexpr glm::vec4 SHADOW_TINT(0.0f, 0.0f, 0.0f, 1.0f);

Font::Font(
    std::vector<std::unique_ptr<Texture>> pages,
    std::vector<Glyph> glyphs,
    int lineHeight,
    int yoffset
)
    : lineHeight(lineHeight),
      glyphInterval(lineHeight / 2),
      yoffset(yoffset),
      pages(std::move(pages)),
      glyphs(std::move(glyphs)) {
}

Font::~Font() = default;

int Font::getYOffset() const {
    return yoffset;
}

int Font::getLineHeight() const {
    return lineHeight;
}

bool Font::isPrintableChar(uint codepoint) const {
    switch (codepoint){
        case ' ':
        case '\t':
        case '\n':
        case '\f':
        case '\r':
            return false;
        default:
            return true;
    }
}

int FontMetrics::calcWidth(std::wstring_view text, size_t offset, size_t length) const {
    return std::min(text.length() - offset, length) * _glyphInterval;
}

int Font::calcWidth(std::wstring_view text, size_t length) const {
    return calcWidth(text, 0, length);
}

int Font::calcWidth(std::wstring_view text, size_t offset, size_t length) const {
    return std::min(text.length()-offset, length) * glyphInterval;
}

static inline void draw_glyph(
    Batch2D& batch, 
    const glm::vec3& pos, 
    const glm::vec2& offset, 
    uint c, 
    const glm::vec3& right,
    const glm::vec3& up,
    float glyphInterval,
    const FontStyle& style
) {
    for (int i = 0; i <= style.bold; i++) {
        glm::vec4 color;

        if (style.color == glm::vec4(1, 1, 1, 1)) {
            color = batch.getColor();
        } else {
            color = style.color;
        }

        batch.sprite(
            pos.x + (offset.x + i / (right.x/glyphInterval/2.0f)) * right.x,
            pos.y + offset.y * up.y,
            right.x / glyphInterval,
            up.y,
            -0.15f * style.italic,
            16,
            c,
            color
        );
    }
}

static inline void draw_glyph(
    Batch3D& batch, 
    const glm::vec3& pos, 
    const glm::vec2& offset, 
    uint c, 
    const glm::vec3& right,
    const glm::vec3& up,
    float glyphInterval,
    const FontStyle& style
) {
    for (int i = 0; i <= style.bold; i++) {
        glm::vec4 color;

        if (style.color == glm::vec4(1, 1, 1, 1)) {
            color = batch.getColor();
        } else {
            color = style.color;
        }
        

        batch.sprite(
            pos + right * (offset.x + i) + up * offset.y,
            up, right / glyphInterval,
            0.5f,
            0.5f,
            16,
            c,
            color
        );
    }
}

template <class Batch>
static inline void draw_text(
    const Font& font,
    Batch& batch,
    std::wstring_view text,
    const glm::vec3& pos,
    const glm::vec3& right,
    const glm::vec3& up,
    float interval,
    const FontStylesScheme* styles,
    size_t styleMapOffset
) {
    static FontStylesScheme defStyles {{{}}, {0}};

    if (styles == nullptr) {
        styles = &defStyles;
    }
    
    uint page = 0;
    uint next = MAX_CODEPAGES;
    int x = 0;
    int y = 0;
    bool hasLines = false;

    do {
        for (size_t i = 0; i < text.length(); i++) {
            uint c = text[i];
            size_t styleIndex = styles->map.at(
                std::min(styles->map.size() - 1, i + styleMapOffset)
            );
            const FontStyle& style = styles->palette.at(styleIndex);
            hasLines |= style.strikethrough;
            hasLines |= style.underline;

            if (!font.isPrintableChar(c)) {
                x++;
                continue;
            }
            int yOffset = 0;
            if (auto glyph = font.getGlyph(c)) {
                yOffset = glyph->yOffset;
            }
            uint charpage = c >> 8;
            if (charpage == page){
                batch.texture(font.getPage(charpage));
                draw_glyph(
                    batch, pos, glm::vec2(x, y - yOffset / static_cast<float>(font.getLineHeight())), c, right, up, interval, style
                );
            }
            else if (charpage > page && charpage < next){
                next = charpage;
            }
            x++;
        }
        page = next;
        next = MAX_CODEPAGES;
        x = 0;
    } while (page < MAX_CODEPAGES);

    if (!hasLines) {
        return;
    }
    batch.texture(font.getPage(0));
    for (size_t i = 0; i < text.length(); i++) {
        size_t styleIndex = styles->map.at(
            std::min(styles->map.size() - 1, i + styleMapOffset)
        );
        const FontStyle& style = styles->palette.at(styleIndex);
        FontStyle lineStyle = style;
        lineStyle.bold = true;
        if (style.strikethrough) {
            draw_glyph(
                batch, pos, glm::vec2(x, y), '-', right, up, interval, lineStyle
            );
        }
        if (style.underline) {
            draw_glyph(
                batch, pos, glm::vec2(x, y), '_', right, up, interval, lineStyle
            );
        }
        x++;
    }
}

const Texture* Font::getPage(int charpage) const {
    Texture* texture = nullptr;
    if (charpage < pages.size()) {
        texture = pages[charpage].get();
    }
    if (texture == nullptr){
        texture = pages[0].get();
    }
    return texture;
}

void Font::draw(
    Batch2D& batch,
    std::wstring_view text,
    int x,
    int y,
    const FontStylesScheme* styles,
    size_t styleMapOffset,
    float scale
) const {
    draw_text(
        *this, batch, text,
        glm::vec3(x, y, 0),
        glm::vec3(glyphInterval*scale, 0, 0),
        glm::vec3(0, lineHeight*scale, 0),
        glyphInterval/static_cast<float>(lineHeight),
        styles,
        styleMapOffset
    );
}

void Font::draw(
    Batch3D& batch,
    std::wstring_view text,
    const FontStylesScheme* styles,
    size_t styleMapOffset,
    const glm::vec3& pos,
    const glm::vec3& right,
    const glm::vec3& up
) const {
    draw_text(
        *this, batch, text, pos,
        right * static_cast<float>(glyphInterval),
        up * static_cast<float>(lineHeight),
        glyphInterval/static_cast<float>(lineHeight),
        styles,
        styleMapOffset
    );
}

std::unique_ptr<Font> Font::createBitmapFont(
    std::vector<std::unique_ptr<ImageData>> pages
) {
    int res = pages.at(0)->getHeight() / 16;
    std::vector<std::unique_ptr<Texture>> textures;
    std::vector<Glyph> glyphs(textures.size() * 256);
    for (auto& page : pages) {
        if (page == nullptr) {
            textures.emplace_back(nullptr);
        } else {
            auto texture = Texture::from(page.get());
            texture->setMipMapping(false, true);
            textures.emplace_back(std::move(texture));
        }
    }
    return std::make_unique<Font>(std::move(textures), std::move(glyphs), res, 4);
}
