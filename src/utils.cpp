// utils.cpp

#include "utils.hpp"
#include "ansi_colors.hpp"
#include "compiler_cxt.hpp"

namespace utils {

void error(
    string prompt,
    CompilerCxt cxt,
    bool is_warning,
    bool fatal
) {
    if (!cxt.show_warnings && is_warning) return;

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

    if (!cxt.current_file.empty()) {
        cerr << ansiColors::reset
             << " in file '"
             << ansiColors::bright_cyan
             << ansiColors::underline
             << cxt.current_file.string()
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

string visualize_whitespaces(const string& s) {
    string out;
    out.reserve(s.size());

    for (unsigned char c : s) {
        switch (c) {
            case '\n': out += "\\n"; break;
            case '\t': out += "\\t"; break;
            case '\r': out += "\\r"; break;

            // normal space
            case ' ':  out += "#"; break;

            default:
                out += c;
                break;
        }
    }
    return out;
}

}