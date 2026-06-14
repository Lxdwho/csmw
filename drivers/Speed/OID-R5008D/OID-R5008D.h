/**
 * @brief:OID-R5008D传感器驱动
 * 共用 BRT38的驱动
 */

#pragma once

#include "BRT38.h"
#include "../../../smw/classFactory/ClassFactory.h"

using OID_R5008D = BRT38;

CLASS_LOADER_REGISTER_CLASS(OID_R5008D, SensorBase)