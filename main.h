//
// Created by hujiayucc on 25-5-4.
//

#ifndef LIBRARY_H
#define LIBRARY_H
#ifdef _WIN32
#ifdef XLZ_EXPORTS
#define XLZ __declspec(dllexport)
#else
#define XLZ __declspec(dllimport)
#endif
#else
#define XLZ
#endif

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif
    /** 初始化 */
    XLZ const char* appload(const char* apidata, const char* pluginkey);

    /**
     * @brief 插件启用初始化函数
     * @return 启用响应状态码
     */
    XLZ int32_t AppStart();

    /**
     * @brief 插件禁用处理函数
     * @return 状态码
     * @note 本函数无法使用框架API
     */
    XLZ int32_t AppEnd();

    /**
     * @brief 插件卸载处理函数
     * @return 状态码
     * @note 本函数无法使用框架API
     */
    XLZ int32_t AppUnload();

    /**
     * @brief 插件控制面板处理
     * @return 状态码
     */
    XLZ int32_t ControlPanel();

    /**
     * @brief 私聊消息处理函数
     * @param data_ptr 消息数据结构指针
     * @return 消息处理状态
     * @note 多线程环境运行，返回1阻止后续插件处理
     */
    XLZ int32_t OnPrivate(int32_t data_ptr);

    /**
     * @brief 群聊消息处理函数
     * @param data_ptr 消息数据结构指针
     * @return 消息处理状态
     * @note 多线程环境运行，返回1阻止后续插件处理
     */
    XLZ int32_t OnGroup(int32_t data_ptr);
#ifdef __cplusplus
}
#endif
#endif