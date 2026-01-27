#include <iostream>
#include <modbus.h>
#include "pzmq.hpp"
#include "cmdline.hpp"
#include <fmt/core.h>
#include <fmt/format.h>
#include <cstdlib>
#include <list>
#include <vector>
#include "json.hpp"
int condition = 1;
const char *rpc_socket_path;
const char *pub_socket_path;

void info_fun(cmdline::parser &a, std::string set_fun, std::string get_fun)
{
    if (a.exist("data") || a.exist("DataRaw")) {
        if (set_fun.length() == 0) return;
        std::string DataRaw = a.get<std::string>("DataRaw");
        if (DataRaw.empty()) {
            DataRaw = fmt::format("{{ \"data\": {} }}", a.get<std::string>("data"));
        }
        StackFlows::pzmq Context(rpc_socket_path);
        Context.call_rpc_action(set_fun, DataRaw,
                                [set_fun](StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data) {
                                    if (data->size() > 0) {
                                        std::cout << data->string() << std::endl;
                                    } else {
                                        std::cout << "call " << set_fun << " faile!" << std::endl;
                                    }
                                });
    } else {
        if (get_fun.length() == 0) {
            std::cout << "please set data: -d or -D" << std::endl;
            return;
        }
        StackFlows::pzmq Context(rpc_socket_path);
        Context.call_rpc_action(get_fun, "{}",
                                [get_fun](StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data) {
                                    if (data->size() > 0) {
                                        std::cout << data->string() << std::endl;
                                    } else {
                                        std::cout << "call " << get_fun << " faile! " << std::endl;
                                    }
                                });
    }
}

void info_set_fun(cmdline::parser &a, std::string set_fun, std::string default_data)
{
    if (set_fun.length() == 0) return;
    std::string DataRaw = default_data;
    StackFlows::pzmq Context(rpc_socket_path);
    Context.call_rpc_action(set_fun, DataRaw,
                            [set_fun](StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data) {
                                if (data->size() > 0) {
                                    std::cout << data->string() << std::endl;
                                } else {
                                    std::cout << "call " << set_fun << " faile!" << std::endl;
                                }
                            });
}

void lcd_putc_fun(cmdline::parser &a)
{
    if (a.exist("data")) {
        std::string DataRaw = a.get<std::string>("data");
        StackFlows::pzmq Context(rpc_socket_path);
        for (auto i = 0; i < DataRaw.length(); i++) {
            Context.call_rpc_action("lcd_putc", fmt::format("{{ \"data\": {} }}", (int)DataRaw[i]),
                                    [](StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data) {
                                        if (data->size() > 0) {
                                        } else {
                                            std::cout << "call lcd_putc faile!" << std::endl;
                                        }
                                    });
        }
    }
    return;
}

void get_info_fun(cmdline::parser &a, std::string fun)
{
    StackFlows::pzmq Context(rpc_socket_path);
    Context.call_rpc_action(fun, "{\"data\":\"None\"}",
                            [fun](StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data) {
                                if (data->size() > 0) {
                                    std::cout << data->string() << std::endl;
                                } else {
                                    std::cout << "call " << fun << " faile!" << std::endl;
                                }
                            });
}

void set_info_fun(cmdline::parser &a, std::string fun)
{
    if (a.exist("data") || a.exist("DataRaw")) {
        std::string DataRaw = a.get<std::string>("DataRaw");
        if (DataRaw.empty()) {
            DataRaw = fmt::format("{{ \"data\": {} }}", a.get<std::string>("data"));
        }
        StackFlows::pzmq Context(rpc_socket_path);
        Context.call_rpc_action(fun, DataRaw,
                                [fun](StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data) {
                                    if (data->size() > 0) {
                                        std::cout << data->string() << std::endl;
                                    } else {
                                        std::cout << "call " << fun << "faile!" << std::endl;
                                    }
                                });
    }
}

void fan_speed_fun(cmdline::parser &a)
{
    StackFlows::pzmq Context(rpc_socket_path);
    Context.call_rpc_action("fan_get_speed", "{}",
                            [](StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data) {
                                if (data->size() > 0) {
                                    std::cout << data->string() << std::endl;
                                } else {
                                    std::cout << "call fan_get_speed faile!" << std::endl;
                                }
                            });
}

void fan_pwm_fun(cmdline::parser &a)
{
    if (a.exist("data") || a.exist("DataRaw")) {
        std::string DataRaw = a.get<std::string>("DataRaw");
        if (DataRaw.empty()) {
            DataRaw = fmt::format("{{ \"data\": {} }}", a.get<std::string>("data"));
        }
        StackFlows::pzmq Context(rpc_socket_path);
        Context.call_rpc_action("fan_set_pwm", DataRaw,
                                [](StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data) {
                                    if (data->size() > 0) {
                                        std::cout << data->string() << std::endl;
                                    } else {
                                        std::cout << "call fan_set_pwm faile!" << std::endl;
                                    }
                                });
    } else {
        StackFlows::pzmq Context(rpc_socket_path);
        Context.call_rpc_action("fan_get_pwm", "{}",
                                [](StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data) {
                                    if (data->size() > 0) {
                                        std::cout << data->string() << std::endl;
                                    } else {
                                        std::cout << "call fan_get_pwm faile!" << std::endl;
                                    }
                                });
    }
}

void rgb_fun(cmdline::parser &a)
{
    if (a.exist("data") || a.exist("DataRaw")) {
        std::string DataRaw = a.get<std::string>("DataRaw");
        if (DataRaw.empty()) {
            DataRaw = fmt::format("{{ \"data\": {} }}", a.get<std::string>("data"));
        }
        StackFlows::pzmq Context(rpc_socket_path);
        Context.call_rpc_action("rgb_set_mode", DataRaw,
                                [](StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data) {
                                    if (data->size() > 0) {
                                        std::cout << data->string() << std::endl;
                                    } else {
                                        std::cout << "call rgb_set_mode faile!" << std::endl;
                                    }
                                });
    } else {
        StackFlows::pzmq Context(rpc_socket_path);
        Context.call_rpc_action("rgb_get_mode", "{}",
                                [](StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data) {
                                    if (data->size() > 0) {
                                        std::cout << data->string() << std::endl;
                                    } else {
                                        std::cout << "call rgb_get_mode faile!" << std::endl;
                                    }
                                });
    }
}

void exec_fun(cmdline::parser &a)
{
    if (a.exist("list")) {
        StackFlows::pzmq Context(rpc_socket_path);
        std::cout << "list" << std::endl;
        Context.call_rpc_action("list_action", "None",
                                [](StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data) {
                                    if (data->size() > 0) {
                                        try {
                                            nlohmann::json _data = nlohmann::json::parse(data->string());
                                            std::cout << _data.dump(4) << std::endl;
                                        } catch (const nlohmann::json::parse_error &e) {
                                            std::cerr << "JSON parsing failed for list_action response. Error: " << e.what() 
                                                      << ". Invalid JSON data received: " << data->string() << std::endl;
                                        }
                                    } else {
                                        std::cout << "call list_action faile!" << std::endl;
                                    }
                                });
        return;
    }

    const std::string fun  = a.get<std::string>("fun");
    const std::string data = a.get<std::string>("data");
    if (fun.length() == 0) {
        std::cout << "fun is empty! cli exec -f <fun> -d <data>" << std::endl;
        return;
    }

    StackFlows::pzmq Context(rpc_socket_path);
    Context.call_rpc_action(fun, data, [](StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data) {
        if (data->size() > 0) {
            std::cout << "" << data->string() << std::endl;
        } else {
            std::cout << "call ax650_ec_prox faile!" << std::endl;
        }
    });
}

void echo_fun(cmdline::parser &a)
{
    if (a.exist("button")) {
        StackFlows::pzmq Context(pub_socket_path, ZMQ_SUB,
                                 [](StackFlows::pzmq *_pzmq, const std::shared_ptr<StackFlows::pzmq_data> &data) {
                                     std::cout << data->string() << std::endl;
                                 });
        while (condition) {
            usleep(100 * 1000);
        }
        exit(0);
    }
}

void signalHandler(int signum)
{
    condition = 0;
    std::exit(0);  // 这会调用全局对象的析构函数
}

struct call_fun {
    std::string fun;
    char flage;
    std::string dec;
    std::function<void(cmdline::parser &a)> fun_call;
};

int main(int argc, char *argv[])
{
    if (argc < 2) {
    error_print_exit:
        std::cout << R"help(
用法：git [--version] [--help] 
           <命令> [<参数>]

这些是各种场合常见的 cli 命令：

直接操作 ec （参见：cli help device）
   device    直接执行一个操作，使用指令传递参数

自定义操作 ec （参见：git help exec）
   exec       执行一个原始函数调用

订阅 prox 的通道数据（参见：git help echo）
   echo      显示 prox 的通道数据

命令 'cli help -a' 和 'cli help -g' 显示可用的子命令和一些概念帮助。
查看 'cli help <命令>' 或 'cli help <概念>' 以获取给定子命令或概念的
帮助。
)help" << std::endl;
        return 0;
    }
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);
    rpc_socket_path = getenv("AX650C_EC_PROXY_RPC_SOCKET");
    if (rpc_socket_path == NULL) {
        rpc_socket_path = "ipc:///tmp/rpc.ec_prox";
    }
    pub_socket_path = getenv("AX650C_EC_PROXY_PUB_SOCKET");
    if (pub_socket_path == NULL) {
        pub_socket_path = "ipc:///tmp/llm/ec_prox.event.socket";
    }
    if (std::string(argv[1]) == "device") {
        cmdline::parser a;
        // clang-format off
        std::list<struct call_fun> cmd_list = {
            {"rgb", 'r', "Get or set RGB LED display mode", std::bind(&info_fun, std::placeholders::_1, "rgb_set_mode", "rgb_get_mode")},
            {"poweron_time", 0, "Get or set the scheduled power-on time", std::bind(&info_fun, std::placeholders::_1, "poweron_set_time", "poweron_get_time")},
            {"poweroff_time", 0, "Get or set the scheduled power-off time", std::bind(&info_fun, std::placeholders::_1, "poweroff_set_time", "poweroff_get_time")},
            {"rgb_size", 0, "Get or set the number of RGB LEDs in the array", std::bind(&info_fun, std::placeholders::_1, "rgb_set_size", "rgb_get_size")},
            {"rgb_get_color", 0, "Get the color value of a specific RGB LED", std::bind(&info_fun, std::placeholders::_1, "rgb_get_color", "")},
            {"rgb_set_color", 0, "Set color for a specific RGB LED, example: cli --rgb_set_color -d '{\"rgb_index\":0,\"rgb_color\":255}'", std::bind(&info_fun, std::placeholders::_1, "rgb_set_color", "")},
            {"fan", 'f', "Get or set the fan PWM duty cycle", std::bind(&info_fun, std::placeholders::_1, "fan_set_pwm", "fan_get_pwm")},
            {"fanspeed", 'F', "Get the current fan rotation speed in RPM", std::bind(&info_fun, std::placeholders::_1, "", "fan_get_speed")},
            {"pd_power_info", 0, "Get the USB PD power delivery information", std::bind(&info_fun, std::placeholders::_1, "", "pd_power_info")},
            {"board", 'B', "Get the board power consumption information", std::bind(&info_fun, std::placeholders::_1, "", "board_get_power_info")},
            {"ip_eth0", 0, "Get or set the IP address for Ethernet interface eth0", std::bind(&info_fun, std::placeholders::_1, "eth0_ip_set", "eth0_ip_get")},
            {"ip_eth1", 0, "Get or set the IP address for Ethernet interface eth1", std::bind(&info_fun, std::placeholders::_1, "eth1_ip_set", "eth1_ip_get")},
            {"ip_wlan", 0, "Get or set the IP address for wireless LAN interface", std::bind(&info_fun, std::placeholders::_1, "wlan_ip_set", "wlan_ip_get")},
            {"lcd", 'l', "Get or set the LCD display mode", std::bind(&info_fun, std::placeholders::_1, "lcd_set_mode", "lcd_get_mode")},
            {"lcd_ram", 0, "Set the LCD RAM buffer data directly", std::bind(&info_fun, std::placeholders::_1, "lcd_set_ram", "")},
            {"vddcpu", 'c', "Get or set the CPU core voltage (VDD)", std::bind(&info_fun, std::placeholders::_1, "vddcpu_set", "vddcpu_get")},
            {"modbus_speed", 0, "Get or set the Modbus communication baud rate", std::bind(&info_fun, std::placeholders::_1, "modbus_set_speed", "modbus_get_speed")},
            {"ext_power", 'p', "Control the external power supply output", std::bind(&info_fun, std::placeholders::_1, "ext_power", "")},
            {"board_power", 'b', "Control the main board power switch", std::bind(&info_fun, std::placeholders::_1, "board_power", "")},
            {"pcie0", 0, "Set the PCIe slot 0 switch on/off state", std::bind(&info_fun, std::placeholders::_1, "pcie0_set_switch", "")},
            {"pcie1", 0, "Set the PCIe slot 1 switch on/off state", std::bind(&info_fun, std::placeholders::_1, "pcie1_set_switch", "")},
            {"gl3510_reset", 0, "Reset the GL3510 USB hub controller", std::bind(&info_fun, std::placeholders::_1, "gl3510_reset", "")},
            {"usbds1_big", 0, "Set the high-power mode for USB downstream port 1", std::bind(&info_fun, std::placeholders::_1, "usbds1_set_big_power", "")},
            {"usbds2_big", 0, "Set the high-power mode for USB downstream port 2", std::bind(&info_fun, std::placeholders::_1, "usbds2_set_big_power", "")},
            {"usbds1", 0, "Set the switch on/off state for USB downstream port 1", std::bind(&info_fun, std::placeholders::_1, "usbds1_set_switch", "")},
            {"usbds2", 0, "Set the switch on/off state for USB downstream port 2", std::bind(&info_fun, std::placeholders::_1, "usbds2_set_switch", "")},
            {"usbds3", 0, "Set the switch on/off state for USB downstream port 3", std::bind(&info_fun, std::placeholders::_1, "usbds3_set_switch", "")},
            {"hdmi_loop_en", 0, "Switch HDMI OUT port input source between IN and AX8850", std::bind(&info_fun, std::placeholders::_1, "usbds3_set_switch", "")},
            {"grove_uart", 0, "Set the switch on/off state for Grove UART interface", std::bind(&info_fun, std::placeholders::_1, "grove_uart_set_switch", "")},
            {"grove_iic", 0, "Set the switch on/off state for Grove I2C interface", std::bind(&info_fun, std::placeholders::_1, "grove_iic_set_switch", "")},
            {"flash_switch", 0, "Save switch configuration to flash memory (stores coil registers 4-14)", std::bind(&info_set_fun, std::placeholders::_1, "flash_save_switch", "{\"data\":1}")},
            {"flash_value", 0, "Save value configuration to flash memory (stores holding registers 1, 2, 11, 12, 14, 16-21)", std::bind(&info_set_fun, std::placeholders::_1, "flash_save_value", "{\"data\":1}")},
            {"poweroff", 0, "Trigger the system power-off sequence", std::bind(&info_fun, std::placeholders::_1, "poweroff", "")},
            {"lcd_brightness", 0, "Get or set the LCD backlight brightness level", std::bind(&info_fun, std::placeholders::_1, "lcd_set_brightness", "lcd_get_brightness")},
            {"lcd_putc", 'P', "Output a character or string to the LCD display", lcd_putc_fun},
            {"i2c_set_reg", 0, "Write a value to an I2C device register", std::bind(&info_fun, std::placeholders::_1, "i2c_set_reg", "i2c_set_reg")},
            {"i2c_get_reg", 0, "Read a value from an I2C device register", std::bind(&info_fun, std::placeholders::_1, "i2c_get_reg", "i2c_get_reg")},
            {"ec_button_head_event", 0, "Get or set the event handler for EC head button press", std::bind(&info_fun, std::placeholders::_1, "ec_button_set_head_event", "ec_button_get_head_event")},
            {"soc_button_head_event", 0, "Get or set the event handler for SoC head button press", std::bind(&info_fun, std::placeholders::_1, "soc_button_set_head_event", "soc_button_get_head_event")},
            {"ec_button_lcd_event", 0, "Get or set the event handler for EC LCD button press", std::bind(&info_fun, std::placeholders::_1, "ec_button_set_lcd_event", "ec_button_get_lcd_event")},
            {"fun_auto", 0, "Get or set the automatic control mode for the proxy service", std::bind(&info_fun, std::placeholders::_1, "fun_set_auto", "fun_get_auto")},
            {"ec_modbus_set_bit", 0, "Set a specific bit in the EC Modbus coil register", std::bind(&info_fun, std::placeholders::_1, "ec_modbus_set_bit", "ec_modbus_set_bit")},
            {"ec_modbus_get_bit", 0, "Get a specific bit value from the EC Modbus coil register", std::bind(&info_fun, std::placeholders::_1, "ec_modbus_get_bit", "ec_modbus_get_bit")},
            {"ec_modbus_input_bits", 0, "Read the EC Modbus discrete input bits status", std::bind(&info_fun, std::placeholders::_1, "ec_modbus_get_input_bits", "ec_modbus_get_input_bits")},
            {"ec_modbus_input_registers", 0, "Read the EC Modbus input registers values", std::bind(&info_fun, std::placeholders::_1, "ec_modbus_get_input_registers", "ec_modbus_get_input_registers")},
            {"ec_modbus_set_hold_registers", 0, "Write values to the EC Modbus holding registers", std::bind(&info_fun, std::placeholders::_1, "ec_modbus_set_hold_registers", "ec_modbus_set_hold_registers")},
            {"ec_modbus_get_hold_registers", 0, "Read values from the EC Modbus holding registers", std::bind(&info_fun, std::placeholders::_1, "ec_modbus_get_hold_registers", "ec_modbus_get_hold_registers")},

            {"pcie0_exists", 0, "Check if a PCIe device is present in slot 0", std::bind(&info_fun, std::placeholders::_1, "", "pcie0_exists")},
            {"pcie1_exists", 0, "Check if a PCIe device is present in slot 1", std::bind(&info_fun, std::placeholders::_1, "", "pcie1_exists")},
            {"V3_3_good", 0, "Check if the 3.3V power rail is within normal operating range", std::bind(&info_fun, std::placeholders::_1, "", "V3_3_good")},
            {"V1_8_good", 0, "Check if the 1.8V power rail is within normal operating range", std::bind(&info_fun, std::placeholders::_1, "", "V1_8_good")},
            {"head_button", 0, "Get the current state of the head button (pressed/released)", std::bind(&info_fun, std::placeholders::_1, "", "head_button")},
            {"lcd_button", 0, "Get the current state of the LCD button (pressed/released)", std::bind(&info_fun, std::placeholders::_1, "", "lcd_button")},
            {"version", 0, "Get the EC firmware version information", std::bind(&info_fun, std::placeholders::_1, "", "version")},

        };
        // clang-format on
        for (auto &cmd : cmd_list) {
            a.add(cmd.fun, cmd.flage, cmd.dec);
        }
        a.add<std::string>("data", 'd', "call param", false);
        a.add<std::string>("DataRaw", 'D', "call param raw", false);
        a.parse_check(argc, argv);
        int not_find_index = 1;
        for (auto &cmd : cmd_list) {
            if (a.exist(cmd.fun)) {
                cmd.fun_call(a);
                not_find_index = 0;
            }
        }
        if (not_find_index) {
            std::cout << a.usage() << std::endl;
        }
    } else if (std::string(argv[1]) == "exec") {
        cmdline::parser a;
        a.add("list", 'l', "list call function");
        a.add<std::string>("fun", 'f', "call ax650c_ec_proxy function", false);
        a.add<std::string>("data", 'd', "call ax650c_ec_proxy function input data", false);
        a.parse_check(argc, argv);
        exec_fun(a);
    } else if (std::string(argv[1]) == "echo") {
        cmdline::parser a;
        a.add("button", 'b', "button event");
        a.parse_check(argc, argv);
        echo_fun(a);
        std::cout << a.usage() << std::endl;
        return 0;
    } else {
        goto error_print_exit;
    }

    return 0;
}
