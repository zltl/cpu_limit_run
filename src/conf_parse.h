/**
 * @file conf_parse.h
 * @brief 配置文件格式解读函数集合.
 */

#ifndef CONF_PARSE_H_
#define CONF_PARSE_H_

#include <stdio.h>

#define CONF_MAX_LINE_LEN 1000

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 解析函数类型定义
 *
 * @param addr 配置项变量地址
 * @param addr_cap 配置项变量字节长度
 * @param value 配置值字符串
 * @param value_len 配置值字符串长度
 * @return 成功返回 0，失败返回 -1
 */
typedef int (*parse_fn)(void *addr, size_t addr_cap, void *value,
                        size_t value_len);

/**
 * @brief 解析配置值 value 到字符串 addr
 */
int conf_parse_string(void *addr, size_t addr_cap, void *value,
                      size_t value_len);
/// 解析配置值 value 到整数 addr
int conf_parse_integer(void *addr, size_t addr_cap, void *value,
                       size_t value_len);
/// 解析配置值 value 到整数 addr.
/// value 可以有单位 k, m, g.
int conf_parse_memspace_as_bytes(void *addr, size_t addr_cap, void *value,
                                 size_t value_len);
/// 解析配置值 value 到整数 addr.
/// value 可以有单位 s, m, h, d, y.
int conf_parse_time_as_second(void *addr, size_t addr_cap, void *value,
                              size_t value_len);
/// 遇到配置项 "include=filename"，就直接打开文件 filename 解析它
int conf_do_include(void *addr, size_t addr_cap, void *value, size_t value_len);
/// 解析 true, false, ok, yes, no, 1 到整数
int conf_parse_bool(void *addr, size_t addr_cap, void *value, size_t value_len);

#define VT_INT 0
#define VT_STR 1

/**
 * @brief 配置文件配置项存储地址、解读函数、默认值
 */
typedef struct _parse_command_t {
    char *cmd;
    parse_fn parse_func;
    void *addr;
    size_t addr_cap;
    char *default_value_string;
    int value_type;
    char *desc;
} parse_command_t;

/**
 * @brief 初始化 parse_command_t，将默认值解析
 *
 * @param cmds parse_command_t 数组，最后一个对象所以值应为 0
 * @return int 成功返回 0
 */
int conf_init(parse_command_t *const cmds);
/**
 * @brief 将配置文件解析到配置项
 *
 * @param cmds parse_command_t 数组，最后一个对象所以值应为 0
 * @param confile 配置文件路径
 * @return int 成功返回 0
 */
int conf_parse_file(parse_command_t *const cmds, char const *const confile);
/**
 * @brief 将命令行参数解析到配置项. 命令行参数形如
 *     --option1=value1 --option2 value2
 *
 * @param cmds parse_command_t 数组，最后一个对象所以值应为 0
 * @param argc 参数数量
 * @param argv 参数数组
 * @return int 成功返回 0
 */
int conf_parse_args(parse_command_t *const cmds, int argc, char const *argv[]);

/**
 * @brief 将环境变量解析到配置项. 忽略大小写，环境变量通常是全大写的。
 *
 * @param cmds parse_command_t 数组，最后一个对象所以值应为 0
 * @return int 成功返回 0
 */
int conf_parse_env(parse_command_t *const cmds);

/**
 * @brief 打印配置项.
 *
 * @param out 打印输出位置，一般是 stdout.
 * @param cmds parse_command_t 数组
 * @return int 成功返回 0
 */
int conf_print_conf(FILE *out, parse_command_t *cmds);

/**
 * @brief 打印配置参数
 *
 * @param out 打印到输出流，一般是 stdout.
 * @param cmds parse_command_t 数组.
 */
void conf_print_usage(FILE *out, parse_command_t *cmds);

long long get_int_from_addr(void *addr, size_t addr_cap);

#define CONF_CMD_END() \
    { NULL, NULL, NULL, 0, NULL, 0, NULL }
#define CONF_CMD_BEGIN(cmds)                                            \
    {"include",                                                         \
     conf_do_include,                                                   \
     cmds,                                                              \
     0,                                                                 \
     NULL,                                                              \
     0,                                                                 \
     "include configuration file"},                                     \
    {                                                                   \
        "conf", conf_do_include, cmds, 0, NULL, 0, "configuration file" \
    }
#define CONF_CMD(conf, key, func, default_value, vt, desc) \
    { #key, func, &conf->key, sizeof(conf->key), default_value, vt, desc }
#define CONF_CMD_INT(conf, key, default_value, desc) \
    CONF_CMD(conf, key, conf_parse_integer, default_value, VT_INT, desc)
#define CONF_CMD_STR(conf, key, default_value, desc) \
    CONF_CMD(conf, key, conf_parse_string, default_value, VT_STR, desc)
#define CONF_CMD_MEM(conf, key, default_value, desc)                         \
    CONF_CMD(conf, key, conf_parse_memspace_as_bytes, default_value, VT_INT, \
             desc)
#define CONF_CMD_TIME(conf, key, default_value, desc) \
    CONF_CMD(conf, key, conf_parse_time_as_second, default_value, VT_INT, desc)
#define CONF_CMD_BOOL(conf, key, default_value, desc) \
    CONF_CMD(conf, key, conf_parse_bool, default_value, VT_INT, desc)

#ifdef __cplusplus
}
#endif

#endif /* CONF_PARSE_H_ */
