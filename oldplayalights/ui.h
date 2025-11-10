#ifndef UI_H
#define UI_H

#include "config.h"

// ── UI Functions ──────────────────────────────────────────────────────────────
void initUI();
void handleButtons();
void drawUI();
bool shouldUpdateUI();  // Non-blocking UI timing check
void loadControls();
void saveControl(Control c);
void saveGlobalBrightness();

#endif