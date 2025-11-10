#ifndef OTA_H
#define OTA_H

#include "config.h"
#include <ArduinoOTA.h>

// ── OTA Functions ─────────────────────────────────────────────────────────────
void initOTA();
void handleOTA();
void setOTACallbacks();

#endif