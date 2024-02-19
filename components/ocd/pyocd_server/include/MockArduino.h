#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <esp_log.h>

using boolean = bool ;
#define PROGMEM
#define sprintf_P sprintf
#define min(a,b) (((a)<(b))?(a):(b))
