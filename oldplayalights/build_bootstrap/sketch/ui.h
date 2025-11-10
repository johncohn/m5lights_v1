#line 1 "/Users/johncohn/Documents/Arduino/playalights_claude_v51/ui.h"
#ifndef UI_H
#define UI_H

#include "config.h"

// ── UI Functions ──────────────────────────────────────────────────────────────
void initUI();
void handleButtons();
void drawUI();
void loadControls();
void saveControl(Control c);

#endif