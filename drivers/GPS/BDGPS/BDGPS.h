/**
 * @brief:BDGPS传感器驱动
 * 共用 ATGM336H的驱动
 */

#pragma once

#include "../ATGM336H/ATGM336H.h"
#include "../../../smw/classFactory/ClassFactory.h"

using BDGPS = ATGM336H;

CLASS_LOADER_REGISTER_CLASS(BDGPS, SensorBase)
