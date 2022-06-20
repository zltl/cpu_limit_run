/**
 * @file conf_parse.c
 * @brief 配置项解析函数实现.
 */

#include "conf_parse.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/**
 * @brief 16进制辅助函数，将16进制字符转换为整数值.
 *
 * @param c 字符
 * @return int 对应的数值
 */
static int char2digit(char c) {
    if (isdigit(c)) {
        return c - '0';
    }
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 10;
    }
    return 0;
}

/**
 * @brief 将整数 \a v 设置到地址 \a addr
 *
 * @param addr 目标地址
 * @param addr_cap 目标变量的字节长度
 * @param v 要设置的整数值
 * @return int 成功返回0
 */
static int put_integer2addr(void *addr, size_t addr_cap, long long int v) {
    switch (addr_cap) {
        case 1:
            *(char *)addr = (char)v;
            return 0;
        case 2:
            *(short *)addr = (short)v;
            return 0;
        case 4:
            *(int *)addr = (int)v;
            return 0;
        case 8:
            *(long long int *)addr = v;
            return 0;
        default:
            return -1;
    }
}

long long get_int_from_addr(void *addr, size_t addr_cap) {
    long v = 0;
    switch (addr_cap) {
        case 1:
            v = *(char *)addr;
            break;
        case 2:
            v = *(short *)addr;
            break;
        case 4:
            v = *(int *)addr;
            break;
        case 8:
            v = *(long long *)addr;
            break;
        default:
            break;
    }
    return v;
}

int conf_parse_bool(void *addr, size_t addr_cap, void *value,
                    size_t value_len) {
    (void)value_len;
    char *src = (char *)value;
    int v = 0;
    if (*src == 'y' || *src == 't' || *src == 'o' || *src == '1') {
        v = 1;
    }
    if (put_integer2addr(addr, addr_cap, v) < 0) {
        printf("addr_cap invalid: %ld\n", (long)addr_cap);
        return -1;
    }

    return 0;
}

int conf_parse_string(void *addr, size_t addr_cap, void *value,
                      size_t value_len) {
    char *dest = (char *)addr, *src = (char *)value;
    int len = addr_cap < value_len ? addr_cap : value_len;
    while (len-- && *src) {
        *dest++ = *src++;
    }
    *dest = '\0';

    return 0;
}

int conf_parse_integer(void *addr, size_t addr_cap, void *value,
                       size_t value_len) {
    char *src = (char *)value;
    int base = 10;
    char *pend;
    if (value_len > 2 && src[0] == '0' && src[1] == 'x') {
        base = 16;
        src += 2;
    }

    long long int valueint = strtoll(src, &pend, base);
    if (put_integer2addr(addr, addr_cap, valueint) < 0) {
        printf("conf_parse_integer invalid addr_cap: %ld\n", (long)addr_cap);
        return -1;
    }

    return 0;
}

#define _CASE_MOVE_INT(flag1, flag2, dst, src) \
    case flag1:                                \
    case flag2:                                \
        dst += src;                            \
        src = 0;                               \
        break

int conf_parse_memspace_as_bytes(void *addr, size_t addr_cap, void *value,
                                 size_t value_len) {
    int i;
    int base = 10;
    char *p = (char *)value;
    long long int bytes = 0, kib = 0, mib = 0, gib = 0, tib = 0, pib = 0,
                  tmp = 0;
    for (i = 0; i < (int)value_len && *p; i++, p++) {
        char c = *p;
        if (i == 1 && (c == 'x' || c == 'X') && tmp == 0) {
            base = 16;
            tmp = 0;
            continue;
        }

        switch (c) {
            _CASE_MOVE_INT('k', 'K', kib, tmp);
            _CASE_MOVE_INT('m', 'M', mib, tmp);
            _CASE_MOVE_INT('g', 'G', gib, tmp);
            _CASE_MOVE_INT('t', 'T', tib, tmp);
            _CASE_MOVE_INT('p', 'P', pib, tmp);
            _CASE_MOVE_INT('b', 'B', bytes, tmp);
            default:
                tmp = tmp * base + char2digit(c);
                break;
        }
    }

    bytes += (kib << 10) + (mib << 20) + (gib << 30) + (tib << 40) +
             (pib << 50) + tmp;

    if (put_integer2addr(addr, addr_cap, bytes) < 0) {
        printf("conf_parse_memspace_as_bytes invalid addr_cap: %ld\n",
               (long)addr_cap);
        return -1;
    }
    return 0;
}

int conf_parse_time_as_second(void *addr, size_t addr_cap, void *value,
                              size_t value_len) {
    int i;
    int base = 10;
    char *p = (char *)value;
    long long int seconds = 0, hours = 0, minutes = 0, days = 0, years = 0,
                  tmp = 0;

    for (i = 0; i < (int)value_len && *p; i++, p++) {
        char c = *p;
        if (i == 1 && (c == 'x' || c == 'X') && tmp == 0) {
            base = 16;
            tmp = 0;
            continue;
        }

        switch (c) {
            _CASE_MOVE_INT('s', 'S', seconds, tmp);
            _CASE_MOVE_INT('m', 'M', minutes, tmp);
            _CASE_MOVE_INT('h', 'H', hours, tmp);
            _CASE_MOVE_INT('d', 'D', days, tmp);
            _CASE_MOVE_INT('y', 'Y', years, tmp);
            default:
                tmp = tmp * base + char2digit(c);
                break;
        }
    }

    seconds += years * 365 * 24 * 60 * 60 + days * 24 * 60 * 60 +
               hours * 60 * 60 + minutes * 60 + tmp;

    if (put_integer2addr(addr, addr_cap, seconds) < 0) {
        printf("conf_parse_memspace_as_second invalid addr_cap: %ld\n",
               (long)addr_cap);
        return -1;
    }
    return 0;
}

int conf_do_include(void *addr, size_t addr_cap, void *value,
                    size_t value_len) {
    (void)addr_cap;
    (void)value_len;
    if (value == NULL || strlen(value) == 0) {
        return 0;
    }
    return conf_parse_file(addr, value);
}

/**
 * @brief 解析一行配置文件
 */
static int conf_parse_line(parse_command_t *const cmds, const char *const line,
                           size_t line_len, const char *const confile,
                           size_t line_num) {
    static char key[CONF_MAX_LINE_LEN], value[CONF_MAX_LINE_LEN];
    char *pkey, *pvalue;
    size_t keylen = 0, valuelen = 0;
    char const *p = line;

    if (line_len == 0) {
        return 0;
    }

    // skip spaces
    while (*p && (*p == ' ' || *p == '\t')) p++;
    if (!*p) {
        printf("invalid conf at file %s, line %ld - %s\n", confile,
               (long)line_num, line);
        return -1;
    }

    // command out line
    if (*p == '#' || *p == '\r' || *p == '\n' || *p == '[') {
        return 0;
    }

    // find key
    pkey = key;
    while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
        *pkey++ = *p++;
        keylen++;
    }
    *pkey = '\0';

    // skip spaces, value may empty
    while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;

    // set value
    pvalue = value;
    while (*p && *p != '\r' && *p != '\n') {
        *pvalue++ = *p++;
        valuelen++;
    }
    // trim valuestring
    while (pvalue > value && (*(pvalue - 1) == ' ' || *(pvalue - 1) == '\t')) {
        pvalue--;
    }
    *pvalue = '\0';

    // call parse_fn
    parse_command_t *it = cmds;
    for (it = cmds; it->cmd; it++) {
        if (strcmp(it->cmd, key) == 0) {
            if (it->parse_func(it->addr, it->addr_cap, value, valuelen) < 0) {
                printf(
                    "parsefunc return error, cmd %s, file %s, line %ld - %s\n",
                    it->cmd, confile, (long)line_num, line);
                return -1;
            }
        }
    }

    return 0;
}

int conf_init(parse_command_t *const cmds) {
    parse_command_t *it = cmds;
    for (it = cmds; it->cmd; it++) {
        size_t slen = 0;
        if (it->default_value_string) {
            slen = strlen(it->default_value_string);
        }
        if (it->parse_func(it->addr, it->addr_cap, it->default_value_string,
                           slen) < 0) {
            printf("default conf error: key=%s, value=%s\n", it->cmd,
                   it->default_value_string);
            return -1;
        }
    }
    return 0;
}

int conf_parse_file(parse_command_t *const cmds, char const *const confile) {
    FILE *f = NULL;
    static char line[CONF_MAX_LINE_LEN];

    if (cmds == NULL || confile == NULL) {
        return -1;
    }

    f = fopen(confile, "r");
    if (f == NULL) {
        return -1;
    }

    int line_num = 0;
    while (fgets(line, sizeof(line), f)) {
        if (conf_parse_line(cmds, line, strlen(line), confile, line_num) < 0) {
            printf("parse conf %s failed\n", confile);
            fclose(f);
            return -1;
        }
    }

    fclose(f);
    return 0;
}

int conf_parse_key_value_arg(parse_command_t *const cmds, const char *key,
                             const char *value, const char *whatarg) {
    parse_command_t *it = cmds;
    for (it = cmds; it->cmd; it++) {
        if (strcmp(it->cmd, key) == 0) {
            if (it->parse_func(it->addr, it->addr_cap, (char *)value,
                               strlen(value)) < 0) {
                printf("parsefunc return error, cmd %s, arg %s\n", it->cmd,
                       whatarg);
                return -1;
            }
        }
    }
    return 0;
}

static void convert_key_underscore(char *key, int len) {
    int i;
    for (i = 0; i < len; i++) {
        if (key[i] == '-' || key[i] == '.') {
            key[i] = '_';
        }
    }
}

int conf_parse_args(parse_command_t *const cmds, int argc, char const *argv[]) {
    int i;
    static char key[CONF_MAX_LINE_LEN], value[CONF_MAX_LINE_LEN];
    int minlen = 0;

    for (i = 1; i < argc; ++i) {
        if (strlen(argv[i]) > 2 && argv[i][0] == '-' && argv[i][1] == '-') {
            const char *line_p = &argv[i][2];
            char *eqindex = strchr(line_p, '=');
            if (eqindex != NULL) {
                minlen = eqindex - line_p;
                if (minlen > CONF_MAX_LINE_LEN) {
                    minlen = CONF_MAX_LINE_LEN;
                }
                strncpy(key, line_p, CONF_MAX_LINE_LEN);
                // foo-bar, foo.bar same as foo_bar
                convert_key_underscore(key, minlen);
                minlen = strlen(line_p) - minlen /*key*/ - 1 /*=*/;
                if (minlen > CONF_MAX_LINE_LEN) {
                    minlen = CONF_MAX_LINE_LEN;
                }
                strncpy(value, eqindex + 1, CONF_MAX_LINE_LEN);
                conf_parse_key_value_arg(cmds, key, value, argv[i]);
                continue;
            }
            if (i + 1 == argc) {
                return 0;
            }
            minlen = strlen(&argv[i][2]) - 2;
            if (minlen > CONF_MAX_LINE_LEN) {
                minlen = CONF_MAX_LINE_LEN;
            }
            // foo-bar, foo.bar same as foo_bar
            convert_key_underscore(key, minlen);

            strncpy(key, &argv[i][2], CONF_MAX_LINE_LEN);
            conf_parse_key_value_arg(cmds, key, argv[i + 1], argv[i]);
            ++i;
        }
    }

    return 0;
}

int conf_parse_env(parse_command_t *const cmds) {
    parse_command_t *it = cmds;
    for (it = cmds; it->cmd; it++) {
        char *value = getenv(it->cmd);
        if (value == NULL) {
            continue;
        }
        if (it->parse_func(it->addr, it->addr_cap, value, strlen(value)) < 0) {
            printf("env conf error: key=%s, value=%s\n", it->cmd, value);
            return -1;
        }
    }
    return 0;
}

int conf_print_conf(FILE *out, parse_command_t *cmds) {
    parse_command_t *it;
    fprintf(out, "# conf parse as: \n");
    for (it = cmds; it->cmd; it++) {
        if (it->parse_func == conf_parse_string) {
            fprintf(out, "%s %s\n", it->cmd, (char *)it->addr);
        } else if (it->parse_func == conf_parse_integer ||
                   it->parse_func == conf_parse_bool ||
                   it->parse_func == conf_parse_memspace_as_bytes ||
                   it->parse_func == conf_parse_time_as_second) {
            long long int value = 0;
            if (it->addr_cap == 1) {
                value = (long long int)*(char *)it->addr;
            } else if (it->addr_cap == 2) {
                value = (long long int)*(short *)it->addr;
            } else if (it->addr_cap == 4) {
                value = (long long int)*(int *)it->addr;
            } else if (it->addr_cap == 8) {
                value = (long long int)*(long long int *)it->addr;
            }
            fprintf(out, "%s %lld\n", it->cmd, value);
        }
    }
    return 0;
}

void conf_print_usage(FILE *out, parse_command_t *cmds) {
    parse_command_t *it;
    char key[CONF_MAX_LINE_LEN];
    int i;
    for (it = cmds; it->cmd; it++) {
        int minlen = strlen(it->cmd);
        if (minlen > CONF_MAX_LINE_LEN) {
            minlen = CONF_MAX_LINE_LEN;
        }
        strncpy(key, it->cmd, CONF_MAX_LINE_LEN);
        for (i = 0; i < minlen; ++i) {
            if (key[i] == '_') {
                key[i] = '-';
            }
        }
        fprintf(out, "--%s\t", key);
        if (it->parse_func == conf_parse_integer) {
            fprintf(out, "INTEGER(example:1/23/0x56...)\t");
        } else if (it->parse_func == conf_parse_bool) {
            fprintf(out, "BOOL(yes/no)\t");
        } else if (it->parse_func == conf_parse_memspace_as_bytes) {
            fprintf(out, "SPACE(example:1g3k/5m/20k/100B...)\t");
        } else if (it->parse_func == conf_parse_time_as_second) {
            fprintf(out, "DURATION(example:3y10d10h6m10s/10h)\t");
        } else if (it->parse_func == conf_parse_string) {
            fprintf(out, "STRING(example:this-is-string)\t");
        } else if (it->parse_func == conf_do_include) {
            fprintf(out, "CONFILE(example:path/to/confile...)\t");
        } else {
            fprintf(out, "unkown type\t");
        }

        if (it->desc) {
            fprintf(out, "%s\t", it->desc);
        }
        if (it->default_value_string) {
            fprintf(out, "default: %s", it->default_value_string);
        }
        fprintf(out, "\n");
    }
}
