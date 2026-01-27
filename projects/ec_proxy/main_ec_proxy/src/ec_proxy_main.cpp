/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "pzmq.hpp"
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <base64.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <modbus.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/select.h>

#include <stdio.h>
#include <utmp.h>
#include <unistd.h>

// #include "cmdline.hpp"
#include <fmt/core.h>
#include <fmt/format.h>
#include "json.hpp"
#include "sample_log.h"
#include <glob.h>

#include "tbl_yield.h"

int main_exit_flage = 0;
static void __sigint(int iSigNo)
{
    printf("llm_ec_prox will be exit!\n");
    main_exit_flage = 1;
}

#define CONFIG_AUTO_SET(obj, key)             \
    if (config_body.contains(#key))           \
        mode_config_.key = config_body[#key]; \
    else if (obj.contains(#key))              \
        mode_config_.key = obj[#key];

using namespace StackFlows;

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <arpa/inet.h>

typedef struct ifconfig_s {
    char name[128];
    char ip[16];
    char mask[16];
    char broadcast[16];
    char mac[20];
} ifconfig_t;

int ifconfig(std::vector<ifconfig_t> &ifcs)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return -10;
    }

    struct ifconf ifc;
    char buf[1024];
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;

    int iRet = ioctl(sock, SIOCGIFCONF, &ifc);
    if (iRet != 0) {
        close(sock);
        return iRet;
    }

    int cnt = ifc.ifc_len / sizeof(struct ifreq);
    // printf("ifc.size=%d\n", cnt);
    if (cnt == 0) {
        close(sock);
        return -20;
    }

    struct ifreq ifr;
    ifcs.clear();
    ifconfig_t tmp;
    for (int i = 0; i < cnt; ++i) {
        // name
        strcpy(ifr.ifr_name, ifc.ifc_req[i].ifr_name);
        // printf("name: %s\n", ifr.ifr_name);
        strncpy(tmp.name, ifr.ifr_name, sizeof(tmp.name));
        // flags
        // iRet = ioctl(sock, SIOCGIFFLAGS, &ifr);
        // short flags = ifr.ifr_flags;
        // addr
        iRet                     = ioctl(sock, SIOCGIFADDR, &ifr);
        struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
        char *ip                 = inet_ntoa(addr->sin_addr);
        // printf("ip: %s\n", ip);
        strncpy(tmp.ip, ip, sizeof(tmp.ip));
        // netmask
        iRet          = ioctl(sock, SIOCGIFNETMASK, &ifr);
        addr          = (struct sockaddr_in *)&ifr.ifr_netmask;
        char *netmask = inet_ntoa(addr->sin_addr);
        // printf("netmask: %s\n", netmask);
        strncpy(tmp.mask, netmask, sizeof(tmp.mask));
        // broadaddr
        iRet            = ioctl(sock, SIOCGIFBRDADDR, &ifr);
        addr            = (struct sockaddr_in *)&ifr.ifr_broadaddr;
        char *broadaddr = inet_ntoa(addr->sin_addr);
        // printf("broadaddr: %s\n", broadaddr);
        strncpy(tmp.broadcast, broadaddr, sizeof(tmp.broadcast));
        // hwaddr
        iRet = ioctl(sock, SIOCGIFHWADDR, &ifr);
        snprintf(tmp.mac, sizeof(tmp.mac), "%02x:%02x:%02x:%02x:%02x:%02x", (unsigned char)ifr.ifr_hwaddr.sa_data[0],
                 (unsigned char)ifr.ifr_hwaddr.sa_data[1], (unsigned char)ifr.ifr_hwaddr.sa_data[2],
                 (unsigned char)ifr.ifr_hwaddr.sa_data[3], (unsigned char)ifr.ifr_hwaddr.sa_data[4],
                 (unsigned char)ifr.ifr_hwaddr.sa_data[5]);
        // printf("mac: %s\n", tmp.mac);
        // printf("\n");

        if (strcmp(tmp.ip, "0.0.0.0") == 0 || strcmp(tmp.ip, "127.0.0.1") == 0 ||
            strcmp(tmp.mac, "00:00:00:00:00:00") == 0) {
            continue;
        }

        ifcs.push_back(tmp);
    }

    close(sock);
    return 0;
}

class llm_ec_prox {
private:
    std::string ec_prox_event_channel = "ipc:///tmp/llm/ec_prox.event.socket";
    modbus_t *modbus_ctx;
    std::mutex modbus_mtx_;
    typedef struct {
        int reg_index;    // 16位寄存器索引
        int byte_offset;  // 在寄存器中字节偏移（0-低字节，1-高字节）
    } RegPosition;
    std::unique_ptr<pzmq> pub_ctx_;
    std::unique_ptr<pzmq> rpc_ctx_;
    std::string StackFlow_pack_ERROR_temp =
        R"format({{ "created":{},"error":{{"code":{},"message":{}}},"object":{},"request_id":{},"work_id":"{}" }})format";
    std::string StackFlow_pack_temp =
        R"format({{ "created":{},"data":{},"object":{},"request_id":{},"work_id":"{}" }})format";
    std::string StackFlow_pack_all_temp =
        R"format({{ "created":{},"data":{},"error":{{"code":{},"message":{}}},"object":{},"request_id":{},"work_id":"{}" }})format";
    std::string return_success_result(std::string work_id, std::string data, std::string tmp = "",
                                      std::string object = "\"\"", std::string request_id = "\"\"")
    {
        return fmt::format(tmp.empty() ? StackFlow_pack_temp : tmp, std::time(nullptr), data, object, request_id,
                           work_id);
    }
    std::string return_err_result(std::string work_id, int code, std::string data, std::string tmp = "",
                                  std::string object = "\"\"", std::string request_id = "\"\"")
    {
        return fmt::format(tmp.empty() ? StackFlow_pack_ERROR_temp : tmp, std::time(nullptr), code, data, object,
                           request_id, work_id);
    }
    static llm_ec_prox *self;
    // 输入寄存器读
    std::string get_input_registers(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data,
                                    int reg)
    {
        uint16_t tab_reg;
        int rc = modbus_read_input_registers(modbus_ctx, reg, 1, &tab_reg);
        return rc == -1 ? return_err_result(__func__, -1, "read failed")
                        : return_success_result(__func__, fmt::format("{}", tab_reg));
    }
    // 离散输入读
    std::string get_input_bit(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data, int reg)
    {
        uint8_t tab_reg;
        int rc = modbus_read_input_bits(modbus_ctx, reg, 1, &tab_reg);
        return rc == -1 ? return_err_result(__func__, -1, "read failed")
                        : return_success_result(__func__, fmt::format("{}", tab_reg));
    }
    // 线圈读
    std::string get_bit(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data, int reg)
    {
        uint8_t tab_reg;
        int rc = modbus_read_bits(modbus_ctx, reg, 1, &tab_reg);
        return rc == -1 ? return_err_result(__func__, -1, "read failed")
                        : return_success_result(__func__, fmt::format("{}", tab_reg));
    }
    // 线圈写
    std::string set_bit(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data, int reg)
    {
        int tab_reg;
        try {
            nlohmann::json fan_data = nlohmann::json::parse(data->string());
            tab_reg                 = fan_data["data"];
        } catch (...) {
            tab_reg = 1;
        }
        int rc = modbus_write_bit(modbus_ctx, reg, tab_reg);
        return rc == -1 ? return_err_result(__func__, -1, "write failed")
                        : return_success_result(__func__, fmt::format("{}", tab_reg));
    }
    // 保持寄存器读
    std::string get_register(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data, int reg)
    {
        uint16_t tab_reg;
        int rc = modbus_read_registers(modbus_ctx, reg, 1, &tab_reg);
        return rc == -1 ? return_err_result(__func__, -1, "read failed")
                        : return_success_result(__func__, fmt::format("{}", tab_reg));
    }

    // 保持寄存器写
    std::string set_register(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data, int reg)
    {
        uint16_t tab_reg;
        try {
            nlohmann::json fan_data = nlohmann::json::parse(data->string());
            tab_reg                 = (int)fan_data["data"];
        } catch (...) {
            return return_err_result(__func__, -2, "json decode failed");
        }
        int rc = modbus_write_registers(modbus_ctx, reg, 1, &tab_reg);
        return rc == -1 ? return_err_result(__func__, -1, "write failed") : return_success_result(__func__, "\"ok\"");
    }

    std::string rgb_set_register(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data, int reg)
    {
        uint16_t tab_reg;
        try {
            nlohmann::json fan_data = nlohmann::json::parse(data->string());
            tab_reg                 = (int)fan_data["data"];
        } catch (...) {
            return return_err_result(__func__, -2, "json decode failed");
        }
        rgb_old_mode = tab_reg;
        int rc       = modbus_write_registers(modbus_ctx, reg, 1, &tab_reg);
        return rc == -1 ? return_err_result(__func__, -1, "write failed") : return_success_result(__func__, "\"ok\"");
    }

    int fun_pwm_val = 200;
    std::string fun_set_pwm(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data, int reg)
    {
        uint16_t tab_reg;
        try {
            nlohmann::json fan_data = nlohmann::json::parse(data->string());
            fun_pwm_val             = (int)fan_data["data"];
        } catch (...) {
            return return_err_result(__func__, -2, "json decode failed");
        }
        if (fun_pwm_val <= 100) {
            tab_reg = fun_pwm_val;
            int rc  = modbus_write_registers(modbus_ctx, reg, 1, &tab_reg);
            return rc == -1 ? return_err_result(__func__, -1, "write failed")
                            : return_success_result(__func__, "\"ok\"");
        } else {
            return return_err_result(__func__, -1, "fun_pwm is over 100, will enable auto fun control");
        }
    }

    std::string board_get_power_info(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data)
    {
        // mbpoll -m rtu -b 115200 -P none -a 1 -r 1 -t 3 -c 13 -l 10 /dev/ttyS3
        uint16_t tab_reg[12];
        int rc = modbus_read_input_registers(modbus_ctx, 0, 12, tab_reg);
        if (rc == -1) {
            return return_err_result(__func__, -1, "read failed");
        } else {
            return return_success_result(
                __func__,
                fmt::format(
                    R"format({{ "pcie0_mv": {} , "pcie0_ma": {} , "pcie1_mv": {} , "pcie1_ma": {} , "usb1_mv": {} , "usb1_ma": {} , "usb2_mv": {} , "usb2_ma": {} , "INVDD_mv": {} , "INVDD_ma": {} , "EXTVDD_mv": {} , "EXTVDD_ma": {} }})format",
                    tab_reg[0], tab_reg[1], tab_reg[2], tab_reg[3], tab_reg[4], tab_reg[5], tab_reg[6], tab_reg[7],
                    tab_reg[8], tab_reg[9], tab_reg[10], tab_reg[11]));
        }
    }

    uint16_t rgb888_to_rgb565(uint32_t rgb888)
    {
        // 提取红、绿、蓝分量
        uint8_t r = (rgb888 >> 16) & 0xFF;
        uint8_t g = (rgb888 >> 8) & 0xFF;
        uint8_t b = rgb888 & 0xFF;

        // RGB888到RGB565转换
        uint16_t r5 = (r >> 3) & 0x1F;  // 取高5位
        uint16_t g6 = (g >> 2) & 0x3F;  // 取高6位
        uint16_t b5 = (b >> 3) & 0x1F;  // 取高5位

        // 拼接输出（高位到低位：红5、绿6、蓝5）
        uint16_t rgb565 = (r5 << 11) | (g6 << 5) | b5;

        return rgb565;
    }
    uint32_t rgb565_to_rgb888(uint16_t rgb565)
    {
        // 提取各通道分量
        uint8_t r5 = (rgb565 >> 11) & 0x1F;  // 高5位红
        uint8_t g6 = (rgb565 >> 5) & 0x3F;   // 中6位绿
        uint8_t b5 = rgb565 & 0x1F;          // 低5位蓝

        // 将5-6位扩展成8位
        uint8_t r8 = (r5 << 3) | (r5 >> 2);  // 5位扩展到8位
        uint8_t g8 = (g6 << 2) | (g6 >> 4);  // 6位扩展到8位
        uint8_t b8 = (b5 << 3) | (b5 >> 2);  // 5位扩展到8位

        // 拼装成RGB888的0x00RRGGBB格式
        uint32_t rgb888 = (r8 << 16) | (g8 << 8) | b8;
        return rgb888;
    }
    std::string rgb_get_color(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data)
    {
        int rgb_index = 0;
        int rgb_color = 0;
        try {
            nlohmann::json fan_data = nlohmann::json::parse(data->string());
            rgb_index               = (int)fan_data["data"];
        } catch (...) {
            return return_err_result(__func__, -2, "json decode failed");
        }
        uint16_t tab_reg;
        int rc = modbus_read_registers(modbus_ctx, rgb_index + 63, 1, &tab_reg);
        if (rc == -1) {
            return return_err_result(__func__, -1, "read failed");
        } else {
            rgb_color = rgb565_to_rgb888(tab_reg);
            return return_success_result(__func__, fmt::format("{}", rgb_color));
        }
    }

    std::string rgb_set_color(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data)
    {
        int rgb_index = 0;
        int rgb_color = 0;
        try {
            nlohmann::json fan_data = nlohmann::json::parse(data->string());
            rgb_index               = (int)fan_data["data"]["rgb_index"];
            rgb_color               = (int)fan_data["data"]["rgb_color"];
        } catch (...) {
            return return_err_result(__func__, -2, "json decode failed");
        }
        if (rgb_index == -1) {
            uint16_t tab_reg = rgb888_to_rgb565(rgb_color);
            uint16_t tab_regs[64];
            uint16_t tab_reg_size;
            for (int i = 0; i < 64; i++) tab_regs[i] = tab_reg;
            int rc = modbus_read_registers(modbus_ctx, 10, 1, &tab_reg_size);
            if (rc == -1) return return_err_result(__func__, -1, "write failed");
            rc = modbus_write_registers(modbus_ctx, 63, tab_reg_size, tab_regs);
            return rc == -1 ? return_err_result(__func__, -1, "write failed")
                            : return_success_result(__func__, "\"ok\"");
        } else {
            uint16_t tab_reg = rgb888_to_rgb565(rgb_color);
            int rc           = modbus_write_registers(modbus_ctx, rgb_index + 63, 1, &tab_reg);
            return rc == -1 ? return_err_result(__func__, -1, "write failed")
                            : return_success_result(__func__, "\"ok\"");
        }
    }

    std::string lcd_set_ram(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data)
    {
        std::string rgb_ram_base64;
        try {
            nlohmann::json fan_data = nlohmann::json::parse(data->string());
            rgb_ram_base64          = fan_data["data"];
        } catch (...) {
            return return_err_result(__func__, -2, "json decode failed");
        }
        uint16_t *rgb_ram = (uint16_t *)malloc(((BASE64_DECODE_OUT_SIZE(rgb_ram_base64.length()) / 2) + 1) * 2);
        int ram_size      = base64_decode(rgb_ram_base64.c_str(), rgb_ram_base64.length(), (uint8_t *)rgb_ram);
        ram_size          = ram_size > 1870 ? 1870 : ram_size;
        int rc            = modbus_write_registers(modbus_ctx, 255, ram_size / 2, rgb_ram);
        free(rgb_ram);
        return rc == -1 ? return_err_result(__func__, -1, "write failed") : return_success_result(__func__, "\"ok\"");
    }

    std::string ip_get(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data, int port)
    {
        uint32_t value      = 0;
        uint16_t tab_reg[2] = {0};

        int rc = modbus_read_registers(modbus_ctx, 15 + port * 2, 2, tab_reg);
        if (rc == -1) {
            return return_err_result(__func__, -1, "read failed");
        } else {
            for (int i = 0; i < 2; i++) tab_reg[i] = ntohs(tab_reg[i]);
            value = ((uint32_t)tab_reg[0] << 16) | tab_reg[1];
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &value, ip_str, INET_ADDRSTRLEN);
            return return_success_result(__func__, fmt::format("\"{}\"", ip_str));
        }
    }
    std::string ip_set(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data, int port)
    {
        uint32_t value;
        try {
            nlohmann::json fan_data = nlohmann::json::parse(data->string());
            std::string ip_str      = fan_data["data"];
            if (inet_pton(AF_INET, ip_str.c_str(), &value) != 1) {
                return std::string("Error: Invalid IP address\n");
            }
        } catch (...) {
            return return_err_result(__func__, -2, "json decode failed");
        }
        uint16_t tab_reg[2];
        tab_reg[0] = (value >> 16) & 0xFFFF;
        tab_reg[1] = value & 0xFFFF;
        for (int i = 0; i < 2; i++) tab_reg[i] = htons(tab_reg[i]);
        int rc = modbus_write_registers(modbus_ctx, 15 + port * 2, 2, tab_reg);
        return rc == -1 ? return_err_result(__func__, -1, "write failed") : return_success_result(__func__, "\"ok\"");
    }

    std::string i2c_get_reg(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data)
    {
        int addr = 0;
        int reg  = 0;
        try {
            nlohmann::json fan_data = nlohmann::json::parse(data->string());
            addr                    = (int)fan_data["data"]["addr"];
            reg                     = (int)fan_data["data"]["reg"];
        } catch (...) {
            return return_err_result(__func__, -2, "json decode failed;-D {\"data\":{\"addr\":1,\"reg\":1}}    ");
        }
        uint16_t tab_reg = (addr & 0xff) << 8 | (reg & 0xff);
        int rc           = modbus_write_registers(modbus_ctx, 7, 1, &tab_reg);
        rc               = modbus_read_registers(modbus_ctx, 8, 2, &tab_reg);
        if (rc == -1) {
            return return_err_result(__func__, -1, "read failed");
        } else {
            return return_success_result(__func__, fmt::format("{}", tab_reg));
        }
    }

    std::string i2c_set_reg(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data)
    {
        int addr  = 0;
        int reg   = 0;
        int value = 0;
        try {
            nlohmann::json fan_data = nlohmann::json::parse(data->string());
            addr                    = (int)fan_data["data"]["addr"];
            reg                     = (int)fan_data["data"]["reg"];
            value                   = (int)fan_data["data"]["value"];
        } catch (...) {
            return return_err_result(__func__, -2,
                                     "json decode failed ;-D {\"data\":{\"addr\":1,\"reg\":1,\"value\":1}}    ");
        }
        uint16_t tab_reg = (addr & 0xff) << 8 | (reg & 0xff);
        int rc           = modbus_write_registers(modbus_ctx, 7, 1, &tab_reg);
        tab_reg          = value & 0xff;
        rc               = modbus_write_registers(modbus_ctx, 9, 2, &tab_reg);
        if (rc == -1) {
            return return_err_result(__func__, -1, "write failed");
        } else {
            return return_success_result(__func__, "\"Ok\"");
        }
    }

    std::string poweron_time(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data, int reg)
    {
        if (reg == 0) {
            uint32_t value      = 0;
            uint16_t tab_reg[2] = {0};

            int rc = modbus_read_registers(modbus_ctx, 5, 2, tab_reg);
            if (rc == -1) {
                return return_err_result(__func__, -1, "read failed");
            } else {
                value = ((uint32_t)tab_reg[0] << 16) | tab_reg[1];
                return return_success_result(__func__, fmt::format("{}", value));
            }
        } else {
            uint32_t value;
            try {
                nlohmann::json fan_data = nlohmann::json::parse(data->string());
                value                   = (int)fan_data["data"];
            } catch (...) {
                return return_err_result(__func__, -2, "json decode failed");
            }
            uint16_t tab_reg[2];
            tab_reg[0] = (value >> 16) & 0xFFFF;
            tab_reg[1] = value & 0xFFFF;
            int rc     = modbus_write_registers(modbus_ctx, 5, 2, tab_reg);
            return rc == -1 ? return_err_result(__func__, -1, "write failed")
                            : return_success_result(__func__, "\"ok\"");
        }
    }

    int ec_button[4] = {0};
    std::string ec_fun_auto(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data, int set)
    {
        if (set == 0) {
            return return_success_result(__func__, fmt::format("{}", ec_button[3]));
        } else {
            uint32_t value;
            try {
                nlohmann::json fan_data = nlohmann::json::parse(data->string());
                value                   = (int)fan_data["data"];
            } catch (...) {
                return return_err_result(__func__, -2, "json decode failed");
            }
            ec_button[3] = value;
            return return_success_result(__func__, "\"ok\"");
        }
    }

    std::string ec_button_head(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data, int set)
    {
        if (set == 0) {
            return return_success_result(__func__, fmt::format("{}", ec_button[0]));
        } else {
            uint32_t value;
            try {
                nlohmann::json fan_data = nlohmann::json::parse(data->string());
                value                   = (int)fan_data["data"];

            } catch (...) {
                return return_err_result(__func__, -2, "json decode failed");
            }
            ec_button[0] = value;
            return return_success_result(__func__, "\"ok\"");
        }
    }
    std::string soc_button_head(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data, int set)
    {
        if (set == 0) {
            return return_success_result(__func__, fmt::format("{}", ec_button[2]));
        } else {
            uint32_t value;
            try {
                nlohmann::json fan_data = nlohmann::json::parse(data->string());
                value                   = (int)fan_data["data"];

            } catch (...) {
                return return_err_result(__func__, -2, "json decode failed");
            }
            ec_button[2] = value;
            return return_success_result(__func__, "\"ok\"");
        }
    }
    std::string ec_button_lcd(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data, int set)
    {
        if (set == 0) {
            return return_success_result(__func__, fmt::format("{}", ec_button[1]));
        } else {
            uint32_t value;
            try {
                nlohmann::json fan_data = nlohmann::json::parse(data->string());
                value                   = (int)fan_data["data"];

            } catch (...) {
                return return_err_result(__func__, -2, "json decode failed");
            }
            ec_button[1] = value;
            return return_success_result(__func__, "\"ok\"");
        }
    }
    void set_modbus_speed(int modbus_speed)
    {
        uint16_t tab_reg[2];
        tab_reg[0] = (modbus_speed >> 16) & 0xFFFF;
        tab_reg[1] = modbus_speed & 0xFFFF;
        int rc     = modbus_write_registers(modbus_ctx, 3, 2, tab_reg);
        if (rc == -1) {
            fprintf(stderr, "Write failed: %s\n", modbus_strerror(errno));
            std::exit(-1);
        }
        modbus_close(modbus_ctx);
        modbus_free(modbus_ctx);
        modbus_ctx = NULL;
        usleep(100 * 1000);
        modbus_ctx = modbus_new_rtu("/dev/ttyS3", modbus_speed, 'N', 8, 1);
        if (modbus_ctx == NULL) {
            fprintf(stderr, "Unable to create the context\n");
            std::exit(-1);
        }
        modbus_set_slave(modbus_ctx, 1);  // 设置 Modbus 从站地址为1
        if (modbus_connect(modbus_ctx) == -1) {
            fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
            modbus_free(modbus_ctx);
            std::exit(-1);
        }
        _modbus_speed = modbus_speed;
    }
    int _modbus_speed = 921600;

    void _init_modbus()
    {
        modbus_ctx = modbus_new_rtu("/dev/ttyS3", 921600, 'N', 8, 1);
        if (modbus_ctx == NULL) {
            fprintf(stderr, "Unable to create the context\n");
            std::exit(-1);
        }
        modbus_set_slave(modbus_ctx, 1);  // 设置 Modbus 从站地址为1
        if (modbus_connect(modbus_ctx) == -1) {
            fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
            modbus_free(modbus_ctx);
            std::exit(-1);
        }
    }

    std::string _None(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data)
    {
        return return_success_result(__func__, "\"None\"");
    }

    std::string ec_modbus_get_bit(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data)
    {
        int index;
        try {
            nlohmann::json fan_data = nlohmann::json::parse(data->string());
            index                   = fan_data["data"];
        } catch (...) {
            return return_err_result(__func__, -2, "json decode failed");
        }
        uint8_t tab_reg;
        int rc = modbus_read_bits(modbus_ctx, index, 1, &tab_reg);
        return rc == -1 ? return_err_result(__func__, -1, "read failed")
                        : return_success_result(__func__, fmt::format("{}", tab_reg));
    }
    std::string ec_modbus_set_bit(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data)
    {
        int index;
        int value;
        try {
            nlohmann::json fan_data = nlohmann::json::parse(data->string());
            index                   = fan_data["data"]["index"];
            value                   = fan_data["data"]["value"];
        } catch (...) {
            return return_err_result(__func__, -2, "json decode failed ;-D {\"data\":{\"index\":1, \"value\":1}}");
        }
        int rc = modbus_write_bit(modbus_ctx, index, value);
        return rc == -1 ? return_err_result(__func__, -1, "read failed")
                        : return_success_result(__func__, fmt::format("{}", value));
    }

    std::string ec_modbus_get_input_bits(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data)
    {
        int index;
        try {
            nlohmann::json fan_data = nlohmann::json::parse(data->string());
            index                   = fan_data["data"];
        } catch (...) {
            return return_err_result(__func__, -2, "json decode failed");
        }
        uint8_t tab_reg;
        int rc = modbus_read_input_bits(modbus_ctx, index, 1, &tab_reg);
        return rc == -1 ? return_err_result(__func__, -1, "read failed")
                        : return_success_result(__func__, fmt::format("{}", tab_reg));
    }
    std::string ec_modbus_get_input_registers(StackFlows::pzmq *_pzmq,
                                              const std::shared_ptr<StackFlows::pzmq_data> &data)
    {
        int index;
        try {
            nlohmann::json fan_data = nlohmann::json::parse(data->string());
            index                   = fan_data["data"];
        } catch (...) {
            return return_err_result(__func__, -2, "json decode failed");
        }
        uint16_t tab_reg;
        int rc = modbus_read_input_registers(modbus_ctx, index, 1, &tab_reg);
        return rc == -1 ? return_err_result(__func__, -1, "read failed")
                        : return_success_result(__func__, fmt::format("{}", tab_reg));
    }

    std::string ec_modbus_get_hold_registers(StackFlows::pzmq *_pzmq,
                                             const std::shared_ptr<StackFlows::pzmq_data> &data)
    {
        int index;
        try {
            nlohmann::json fan_data = nlohmann::json::parse(data->string());
            index                   = fan_data["data"];
        } catch (...) {
            return return_err_result(__func__, -2, "json decode failed");
        }
        uint16_t tab_reg;
        int rc = modbus_read_registers(modbus_ctx, index, 1, &tab_reg);
        return rc == -1 ? return_err_result(__func__, -1, "read failed")
                        : return_success_result(__func__, fmt::format("{}", tab_reg));
    }
    std::string ec_modbus_set_hold_registers(StackFlows::pzmq *_pzmq,
                                             const std::shared_ptr<StackFlows::pzmq_data> &data)
    {
        int index;
        int value;
        try {
            nlohmann::json fan_data = nlohmann::json::parse(data->string());
            index                   = fan_data["data"]["index"];
            value                   = fan_data["data"]["value"];
        } catch (...) {
            return return_err_result(__func__, -2, "json decode failed ;-D {\"data\":{\"index\":1, \"value\":1}}");
        }
        uint16_t tab_reg = value;
        int rc           = modbus_write_registers(modbus_ctx, index, 1, &tab_reg);
        return rc == -1 ? return_err_result(__func__, -1, "read failed")
                        : return_success_result(__func__, fmt::format("{}", tab_reg));
    }

    std::string pd_info(StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data)
    {
        int addr                  = 8;
        int reg                   = 0;
        uint16_t tab_reg          = (addr & 0xff) << 8 | (reg & 0xff);
        int rc                    = modbus_write_registers(modbus_ctx, 7, 1, &tab_reg);
        rc                        = modbus_read_registers(modbus_ctx, 8, 2, &tab_reg);
        std::string voltage_map[] = {"Unattached", "5 V",  "9 V",  "12 V", "15 V", "18 V", "20 V", "NULL",
                                     "NULL",       "NULL", "NULL", "NULL", "NULL", "NULL", "NULL", "NULL"};
        std::string current_map[] = {"0.5 A", "0.7 A",  "1 A", "1.25 A", "1.5 A", "1.75 A", "2 A",   "2.25 A",
                                     "2.5 A", "2.75 A", "3 A", "3.25 A", "3.5 A", "4 A",    "4.5 A", "5 A"};
        if (rc == -1) {
            return return_err_result(__func__, -1, "read failed");
        } else {
            return return_success_result(
                __func__, fmt::format("{{\"voltage\":\"{}\",\"current\":\"{}\"}}", voltage_map[(tab_reg >> 4) & 0x0f],
                                      current_map[tab_reg & 0x0F]));
        }
    }

public:
    llm_ec_prox()
    {
        // setup("", "audio.play", "{\"None\":\"None\"}");
        self = this;
        // prob modbus speed
        _init_modbus();
        _init_ec();
        pub_ctx_ = std::make_unique<pzmq>("ipc:///tmp/llm/ec_prox.event.socket", ZMQ_PUB);
        rpc_ctx_ = std::make_unique<pzmq>("ipc:///tmp/rpc.ec_prox", ZMQ_RPC_FUN);

        // clang-format off
        rpc_ctx_->register_rpc_action("setup", std::bind(&llm_ec_prox::_None, this, std::placeholders::_1, std::placeholders::_2));
        rpc_ctx_->register_rpc_action("pause", std::bind(&llm_ec_prox::_None, this, std::placeholders::_1, std::placeholders::_2));
        rpc_ctx_->register_rpc_action("work", std::bind(&llm_ec_prox::_None, this, std::placeholders::_1, std::placeholders::_2));
        rpc_ctx_->register_rpc_action("exit", std::bind(&llm_ec_prox::_None, this, std::placeholders::_1, std::placeholders::_2));
        rpc_ctx_->register_rpc_action("link", std::bind(&llm_ec_prox::_None, this, std::placeholders::_1, std::placeholders::_2));
        rpc_ctx_->register_rpc_action("unlink", std::bind(&llm_ec_prox::_None, this, std::placeholders::_1, std::placeholders::_2));
        rpc_ctx_->register_rpc_action("taskinfo", std::bind(&llm_ec_prox::_None, this, std::placeholders::_1, std::placeholders::_2));
        
#define REGISTER_RPC_ACTION(name, func) \
        rpc_ctx_->register_rpc_action(name, [this](StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data){ \
            std::unique_lock<std::mutex> lock(this->modbus_mtx_); \
            return this->func(_pzmq, data); \
        });

#define REGISTER_RPC_ACTION_REG(name, func, reg) \
        rpc_ctx_->register_rpc_action(name, [this](StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data){ \
            std::unique_lock<std::mutex> lock(this->modbus_mtx_); \
            return this->func(_pzmq, data, reg); \
        });


        REGISTER_RPC_ACTION_REG("fan_get_speed", get_input_registers, 12);
        REGISTER_RPC_ACTION_REG("version", get_input_registers, 13);
        REGISTER_RPC_ACTION_REG("fan_get_pwm", get_register, 1);
        REGISTER_RPC_ACTION_REG("rgb_get_mode", get_register, 11);
        REGISTER_RPC_ACTION_REG("rgb_get_size", get_register, 10);
        REGISTER_RPC_ACTION_REG("lcd_get_mode", get_register, 13);
        REGISTER_RPC_ACTION_REG("lcd_get_brightness", get_register, 12);
        REGISTER_RPC_ACTION_REG("poweroff_get_time", get_register, 0);

        REGISTER_RPC_ACTION_REG("fan_set_pwm", fun_set_pwm, 1);
        REGISTER_RPC_ACTION_REG("rgb_set_mode", rgb_set_register, 11);
        REGISTER_RPC_ACTION_REG("rgb_set_size", set_register, 10);
        REGISTER_RPC_ACTION_REG("lcd_set_mode", set_register, 13);
        REGISTER_RPC_ACTION_REG("lcd_set_brightness", set_register, 12);
        REGISTER_RPC_ACTION_REG("poweroff_set_time", set_register, 0);

        REGISTER_RPC_ACTION_REG("lcd_putc", set_register, 14);


        REGISTER_RPC_ACTION_REG("vddcpu_get", get_register, 2);
        REGISTER_RPC_ACTION_REG("vddcpu_set", set_register, 2);

        REGISTER_RPC_ACTION("rgb_get_color",                rgb_get_color);
        REGISTER_RPC_ACTION("rgb_set_color",                rgb_set_color);
        REGISTER_RPC_ACTION("board_get_power_info",         board_get_power_info);
        REGISTER_RPC_ACTION_REG("eth0_ip_get",                       ip_get, 0);
        REGISTER_RPC_ACTION_REG("eth0_ip_set",                       ip_set, 0);
        REGISTER_RPC_ACTION_REG("eth1_ip_get",                       ip_get, 1);
        REGISTER_RPC_ACTION_REG("eth1_ip_set",                       ip_set, 1);
        REGISTER_RPC_ACTION_REG("wlan_ip_get",                       ip_get, 2);
        REGISTER_RPC_ACTION_REG("wlan_ip_set",                       ip_set, 2);



        REGISTER_RPC_ACTION("lcd_set_ram",                  lcd_set_ram);
        REGISTER_RPC_ACTION_REG("poweron_get_time", poweron_time, 0);
        REGISTER_RPC_ACTION_REG("poweron_set_time", poweron_time, 1);

        //将下面的REGISTER_RPC_ACTION转换成REGISTER_RPC_ACTION_REG，注意查寻相关寄存器
        REGISTER_RPC_ACTION_REG("ext_power", set_bit, 0);
        REGISTER_RPC_ACTION_REG("board_poweroff", set_bit, 1);
        REGISTER_RPC_ACTION_REG("pcie0_set_switch", set_bit, 4);
        REGISTER_RPC_ACTION_REG("pcie1_set_switch", set_bit, 5);
        REGISTER_RPC_ACTION_REG("gl3510_reset", set_bit, 6);
        REGISTER_RPC_ACTION_REG("usbds1_set_big_power", set_bit, 7);
        REGISTER_RPC_ACTION_REG("usbds2_set_big_power", set_bit, 8);
        REGISTER_RPC_ACTION_REG("usbds1_set_switch", set_bit, 9);
        REGISTER_RPC_ACTION_REG("usbds2_set_switch", set_bit, 10);
        REGISTER_RPC_ACTION_REG("usbds3_set_switch", set_bit, 11);
        REGISTER_RPC_ACTION_REG("grove_iic_set_switch", set_bit, 12);
        REGISTER_RPC_ACTION_REG("grove_uart_set_switch", set_bit, 13);
        REGISTER_RPC_ACTION_REG("flash_save_switch", set_bit, 14);
        REGISTER_RPC_ACTION_REG("flash_save_value", set_bit, 15);
        REGISTER_RPC_ACTION_REG("poweroff", set_bit, 16);
        
        REGISTER_RPC_ACTION("i2c_set_reg",                  i2c_set_reg);
        REGISTER_RPC_ACTION("i2c_get_reg",                  i2c_get_reg);

        ec_button[2] = 1;
        REGISTER_RPC_ACTION_REG("ec_button_get_head_event", ec_button_head, 0);
        REGISTER_RPC_ACTION_REG("ec_button_set_head_event", ec_button_head, 1);
        REGISTER_RPC_ACTION_REG("ec_button_get_lcd_event", ec_button_lcd, 0);
        REGISTER_RPC_ACTION_REG("ec_button_set_lcd_event", ec_button_lcd, 1);
        REGISTER_RPC_ACTION_REG("soc_button_get_head_event", soc_button_head, 0);
        REGISTER_RPC_ACTION_REG("soc_button_set_head_event", soc_button_head, 1);
        ec_button[3] = 1;
        REGISTER_RPC_ACTION_REG("fun_get_auto", ec_fun_auto, 0);
        REGISTER_RPC_ACTION_REG("fun_set_auto", ec_fun_auto, 1);


        REGISTER_RPC_ACTION("ec_modbus_get_bit",                ec_modbus_get_bit);
        REGISTER_RPC_ACTION("ec_modbus_set_bit",                ec_modbus_set_bit);
        
        REGISTER_RPC_ACTION("ec_modbus_get_input_bits",                ec_modbus_get_input_bits);
        REGISTER_RPC_ACTION("ec_modbus_get_input_registers",           ec_modbus_get_input_registers);

        REGISTER_RPC_ACTION("ec_modbus_get_hold_registers",                ec_modbus_get_hold_registers);
        REGISTER_RPC_ACTION("ec_modbus_set_hold_registers",                ec_modbus_set_hold_registers);  
        
        REGISTER_RPC_ACTION("pd_power_info",                pd_info);

        REGISTER_RPC_ACTION_REG("pcie0_exists",         get_input_bit, 0);
        REGISTER_RPC_ACTION_REG("pcie1_exists",         get_input_bit, 1);
        REGISTER_RPC_ACTION_REG("V3_3_good",         get_input_bit, 2);
        REGISTER_RPC_ACTION_REG("V1_8_good",         get_input_bit, 3);
        REGISTER_RPC_ACTION_REG("head_button",         get_input_bit, 4);
        REGISTER_RPC_ACTION_REG("lcd_button",         get_input_bit, 8);




#undef REGISTER_RPC_ACTION
#undef REGISTER_RPC_ACTION_REG
        // clang-format on
    }
    // int setup(const std::string &work_id, const std::string &object, const std::string &data) override
    // {
    //     send(std::string("None"), std::string("None"), std::string(""), unit_name_);
    //     return -1;
    // }
    static inline uint64_t get_uptime_ms(void)
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);  // 单调时钟：适合计算运行时/耗时
        return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
    }

    void eth_ec_thread()
    {
        static uint32_t value_old[3] = {0};
        static int value_status[3]   = {0};
        static uint64_t T_last       = 0;
        CORO_BEGIN(eth_ec);
        for (;;) {
            if (get_uptime_ms() - T_last < 2000) {
                CORO_YIELD(eth_ec);
                continue;
            }
            T_last = get_uptime_ms();
            memset(value_status, 0, sizeof(value_status));
            std::vector<ifconfig_t> eth_info;
            ifconfig(eth_info);
            for (auto eth : eth_info) {
                if (std::string(eth.name) == "eth0") {
                    value_status[0] = 1;
                    uint32_t value  = inet_addr(eth.ip);
                    if (value == value_old[0]) {
                        continue;
                    }
                    value_old[0] = value;
                    uint16_t tab_reg[2];
                    tab_reg[1] = (value >> 16) & 0xFFFF;
                    tab_reg[0] = value & 0xFFFF;
                    for (int i = 0; i < 2; i++) tab_reg[i] = htons(tab_reg[i]);
                    int rc = modbus_write_registers(modbus_ctx, 15 + 0 * 2, 2, tab_reg);
                }
                if (std::string(eth.name) == "eth1") {
                    value_status[1] = 1;
                    // std::cout << eth.name << " : " << eth.ip << std::endl;
                    uint32_t value = inet_addr(eth.ip);
                    if (value == value_old[1]) {
                        continue;
                    }
                    value_old[1] = value;
                    uint16_t tab_reg[2];
                    tab_reg[1] = (value >> 16) & 0xFFFF;
                    tab_reg[0] = value & 0xFFFF;
                    for (int i = 0; i < 2; i++) tab_reg[i] = htons(tab_reg[i]);
                    int rc = modbus_write_registers(modbus_ctx, 15 + 1 * 2, 2, tab_reg);
                    // std::cout << eth.name << " :--- " << eth.ip << std::endl;
                }
                if (std::string(eth.name) == "wlan0") {
                    value_status[2] = 1;
                    uint32_t value  = inet_addr(eth.ip);
                    if (value == value_old[2]) {
                        continue;
                    }
                    value_old[2] = value;
                    uint16_t tab_reg[2];
                    tab_reg[1] = (value >> 16) & 0xFFFF;
                    tab_reg[0] = value & 0xFFFF;
                    for (int i = 0; i < 2; i++) tab_reg[i] = htons(tab_reg[i]);
                    int rc = modbus_write_registers(modbus_ctx, 15 + 2 * 2, 2, tab_reg);
                }
            }
            if (value_status[0] == 0 && value_old[0] != 0) {
                value_old[0]        = 0;
                uint16_t tab_reg[2] = {0};
                int rc              = modbus_write_registers(modbus_ctx, 15 + 0 * 2, 2, tab_reg);
            }
            if (value_status[1] == 0 && value_old[1] != 0) {
                value_old[1]        = 0;
                uint16_t tab_reg[2] = {0};
                int rc              = modbus_write_registers(modbus_ctx, 15 + 1 * 2, 2, tab_reg);
            }
            if (value_status[2] == 0 && value_old[2] != 0) {
                value_old[2]        = 0;
                uint16_t tab_reg[2] = {0};
                int rc              = modbus_write_registers(modbus_ctx, 15 + 2 * 2, 2, tab_reg);
            }
        }
        CORO_END(eth_ec);
    }

    void buttons_ec_thread()
    {
        static int fd = 0;
        struct input_event ev;
        fd_set readfds;
        int ret;
        CORO_BEGIN(ec);
        for (;;) {
            if (ec_button[0]) {
                uint8_t tab_reg[4];
                {
                    std::unique_lock<std::mutex> lock(this->modbus_mtx_);
                    int rc = modbus_read_input_bits(modbus_ctx, 4, 4, tab_reg);
                    if (rc == -1) {
                        CORO_YIELD(ec);
                        continue;
                    }
                }
                if (tab_reg[1]) {
                    pub_ctx_->send_data(return_success_result(__func__, "{\"code\":0,\"vale\":0}"));
                }
                if (tab_reg[2]) {
                    pub_ctx_->send_data(return_success_result(__func__, "{\"code\":1,\"vale\":0}"));
                }
                if (tab_reg[3]) {
                    pub_ctx_->send_data(return_success_result(__func__, "{\"code\":2,\"vale\":0}"));
                }
            }
            if (ec_button[1]) {
                uint8_t tab_reg[4];
                {
                    std::unique_lock<std::mutex> lock(this->modbus_mtx_);
                    int rc = modbus_read_input_bits(modbus_ctx, 8, 4, tab_reg);
                    if (rc == -1) {
                        CORO_YIELD(ec);
                        continue;
                    }
                }
                if (tab_reg[1]) {
                    pub_ctx_->send_data(return_success_result(__func__, "{\"code\":0,\"vale\":1}"));
                }
                if (tab_reg[2]) {
                    pub_ctx_->send_data(return_success_result(__func__, "{\"code\":1,\"vale\":1}"));
                }
                if (tab_reg[3]) {
                    pub_ctx_->send_data(return_success_result(__func__, "{\"code\":2,\"vale\":1}"));
                }
            }
            CORO_YIELD(ec);
            continue;
        }
        CORO_END(ec);
    }

    void buttons_thread()
    {
        static int fd = 0;
        struct input_event ev;
        fd_set readfds;
        int ret;
        if (!ec_button[2]) {
            usleep(100000);
            return;
        }
        CORO_BEGIN(buttons);
        for (;;) {
            if (fd == 0) {
                fd = open("/dev/input/by-path/platform-buttons-event", O_RDONLY);
                if (fd < 0) {
                    fd = 0;
                    CORO_YIELD(buttons);
                    usleep(100000);
                    continue;
                }
            }
            FD_ZERO(&readfds);
            FD_SET(fd, &readfds);
            struct timeval tv = {0};
            tv.tv_usec        = 100000;  // 0.1秒
            ret               = select(fd + 1, &readfds, NULL, NULL, &tv);
            if (ret > 0) {
                if (FD_ISSET(fd, &readfds)) {
                    ssize_t n = read(fd, &ev, sizeof(ev));
                    if (n == (ssize_t)sizeof(ev)) {
                        // printf("type:%d, code:%d, value:%d\n", ev.type, ev.code, ev.value);
                        if (ev.value == 1 && ev.code == 204) {
                            pub_ctx_->send_data(return_success_result(__func__, "{\"code\":1,\"vale\":204}"));
                        }
                        if (ev.value == 0 && ev.code == 204) {
                            pub_ctx_->send_data(return_success_result(__func__, "{\"code\":0,\"vale\":204}"));
                        }
                    }
                }
            }
            if (ret < 0) {
                close(fd);
                fd = 0;
            }
            CORO_YIELD(buttons);
            continue;
        }
        CORO_END(buttons);
    }
    void fun_thread()
    {
        static int fd = 0;
        static uint16_t pwm_value_old;
        if (!ec_button[3]) {
            pwm_value_old = -1;
            return;
        }
        CORO_BEGIN(fun);
        for (;;) {
            if (fun_pwm_val <= 100) {
                CORO_YIELD(fun);
                continue;
            }
            fd = open("/sys/class/thermal/cooling_device0/fan_cur_pwm", O_RDONLY);
            if (fd < 0) {
                fd = 0;
                CORO_YIELD(fun);
                continue;
            }
            char pwm_buff[32];
            ssize_t n = read(fd, pwm_buff, sizeof(pwm_buff));
            if (n > 0) {
                uint16_t tab_reg = atoi(pwm_buff);
                if (pwm_value_old != tab_reg) {
                    std::unique_lock<std::mutex> lock(this->modbus_mtx_);
                    modbus_write_registers(modbus_ctx, 1, 1, &tab_reg);
                    pwm_value_old = tab_reg;
                }
            }
            close(fd);
            fd = 0;
            CORO_YIELD(fun);
            continue;
        }
        CORO_END(fun);
    }

    void llm_kws_callback(pzmq *_pzmq, const std::shared_ptr<pzmq_data> &raw)
    {
        uint16_t tab_reg = 10;
        ec_button[3]     = 0;
        modbus_write_registers(modbus_ctx, 1, 1, &tab_reg);
        tab_reg = 5;
        modbus_write_registers(modbus_ctx, 11, 1, &tab_reg);
    }
    void llm_asr_callback(pzmq *_pzmq, const std::shared_ptr<pzmq_data> &raw)
    {
        std::string data = raw->string();
        // std::cout << "llm_asr_callback data: " << data << std::endl;
        std::size_t pos = data.find("\"finish\"");
        // std::cout << "substr data: " << data.substr(pos, 16) << std::endl;
        if ((pos != std::string::npos) && (data.substr(pos, 16).find("true") != std::string::npos)) {
            ec_button[3]     = 1;
            uint16_t tab_reg = rgb_old_mode;
            modbus_write_registers(modbus_ctx, 11, 1, &tab_reg);
            // std::cout << "find end" << std::endl;
        }
    }

    std::unique_ptr<pzmq> llm_asr_pub_ctx_;
    std::unique_ptr<pzmq> llm_kws_pub_ctx_;
    void llm_status_thread()
    {
        const char *asr_pattern = "/tmp/llm/*.sock.asr.output_url";
        const char *kws_pattern = "/tmp/llm/*.sock.kws.output_url";
        CORO_BEGIN(llm_status);
        for (;;) {
            if (llm_asr_pub_ctx_ == nullptr) {
                glob_t glob_result;
                memset(&glob_result, 0, sizeof(glob_result));
                if (glob(asr_pattern, GLOB_TILDE, NULL, &glob_result) == 0) {
                    if (glob_result.gl_pathc > 0) {
                        llm_asr_pub_ctx_ =
                            std::make_unique<pzmq>(fmt::format("ipc://{}", glob_result.gl_pathv[0]), ZMQ_SUB,
                                                   std::bind(&llm_ec_prox::llm_asr_callback, this,
                                                             std::placeholders::_1, std::placeholders::_2));
                    }
                }
                globfree(&glob_result);
            }
            if (llm_asr_pub_ctx_ == nullptr) {
                CORO_YIELD(llm_status);
                continue;
            }
            break;
        }
        for (;;) {
            if (llm_kws_pub_ctx_ == nullptr) {
                glob_t glob_result;
                memset(&glob_result, 0, sizeof(glob_result));
                if (glob(kws_pattern, GLOB_TILDE, NULL, &glob_result) == 0) {
                    if (glob_result.gl_pathc > 0) {
                        llm_kws_pub_ctx_ =
                            std::make_unique<pzmq>(fmt::format("ipc://{}", glob_result.gl_pathv[0]), ZMQ_SUB,
                                                   std::bind(&llm_ec_prox::llm_kws_callback, this,
                                                             std::placeholders::_1, std::placeholders::_2));
                    }
                }
                globfree(&glob_result);
            }
            if (llm_kws_pub_ctx_ == nullptr) {
                CORO_YIELD(llm_status);
                continue;
            }
            break;
        }
        for (;;) {
            CORO_YIELD(llm_status);
        }
        CORO_END(llm_status);
    }

    void loop()
    {
        while (!main_exit_flage) {
            buttons_ec_thread();
            buttons_thread();
            fun_thread();
            eth_ec_thread();
            llm_status_thread();
        }
    }
    int rgb_old_mode;
    void _init_ec()
    {
        uint16_t tab_reg;

        char *env_param = getenv("AX650_EC_RGB_MODE");
        tab_reg         = env_param ? atoi(env_param) : 1;
        rgb_old_mode    = tab_reg;
        modbus_write_registers(modbus_ctx, 11, 1, &tab_reg);
        env_param = getenv("AX650_EC_LCD_MODE");
        tab_reg   = env_param ? atoi(env_param) : 2;
        modbus_write_registers(modbus_ctx, 13, 1, &tab_reg);

        tab_reg   = 30000;
        env_param = getenv("AX650_EC_POWER_OFF_TIME");
        if (env_param) {
            tab_reg = atoi(env_param);
        }
        modbus_write_registers(modbus_ctx, 0, 1, &tab_reg);
    }

    int is_poweroff_or_reboot()
    {
        FILE *fp;
        char buffer[1024];
        int is_poweroff = 0;
        int is_reboot   = 0;

        fp = popen("systemctl list-jobs", "r");
        if (fp == NULL) {
            return -1;
        }

        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
            if (strstr(buffer, "poweroff.target") != NULL) {
                is_poweroff = 1;
                break;
            }
            if (strstr(buffer, "reboot.target") != NULL) {
                is_reboot = 1;
                break;
            }
        }

        pclose(fp);

        if (is_poweroff) return 0;  // poweroff
        if (is_reboot) return 1;    // reboot
        return -1;                  // 未知
    }

    void ax650_ec_prox_exit()
    {
        // rgb 模式复位
        uint16_t tab_reg = 0;
        std::cout << "reg reset" << std::endl;
        modbus_write_registers(modbus_ctx, 11, 1, &tab_reg);
        std::cout << "lcd_reset" << std::endl;
        // lcd mode reset
        modbus_write_registers(modbus_ctx, 13, 1, &tab_reg);
        // fun full speed
        tab_reg = 100;
        modbus_write_registers(modbus_ctx, 1, 1, &tab_reg);
        {
            int runlevel = is_poweroff_or_reboot();
            if (runlevel == 0) {
                tab_reg = 1;
                modbus_write_bit(modbus_ctx, 16, tab_reg);
            }
        }
        pub_ctx_.reset();
        modbus_close(modbus_ctx);
        modbus_free(modbus_ctx);
        std::cout << "all exit!" << std::endl;
    }
    ~llm_ec_prox()
    {
        ax650_ec_prox_exit();
    }
};

llm_ec_prox *llm_ec_prox::self;
int main(int argc, char *argv[])
{
    signal(SIGTERM, __sigint);
    signal(SIGINT, __sigint);
    mkdir("/tmp/llm", 0777);
    llm_ec_prox ec_prox;
    ec_prox.loop();
    return 0;
}
