//
// Created by hujiayucc on 25-5-4.
//

#include "main.h"

#include <appinfo.h>
#include <constant.h>
#include <global.h>
#include <MessageTools.h>
#include <nlohmann/json.hpp>
#include <utils.h>
#include <tool.h>

XLZ const char *appload(const char *apidata, const char *pluginkey) {
    try {
        Api.init(nlohmann::json::parse(gbk2utf8(apidata)), pluginkey);
    } catch (std::exception &e) {
        MessageBox(nullptr, (std::string("发生错误\n") + e.what()).c_str(), "错误提示", MB_OK);
    }
    auto appInfo = ApplicationInfo();
    appInfo.SetAppName("群助手");
    appInfo.SetAuthor("hujiayucc");
    appInfo.SetVersion("1.0.0");
    appInfo.SetDescription("群助手");
    appInfo.AddAllPermissions("群助手所需的权限");
    appInfo.SetEnableHandler(&AppStart);
    appInfo.SetDisableHandler(&AppEnd);
    appInfo.SetUninstallHandler(&AppUnload);
    appInfo.SetSettingsHandler(&ControlPanel);
    appInfo.SetPrivateMsgHandler(&OnPrivate);
    appInfo.SetGroupMsgHandler(&OnGroup);
    return appInfo.GetData();
}

XLZ int32_t AppStart() {
    try {
        PLUGIN_DATA_DIR = Api.GetPluginDataDir();
    } catch (std::exception &e) {
        MessageBox(nullptr, (std::string("发生错误\n") + e.what()).c_str(), "错误提示", MB_OK);
    }
    return ENABLE_RESPONSE_SUCCESS;
}

XLZ int32_t AppEnd() {
    return 0;
}

XLZ int32_t AppUnload() {
    return 0;
}

XLZ int32_t ControlPanel() {
    return 0;
}

XLZ int32_t OnPrivate(const int32_t data_ptr) {
    PrivateMessageData data{};
    MessageTools::ReadPrivateMessage(data_ptr, data);

    if (data.msgType != MSG_TYPE_FRIEND_NORMAL || data.senderQQ != 2792607647) {
        return MSG_CONTINUE;
    }

    const auto &content = data.content;

    const auto sendResponse = [&](const char *reply) {
        Api.SendPrivateMessage(data.frameworkQQ, data.senderQQ, reply);
    };

    if (str_equal(content, "重启")) {
        sendResponse("已执行重启");
        Api.rebootFamework();
        return MSG_INTERCEPT;
    }

    if (str_equal(content, "重载")) {
        sendResponse("已执行重载");
        Api.reload();
        return MSG_INTERCEPT;
    }

    auto handleConfigCommand = [&](const std::string &prefix, const std::string &configKey, const char *successMsg,
                                   const char *failMsg) -> bool {
        if (!str_starts_with(content, prefix)) return false;

        const bool result = WriteConfigItem(PLUGIN_DATA_DIR + "配置.ini", std::to_string(data.frameworkQQ), configKey,
                                            text_get_right(content, "#"));
        Api.SendPrivateMessage(data.frameworkQQ, data.senderQQ, result ? successMsg : failMsg);
        return true;
    };

    if (handleConfigCommand("支付密码#", "支付密码", "已设置支付密码", "设置支付密码失败") ||
        handleConfigCommand("分群开关#", "分群开关", "分群开关设置成功", "分群开关设置失败")) {
        return MSG_INTERCEPT;
    }

    return MSG_CONTINUE;
}

auto master(const GroupMessageData &data) -> bool {
    auto handleConfigCommand = [&](const std::string &prefix, const std::string &configKey, const char *successMsg,
                                   const char *failMsg) -> bool {
        if (!str_starts_with(data.content, prefix)) return false;
        const bool result = writeGroupConfig(data.groupNumber, configKey, text_get_right(data.content, "#"));
        Api.SendGroupMessage(data.frameworkQQ, data.groupNumber, result ? successMsg : failMsg);
        return true;
    };
    if (str_starts_with(data.content, "艾特全体")) {
        auto content = std::string(data.content);
        replaceAll(content, "艾特全体", TextCode::atAll());
        Api.SendGroupMessage(data.frameworkQQ, data.groupNumber, content.c_str());
        return true;
    }

    if (str_starts_with(data.content, "复读")) {
        Api.SendGroupMessage(data.frameworkQQ, data.groupNumber, text_get_right(data.content, "复读").c_str());
        return true;
    }

    if (handleConfigCommand("最小红包#", "最小红包", "最小红包设置成功", "最小红包设置失败") ||
        handleConfigCommand("最大红包#", "最大红包", "最大红包设置成功", "最大红包设置失败") ||
        handleConfigCommand("每日限量#", "每日限量", "每日限量设置成功", "每日限量设置失败") ||
        handleConfigCommand("红包口令#", "红包口令", "红包口令设置成功", "红包口令设置失败")) {
        return true;
    }

    if (str_equal(data.content, "红包口令")) {
        const auto result = std::stringstream()
                            << "红包口令：" << readGroupConfig(data.groupNumber, "红包口令", "测试红包")
                            << "\n最小：" << readGroupConfig(data.groupNumber, "最小红包", "10")
                            << "\n最大：" << readGroupConfig(data.groupNumber, "最大红包", "20")
                            << "\n已领取：" << std::to_string(getRedPacketItem(data.groupNumber))
                            << "\n每日限量：" << readGroupConfig(data.groupNumber, "每日限量", "10")
                            << "\n本群开关：" << readGroupConfig(data.groupNumber, "本群开关", "开");
        Api.SendGroupMessage(data.frameworkQQ, data.groupNumber, result.str().c_str());
        return true;
    }

    if (str_equal(data.content, "开启红包") || str_equal(data.content, "关闭红包")) {
        const bool enable = str_equal(data.content, "开启红包");
        if (writeGroupConfig(data.groupNumber, "本群开关", enable ? "开" : "关")) {
            Api.SendGroupMessage(data.frameworkQQ, data.groupNumber, enable ? "本群红包开启成功" : "本群红包关闭成功");
        }
        return true;
    }
    return false;
}

bool sendRedPacket(const GroupMessageData &data) {
    if (str_starts_with(data.content, "拼手气#")) {
        // 拼手气#10#10#测试红包
        const auto args = splitString(text_get_right(data.content, "拼手气#"), '#');
        // 红包数量
        const auto total = static_cast<int32_t>(str2ll(args[0]));
        // 红包金额
        const auto amount = static_cast<int32_t>(str2ll(args[1]));
        // 红包文本
        const auto &bless = args[2].c_str();
        // ReSharper disable once CppExpressionWithoutSideEffects
        Api.GroupLuckyRedPacket(data.frameworkQQ, total, amount, data.groupNumber, bless,
                                ReadConfigItem(PLUGIN_DATA_DIR + "配置.ini", std::to_string(data.frameworkQQ), "支付密码",
                                               ""));
        return true;
    }
    if (str_starts_with(data.content, "语音#")) {
        // 语音#10#10#测试红包
        const auto args = splitString(text_get_right(data.content, "语音#"), '#');
        // 红包数量
        const auto total = static_cast<int32_t>(str2ll(args[0]));
        // 红包金额
        const auto amount = static_cast<int32_t>(str2ll(args[1]));
        // 红包文本
        const auto &voice = args[2].c_str();
        // ReSharper disable once CppExpressionWithoutSideEffects
        Api.GroupVoiceRedPacket(data.frameworkQQ, total, amount, data.groupNumber, voice,
                                ReadConfigItem(PLUGIN_DATA_DIR + "配置.ini", std::to_string(data.frameworkQQ), "支付密码",
                                               ""));
        return true;
    }
    if (str_starts_with(data.content, "赏#")) {
        // 赏#10#测试红包#领取人QQ
        const auto args = splitString(text_get_right(data.content, "赏#"), '#');
        // 红包金额
        const auto amount = static_cast<int32_t>(str2ll(args[0]));
        // 红包文本
        const auto &bless = args[1].c_str();
        // 领取人QQ
        auto qq = args[2];
        replaceAll(qq, "[@", "");
        replaceAll(qq, "]", "");
        replaceAll(qq, " ", "");
        // ReSharper disable once CppExpressionWithoutSideEffects
        Api.GroupExclusiveRedPacket(data.frameworkQQ, 1, amount, data.groupNumber, qq.c_str(), bless, false,
                                    ReadConfigItem(PLUGIN_DATA_DIR + "配置.ini", std::to_string(data.frameworkQQ), "支付密码",
                                                   ""));
        return true;
    }
    return false;
}

XLZ int OnGroup(const int32_t data_ptr) {
    GroupMessageData data{};
    MessageTools::ReadGroupMessage(data_ptr, data);
    // 定义常量路径
    const std::string kConfigIni = PLUGIN_DATA_DIR + "配置.ini";
    const std::string kRedPacketRecordIni = PLUGIN_DATA_DIR + "红包记录.ini";
    const std::string currentDate = GetCurrentDate();

    // 基础信息解包
    const int64_t groupNumber = data.groupNumber;
    const int64_t senderQQ = data.senderQQ;
    const int64_t frameworkQQ = data.frameworkQQ;

    // 过滤自身消息
    if (frameworkQQ == senderQQ || data.frameworkAnonId == frameworkQQ)
        return MSG_CONTINUE;

    // 主人信息处理
    const int64_t masterQQ = str2ll(ReadConfigItem(kConfigIni, std::to_string(frameworkQQ),
                                                   "主人QQ", "2792607647"));
    if (senderQQ == masterQQ) {
        if (master(data)) return MSG_CONTINUE;

        // 合并命令判断条件
        constexpr std::array<const char *, 3> COMMAND_PREFIXES = {"拼手气#", "语音#", "赏#"};
        const bool hasValidPrefix = std::any_of(COMMAND_PREFIXES.begin(), COMMAND_PREFIXES.end(),
                                                [&](const char *prefix) {
                                                    return str_starts_with(data.content, prefix);
                                                });

        if (hasValidPrefix && sendRedPacket(data))
            return MSG_INTERCEPT;
    }

    // 红包口令处理
    if (str_equal(data.content, readGroupConfig(groupNumber, "红包口令", "测试红包"))) {
        // 开关状态判断
        const bool globalSwitch = str_equal(ReadConfigItem(kConfigIni, std::to_string(frameworkQQ),
                                                           "分群开关", "开"), "开");
        const bool groupSwitch = str_equal(readGroupConfig(groupNumber, "本群开关", "关"), "开");
        if (!(!globalSwitch || groupSwitch))
            return MSG_CONTINUE;

        // 领取状态验证
        const std::string receiveStatus = ReadConfigItem(kRedPacketRecordIni, currentDate,
                                                         std::to_string(senderQQ), "未领取");
        if (str_equal(receiveStatus.c_str(), "已领取")) {
            Api.SendGroupMessage(frameworkQQ, groupNumber,
                                 (TextCode::atUser(senderQQ, true) + "你今天已经领过红包了").c_str());
            return MSG_INTERCEPT;
        }

        // 红包上限检查
        const int64_t currentCount = getRedPacketItem(groupNumber);
        const int64_t maxLimit = str2ll(readGroupConfig(groupNumber, "红包上限", "10"));
        if (currentCount >= maxLimit) {
            Api.SendGroupMessage(frameworkQQ, groupNumber,
                                 (TextCode::atUser(senderQQ, true) + "今日发放红包已达上限").c_str());
            return MSG_INTERCEPT;
        }

        // 红包参数准备
        const int32_t minAmount = static_cast<int32_t>(str2ll(readGroupConfig(groupNumber, "最小红包", "10")));
        const int32_t maxAmount = static_cast<int32_t>(str2ll(readGroupConfig(groupNumber, "最大红包", "30")));
        const std::string paymentPassword = ReadConfigItem(kConfigIni,
                                                           std::to_string(frameworkQQ), "支付密码", "");

        // 发送红包
        const char *result = Api.GroupExclusiveRedPacket(frameworkQQ, 1, GetRandom(minAmount, maxAmount),
                                                         groupNumber, std::to_string(senderQQ).c_str(), data.content,
                                                         false, paymentPassword.c_str());

        // 结果处理
        nlohmann::json json = nlohmann::json::parse(gbk2utf8(result));
        if (str_equal(json["retmsg"].get<std::string>().c_str(), "success")) {
            WriteConfigItem(kRedPacketRecordIni, currentDate, std::to_string(senderQQ), "已领取");
            WriteConfigItem(kRedPacketRecordIni, std::to_string(groupNumber), currentDate + "数量",
                            std::to_string(currentCount + 1));
        } else {
            const std::string errorPublic = TextCode::atUser(senderQQ, true) + "红包发送失败";
            const std::string errorPrivate = "红包发送失败：" + utf82gbk(json["retmsg"].get<std::string>() + "\n")
                                                                                + "支付密码：" + paymentPassword;
            Api.SendGroupMessage(frameworkQQ, groupNumber, errorPublic.c_str());
            Api.SendPrivateMessage(frameworkQQ, masterQQ, errorPrivate.c_str());
        }
        return MSG_INTERCEPT;
    }
    return MSG_CONTINUE;
}
