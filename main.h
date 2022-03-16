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
std::map<std::string, std::string> FetchDescriptions();
template<class T>
std::string getDescriptionForStat(const std::map<std::string, std::string>& desc, T stat);
PlayerStats FetchResults(const std::string& apiUrl);
void ParseResults(const PlayerStats& stats);