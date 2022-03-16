#pragma once

#include <string>
#include <regex>
#include <type_traits>
#include <chrono>

using dseconds = std::chrono::duration<double>;
using dminutes = std::chrono::duration<double, std::ratio<60>>;
using dhours = std::chrono::duration<double, std::ratio<3600>>;
using ddays = std::chrono::duration<double, std::ratio<86400>>;
using dweeks = std::chrono::duration<double, std::ratio<604800>>;
using dmonths = std::chrono::duration<double, std::ratio<2629746>>;
using dyears = std::chrono::duration<double, std::ratio<31556952>>;

enum class TfClass {
    Scout = 0,
    Soldier = 1,
    Pyro = 2,
    Demoman = 3,
    Heavy = 4,
    Engineer = 5,
    Medic = 6,
    Sniper = 7,
    Spy = 8
};

enum class StatType {
    accum = 0,
    max = 1
};

enum class GameType {
    pvp = 0,
    coop = 1 // mvm
};

const std::map<std::string, std::string> gamemodes = {
        {"arena", "Arena"},
        {"cp", "Capture Point"},
        {"ctf", "Capture the Flag"},
        {"koth", "King of the Hill"},
        {"pl", "Payload"},
        {"plr", "Payload Race"},
        {"sd", "Special Delivery"}
};

class ClassStat {
public:
    static inline std::regex regPvp = std::regex("(Class|Scout|Soldier|Pyro|Demoman|Heavy|Engineer|Medic|Sniper|Spy)\\.(accum|max)\\.i([a-zA-Z]+)", std::regex::optimize);
    static inline std::regex regMvm = std::regex("(Class|Scout|Soldier|Pyro|Demoman|Heavy|Engineer|Medic|Sniper|Spy)\\.mvm\\.(accum|max)\\.i([a-zA-Z]+)", std::regex::optimize);
    std::string fullName;
    std::string className;
    std::string shortName;
    std::string description;
    int value;
    GameType gameType;
    StatType statType;
    TfClass tfClass;
};

class MapStat {
public:
    static inline std::regex reg = std::regex("((arena|cp|ctf|koth|pl|plr|sd)_[a-zA-Z0-9_]+)\\.accum\\.iPlayTime", std::regex::optimize);
    std::string name;
    std::string gamemode;
    int playTime;
};

class AchievementStat {
public:
    static inline std::regex reg = std::regex("TF_.*_STAT", std::regex::optimize);
    std::string name;
    std::string description;
    int value;
};

class PlayerStats {
public:
    std::vector<ClassStat> pvpStats;
    std::vector<ClassStat> mvmStats;
    std::vector<MapStat> mapStats;
    std::vector<AchievementStat> achievementStats;
};

std::string ConvertMSToHHMMSSMS(std::chrono::milliseconds ms)
{
    using namespace std::chrono;
    std::stringstream ss;

    // compute h, m, s, ms
    auto secs = duration_cast<seconds>(ms);
    ms -= duration_cast<milliseconds>(secs);

    auto mins = duration_cast<minutes>(secs);
    secs -= duration_cast<seconds>(mins);

    auto hour = duration_cast<hours>(mins);
    mins -= duration_cast<minutes>(hour);

    std::string hr(std::to_string(hour.count()));
    std::string min(std::to_string(mins.count()));
    std::string s(std::to_string(secs.count()));
    std::string msecs(std::to_string(ms.count()));

    msecs = msecs.substr(0, 2);

    // add leading zero if needed
    std::string hh = std::string(4 - hr.length(), '0') + hr;
    std::string mm = std::string(2 - min.length(), '0') + min;
    std::string sec = std::string(2 - s.length(), '0') + s;
    std::string millisec = std::string(2 - msecs.length(), '0') + msecs;

    // return mm:ss if hh is 0000
    if (hh.compare("0000") != 0)
    {
        ss << hr << ":" << mm << ":" << sec << "." << millisec;
    }
    else
    {
        ss << mm << ":" << sec << "." << millisec;
    }

    return ss.str();
}