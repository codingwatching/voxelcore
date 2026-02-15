#pragma once

#include <memory>
#include <string>

class Font;

namespace vector_fonts {
    class FontFile {
    public:
        virtual ~FontFile() {}
    
        virtual std::unique_ptr<Font> createInstance(int size) = 0;
    };

    void initialize();
    void finalize();

    std::unique_ptr<vector_fonts::FontFile> load_font(const std::string& filename);
}
