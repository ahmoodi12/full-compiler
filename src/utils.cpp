#include "utils.hpp"
#include "ansi_colors.hpp"
#include "compiler_cxt.hpp"

#include "combined_include.hpp"

namespace utils {

void error(
    std::string prompt,
    CompilerCxt& cxt,
    std::string note_info,
    bool is_warning,
    bool fatal
) {
    if (!cxt.show_warnings && is_warning) return;

    if (is_warning) {
        std::cerr << ansiColors::yellow
             << "Warning";
    } else {
        const char* color = ansiColors::orange;

        if (fatal) {
            color = ansiColors::bright_red;
        }

        std::cerr << color
             << "Error";
    }

    if (!cxt.current_file->empty()) {
        std::cerr << ansiColors::reset
             << " in file '"
             << ansiColors::bright_cyan
             << ansiColors::underline
             << cxt.current_file->string()
             << ansiColors::reset
             << "'";
    }

    std::cerr << ansiColors::reset
         << ":\n";

    std::cerr << ansiColors::bright_blue
         << ">> "
         << prompt
         << " <<\n\n"
         << ansiColors::reset;

    if (!note_info.empty()) {
        std::cout << note_info << std::endl;
    }

    if (!is_warning && fatal) {
        std::exit(1);
    }
}

std::filesystem::path& validate_path(const std::filesystem::path& path, CompilerCxt& cxt) {
    if (!std::filesystem::exists(path)) {
        utils::error("path does not exist: " + path.string(), cxt);
    }
    return const_cast<std::filesystem::path&>(path);
}

std::filesystem::path get_path(std::string filename, CompilerCxt& cxt) {
    std::filesystem::path path(filename);

    if (path.is_relative()) {
        path = std::filesystem::current_path() / path;
    }

    if (!std::filesystem::exists(path)) {
        utils::error("path does not exist: " + path.string(), cxt);
    }

    return path;
}

std::string read_file(const std::filesystem::path& path, CompilerCxt& cxt) {
    std::ifstream file(path, std::ios::binary);

    if (!file) {
        utils::error("Failed to open file: " + path.string(), cxt);
    }

    return std::string(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
}

std::string visualize_whitespaces(const std::string& s) {
    std::string out;
    out.reserve(s.size());

    for (unsigned char c : s) {
        switch (c) {
            case '\n': out += "\\n"; break;
            case '\t': out += "\\t"; break;
            case '\r': out += "\\r"; break;

            case ' ':  out += "#"; break;

            default:
                out += c;
                break;
        }
    }
    return out;
}

}