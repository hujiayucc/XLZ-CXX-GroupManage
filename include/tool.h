//
// Created by hujiayucc on 25-5-13.
//

#ifndef TOOL_H
#define TOOL_H

inline auto GetCurrentDate() {
    const std::time_t currentTime = std::time(nullptr);
    std::tm localTime{};
    localtime_s(&localTime, &currentTime);
    const int year = localTime.tm_year + 1900;
    const int month = localTime.tm_mon + 1;
    const int day = localTime.tm_mday;
    return std::to_string(year) + "年" + std::to_string(month) + "月" + std::to_string(day) + "日";
}

inline auto readGroupConfig(const int64_t group, const std::string &name, const std::string &value) {
    return ReadConfigItem(PLUGIN_DATA_DIR + "群配置.ini", std::to_string(group), name, value);
}

inline auto writeGroupConfig(const int64_t group, const std::string &name, const std::string &value) {
    return WriteConfigItem(PLUGIN_DATA_DIR + "群配置.ini", std::to_string(group), name, value);
}

inline auto getRedPacketItem(const int64_t group) {
    return static_cast<int32_t>(str2ll(ReadConfigItem(PLUGIN_DATA_DIR + "红包记录.ini",
    std::to_string(group), GetCurrentDate() + "数量", "0")));
}
#endif //TOOL_H
