/*
 * Scientific Calculator with Windows GUI
 * Build: gcc -o CalculatorGUI.exe CalculatorGUI.c -lm -mwindows
 */

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Theme colors */
#define COL_BG          RGB(38, 40, 46)
#define COL_DISPLAY_BG  RGB(28, 30, 34)
#define COL_DISPLAY_FG  RGB(230, 232, 235)
#define COL_EXPR_FG     RGB(160, 165, 175)
#define COL_BTN_DIGIT   RGB(65, 68, 75)
#define COL_BTN_OP      RGB(0, 120, 215)
#define COL_BTN_EQ      RGB(16, 137, 62)
#define COL_BTN_ACTION  RGB(180, 60, 70)
#define COL_BTN_MOD     RGB(114, 46, 209)
#define COL_BTN_TEXT    RGB(255, 255, 255)
#define COL_BTN_HOVER   RGB(85, 88, 98)
#include <math.h>

#define PI 3.14159265358979323846
#define IDC_EXPR      100
#define IDC_DISPLAY   101
#define IDC_BTN_0     200
#define IDC_BTN_9     209
#define IDC_BTN_PLUS  210
#define IDC_BTN_MINUS 211
#define IDC_BTN_MUL   212
#define IDC_BTN_DIV   213
#define IDC_BTN_MOD   214
#define IDC_BTN_EQ    215
#define IDC_BTN_C     216
#define IDC_BTN_DOT   217
#define IDC_BTN_BS    218
#define IDC_BTN_POW   219
#define IDC_BTN_PERC  220
#define IDC_BTN_SQRT  221
#define IDC_BTN_SIN   222
#define IDC_BTN_COS   223
#define IDC_BTN_TAN   224
#define IDC_BTN_LN    225
#define IDC_BTN_LOG   226
#define IDC_BTN_EXP   227
#define IDC_BTN_1X    228
#define IDC_BTN_X2    229
#define IDC_BTN_ABS   230
#define IDC_BTN_PI    231
#define IDC_BTN_EE    232
#define IDC_BTN_INV   233
#define IDC_RAD        234

#define MAX_DISPLAY   80
#define MAX_EXPR      120

static char display[MAX_DISPLAY] = "0";
static char expression[MAX_EXPR] = "";
static double operand1 = 0;
static char pending_op = 0;
static int fresh_display = 1;
static int degree_mode = 1;  /* 1=deg, 0=rad */

#define NUM_BUTTONS  19
#define COLS         4
#define ROWS         5

static HWND hDisplay, hExpr;
static HWND hButtons[NUM_BUTTONS];  /* row-major: 7,8,9,/, 4,5,6,* ... C,BS,% */
static HFONT hFontDisplay, hFontBtn;
static HBRUSH hBrushBg, hBrushDisplay;
static int minClientW = 260, minClientH = 320;

static void update_display(HWND hwnd)
{
    (void)hwnd;
    SetWindowTextA(hDisplay, display);
}

static void update_expression(HWND hwnd)
{
    (void)hwnd;
    SetWindowTextA(hExpr, expression);
}

static void append_digit(HWND hwnd, char digit)
{
    if (fresh_display)
    {
        display[0] = digit;
        display[1] = '\0';
        fresh_display = 0;
    }
    else
    {
        size_t len = strlen(display);
        if (len < MAX_DISPLAY - 1)
        {
            display[len] = digit;
            display[len + 1] = '\0';
        }
    }
    update_display(hwnd);
}

static void append_dot(HWND hwnd)
{
    if (strchr(display, '.')) return;
    size_t len = strlen(display);
    if (len < MAX_DISPLAY - 1)
    {
        if (fresh_display) { display[0] = '0'; display[1] = '.'; display[2] = '\0'; fresh_display = 0; }
        else { display[len] = '.'; display[len + 1] = '\0'; }
    }
    update_display(hwnd);
}

static double get_display_value(void)
{
    return atof(display);
}

static void set_display_value(double val)
{
    snprintf(display, MAX_DISPLAY, "%.12g", val);
    char *p = strchr(display, '.');
    if (p)
    {
        size_t len = strlen(p);
        while (len > 1 && p[len - 1] == '0') { p[--len] = '\0'; }
        if (len == 1) *p = '\0';
    }
    fresh_display = 1;
}

static void set_expression(const char *op_str)
{
    if (op_str && *op_str)
        snprintf(expression, MAX_EXPR, "%.10g %s ", operand1, op_str);
    else
        expression[0] = '\0';
}

static double to_rad(double deg) { return deg * PI / 180.0; }

static void do_operation(HWND hwnd, char op)
{
    double b = get_display_value();
    double result = 0;
    int error = 0;

    if (pending_op)
    {
        double a = operand1;
        switch (pending_op)
        {
            case '+': result = a + b; break;
            case '-': result = a - b; break;
            case '*': result = a * b; break;
            case '/':
                if (b == 0) { error = 1; break; }
                result = a / b;
                break;
            case '%':
                if ((long)b == 0) { error = 1; break; }
                result = (double)((long)a % (long)b);
                break;
            case '^':
                result = pow(a, b);
                break;
            default: break;
        }
        if (!error)
        {
            set_display_value(result);
            update_display(hwnd);
        }
    }
    else
    {
        operand1 = b;
    }

    if (error)
    {
        MessageBoxA(hwnd, "Division by zero.", "Error", MB_OK | MB_ICONERROR);
        strcpy(display, "0");
        fresh_display = 1;
        pending_op = 0;
        expression[0] = '\0';
        update_display(hwnd);
        update_expression(hwnd);
        return;
    }

    if (op == '=')
    {
        pending_op = 0;
        expression[0] = '\0';
        update_expression(hwnd);
    }
    else
    {
        pending_op = op;
        operand1 = get_display_value();
        fresh_display = 1;
        const char *s = (op == '*') ? "*" : (op == '/') ? "/" : (op == '%') ? "%" :
                       (op == '+') ? "+" : (op == '-') ? "-" : "^";
        set_expression(s);
        update_expression(hwnd);
    }
}

static void do_unary(HWND hwnd, int op_id)
{
    double x = get_display_value();
    double result = 0;
    int error = 0;

    switch (op_id)
    {
        case IDC_BTN_SQRT:
            if (x < 0) error = 1;
            else result = sqrt(x);
            break;
        case IDC_BTN_X2:
            result = x * x;
            break;
        case IDC_BTN_1X:
            if (x == 0) error = 1;
            else result = 1.0 / x;
            break;
        case IDC_BTN_SIN:
            result = sin(degree_mode ? to_rad(x) : x);
            break;
        case IDC_BTN_COS:
            result = cos(degree_mode ? to_rad(x) : x);
            break;
        case IDC_BTN_TAN:
            result = tan(degree_mode ? to_rad(x) : x);
            break;
        case IDC_BTN_LN:
            if (x <= 0) error = 1;
            else result = log(x);
            break;
        case IDC_BTN_LOG:
            if (x <= 0) error = 1;
            else result = log10(x);
            break;
        case IDC_BTN_EXP:
            result = exp(x);
            break;
        case IDC_BTN_ABS:
            result = fabs(x);
            break;
        case IDC_BTN_PI:
            result = PI;
            break;
        case IDC_BTN_EE:
            result = 2.718281828459045;
            break;
        case IDC_BTN_INV:
            /* sin^-1, cos^-1, tan^-1 - simplified: atan(x) */
            if (x < -1 || x > 1) error = 1;
            else result = degree_mode ? (asin(x) * 180.0 / PI) : asin(x);
            break;
        default:
            break;
    }

    if (error)
    {
        MessageBoxA(hwnd, "Invalid input.", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    set_display_value(result);
    pending_op = 0;
    expression[0] = '\0';
    update_display(hwnd);
    update_expression(hwnd);
}

static void on_digit(HWND hwnd, int digit)
{
    char c = (char)('0' + digit);
    append_digit(hwnd, c);
}

static void on_operator(HWND hwnd, char op)
{
    do_operation(hwnd, op);
}

static void on_clear(HWND hwnd)
{
    strcpy(display, "0");
    expression[0] = '\0';
    fresh_display = 1;
    pending_op = 0;
    operand1 = 0;
    update_display(hwnd);
    update_expression(hwnd);
}

static void on_backspace(HWND hwnd)
{
    if (fresh_display) return;
    size_t len = strlen(display);
    if (len <= 1)
    {
        strcpy(display, "0");
        fresh_display = 1;
    }
    else
    {
        display[len - 1] = '\0';
    }
    update_display(hwnd);
}

/* Returns button type for coloring: 0=digit, 1=operator, 2=equals, 3=action(C/BS), 4=mod */
static int get_button_type(int id)
{
    if (id >= IDC_BTN_0 && id <= IDC_BTN_9) return 0;
    if (id == IDC_BTN_PLUS || id == IDC_BTN_MINUS || id == IDC_BTN_MUL || id == IDC_BTN_DIV) return 1;
    if (id == IDC_BTN_EQ) return 2;
    if (id == IDC_BTN_C || id == IDC_BTN_BS) return 3;
    if (id == IDC_BTN_DOT) return 0;
    return 4; /* IDC_BTN_MOD */
}

static HWND make_button(HWND parent, const char *text, int id, int idx, int x, int y, int w, int h)
{
    HWND btn = CreateWindowExA(0, "BUTTON", text,
                               WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                               x, y, w, h, parent, (HMENU)(INT_PTR)id,
                               GetModuleHandle(NULL), NULL);
    if (hFontBtn)
        SendMessage(btn, WM_SETFONT, (WPARAM)hFontBtn, TRUE);
    if (idx >= 0 && idx < NUM_BUTTONS)
        hButtons[idx] = btn;
    return btn;
}

static void layout_children(HWND hwnd, int clientW, int clientH)
{
    const int pad = 10;
    const int gap = 6;
    int keypadTop = clientH * 22 / 100;
    int keypadH = clientH - keypadTop - pad;
    int cellW = (clientW - 2 * pad - (COLS - 1) * gap) / COLS;
    int cellH = (keypadH - (ROWS - 1) * gap) / ROWS;
    if (cellW < 40) cellW = 40;
    if (cellH < 32) cellH = 32;

    MoveWindow(hExpr, pad, pad, clientW - 2 * pad, keypadTop - pad - 22, TRUE);
    MoveWindow(hDisplay, pad, keypadTop - 52, clientW - 2 * pad, 44, TRUE);

    for (int row = 0; row < ROWS; row++)
        for (int col = 0; col < COLS; col++)
        {
            int idx = row * COLS + col;
            if (idx < NUM_BUTTONS && hButtons[idx])
            {
                int x = pad + col * (cellW + gap);
                int y = keypadTop + row * (cellH + gap);
                int w = cellW;
                int h = cellH;
                if (row == ROWS - 1 && col == 0) { /* C spans 1, BS and % next */
                    /* keep default single cell */
                }
                MoveWindow(hButtons[idx], x, y, w, h, TRUE);
            }
        }
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            hFontDisplay = CreateFontA(24, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                      CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");
            if (!hFontDisplay)
                hFontDisplay = (HFONT)GetStockObject(SYSTEM_FIXED_FONT);
            hFontBtn = CreateFontA(14, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

            int col0 = 10, y0 = 8;
            hExpr = CreateWindowExA(0, "STATIC", "",
                                    WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                    col0, y0, 428, 20, hwnd, (HMENU)(INT_PTR)IDC_EXPR,
                                    GetModuleHandle(NULL), NULL);
            SendMessage(hExpr, WM_SETFONT, (WPARAM)hFontBtn, TRUE);

            hDisplay = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "0",
                                       WS_CHILD | WS_VISIBLE | ES_RIGHT | ES_READONLY,
                                       col0, y0 + 24, 428, 36, hwnd, (HMENU)(INT_PTR)IDC_DISPLAY,
                                       GetModuleHandle(NULL), NULL);
            SendMessage(hDisplay, WM_SETFONT, (WPARAM)hFontDisplay, TRUE);

            int bw = 50, bh = 34, gap = 4;
            int row0 = 78;

            /* Scientific row */
            make_button(hwnd, "sqrt", IDC_BTN_SQRT, col0, row0, bw, bh);
            make_button(hwnd, "x^2", IDC_BTN_X2, col0 + (bw + gap), row0, bw, bh);
            make_button(hwnd, "1/x", IDC_BTN_1X, col0 + 2 * (bw + gap), row0, bw, bh);
            make_button(hwnd, "|x|", IDC_BTN_ABS, col0 + 3 * (bw + gap), row0, bw, bh);
            make_button(hwnd, "sin", IDC_BTN_SIN, col0 + 4 * (bw + gap), row0, bw, bh);
            make_button(hwnd, "cos", IDC_BTN_COS, col0 + 5 * (bw + gap), row0, bw, bh);
            make_button(hwnd, "tan", IDC_BTN_TAN, col0 + 6 * (bw + gap), row0, bw, bh);
            make_button(hwnd, "asin", IDC_BTN_INV, col0 + 7 * (bw + gap), row0, bw, bh);
            row0 += bh + gap;

            make_button(hwnd, "ln", IDC_BTN_LN, col0, row0, bw, bh);
            make_button(hwnd, "log", IDC_BTN_LOG, col0 + (bw + gap), row0, bw, bh);
            make_button(hwnd, "exp", IDC_BTN_EXP, col0 + 2 * (bw + gap), row0, bw, bh);
            make_button(hwnd, "pi", IDC_BTN_PI, col0 + 3 * (bw + gap), row0, bw, bh);
            make_button(hwnd, "e", IDC_BTN_EE, col0 + 4 * (bw + gap), row0, bw, bh);
            row0 += bh + gap + 4;

            /* Numpad */
            make_button(hwnd, "7", IDC_BTN_0 + 7, col0, row0, bw, bh);
            make_button(hwnd, "8", IDC_BTN_0 + 8, col0 + bw + gap, row0, bw, bh);
            make_button(hwnd, "9", IDC_BTN_0 + 9, col0 + 2 * (bw + gap), row0, bw, bh);
            make_button(hwnd, "/", IDC_BTN_DIV, col0 + 3 * (bw + gap), row0, bw, bh);
            make_button(hwnd, "%", IDC_BTN_MOD, col0 + 4 * (bw + gap), row0, bw, bh);
            make_button(hwnd, "^", IDC_BTN_POW, col0 + 5 * (bw + gap), row0, bw, bh);
            row0 += bh + gap;

            make_button(hwnd, "4", IDC_BTN_0 + 4, col0, row0, bw, bh);
            make_button(hwnd, "5", IDC_BTN_0 + 5, col0 + bw + gap, row0, bw, bh);
            make_button(hwnd, "6", IDC_BTN_0 + 6, col0 + 2 * (bw + gap), row0, bw, bh);
            make_button(hwnd, "*", IDC_BTN_MUL, col0 + 3 * (bw + gap), row0, bw, bh);
            row0 += bh + gap;

            make_button(hwnd, "1", IDC_BTN_0 + 1, col0, row0, bw, bh);
            make_button(hwnd, "2", IDC_BTN_0 + 2, col0 + bw + gap, row0, bw, bh);
            make_button(hwnd, "3", IDC_BTN_0 + 3, col0 + 2 * (bw + gap), row0, bw, bh);
            make_button(hwnd, "-", IDC_BTN_MINUS, col0 + 3 * (bw + gap), row0, bw, bh);
            row0 += bh + gap;

            make_button(hwnd, "0", IDC_BTN_0, col0, row0, bw, bh);
            make_button(hwnd, ".", IDC_BTN_DOT, col0 + bw + gap, row0, bw, bh);
            make_button(hwnd, "=", IDC_BTN_EQ, col0 + 2 * (bw + gap), row0, bw, bh);
            make_button(hwnd, "+", IDC_BTN_PLUS, col0 + 3 * (bw + gap), row0, bw, bh);
            row0 += bh + gap;

            make_button(hwnd, "C", IDC_BTN_C, col0, row0, bw, bh);
            make_button(hwnd, "BS", IDC_BTN_BS, col0 + bw + gap, row0, bw, bh);
            break;
        }
        case WM_COMMAND:
        {
            int id = LOWORD(wParam);
            if (id >= IDC_BTN_0 && id <= IDC_BTN_9)
                on_digit(hwnd, id - IDC_BTN_0);
            else if (id == IDC_BTN_DOT)
                append_dot(hwnd);
            else if (id == IDC_BTN_C)
                on_clear(hwnd);
            else if (id == IDC_BTN_BS)
                on_backspace(hwnd);
            else if (id == IDC_BTN_MOD)
                on_operator(hwnd, '%');
            else if (id == IDC_BTN_EQ)
                do_operation(hwnd, '=');
            else if (id == IDC_BTN_PLUS)
                on_operator(hwnd, '+');
            else if (id == IDC_BTN_MINUS)
                on_operator(hwnd, '-');
            else if (id == IDC_BTN_MUL)
                on_operator(hwnd, '*');
            else if (id == IDC_BTN_DIV)
                on_operator(hwnd, '/');
            else if (id == IDC_BTN_POW)
                on_operator(hwnd, '^');
            else if (id == IDC_BTN_SQRT || id == IDC_BTN_X2 || id == IDC_BTN_1X ||
                     id == IDC_BTN_SIN || id == IDC_BTN_COS || id == IDC_BTN_TAN ||
                     id == IDC_BTN_LN || id == IDC_BTN_LOG || id == IDC_BTN_EXP ||
                     id == IDC_BTN_ABS || id == IDC_BTN_PI || id == IDC_BTN_EE || id == IDC_BTN_INV)
                do_unary(hwnd, id);
            break;
        }
        case WM_KEYDOWN:
        {
            switch (wParam)
            {
                case VK_ESCAPE:
                    on_clear(hwnd);
                    return 0;
                case VK_BACK:
                    on_backspace(hwnd);
                    return 0;
                case VK_RETURN:
                    do_operation(hwnd, '=');
                    return 0;
                case '0': case '1': case '2': case '3': case '4':
                case '6': case '7': case '8': case '9':
                    on_digit(hwnd, (int)(wParam - '0'));
                    return 0;
                case VK_OEM_2:
                    if (GetKeyState(VK_SHIFT) & 0x8000)
                        on_operator(hwnd, '/');
                    return 0;
                case VK_OEM_PLUS:
                    on_operator(hwnd, (GetKeyState(VK_SHIFT) & 0x8000) ? '+' : '=');
                    return 0;
                case VK_OEM_MINUS:
                    on_operator(hwnd, '-');
                    return 0;
                case VK_MULTIPLY:
                    on_operator(hwnd, '*');
                    return 0;
                case VK_DIVIDE:
                    on_operator(hwnd, '/');
                    return 0;
                case VK_OEM_PERIOD:
                case VK_DECIMAL:
                    append_dot(hwnd);
                    return 0;
                case 'C':
                case 'c':
                    on_clear(hwnd);
                    return 0;
                case 0x35:
                    if (GetKeyState(VK_SHIFT) & 0x8000)
                        on_operator(hwnd, '%');
                    else
                        on_digit(hwnd, 5);
                    return 0;
                default:
                    break;
            }
            break;
        }
        case WM_DESTROY:
            if (hFontDisplay) DeleteObject(hFontDisplay);
            if (hFontBtn) DeleteObject(hFontBtn);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;

    WNDCLASSEXA wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = "CalculatorGUI";

    if (!RegisterClassExA(&wc))
        return 1;

    HWND hwnd = CreateWindowExA(0, "CalculatorGUI", "Scientific Calculator",
                                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                                CW_USEDEFAULT, CW_USEDEFAULT, 458, 380,
                                NULL, NULL, hInstance, NULL);
    if (!hwnd)
        return 1;

    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
