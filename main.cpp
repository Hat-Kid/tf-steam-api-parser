#include <regex>
#include <string>

#include <curl/curl.h>
#include <Poco/JSON/Parser.h>
#include <inja/inja.hpp>

#include "main.h"
#include "data_classes.h"

using namespace std;

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real_size = size * nmemb;
    auto mem = (struct MemoryStruct *) userp;

    char *ptr = static_cast<char *>(realloc(mem->memory, mem->size + real_size + 1));
    if (!ptr) {
        printf("not enough memory (realloc returned nullptr)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, real_size);
    mem->size += real_size;
    mem->memory[mem->size] = 0;

    return real_size;
}

map<string, string> FetchDescriptions() {
    map<string, string> descriptions;
    ifstream stream("stat_names.json");
    stringstream buf;
    Poco::JSON::Parser parser;
    Poco::Dynamic::Var parseResult;
    string json;

    if (stream) {
        buf << stream.rdbuf();
        json = buf.str();
        parseResult = parser.parse(json);
        auto stats = parseResult.extract<Poco::JSON::Object::Ptr>()->getArray("stats");

        for (int i = 0; i < stats->size(); i++) {
            auto stat = stats->getObject(i);
            auto name = stat->getValue<string>("name");
            auto desc = stat->getValue<string>("description");
            descriptions.insert({name, desc});
        }
        return descriptions;
    } else {
        cout << "Error: File stat_names.json was not found in root dir!" << endl;
        return {};
    }
}

template<class T>
string getDescriptionForStat(const map<string, string> &desc, T stat) {
    if constexpr (std::is_same_v<T, ClassStat>) {
        auto m = stat.fullName;
        auto c = stat.className;
        FindAndReplaceAll(m, c, "Class");
        auto reg = regex(m, regex::optimize);
        if (stat.gameType == GameType::pvp) {
            for (auto &it: desc) {
                if (regex_match(it.first, reg)) {
                    auto result = it.second;
                    FindAndReplaceAll(result, "Class", c);
                    return result;
                }
            }
        } else {
            for (auto &it: desc) {
                if (regex_match(it.first, reg)) {
                    auto result = it.second;
                    FindAndReplaceAll(result, "Class", c);
                    return result;
                }
            }
        }
    } else if constexpr(std::is_same_v<T, AchievementStat>) {
        auto m = stat.name;
        for (auto &it: desc) {
            if (m == it.first) {
                return it.second;
            }
        }
    }
    return "null";
};

PlayerStats FetchResults(const string &apiUrl) {
    PlayerStats playerStats;
    ostringstream jsonStream;
    Poco::JSON::Parser parser;
    Poco::Dynamic::Var parseResult;
    string resultStr;
    cmatch match;
    MemoryStruct data{};

    data.memory = static_cast<char *>(malloc(1));
    data.size = 0;

    CURLcode code;
    CURL *conn;
    conn = curl_easy_init();
    if (!conn) {
        fprintf(stderr, "Error: Failed to create CURL connection!\n");
        exit(EXIT_FAILURE);
    }

    code = curl_easy_setopt(conn, CURLOPT_URL, apiUrl.c_str());
    curl_easy_setopt(conn, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(conn, CURLOPT_WRITEDATA, (void *) &data);
    curl_easy_setopt(conn, CURLOPT_USERAGENT, "libcurl-agent/1.0");
#ifdef _WIN32
    curl_easy_setopt(conn, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(conn, CURLOPT_SSL_VERIFYHOST, 0);
#endif
    code = curl_easy_perform(conn);
    if (code != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(code));
    } else {
        printf("%lu bytes retrieved from request\n\n", (unsigned long) data.size);
    }

    map<string, string> descriptions = FetchDescriptions();
    jsonStream << data.memory;
    resultStr = jsonStream.str();
    parseResult = parser.parse(resultStr);
    auto stats = parseResult.extract<Poco::JSON::Object::Ptr>()->getObject("playerstats")->getArray("stats");

    for (int i = 0; i < stats->size(); i++) {
        auto stat = stats->getObject(i);
        if (stat->has("name")) {
            auto statName = stat->getValue<string>("name");
            cout << "Stat: " << statName << endl;
            if (regex_match(statName.c_str(), match, ClassStat::regPvp)) {
                ClassStat classStat;
                classStat.fullName = match.str(0);
                classStat.className = match.str(1);
                classStat.tfClass = GetEnumFromClassString(classStat.className);
                classStat.statType = match.str(2) == "accum" ? StatType::accum : StatType::max;
                classStat.shortName = match.str(3);
                classStat.gameType = GameType::pvp;
                classStat.value = stat->getValue<int>("value");
                classStat.description = getDescriptionForStat(descriptions, classStat);
                cout << "Got " << classStat.className << " " << classStat.shortName << " pvp stat with value "
                     << classStat.value << endl;
                cout << "Description: " << classStat.description << endl << endl;
                playerStats.pvpStats.push_back(classStat);
            } else if (std::regex_match(statName.c_str(), match, ClassStat::regMvm)) {
                ClassStat classStat;
                classStat.fullName = match.str(0);
                classStat.className = match.str(1);
                classStat.tfClass = GetEnumFromClassString(classStat.className);
                classStat.statType = match.str(2) == "accum" ? StatType::accum : StatType::max;
                classStat.shortName = match.str(3);
                classStat.gameType = GameType::coop;
                classStat.value = stat->getValue<int>("value");
                classStat.description = getDescriptionForStat(descriptions, classStat);
                cout << "Got " << classStat.className << " " << classStat.shortName << " mvm stat with value "
                     << classStat.value << endl;
                cout << "Description: " << classStat.description << endl << endl;
                playerStats.mvmStats.push_back(classStat);
            } else if (std::regex_match(statName.c_str(), match, MapStat::reg)) {
                MapStat mapStat;
                mapStat.name = match.str(1);
                mapStat.gamemode = match.str(2);
                mapStat.playTime = stat->getValue<int>("value");
                cout << "Got " << mapStat.name << " stat for gamemode " << mapStat.gamemode << " with time "
                     << mapStat.playTime << endl << endl;
                playerStats.mapStats.push_back(mapStat);
            } else if (std::regex_match(statName.c_str(), match, AchievementStat::reg)) {
                AchievementStat achievementStat;
                achievementStat.name = match.str(0);
                achievementStat.value = stat->getValue<int>("value");
                achievementStat.description = getDescriptionForStat(descriptions, achievementStat);
                cout << "Got " << achievementStat.name << " achievement stat with value " << achievementStat.value
                     << endl;
                cout << "Description: " << achievementStat.description << endl << endl;
                playerStats.achievementStats.push_back(achievementStat);
            }
        }
    }
    curl_global_cleanup();
    return playerStats;
}

string getPersonaName(const string &apiUrl) {
    CURLcode code;
    CURL *conn;
    ostringstream jsonStream;
    Poco::JSON::Parser parser;
    Poco::Dynamic::Var parseResult;
    string resultStr;
    MemoryStruct data{};
    data.memory = static_cast<char *>(malloc(1));
    data.size = 0;
    conn = curl_easy_init();
    if (!conn) {
        fprintf(stderr, "Error: Failed to create CURL connection!\n");
        exit(EXIT_FAILURE);
    }

    code = curl_easy_setopt(conn, CURLOPT_URL, apiUrl.c_str());
    curl_easy_setopt(conn, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(conn, CURLOPT_WRITEDATA, (void *) &data);
    curl_easy_setopt(conn, CURLOPT_USERAGENT, "libcurl-agent/1.0");
#ifdef _WIN32
    curl_easy_setopt(conn, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(conn, CURLOPT_SSL_VERIFYHOST, 0);
#endif
    code = curl_easy_perform(conn);
    if (code != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(code));
        return "User";
    } else {
        printf("%lu bytes retrieved from request\n\n", (unsigned long) data.size);
    }
    jsonStream << data.memory;
    resultStr = jsonStream.str();
    parseResult = parser.parse(resultStr);

    return parseResult.extract<Poco::JSON::Object::Ptr>()->getObject("response")->getArray("players")->getObject(
            0)->getValue<string>("personaname");
}

void ParseStats(const PlayerStats &stats, const string &user) {
    using namespace inja;

    json pvpData;
    json mvmData;
    json mapData;
    json achData;
    Environment env{"templates/"};
    Template pvpClassStatTemp = env.parse_template("class_stats_pvp.md");
    Template mvmClassStatTemp = env.parse_template("class_stats_mvm.md");
    Template mapStatTemp = env.parse_template("map_stats.md");
    Template achievementStatTemp = env.parse_template("achievement_stats.md");
    stringstream result;
    result << "## TF2 Statistics for " << user << "\n\n---\n";

    vector<ClassStat> pvpStats = stats.pvpStats;
    vector<ClassStat> mvmStats = stats.mvmStats;
    vector<MapStat> mapStats = stats.mapStats;
    vector<AchievementStat> achievementStats = stats.achievementStats;

    sort(pvpStats.begin(), pvpStats.end(), [](const ClassStat &stat, const ClassStat &stat1) {
        return (stat.tfClass < stat1.tfClass);
    });
    sort(mvmStats.begin(), mvmStats.end(), [](const ClassStat &stat, const ClassStat &stat1) {
        return (stat.tfClass < stat1.tfClass);
    });

    // this is incredibly ugly
    vector<string> classes = {"Scout", "Soldier", "Pyro", "Demoman", "Heavy", "Engineer", "Medic", "Sniper", "Spy"};

    long scoutPvpCount = count_if(pvpStats.begin(), pvpStats.end(),
                                  [&classes](const ClassStat &stat) { return stat.className == classes.at(0); });
    long soldierPvpCount = count_if(pvpStats.begin(), pvpStats.end(),
                                    [&classes](const ClassStat &stat) { return stat.className == classes.at(1); });
    long pyroPvpCount = count_if(pvpStats.begin(), pvpStats.end(),
                                 [&classes](const ClassStat &stat) { return stat.className == classes.at(2); });
    long demoPvpCount = count_if(pvpStats.begin(), pvpStats.end(),
                                 [&classes](const ClassStat &stat) { return stat.className == classes.at(3); });
    long heavyPvpCount = count_if(pvpStats.begin(), pvpStats.end(),
                                  [&classes](const ClassStat &stat) { return stat.className == classes.at(4); });
    long engiePvpCount = count_if(pvpStats.begin(), pvpStats.end(),
                                  [&classes](const ClassStat &stat) { return stat.className == classes.at(5); });
    long medicPvpCount = count_if(pvpStats.begin(), pvpStats.end(),
                                  [&classes](const ClassStat &stat) { return stat.className == classes.at(6); });
    long sniperPvpCount = count_if(pvpStats.begin(), pvpStats.end(),
                                   [&classes](const ClassStat &stat) { return stat.className == classes.at(7); });

    long scoutMvmCount = count_if(mvmStats.begin(), mvmStats.end(),
                                  [&classes](const ClassStat &stat) { return stat.className == classes.at(0); });
    long soldierMvmCount = count_if(mvmStats.begin(), mvmStats.end(),
                                    [&classes](const ClassStat &stat) { return stat.className == classes.at(1); });
    long pyroMvmCount = count_if(mvmStats.begin(), mvmStats.end(),
                                 [&classes](const ClassStat &stat) { return stat.className == classes.at(2); });
    long demoMvmCount = count_if(mvmStats.begin(), mvmStats.end(),
                                 [&classes](const ClassStat &stat) { return stat.className == classes.at(3); });
    long heavyMvmCount = count_if(mvmStats.begin(), mvmStats.end(),
                                  [&classes](const ClassStat &stat) { return stat.className == classes.at(4); });
    long engieMvmCount = count_if(mvmStats.begin(), mvmStats.end(),
                                  [&classes](const ClassStat &stat) { return stat.className == classes.at(5); });
    long medicMvmCount = count_if(mvmStats.begin(), mvmStats.end(),
                                  [&classes](const ClassStat &stat) { return stat.className == classes.at(6); });
    long sniperMvmCount = count_if(mvmStats.begin(), mvmStats.end(),
                                   [&classes](const ClassStat &stat) { return stat.className == classes.at(7); });

    int i = 0;

    result << "### PvP\n\n";

    for (auto &pvpstat: pvpStats) {
        pvpData["pvpClass"] = pvpstat.className;
        pvpData["pvpClassStatDescription"] = pvpstat.description;
        if (pvpstat.shortName == "PlayTime") {
            pvpData["pvpClassStatValue"] = ConvertMSToHHMMSS(
                    duration_cast<chrono::milliseconds>(dseconds(pvpstat.value)));
        } else {
            pvpData["pvpClassStatValue"] = pvpstat.value;
        }

        cout << "Parsing " << pvpstat.fullName << "..." << endl;

        if (
                (i == 0) ||
                (pvpstat.className == "Soldier" && i == scoutPvpCount) ||
                (pvpstat.className == "Pyro" && i == (scoutPvpCount + soldierPvpCount)) ||
                (pvpstat.className == "Demoman" && i == (scoutPvpCount + soldierPvpCount + pyroPvpCount)) ||
                (pvpstat.className == "Heavy" &&
                 i == (scoutPvpCount + soldierPvpCount + pyroPvpCount + demoPvpCount)) ||
                (pvpstat.className == "Engineer" &&
                 i == (scoutPvpCount + soldierPvpCount + pyroPvpCount + demoPvpCount + heavyPvpCount)) ||
                (pvpstat.className == "Medic" && i == (scoutPvpCount + soldierPvpCount + pyroPvpCount + demoPvpCount +
                                                       heavyPvpCount + engiePvpCount)) ||
                (pvpstat.className == "Sniper" && i == (scoutPvpCount + soldierPvpCount + pyroPvpCount + demoPvpCount +
                                                        heavyPvpCount + engiePvpCount + medicPvpCount)) ||
                (pvpstat.className == "Spy" && i == (scoutPvpCount + soldierPvpCount + pyroPvpCount + demoPvpCount +
                                                     heavyPvpCount + engiePvpCount + medicPvpCount + sniperPvpCount))) {
            result << env.render("- {{ pvpClass }}", pvpData) << "\n";
        }
        result << env.render(pvpClassStatTemp, pvpData) << "\n";
        i++;
    }

    int j = 0;

    result << "---\n\n## MvM\n\n";

    for (auto &mvmstat: mvmStats) {
        mvmData["mvmClass"] = mvmstat.className;
        mvmData["mvmClassStatDescription"] = mvmstat.description;
        if (mvmstat.shortName == "PlayTime") {
            mvmData["mvmClassStatValue"] = ConvertMSToHHMMSS(
                    duration_cast<chrono::milliseconds>(dseconds(mvmstat.value)));
        } else {
            mvmData["mvmClassStatValue"] = mvmstat.value;
        }

        cout << "Parsing " << mvmstat.fullName << "..." << endl;

        if (
                (j == 0) ||
                (mvmstat.className == "Soldier" && j == scoutMvmCount) ||
                (mvmstat.className == "Pyro" && j == (scoutMvmCount + soldierMvmCount)) ||
                (mvmstat.className == "Demoman" && j == (scoutMvmCount + soldierMvmCount + pyroMvmCount)) ||
                (mvmstat.className == "Heavy" &&
                 j == (scoutMvmCount + soldierMvmCount + pyroMvmCount + demoMvmCount)) ||
                (mvmstat.className == "Engineer" &&
                 j == (scoutMvmCount + soldierMvmCount + pyroMvmCount + demoMvmCount + heavyMvmCount)) ||
                (mvmstat.className == "Medic" && j == (scoutMvmCount + soldierMvmCount + pyroMvmCount + demoMvmCount +
                                                       heavyMvmCount + engieMvmCount)) ||
                (mvmstat.className == "Sniper" && j == (scoutMvmCount + soldierMvmCount + pyroMvmCount + demoMvmCount +
                                                        heavyMvmCount + engieMvmCount + medicMvmCount)) ||
                (mvmstat.className == "Spy" && j == (scoutMvmCount + soldierMvmCount + pyroMvmCount + demoMvmCount +
                                                     heavyMvmCount + engieMvmCount + medicMvmCount + sniperMvmCount))) {
            result << env.render("- {{ mvmClass }}", mvmData) << "\n";
        }

        result << env.render(mvmClassStatTemp, mvmData) << "\n";
        j++;
    }

    result << "---\n\n## Maps\n\n";

    for (auto &mapstat: mapStats) {
        mapData["mapName"] = mapstat.name;
        mapData["playTime"] = ConvertMSToHHMMSS(duration_cast<chrono::milliseconds>(dseconds(mapstat.playTime)));

        cout << "Parsing " << mapstat.name << "..." << endl;
        result << env.render(mapStatTemp, mapData) << "\n";
    }

    result << "---\n\n## Achievements\n\n";

    for (auto &achievementstat: achievementStats) {
        achData["achievementStatDescription"] = achievementstat.description;
        achData["achievementStatValue"] = achievementstat.value;

        cout << "Parsing " << achievementstat.name << "..." << endl;
        result << env.render(achievementStatTemp, achData) << "\n";
    }

    string file("stats.md");
    ofstream out(file);
    out << result.rdbuf();
    out.close();
}

int main(int argc, char **argv) {
    string apiUrl = "https://api.steampowered.com/ISteamUserStats/GetUserStatsForGame/v0002/?appid=440&key=apikey&steamid=id64";
    string playerUrl = "https://api.steampowered.com/ISteamUser/GetPlayerSummaries/v2/?key=apikey&format=json&steamids=id64";
    uint64_t steamId;
    if ((!argv[1] && !argv[2]) || (!argv[1] && argv[2]) || (argv[1] && !argv[2])) {
        string apiKey;
        cout << "Enter the user's SteamID64: ";
        cin >> steamId;
        FindAndReplaceAll(apiUrl, "id64", to_string(steamId));
        FindAndReplaceAll(playerUrl, "id64", to_string(steamId));
        cout << "Enter your Steam API key: ";
        cin >> apiKey;
        FindAndReplaceAll(apiUrl, "apikey", apiKey);
        FindAndReplaceAll(playerUrl, "apikey", apiKey);
    } else {
        steamId = stoull(argv[1]);
        FindAndReplaceAll(apiUrl, "id64", argv[1]);
        FindAndReplaceAll(apiUrl, "apikey", argv[2]);
        FindAndReplaceAll(playerUrl, "id64", argv[1]);
        FindAndReplaceAll(playerUrl, "apikey", argv[2]);
    }
    ParseStats(FetchResults(apiUrl), getPersonaName(playerUrl));
}