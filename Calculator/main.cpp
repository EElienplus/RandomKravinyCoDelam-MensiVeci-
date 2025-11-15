#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <raylib.h>
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

static void DrawShadow(Rectangle r, float offset, Color color)
{
    Rectangle s{ r.x + offset, r.y + offset, r.width, r.height };
    DrawRectangleRounded(s, 0.18f, 8, color);
}

static std::string formatNumber(double v) {
    if (std::isnan(v)) return "NaN";
    if (std::isinf(v)) return (v < 0) ? "-Inf" : "Inf";
    char buf[128];
    // use %.10g for compact representation
    snprintf(buf, sizeof(buf), "%.10g", v);
    return std::string(buf);
}

static double parseNumber(const std::string &s) {
    if (s.empty() || s == "NaN" || s == "Inf" || s == "-Inf") return 0.0;
    char *end = nullptr;
    double v = strtod(s.c_str(), &end);
    if (end == s.c_str()) return 0.0;
    return v;
}

static double applyBinary(double a, double b, char op, bool &ok) {
    ok = true;
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': if (b == 0.0) { ok = false; return 0.0; } return a / b;
    }
    ok = false; return 0.0;
}

int main() {
    const int screenWidth = 600;
    const int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "Calculator");
    SetTargetFPS(60);

    std::string display = "0";
    double stored = 0.0;
    char pendingOp = 0; // '+','-','*','/'
    bool newInput = true; // when true, next digit replaces display
    bool sciMode = false;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground((Color){ 22, 24, 28, 255 });

        // Responsive layout metrics
        int W = GetScreenWidth();
        int H = GetScreenHeight();
        float gap = H * 0.02f;
        float margin = gap;
        float displayH = H * 0.18f;
        float statusH = H * 0.045f;

        // High-contrast raygui style (scaled by control size)
        // Colors are RRGGBBAA in hex
        GuiSetStyle(DEFAULT, TEXT_SIZE, (int)std::clamp(displayH * 0.22f, 14.0f, 28.0f));
        GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, 0x3a3f46ff);   // dark button base
        GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED, 0x4a515aff);
        GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED, 0x2196f3ff);  // accent
        GuiSetStyle(DEFAULT, BASE_COLOR_DISABLED, 0x5a5f66ff);
        GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, 0xf5f5f5ff);
        GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, 0xffffffff);
        GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED, 0xffffffff);
        GuiSetStyle(DEFAULT, TEXT_COLOR_DISABLED, 0x9aa0a6ff);

        // Display area
        Rectangle displayRect{ margin, margin, (float)W - 2.0f * margin, displayH };
        DrawShadow(displayRect, 6.0f, (Color){ 0, 0, 0, 90 });
        DrawRectangleRounded(displayRect, 0.14f, 10, (Color){ 40, 44, 52, 255 });
        DrawRectangleRoundedLines(displayRect, 0.14f, 10, (Color){ 20, 20, 22, 180 });
        std::string dispToShow = display;
        int fontSize = (int)(displayRect.height * 0.45f);
        if (fontSize < 18) fontSize = 18;
        int textWidth = MeasureText(dispToShow.c_str(), fontSize);
        DrawText(dispToShow.c_str(), (int)(displayRect.x + displayRect.width - 0.5f * gap) - textWidth, (int)(displayRect.y + displayRect.height * 0.35f), fontSize, RAYWHITE);
        // Operator badge (visible cue)
        if (pendingOp) {
            int opSize = (int)(fontSize * 0.6f);
            std::string opStr(1, pendingOp);
            int opW = MeasureText(opStr.c_str(), opSize);
            Rectangle badge{ displayRect.x + 10.0f, displayRect.y + 10.0f, (float)opW + 16.0f, (float)opSize + 10.0f };
            DrawShadow(badge, 4.0f, (Color){ 0,0,0,100 });
            DrawRectangleRounded(badge, 0.4f, 6, (Color){ 33, 150, 243, 230 });
            DrawText(opStr.c_str(), (int)badge.x + 8, (int)badge.y + (int)((badge.height - opSize) * 0.5f), opSize, WHITE);
        }

        // Status line (shows stored value and pending operator)
        Rectangle statusRect{ displayRect.x, displayRect.y + displayRect.height + 0.5f * gap, displayRect.width, statusH };
        DrawShadow(statusRect, 4.0f, (Color){ 0,0,0,80 });
        DrawRectangleRounded(statusRect, 0.2f, 6, (Color){ 45, 49, 57, 255 });
        DrawRectangleRoundedLines(statusRect, 0.2f, 6, (Color){ 25, 25, 27, 160 });
        if (pendingOp) {
            std::string status = formatNumber(stored) + " " + std::string(1, pendingOp);
            DrawText(status.c_str(), (int)(statusRect.x + 0.5f * gap), (int)(statusRect.y + statusRect.height * 0.2f), (int)(statusRect.height * 0.6f), RAYWHITE);
        }

        // Scientific toggle button near status line right side
        float sciW = 110.0f;
        float sciH = statusRect.height;
        Rectangle sciBtn{ statusRect.x + statusRect.width - sciW - 0.5f * gap, statusRect.y, sciW, sciH };
        DrawShadow(sciBtn, 3.0f, (Color){ 0,0,0,70 });
        if (GuiButton(sciBtn, sciMode ? "SCI:ON" : "SCI:OFF")) sciMode = !sciMode;

        // Buttons grid (4 columns x 4 rows primary)
        float startX = margin;
        float startY = statusRect.y + statusRect.height + gap;
        int totalRows = 1 /*top*/ + 4 /*primary*/ + (sciMode ? 2 : 0);
        float btnW = ((float)W - 2.0f * margin - 3.0f * gap) / 4.0f;
        float availH = (float)H - startY - margin;
        float btnH = (availH - (totalRows - 1) * gap) / (float)totalRows;
        // Avoid overly tall buttons; keep a pleasant ratio
        float maxBtnH = btnW * 0.7f;
        if (btnH > maxBtnH) btnH = maxBtnH;
        // Recompute total vertical used, and if there's extra, center vertically in remaining space
        float usedH = btnH * totalRows + (totalRows - 1) * gap;
        if (usedH < availH) startY += (availH - usedH) * 0.5f;

        // Update raygui text size to match button height
        GuiSetStyle(DEFAULT, TEXT_SIZE, (int)std::clamp(btnH * 0.45f, 14.0f, 32.0f));

        std::vector<std::vector<std::string>> grid = {
            {"7","8","9","/"},
            {"4","5","6","*"},
            {"1","2","3","-"},
            {"0",".","=","+"}
        };

        // Extra top row for C, DEL, sqrt, +/-
        std::vector<std::string> topRow = {"C","DEL","sqrt","+/-"};
        for (int c = 0; c < 4; ++c) {
            Rectangle r{ startX + c * (btnW + gap), startY, btnW, btnH };
            DrawShadow(r, 4.0f, (Color){ 0,0,0,85 });
            if (GuiButton(r, topRow[c].c_str())) {
                const std::string &lbl = topRow[c];
                if (lbl == "C") {
                    display = "0"; stored = 0.0; pendingOp = 0; newInput = true;
                } else if (lbl == "DEL") {
                    if (!display.empty() && !newInput) {
                        display.pop_back();
                        if (display.empty() || display == "-") display = "0", newInput = true;
                    } else {
                        display = "0"; newInput = true;
                    }
                } else if (lbl == "sqrt") {
                    double v = parseNumber(display);
                    if (v < 0) display = "NaN"; else display = formatNumber(sqrt(v));
                    newInput = true; pendingOp = 0;
                } else if (lbl == "+/-") {
                    double v = parseNumber(display);
                    v = -v;
                    display = formatNumber(v);
                }
            }
        }
        // Advance to next row start (primary grid row 0)
        float rowY0 = startY + (btnH + gap);

        // Draw primary grid buttons
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                float x = startX + c * (btnW + gap);
                float y = rowY0 + r * (btnH + gap);
                Rectangle btnR{ x, y, btnW, btnH };
                DrawShadow(btnR, 4.0f, (Color){ 0,0,0,85 });
                const std::string &lbl = grid[r][c];
                if (GuiButton(btnR, lbl.c_str())) {
                    // handle
                    if (lbl == "+" || lbl == "-" || lbl == "*" || lbl == "/") {
                        double cur = parseNumber(display);
                        if (pendingOp && !newInput) {
                            bool ok; double res = applyBinary(stored, cur, pendingOp, ok);
                            if (!ok) display = "NaN";
                            else { stored = res; display = formatNumber(stored); }
                        } else {
                            stored = cur;
                        }
                        pendingOp = lbl[0];
                        newInput = true;
                    } else if (lbl == "=") {
                        if (pendingOp) {
                            double cur = parseNumber(display);
                            bool ok; double res = applyBinary(stored, cur, pendingOp, ok);
                            if (!ok) display = "NaN"; else display = formatNumber(res);
                            pendingOp = 0; stored = 0.0; newInput = true;
                        }
                    } else if (lbl == ".") {
                        if (newInput) { display = "0."; newInput = false; }
                        else if (display.find('.') == std::string::npos) display.push_back('.');
                    } else {
                        // digit
                        if (newInput || display == "0") { display = lbl; newInput = false; }
                        else display += lbl;
                    }
                }
            }
        }

        // Scientific row
        if (sciMode) {
            std::vector<std::string> sciBtns = {"sin","cos","tan","log","pow","exp"};
            for (size_t i = 0; i < sciBtns.size(); ++i) {
                float x = startX + (i % 3) * (btnW + gap);
                // sci rows start after primary grid: rowY0 + 4 rows
                float y = rowY0 + 4 * (btnH + gap) + (i / 3) * (btnH + gap);
                Rectangle r{ x, y, btnW, btnH };
                DrawShadow(r, 4.0f, (Color){ 0,0,0,85 });
                if (GuiButton(r, sciBtns[i].c_str())) {
                    double v = parseNumber(display);
                    if (sciBtns[i] == "sin") display = formatNumber(sin(v));
                    else if (sciBtns[i] == "cos") display = formatNumber(cos(v));
                    else if (sciBtns[i] == "tan") display = formatNumber(tan(v));
                    else if (sciBtns[i] == "log") display = formatNumber(v > 0 ? log(v) : NAN);
                    else if (sciBtns[i] == "exp") display = formatNumber(exp(v));
                    else if (sciBtns[i] == "pow") {
                        display = formatNumber(pow(v, 2.0));
                    }
                    newInput = true; pendingOp = 0;
                }
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}