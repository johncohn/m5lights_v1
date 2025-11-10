#ifndef NETWORKING_H
#define NETWORKING_H

#include "config.h"

// ── Networking Functions ──────────────────────────────────────────────────────
void initNetworking();
void handleNetworking();
void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len);
void sendRaw();
void sendToken();
void forceSyncReset();
void handleWiFiTransition(bool wasConnected, bool nowConnected);


#endif