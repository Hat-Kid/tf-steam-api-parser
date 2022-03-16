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
    size_t realsize = size * nmemb;
    auto *mem = (struct MemoryStruct *) userp;

    char* ptr = static_cast<char *>(realloc(mem->memory, mem->size + realsize + 1));
    if (!ptr) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
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
string getDescriptionForStat(const map<string, string>& desc, T stat) {
    if constexpr (std::is_same_v<T, ClassStat>) {
        auto m = stat.fullName;
        auto c = stat.className;
        FindAndReplaceAll(m, c, "Class");
        auto reg = regex(m, regex::optimize);
        if (stat.gameType == GameType::pvp) {
            for (auto& it : desc) {
                if (regex_match(it.first, reg)) {
                    auto result = it.second;
                    FindAndReplaceAll(result, "Class", c);
                    return result;
                }
            }
        } else {
            for (auto& it : desc) {
                if (regex_match(it.first, reg)) {
                    auto result = it.second;
                    FindAndReplaceAll(result, "Class", c);
                    return result;
                }
            }
        }
    } else if constexpr(std::is_same_v<T, AchievementStat>) {
        auto m = stat.name;
        for (auto& it : desc) {
            if (m == it.first) {
                return it.second;
            }
        }
    }
    return "null";
};

PlayerStats FetchResults(const string& apiUrl) {
    PlayerStats playerStats;
    ostringstream jsonStream;
    Poco::JSON::Parser parser;
    Poco::Dynamic::Var parseResult;
    string resultStr;
    cmatch match;
    struct MemoryStruct chunk{};

    chunk.memory = static_cast<char *>(malloc(1));  /* will be grown as needed by the realloc above */
    chunk.size = 0;    /* no data at this point */

    CURLcode code;
    CURL* conn;
    conn = curl_easy_init();
    if (!conn) {
        fprintf(stderr, "Error: Failed to create CURL connection!\n");
        exit(EXIT_FAILURE);
    }

    code = curl_easy_setopt(conn, CURLOPT_URL, apiUrl.c_str());
    curl_easy_setopt(conn, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(conn, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(conn, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    code = curl_easy_perform(conn);
    if(code != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(code));
    }
    else {
        /*
         * Now, our chunk.memory points to a memory block that is chunk.size
         * bytes big and contains the remote file.
         *
         * Do something nice with it!
         */
        printf("%lu bytes retrieved\n", (unsigned long) chunk.size);
    }

    map<string, string> descriptions = FetchDescriptions();
    jsonStream << chunk.memory;
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
                cout << "Matches: " << match.str(0) << " | " << match.str(1) << " | " << match.str(2) << " | " << match.str(3) << endl;
                classStat.fullName = match.str(0);
                classStat.className = match.str(1);
                classStat.statType = match.str(2) == "accum" ? StatType::accum : StatType::max;
                classStat.shortName = match.str(3);
                classStat.gameType = GameType::pvp;
                classStat.value = stat->getValue<int>("value");
                classStat.description = getDescriptionForStat(descriptions, classStat);
                cout << "Got " << classStat.className << " " << classStat.shortName << " pvp stat with value " << classStat.value << endl;
                cout << "Description: " << classStat.description << endl << endl;
                playerStats.pvpStats.push_back(classStat);
            } else if (std::regex_match(statName.c_str(), match, ClassStat::regMvm)) {
                ClassStat classStat;
                classStat.fullName = match.str(0);
                classStat.className = match.str(1);
                classStat.statType = match.str(2) == "accum" ? StatType::accum : StatType::max;
                classStat.shortName = match.str(3);
                classStat.gameType = GameType::coop;
                classStat.value = stat->getValue<int>("value");
                classStat.description = getDescriptionForStat(descriptions, classStat);
                cout << "Got " << classStat.className << " " << classStat.shortName << " mvm stat with value " << classStat.value << endl;
                cout << "Description: " << classStat.description << endl << endl;
                playerStats.mvmStats.push_back(classStat);
            } else if (std::regex_match(statName.c_str(), match, MapStat::reg)) {
                MapStat mapStat;
                cout << "Matches: " << match.str(0) << " | " << match.str(1) << match.str(2) << endl;
                mapStat.name = match.str(1);
                mapStat.gamemode = match.str(2);
                mapStat.playTime = stat->getValue<int>("value");
                cout << "Got " << mapStat.name << " stat for gamemode " << mapStat.gamemode << " with time " << mapStat.playTime << endl << endl;
                playerStats.mapStats.push_back(mapStat);
            } else if (std::regex_match(statName.c_str(), match, AchievementStat::reg)) {
                AchievementStat achievementStat;
                cout << "Matches: " << match.str(0) << endl;
                achievementStat.name = match.str(0);
                achievementStat.value = stat->getValue<int>("value");
                achievementStat.description = getDescriptionForStat(descriptions, achievementStat);
                cout << "Got " << achievementStat.name << " achievement stat with value " << achievementStat.value << endl;
                cout << "Description: " << achievementStat.description << endl << endl;
                playerStats.achievementStats.push_back(achievementStat);
            }
        }
    }
    return playerStats;
    curl_global_cleanup();
}

void ParseResults(const PlayerStats& stats) {
    using namespace inja;

    json data;
    Environment env;
    Template tmp = env.parse_template("template.md");
    string result;

    vector<ClassStat> pvpStats = stats.pvpStats;
    vector<ClassStat> mvmStats = stats.mvmStats;
    vector<MapStat> mapStats = stats.mapStats;
    vector<AchievementStat> achievementStats = stats.achievementStats;

    for (auto& pvpstat : pvpStats) {
        data["class"] = pvpstat.className;
        data["classStatDescription"] = pvpstat.description;
    }

    string file("stats.md");
    ofstream out(file);
    out << result;
    out.close();
}

int main(int argc, char **argv) {
    std::string apiUrl = "https://api.steampowered.com/ISteamUserStats/GetUserStatsForGame/v0002/?appid=440&key=apikey&steamid=id64";
    if ((!argv[1] && !argv[2]) || (!argv[1] && argv[2]) || (argv[1] && !argv[2])) {
        uint64_t steamId;
        string apiKey;
        cout << "Enter the user's SteamID64: ";
        cin >> steamId;
        FindAndReplaceAll(apiUrl, "id64", to_string(steamId));
        cout << "Enter your Steam API key: ";
        cin >> apiKey;
        FindAndReplaceAll(apiUrl, "apikey", apiKey);
    } else {
        FindAndReplaceAll(apiUrl, "id64", argv[1]);
        FindAndReplaceAll(apiUrl, "apikey", argv[2]);
    }
    FetchResults(apiUrl);
}