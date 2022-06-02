/* Wi-Fi Provisioning Manager Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "console.hpp"
#include "esp_console.h"

//-------------------------------------------------------------------
// Console
//-------------------------------------------------------------------
Console& Console::create()
{
    static Console console;
    return console;
}

Console::Console()
{
    /* Register commands */
    esp_console_register_help_command();
    
    esp_console_dev_usb_serial_jtag_config_t dummy;
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "WifiDebugger >";
    repl_config.max_cmdline_length = 1024;
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&dummy, &repl_config, &repl));

    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}