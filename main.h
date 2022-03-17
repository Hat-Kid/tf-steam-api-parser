#pragma once

#include <iostream>
#include <map>

class PlayerStats;

void FindAndReplaceAll(std::string &data, const std::string& toSearch, const std::string& replaceStr)
{
    // Get the first occurrence
    size_t pos = data.find(toSearch);

    // Repeat until end is reached
    while (pos != std::string::npos)
    {
        // Replace this substring occurrence
        data.replace(pos, toSearch.size(), replaceStr);
        // Get the next occurrence from the current position
        pos = data.find(toSearch, pos + replaceStr.size());
    }
}
std::string ConvertMSToHHMMSS(std::chrono::milliseconds ms)
{
    using namespace std::chrono;
    std::stringstream ss;

    // compute h, m, s
    auto secs = duration_cast<seconds>(ms);
    ms -= duration_cast<milliseconds>(secs);

    auto mins = duration_cast<minutes>(secs);
    secs -= duration_cast<seconds>(mins);

    auto hour = duration_cast<hours>(mins);
    mins -= duration_cast<minutes>(hour);

    std::string hr(std::to_string(hour.count()));
    std::string min(std::to_string(mins.count()));
    std::string s(std::to_string(secs.count()));
    //std::string msecs(std::to_string(ms.count()));

    // msecs = msecs.substr(0, 2);

    // add leading zero if needed
    std::string hh = std::string(4 - hr.length(), '0') + hr;
    std::string mm = std::string(2 - min.length(), '0') + min;
    std::string sec = std::string(2 - s.length(), '0') + s;

    // return mm:ss if hh is 0000
    if (hh.compare("0000") != 0)
    {
        ss << hr << ":" << mm << "." << sec;
    }
    else
    {
        ss << mm << ":" << sec;
    }

    return ss.str();
}
std::map<std::string, std::string> FetchDescriptions();
template<class T>
std::string getDescriptionForStat(const std::map<std::string, std::string>& desc, T stat);
PlayerStats FetchResults(const std::string& apiUrl);
void ParseResults(const PlayerStats& stats);