// organizer.cpp
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <json/json.h> // sudo apt-get install libjsoncpp-dev

using namespace std;
namespace fs = std::filesystem;

const string RESET = "\033[0m";
const string GREEN = "\033[92m";
const string RED = "\033[91m";
const string YELLOW = "\033[93m";
const string BLUE = "\033[94m";

string colorize(const string& text, const string& color) {
    return color + text + RESET;
}

map<string, vector<string>> defaultRules = {
    {"Images", {"jpg", "jpeg", "png", "gif", "bmp", "svg", "webp", "ico"}},
    {"Documents", {"pdf", "doc", "docx", "txt", "xls", "xlsx", "ppt", "pptx", "odt", "ods", "odp", "rtf"}},
    {"Archives", {"zip", "rar", "7z", "tar", "gz", "bz2", "xz", "tgz"}},
    {"Videos", {"mp4", "avi", "mkv", "mov", "wmv", "flv", "webm", "m4v"}},
    {"Music", {"mp3", "wav", "flac", "aac", "ogg", "wma", "m4a"}},
    {"Programs", {"exe", "msi", "dmg", "deb", "rpm", "sh", "bat", "cmd"}},
    {"Other", {}}
};

map<string, vector<string>> loadConfig(const string& path) {
    if (path.empty()) return defaultRules;
    ifstream f(path);
    if (!f) {
        cerr << colorize("Warning: cannot open config, using defaults", YELLOW) << endl;
        return defaultRules;
    }
    Json::Value root;
    f >> root;
    map<string, vector<string>> rules = defaultRules;
    for (const auto& key : root.getMemberNames()) {
        vector<string> exts;
        for (const auto& v : root[key]) {
            exts.push_back(v.asString());
        }
        rules[key] = exts;
    }
    return rules;
}

string getCategory(const string& filename, const map<string, vector<string>>& rules) {
    size_t dot = filename.rfind('.');
    if (dot == string::npos) return "Other";
    string ext = filename.substr(dot+1);
    for (auto& pair : rules) {
        for (auto& e : pair.second) {
            if (e == ext) return pair.first;
        }
    }
    return "Other";
}

string getDateSub(time_t t) {
    tm* tm = localtime(&t);
    char buf[8];
    strftime(buf, sizeof(buf), "%Y-%m", tm);
    return string(buf);
}

void organize(const string& folder, const map<string, vector<string>>& rules,
              const string& mode, bool dateFolders, bool recursive,
              bool dryRun, bool verbose, const string& logFile) {
    fs::path root = fs::absolute(folder);
    if (!fs::exists(root)) throw runtime_error("Folder not found: " + folder);
    if (!fs::is_directory(root)) throw runtime_error("Not a directory: " + folder);

    vector<string> logLines;
    int processed = 0;

    function<void(const fs::path&)> walk = [&](const fs::path& dir) {
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (entry.is_directory()) {
                if (recursive) walk(entry.path());
                continue;
            }
            string fname = entry.path().filename().string();
            if (fname[0] == '.') continue;
            string cat = getCategory(fname, rules);
            fs::path targetDir = root / cat;
            if (dateFolders) {
                auto ftime = fs::last_write_time(entry.path());
                auto sctp = chrono::time_point_cast<chrono::system_clock::duration>(
                    ftime - decltype(ftime)::clock::now() + chrono::system_clock::now()
                );
                time_t t = chrono::system_clock::to_time_t(sctp);
                string dateSub = getDateSub(t);
                targetDir = targetDir / dateSub;
            }
            if (!dryRun) {
                fs::create_directories(targetDir);
            }
            fs::path dest = targetDir / fname;

            // Conflict resolution
            if (fs::exists(dest)) {
                string base = dest.stem().string();
                string ext = dest.extension().string();
                int counter = 1;
                fs::path tryPath;
                do {
                    tryPath = targetDir / (base + "_" + to_string(counter) + ext);
                    counter++;
                } while (fs::exists(tryPath));
                dest = tryPath;
                if (verbose) {
                    cout << colorize("Conflict, new name: " + dest.filename().string(), YELLOW) << endl;
                }
            }

            string action = (mode == "move") ? "Move" : "Copy";
            if (!dryRun) {
                if (mode == "move") {
                    fs::rename(entry.path(), dest);
                } else {
                    fs::copy(entry.path(), dest, fs::copy_options::overwrite_existing);
                }
                processed++;
                logLines.push_back(action + ": " + entry.path().string() + " -> " + dest.string());
                if (verbose) {
                    cout << colorize(action + ": " + fname + " -> " + cat + "/", GREEN) << endl;
                }
            } else {
                logLines.push_back("[DRY RUN] " + action + ": " + entry.path().string() + " -> " + dest.string());
                if (verbose) {
                    cout << colorize("[DRY RUN] " + action + ": " + fname + " -> " + cat + "/", BLUE) << endl;
                }
                processed++;
            }
        }
    };
    walk(root);

    if (!logFile.empty()) {
        ofstream lf(logFile);
        for (auto& line : logLines) lf << line << endl;
        cout << colorize("Log saved to " + logFile, GREEN) << endl;
    }
    if (dryRun) {
        cout << colorize("Dry run completed. Would process " + to_string(processed) + " files.", GREEN) << endl;
    } else {
        cout << colorize("Processed " + to_string(processed) + " files.", GREEN) << endl;
    }
}

int main(int argc, char* argv[]) {
    string folder, configPath, mode = "move", logFile;
    bool dateFolders = false, recursive = false, dryRun = false, verbose = false;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-c" && i+1 < argc) configPath = argv[++i];
        else if (arg == "-m" && i+1 < argc) mode = argv[++i];
        else if (arg == "-d") dateFolders = true;
        else if (arg == "-r") recursive = true;
        else if (arg == "-n") dryRun = true;
        else if (arg == "-l" && i+1 < argc) logFile = argv[++i];
        else if (arg == "-v") verbose = true;
        else if (arg == "-h" || arg == "--help") {
            cout << "Usage: organizer [folder] [options]\n"
                 << "Options:\n"
                 << "  -c <file>   Config file\n"
                 << "  -m <mode>   move or copy (default move)\n"
                 << "  -d          Date folders\n"
                 << "  -r          Recursive\n"
                 << "  -n          Dry run\n"
                 << "  -l <file>   Log file\n"
                 << "  -v          Verbose\n";
            return 0;
        } else if (folder.empty()) {
            folder = arg;
        }
    }
    if (folder.empty()) {
        const char* home = getenv("HOME");
        folder = home ? string(home) + "/Downloads" : ".";
    }

    try {
        auto rules = loadConfig(configPath);
        organize(folder, rules, mode, dateFolders, recursive, dryRun, verbose, logFile);
    } catch (const exception& e) {
        cerr << colorize("Error: " + string(e.what()), RED) << endl;
        return 1;
    }
    return 0;
}
