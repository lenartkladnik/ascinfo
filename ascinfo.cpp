#include <vector>
#include <map>
#include <string>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <array>
#include <regex>
#include <format>
#include <fstream>
#include <cctype>
#include <algorithm>
#include <stdexcept>
#include <fstream>

const std::string VERSION = "1.0.0";

// Replace in string
std::string replace(std::string string, std::string old_str, std::string new_str){
    size_t pos = 0;
    while ((pos = string.find(old_str, pos)) != std::string::npos) {
        string.replace(pos, old_str.length(), new_str);
        pos += new_str.length();
    }

    return string;
}

// Capitalize the first char of string
std::string capitalize(std::string& string) {
    if (!string.empty()) {
        string[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(string[0])));
    }

    return string;
}

// Remove whitespace chars from string
std::string strip(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(), ::isspace);
    auto end = std::find_if_not(str.rbegin(), str.rend(), ::isspace).base();

    if (start >= end) {
        return "";
    }
    return std::string(start, end);
}

// Check if a command "exists" / is valid
bool commandExists(const std::string& command) {
    std::string cmd = "which " + command + " > /dev/null 2>&1";
    return std::system(cmd.c_str()) == 0;
}

// Convert string in format "x = y" to {{ x, y }} map
std::map<std::string, std::string> parseToMap(const std::string& input) {
    std::map<std::string, std::string> result;
    std::istringstream stream(input);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.empty()) continue;

        auto equal_pos = line.find('=');
        if (equal_pos == std::string::npos) continue;

        std::string key = line.substr(0, equal_pos);
        std::string value = line.substr(equal_pos + 1);

        if (!value.empty() && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }

        result[key] = value;
    }

    return result;
}

// Split string by delimiter and return the vector of strings
std::vector<std::string> split(std::string string, char delimiter) {
    std::stringstream ss(string);
	
    std::string t;
    std::vector<std::string> res;

    while (getline(ss, t, delimiter)){
        res.push_back(t);
    }

    return res;
}

// Run an external command, suppress the output and return the stdout
std::string exec(const char* cmd, bool rem_nl = true) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen((std::string(cmd) + std::string(" 2>/dev/null")).c_str(), "r"), static_cast<int(*)(FILE*)>(pclose));
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    auto stripped = strip(result);

    if (rem_nl) return replace(stripped, "\n", " ");
    else return stripped;
}

// Get name of process by PID
std::string getProcessName(pid_t pid) {
    std::ifstream comm("/proc/" + std::to_string(pid) + "/comm");
    std::string name;
    std::getline(comm, name);
    return name;
}

pid_t getParentPid(pid_t pid) {
    std::ifstream status("/proc/" + std::to_string(pid) + "/status");
    std::string line;
    while (std::getline(status, line)) {
        if (line.find("PPid:") == 0) {
            std::istringstream iss(line.substr(5));
            pid_t ppid;
            iss >> ppid;
            return ppid;
        }
    }
    return -1;
}

// Return the terminal emulator used by the user
std::string terminalEmulator() {
    pid_t pid = getppid();
    for (int i = 0; i < 5; ++i) {
        pid = getParentPid(pid); // Iterate to pid of grandparent, then great grandparent and so on until a terminal emulator is found
        std::string pname = getProcessName(pid); // Get name of process to check if it is a terminal emulator

        if (pname.find("terminal") != std::string::npos ||
            pname.find("gnome") != std::string::npos ||
            pname.find("konsole") != std::string::npos ||
            pname.find("alacritty") != std::string::npos ||
            pname.find("xterm") != std::string::npos ||
            pname.find("kitty") != std::string::npos) {

            return exec((pname + " --version").c_str());
        }
    }

    return "Unknown";
}

// Return the desktop environment used by the user
std::string desktopEnv() {
    auto de = exec("echo $DESKTOP_SESSION");

    if (de == "plasma") {
        return "Plasma " + split(exec("plasmashell --version"), ' ').back();

    } else if (de == "gnome") {
        return "Gnome " + split(exec("gnome-shell --version"), ' ').back();

    } else if (de == "xfce") {
        return "Xfce " + split(exec("xfce4-session --version"), ' ').back();

    } else {
        return capitalize(de);

    }
}

// Chat-GPT made this (it's 1am rn i can't be bothered to do it myself)
std::vector<std::vector<int>> getDarkenedColors(const std::vector<int>& color) {
    // Sanity check: Must be an RGB triplet
    if (color.size() != 3) {
        throw std::invalid_argument("Color must have exactly 3 components (R, G, B).");
    }

    std::vector<std::vector<int>> darkenedColors;

    // Define darkening factors (from light to dark)
    std::vector<float> factors = {0.8f, 0.6f, 0.4f, 0.3f};

    for (float factor : factors) {
        std::vector<int> darkened;
        for (int c : color) {
            int dc = static_cast<int>(c * factor);
            darkened.push_back(std::clamp(dc, 0, 255));
        }
        darkenedColors.push_back(darkened);
    }

    return darkenedColors;
}

std::map<std::string, std::string> getData() {
    std::regex users_re("([0-9]+) user");
    std::regex cpus_re("CPU\\(s\\):\\s+([0-9]+)");
    std::regex cpu_model_re("Model name:\\s+(.*)");
    std::regex cpu_mhz_re("CPU max MHz:\\s+([0-9]+)");
    std::regex mem_re("Mem:\\s+(\\S+)\\s+(\\S+)");
    std::regex ip_re("[0-9]: ([^:]+)[\\s\\S]*?inet ([^\\/]+)");

    auto uname = split(exec("/usr/bin/uname -a"), ' ');
    auto os_release = parseToMap(exec("/usr/bin/cat /etc/os-release", false));
    auto uptime = exec("/usr/bin/uptime -p");
    auto uptime_raw = exec("/usr/bin/uptime");
    auto lscpu = exec("/usr/bin/lscpu", false);
    auto gpu = exec("if command -v nvidia-smi &>/dev/null; then nvidia-smi --query-gpu=name --format=csv,noheader,nounits; elif command -v lspci &>/dev/null; then lspci | grep -iE 'vga|3d' | awk -F \": \" '{print $2}'; else echo \"Unavailable\"; fi");
    auto free = exec("/usr/bin/free -h");
    auto ip_a = exec("/usr/bin/ip a");
    auto shell_prog = exec("/usr/bin/echo $SHELL");
    auto shell = split(split(exec((shell_prog + " --version").c_str()), '\n')[0], '(')[0];
    shell.erase(shell.size() - 1);
    auto system_age_raw = exec("OLDEST=$(find /var/log -type f -printf '%T@ \%p\\n' 2>/dev/null | sort -n | head -n1 | cut -d' ' -f1); NOW=$(date +\%s); AGE=$((NOW - ${OLDEST%.*})); Y=$((AGE/31536000)); D=$(((AGE%31536000)/86400)); H=$(((AGE%86400)/3600)); M=$(((AGE%3600)/60)); echo \"$Y,$D,$H,$M\"");
    auto cpu_temp = exec("CPU_TEMP=\"Unavailable\"; for z in /sys/class/thermal/thermal_zone*; do TYPE=$(cat \"$z/type\" 2>/dev/null); TEMP=$(cat \"$z/temp\" 2>/dev/null); if echo \"$TEMP\" | grep -qE '^[0-9]+$' && echo \"$TYPE\" | grep -iqE 'cpu|core|pkg|x86_pkg_temp|acpitz|coretemp|physical id'; then CPU_TEMP=\"$(awk -v t=\"$TEMP\" 'BEGIN { printf \"\%.1f °C\\n\", t / 1000 }')\"; break; fi; done; echo \"$CPU_TEMP\"; ");
    auto gpu_temp = exec("if command -v nvidia-smi >/dev/null; then nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader; else GPU_TEMP=\"Unavailable\"; for f in /sys/class/hwmon/hwmon*/temp1_input; do CHIP=$(cat \"$(dirname \"$f\")/name\" 2>/dev/null); if echo \"$CHIP\" | grep -iqE 'amdgpu|radeon|nvidia'; then TEMP=$(cat \"$f\" 2>/dev/null); GPU_TEMP=\"$(awk -v t=\"$TEMP\" 'BEGIN { printf \"\%.1f °C\\n\", t / 1000 }')\"; break; fi; done; echo \"$GPU_TEMP\"; fi");
    auto theme = exec("{ gsettings get org.gnome.desktop.interface gtk-theme 2>/dev/null || xfconf-query -c xsettings -p /Net/ThemeName 2>/dev/null || cat ~/.config/gtk-3.0/settings.ini 2>/dev/null | grep -i 'gtk-theme-name' | cut -d '=' -f2 || kwriteconfig5 --file kwinrc 2>/dev/null | grep 'theme=' | cut -d '=' -f2 || cat ~/.config/plasma-org.kde.plasma.desktop-appletsrc 2>/dev/null | grep -i 'theme' | cut -d '=' -f2 || echo \"No theme found\"; } | tr -d \\'");

    // Parse the approximate system age
    auto split_system_age = split(system_age_raw, ',');
    std::string sys_age;
    bool sys_age_start_found = false;
    std::vector<std::string> time_lookup = { "year", "day", "hour", "minute" };

    for (int i = 0; i < split_system_age.size(); i++) {
        if (!sys_age_start_found) {
            sys_age_start_found = split_system_age[i] == "0" ? false: true;
        }

        if (sys_age_start_found) {
            sys_age += std::format("{} {}, ", split_system_age[i], std::stoi(split_system_age[i]) == 1 ? time_lookup[i]: time_lookup[i] + "s");
        }
    }

    sys_age.erase(sys_age.size() - 2);

    // Get the amount of users that contributed to uptime
    std::smatch uptime_u_match;
    std::string uptime_u;
    if (std::regex_search(uptime_raw, uptime_u_match, users_re)) {
        if (uptime_u_match[1].matched) {
            uptime_u = uptime_u_match[1].str();
        }
    }

    // Get the amount of cpu cores
    std::smatch cpu_num_match;
    std::string cpu_num;
    if (std::regex_search(lscpu, cpu_num_match, cpus_re)) {
        if (cpu_num_match[1].matched) {
            cpu_num = cpu_num_match[1].str();
        }
    }

    // Get the cpu model name
    std::smatch cpu_model_match;
    std::string cpu_model;
    if (std::regex_search(lscpu, cpu_model_match, cpu_model_re)) {
        if (cpu_model_match[1].matched) {
            cpu_model = cpu_model_match[1].str();
        }
    }

    // Get the cpu max clock speed
    std::smatch cpu_ghz_match;
    std::string cpu_ghz;
    if (std::regex_search(lscpu, cpu_ghz_match, cpu_mhz_re)) {
        if (cpu_ghz_match[1].matched) {
            double ghz_value = std::stod(cpu_ghz_match[1].str()) / 1000.0;
            std::ostringstream oss;
            oss << ghz_value;
            cpu_ghz = oss.str();
        }
    }

    // Get the amount of used memory out of total memory
    std::smatch mem_match;
    std::string mem;
    if (std::regex_search(free, mem_match, mem_re)) {
        if (mem_match[1].matched && mem_match[2].matched) {
            mem = mem_match[2].str() + "/" + mem_match[1].str();
        }
    }

    // Get all the ip interfaces and their corresponding ip addresses
    std::smatch ip_match;
    std::string ip;

    std::sregex_iterator ip_iter(ip_a.begin(), ip_a.end(), ip_re);
    std::sregex_iterator end;

    while (ip_iter != end) { // Iterate over all regex matches
        ip += ip_iter->str(1) + ": " + ip_iter->str(2) + ", ";

        ++ip_iter;
    }

    if (!ip.empty()) {
        ip.erase(ip.size() - 2); // Remove the ", " from the end
    }

    // Get the package manager and number of installed packages
    std::string pkmg;
    if (commandExists("dpkg")) { pkmg = exec("dpkg -l | grep ^ii | wc -l") + " (dpkg)"; }
    else if (commandExists("rpm")) { pkmg = exec("rpm -qa | wc -l") + " (rpm)"; }
    else if (commandExists("pacman")) { pkmg = exec("pacman -Q | wc -l") + " (pacman)"; };

    // Construct the actual map that gets returned
    std::map<std::string, std::string> data = {
        {"kernel", uname[0] + " (" + uname[2] + ")"},
        {"hostname", uname[1]},
        {"architecture", uname[12]},
        {"pretty_name", os_release["PRETTY_NAME"]},
        {"uptime_users", uptime_u},
        {"uptime", uptime.erase(0, 3) + " (" + uptime_u + " " + (uptime_u == "1" ? "user": "users") + ")"},
        {"cpu_model", cpu_model},
        {"cpu_count", cpu_num},
        {"cpu_ghz", cpu_ghz},
        {"cpu", std::format("{} ({}) @ {}GHz", cpu_model, cpu_num, cpu_ghz)},
        {"gpu", gpu},
        {"used_mem", mem},
        {"ip", ip},
        {"username", exec("whoami")},
        {"pkmg", pkmg},
        {"shell", shell},
        {"terminal", terminalEmulator()},
        {"de", desktopEnv()},
        {"sys_age", sys_age},
        {"cpu_temp", cpu_temp},
        {"gpu_temp", gpu_temp},
        {"desktop_theme", theme}
    };

    return data;
}

int lc = 0;
int display_line(std::string name, std::string value, std::string icon, int max_name_width, std::vector<int> bc, std::vector<int> sc, std::string prefix) {
    auto br = std::to_string(bc[0]);
    auto bg = std::to_string(bc[1]);
    auto bb = std::to_string(bc[2]);

    auto sr = std::to_string(sc[0]);
    auto sg = std::to_string(sc[1]);
    auto sb = std::to_string(sc[2]);

    std::cout << std::format("{}\x1b[1m\x1b[38;2;{};{};{}m{}", prefix, br, bg, bb, icon)  // Icon with background color
              << " " 
              << name 
              << "\x1b[0m"  // Reset formatting after name
              << std::string(max_name_width - name.size(), ' ')  // Padding the name to max width
              << std::format(" \x1b[38;5;242m>\x1b[0m  \x1b[38;2;{};{};{}m{}", sr, sg, sb, value) // Value with text color
              << "\x1b[0m"  // Reset formatting after value
              << std::endl;  // End the line

    lc++;

    return 0;
}

int display_top(std::string hostname, std::string username, std::vector<int> bc, std::vector<int> sc, std::string prefix) {
    auto br = std::to_string(bc[0]);
    auto bg = std::to_string(bc[1]);
    auto bb = std::to_string(bc[2]);

    auto sr = std::to_string(sc[0]);
    auto sg = std::to_string(sc[1]);
    auto sb = std::to_string(sc[2]);

    auto bdark = getDarkenedColors(bc);
    auto sdark = getDarkenedColors(sc);

    std::cout << prefix
              << std::format("\x1b[48;2;{};{};{}m   ", bdark[3][0], bdark[3][1], bdark[3][2])
              << std::format("\x1b[48;2;{};{};{}m  ", bdark[2][0], bdark[2][1], bdark[2][2])
              << std::format("\x1b[48;2;{};{};{}m \u2009", bdark[1][0], bdark[1][1], bdark[1][2])
              << std::format("\x1b[48;2;{};{};{}m ", bdark[0][0], bdark[0][1], bdark[0][2])
              << "\x1b[0m "
              << std::format("\x1b[38;2;{};{};{}m{}", br, bg, bb, capitalize(username))
              << "\x1b[38;5;242m @ " << std::format("\x1b[38;2;{};{};{}m{} ", sr, sg, sb, capitalize(hostname))
              << std::format("\x1b[48;2;{};{};{}m ", sdark[0][0], sdark[0][1], sdark[0][2])
              << std::format("\x1b[48;2;{};{};{}m \u2009", sdark[1][0], sdark[1][1], sdark[1][2])
              << std::format("\x1b[48;2;{};{};{}m  ", sdark[2][0], sdark[2][1], sdark[2][2])
              << std::format("\x1b[48;2;{};{};{}m   ", sdark[3][0], sdark[3][1], sdark[3][2])
              << "\x1b[0m"
              << std::endl;

    lc += 2;

    return 0;
}

void usage(std::string prog) {
    std::cout << "Usage:" << std::endl;
    std::cout << "  " << prog << " [options] (run '" << prog << " -h/--help' for more information)" << std::endl;
}

std::string getFlagOption(int argc, char *argv[], std::string flag_short, std::string flag_long) {
    if (flag_long.empty()) {
        flag_long = flag_short;
    }
    if (flag_short.empty()) {
        flag_short = flag_long;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], flag_short.c_str()) == 0 || strcmp(argv[i], flag_long.c_str()) == 0) {
            if (i + 1 < argc) {
                return argv[i + 1];

            }
            else {
                usage(argv[0]);
                std::cerr << "Invalid argument: '" << argv[i] << "' requires a value." << std::endl;

                throw std::invalid_argument(std::format("No value for flag argument '{}'.", argv[i]).c_str());
            }
        }
    }

    return "";
}

bool existsFlagOption(int argc, char *argv[], std::string flag_short, std::string flag_long) {
    if (flag_long.empty()) {
        flag_long = flag_short;
    }
    if (flag_short.empty()) {
        flag_short = flag_long;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], flag_short.c_str()) == 0 || strcmp(argv[i], flag_long.c_str()) == 0) {
            return true;
        }
    }

    return false;
}

std::string getPositionalOption(int argc, char *argv[], int relative_position, std::string name) {
    int pos = 0;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            i++;
        }
        else {
            if (relative_position == pos) {
                return argv[i];
            }

            pos++;
        }
    }

    usage(argv[0]);
    std::cerr << "Invalid argument: Missing argument '" << name << "'." << std::endl;

    throw std::invalid_argument(std::format("Missing positional argument '{}'.", name).c_str());
}

void help(std::string prog, std::vector<std::pair<std::string, std::string>> options) {
    int len = 0;
    for (int i = 0; i < options.size(); i++) {
        if (options[i].first.size() > len) {
            len = options[i].first.size();
        }
    }

    std::cout << prog << " - Display system info.";

    usage(prog);

    std::cout << "\nOptions:" << std::endl;
    for (int i = 0; i < options.size(); i++) {
        std::cout << "  " << options[i].first << std::string(len - options[i].first.size() + 4, ' ') << "\x1b[38;5;245m" << options[i].second << "\x1b[0m" << std::endl;
    }
}

char32_t decodeUnicodeEscape(const std::string& str) {
    if (str.size() != 6 || str[0] != '\\' || str[1] != 'u') {
        throw std::invalid_argument("Invalid Unicode escape sequence");
    }

    uint32_t value = 0;
    std::istringstream iss(str.substr(2));
    iss >> std::hex >> value;

    return static_cast<char32_t>(value);
}

std::string char32_t_unicode_codepoint_to_utf_8(char32_t codepoint) {
    std::string result;

    if (codepoint <= 0x7F) {
        result.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        result.push_back(static_cast<char>((codepoint >> 6) | 0xC0));
        result.push_back(static_cast<char>((codepoint & 0x3F) | 0x80));
    } else if (codepoint <= 0xFFFF) {
        result.push_back(static_cast<char>((codepoint >> 12) | 0xE0));
        result.push_back(static_cast<char>(((codepoint >> 6) & 0x3F) | 0x80));
        result.push_back(static_cast<char>((codepoint & 0x3F) | 0x80));
    } else if (codepoint <= 0x10FFFF) {
        result.push_back(static_cast<char>((codepoint >> 18) | 0xF0));
        result.push_back(static_cast<char>(((codepoint >> 12) & 0x3F) | 0x80));
        result.push_back(static_cast<char>(((codepoint >> 6) & 0x3F) | 0x80));
        result.push_back(static_cast<char>((codepoint & 0x3F) | 0x80));
    } else {
        throw std::invalid_argument("Invalid Unicode codepoint");
    }

    return result;
}

void cfgError(int lnum, std::string line, std::string message) {
    std::cerr << std::format("Config formatting error at line {}: '{}'", lnum, line) << std::endl;
    std::cerr << " -> " << message << std::endl;
}

std::vector<int> hexToRgb(unsigned long hex) {
    std::vector<int> rgb(3);

    rgb[0] = (hex >> 16) & 0xFF;
    rgb[1] = (hex >> 8) & 0xFF;
    rgb[2] = hex & 0xFF;

    return rgb;
}

std::string parsePath(const std::string& path) {
    if (path[0] == '~') {
        const char* home = std::getenv("HOME");
        if (home) {
            return std::string(home) + path.substr(1);
        }
    }

    return path;
}

int main(int argc, char *argv[]) {
    bool off = false;
    std::string config;
    std::string gen_config;

    try {
        if (existsFlagOption(argc, argv, "-h", "--help")) {
            std::vector<std::pair<std::string, std::string>> opts = {
                { "-h, --help", "Show this message and exit." },
                { "-v, --version", "Show version information and exit." },
                { "    --off", "Hide the ascii art on the side." },
                { "    --config <path>", "Pass a configuration file."},
                { "    --gen-config <path>", "Generate a default config file."}
            };

            help(argv[0], opts);

            return 0;
        }

        if (existsFlagOption(argc, argv, "-v", "--version")) {
            std::cout << argv[0] << ": v" << VERSION << std::endl;

            return 0;
        }

        off = existsFlagOption(argc, argv, "", "--off");

        config = getFlagOption(argc, argv, "", "--config");
        config = parsePath(config);

        gen_config = getFlagOption(argc, argv, "", "--gen-config");
        gen_config = parsePath(gen_config);
        if (!gen_config.empty()) { off = true; }
    }
    catch (const std::invalid_argument& e) {
        return 1;
    }

    auto data = getData();

    auto imgFP = exec("find ascinfo.sideimage");
    std::string height;
    std::string width;

    std::vector<int> bc = {0, 153, 255};
    std::vector<int> sc = {255, 204, 255};

    std::vector<std::pair<std::string, std::vector<std::vector<std::string>>>> default_cfg = {
            {"SYSTEM", {
                        {"OS", "pretty_name", "\\uf31a"},
                        {"Kernel", "kernel", "\\uf305"},
                        {"Arch.", "architecture", "\\ue225"},
                        {"IP", "ip", "\\ueb1c"}
                       }
            },
            {"DESKTOP", {
                        {"DE", "de", "\\uf4a9"},
                        {"Packages", "pkmg", "\\uf487"},
                        {"Shell", "shell", "\\uebca"},
                        {"Terminal", "terminal", "\\uf489"},
                        {"Theme", "desktop_theme", "\\uf4a9"}
                       }
            },
            {"HARDWARE", {
                       {"CPU", "cpu", "\\ue266"},
                       {"C. Temp", "cpu_temp", "\\ueaf2"},
                       {"GPU", "gpu", "\\ue266"},
                       {"G. Temp", "gpu_temp", "\\ueaf2"},
                       {"RAM", "used_mem", "󰄩"},
                       }
            },
            {"USAGE", {
                      {"Up", "uptime", "\\ue641"},
                      {"Sys. Age", "sys_age", "\\uea98"}
                      }
            }
    };

    if (!gen_config.empty()) {
        std::ofstream clearFile(gen_config, std::ios::trunc);
        if (!clearFile) {
            std::cerr << "Error clearing file\n";
            return 1;
        }
        clearFile.close();

        std::ofstream file(gen_config, std::ios::app);

        file << "sideimage:ascinfo.sideimage" << "\n";
        file << std::format("base_color:{},{},{}", bc[0], bc[1], bc[2]) << "\n";
        file << std::format("secondary_color:{},{},{}", sc[0], sc[1], sc[2]) << "\n";
        file << "\n";

        for (int i = 0; i < default_cfg.size(); i++) {
            file << "[" << default_cfg[i].first << "]" << "\n";

            for (int j = 0; j < default_cfg[i].second.size(); j++) {
                file << default_cfg[i].second[j][0] << ","
                     << default_cfg[i].second[j][1] << ","
                     << default_cfg[i].second[j][2] << "\n";
            }

            file << "\n";
        }

        file.close();

        std::cout << "Generated config file at '" << gen_config << "'." << std::endl;

        return 0;
    }

    std::vector<std::pair<std::string, std::vector<std::vector<std::string>>>> lines;

    if (config.empty()) {
        lines = default_cfg;
    }
    else {
        std::fstream file(config);
        if (!file.is_open()) {
            std::cerr << "Config error: File '" << config << "' does not exist." << std::endl;
            return 1;
        }

        std::string line;

        std::vector<std::vector<std::string>> section = {};
        std::string sec_name = "";

        int lnum = 0;
        while (std::getline(file, line)) {
            auto ln = strip(line);
            std::string sideimage = "sideimage:";
            std::string base_color = "base_color:";
            std::string secondary_color = "secondary_color:";

            if (!ln.empty() && ln[0] != '#') {
                if (ln.rfind(sideimage, 0) == 0) {
                    size_t pos = ln.rfind(sideimage);

                    imgFP = ln.substr(pos + sideimage.size());
                }

                else if (ln.rfind(base_color, 0) == 0) {
                    size_t pos = ln.rfind(base_color);
                    std::string bc_subs = ln.substr(pos + base_color.size());

                    if (bc_subs[0] == '#' && bc_subs.size() > 1) {
                        bc = hexToRgb(std::stoul(bc_subs.erase(0, 1), nullptr, 16));
                    }
                    else {
                        try {
                            auto bc_r = split(replace(bc_subs, ", ", ","), ',');

                            bc = { std::stoi(bc_r[0]), std::stoi(bc_r[1]), std::stoi(bc_r[2]) };
                        } catch (std::invalid_argument& e) {}
                    }
                }

                else if (ln.rfind(secondary_color, 0) == 0) {
                    size_t pos = ln.rfind(secondary_color);
                    std::string sc_subs = ln.substr(pos + secondary_color.size());

                    if (sc_subs[0] == '#' && sc_subs.size() > 1) {
                        sc = hexToRgb(std::stoul(sc_subs.erase(0, 1), nullptr, 16));
                    }
                    else {
                        try {
                            auto sc_r = split(replace(sc_subs, ", ", ","), ',');

                            sc = { std::stoi(sc_r[0]), std::stoi(sc_r[1]), std::stoi(sc_r[2]) };
                        } catch (std::invalid_argument& e){}
                    }

                }

                else if (ln[0] == '[' && ln[ln.size() - 1] == ']') {
                    if (!sec_name.empty()) {
                        lines.push_back({ sec_name, section });
                    }

                    sec_name = ln.substr(1, ln.size() - 2);
                    section = {};
                }

                else {
                    if (sec_name.empty()) {
                        cfgError(lnum, line, std::format("Expected section header ('[HEADER]'), got '{}'.", line));
                        return 1;
                    }

                    auto ln_s = split(replace(ln, ", ", ","), ',');

                    if (ln_s.size() != 3) {
                        cfgError(lnum, line, std::format("Line expects 3 arguments ('arg1,arg2,arg3'), got '{}'.", ln_s.size()));
                        return 1;
                    }

                    section.push_back({ ln_s[0], ln_s[1], ln_s[2] });
                }
            }

            lnum++;
        }

        lines.push_back({ sec_name, section });
    }

    if (!off) {
        if (!imgFP.empty()) {
            height = exec(("/usr/bin/cat " + imgFP + " | wc -l").c_str());
            width = exec(("/usr/bin/cat " + imgFP + " | head -n1 | sed 's/\x1b\[[0-9;]*m//g' | wc -m").c_str());
            system(("/usr/bin/cat " + imgFP).c_str());
            std::cout << std::format("\x1b[{}A", height);

        }
        else if (commandExists("neofetch")) {
            system("neofetch -L");
            std::cout << "\x1b[21A";
            width = "40";
            height = "20";
        }
        else {
            off = true;
        }
    }

    std::string prefix = off ? "": std::format("\x1b[{}C     ", width);

    int w = 0;

    for (int i = 0; i < lines.size(); i++){
        for (int j = 0; j < lines[i].second.size(); j++) {
            auto ln = lines[i].second[j][0];

            if (ln.size() + 1 > w) {
                w = ln.size() + 1;
            }
        }
    }

    display_top(data["hostname"], data["username"], bc, sc, prefix);

    for (int i = 0; i < lines.size(); i++){
        std::cout << "\n" << prefix << std::format("\x1b[38;5;255m\x1b[1m{}\x1b[0m", lines[i].first) << std::endl;
        lc += 2;

        for (int j = 0; j < lines[i].second.size(); j++) {
            auto ln = lines[i].second[j];

            std::string u = ln[2];
            try {
                u = char32_t_unicode_codepoint_to_utf_8(decodeUnicodeEscape(ln[2]));
            } catch (std::invalid_argument& e) {}

            display_line(ln[0], data[ln[1]], u, w, bc, sc, prefix);
        }

    }

    if (!off) {
        if (lc < std::stoi(height)) {
            std::cout << std::string(std::stoi(height) - lc, '\n');
        }
    }

    return 0;
}
