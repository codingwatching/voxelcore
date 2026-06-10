#include "logic/scripting/io_descriptors.hpp"

#include "debug/Logger.hpp"
#include "io/io.hpp"

#include <memory>
#include <string>
#include <vector>
#include <optional>

static debug::Logger logger("descriptors-manager");

using namespace scripting;

namespace {
    struct StreamDescriptor {
        std::unique_ptr<std::istream> in;
        std::unique_ptr<std::ostream> out;
    };
    std::vector<std::optional<StreamDescriptor>> descriptors;
}

std::istream* io_descriptors::get_input(int id) {
    if (!is_readable(id)) {
        return nullptr;
    }
    return ::descriptors[id]->in.get();
}

std::ostream* io_descriptors::get_output(int id) {
    if (!is_writeable(id)) {
        return nullptr;
    }
    return ::descriptors[id]->out.get();
}

void io_descriptors::flush(int id) {
    if (is_writeable(id)) {
        ::descriptors[id]->out->flush();
    }
}

bool io_descriptors::has_descriptor(int id) {
    return id >= 0 && id < static_cast<int>(::descriptors.size()) &&
           ::descriptors[id].has_value() &&
           (::descriptors[id]->in != nullptr ||
            ::descriptors[id]->out != nullptr);
}

bool io_descriptors::is_readable(int id) {
    return id >= 0 && id < static_cast<int>(::descriptors.size())
        && ::descriptors[id].has_value()
        && ::descriptors[id]->in != nullptr;
}

bool io_descriptors::is_writeable(int id) {
    return id >= 0 && id < static_cast<int>(::descriptors.size())
        && ::descriptors[id].has_value()
        && ::descriptors[id]->out != nullptr;
}

void io_descriptors::close(int id) {
    if (!has_descriptor(id)) {
        return;
    }
    auto& desc = ::descriptors[id].value();
    if (desc.out) {
        desc.out->flush();
    }
    desc.in.reset();
    desc.out.reset();
    ::descriptors[id] = std::nullopt;
}

int io_descriptors::open_descriptor(const io::path& path, bool write, bool read) {
    std::unique_ptr<std::istream> in;
    std::unique_ptr<std::ostream> out;

    try {
        if (read) {
            in = io::read(path);
        }
        if (write) {
            out = io::write(path);
        }
    } catch (const std::exception& e) {
        logger.error() << "failed to open descriptor for " << path.string()
                        << ": " << e.what();
        return -1;
    }

    for (int i = 0; i < static_cast<int>(descriptors.size()); ++i) {
        if (!descriptors[i].has_value()) {
            descriptors[i] = StreamDescriptor{ std::move(in), std::move(out) };
            return i;
        }
    }

    ::descriptors.emplace_back(StreamDescriptor{ std::move(in), std::move(out) });
    return static_cast<int>(::descriptors.size() - 1);
}


void io_descriptors::close_all_descriptors() {
    for (int i = 0; i < static_cast<int>(::descriptors.size()); ++i) {
        if (::descriptors[i].has_value()) {
            close(i);
        }
    }
    ::descriptors.clear();
}
