// utils.cpp

#include "utils.hpp"
#include "ansi_colors.hpp"

namespace utils {

void error(
    string prompt,
    string filename,
    bool show_warnings,
    bool is_warning,
    bool fatal
) {
    if (!show_warnings && is_warning) return;

    if (is_warning) {
        cerr << ansiColors::yellow
             << "Warning";
    } else {
        const char* color = ansiColors::orange;

        if (fatal) {
            color = ansiColors::bright_red;
        }

        cerr << color
             << "Error";
    }

    if (!filename.empty()) {
        cerr << ansiColors::reset
             << " in file '"
             << ansiColors::bright_cyan
             << ansiColors::underline
             << filename
             << ansiColors::reset
             << "'";
    }

    cerr << ansiColors::reset
         << ":\n";

    cerr << ansiColors::bright_blue
         << ">> "
         << prompt
         << " <<\n\n"
         << ansiColors::reset;

    if (!is_warning && fatal) {
        exit(1);
    }
}

filesystem::path get_file_path(string filename) {
    filesystem::path file_path(filename);

    if (file_path.is_relative()) {
        file_path = filesystem::current_path() / file_path;
    }

    if (!filesystem::exists(file_path)) {
        utils::error("File does not exist: " + filename);
    }

    return file_path;
}

string read_file(const filesystem::path& path) {
    ifstream file(path, ios::binary);

    if (!file) {
        utils::error("Failed to open file: " + path.string());
    }

    return string(
        istreambuf_iterator<char>(file),
        istreambuf_iterator<char>()
    );
}

}