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
#define COL_BTN_FUNC    RGB(90, 70, 140)   /* scientific functions */
#include <math.h>

#define PI 3.14159265358979323846
#define IDC_EXPR      100
#define IDC_DISPLAY   101
#define IDC_FEEDBACK  102
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

/* Menu command IDs */
#define IDM_FILE_EXIT     3000
#define IDM_EDIT_CLEAR   3001
#define IDM_EDIT_HISTORY  3003
#define IDM_HELP_ABOUT   3002

#define MAX_DISPLAY   80
#define MAX_HISTORY   50
#define MAX_HIST_LINE 120
#define MAX_EXPR      120

static char display[MAX_DISPLAY] = "0";
static char expression[MAX_EXPR] = "";
static double operand1 = 0;
static char pending_op = 0;
static int fresh_display = 1;
static int degree_mode = 1;  /* 1=deg, 0=rad */

#define MAX_BUTTONS    40
#define KEYPAD_COLS    4
#define KEYPAD_ROWS    9

static HWND hDisplay, hExpr, hFeedback;
#define MAX_FEEDBACK   64
static char last_result[MAX_FEEDBACK];  /* e.g. "Result: -5" for main-screen feedback */
static HFONT hFontDisplay, hFontBtn;
static HBRUSH hBrushBg, hBrushDisplay;
static int minClientW = 280, minClientH = 420;

typedef struct { HWND h; int row; int col; } ButtonPlace;
static ButtonPlace s_buttons[MAX_BUTTONS];
static int s_button_count = 0;

static char s_history[MAX_HISTORY][MAX_HIST_LINE];
static int s_history_count = 0;

static void add_history(const char *line)
{
    if (s_history_count >= MAX_HISTORY)
    {
        memmove(s_history[0], s_history[1], sizeof(s_history[0]) * (MAX_HISTORY - 1));
        s_history_count = MAX_HISTORY - 1;
    }
    strncpy(s_history[s_history_count], line, MAX_HIST_LINE - 1);
    s_history[s_history_count][MAX_HIST_LINE - 1] = '\0';
    s_history_count++;
}

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

static void update_feedback(HWND hwnd)
{
    (void)hwnd;
    if (hFeedback)
        SetWindowTextA(hFeedback, last_result);
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
    /* Format so negative numbers (e.g. -5) show clearly on main screen */
    snprintf(display, MAX_DISPLAY, "%.12g", val);
    char *p = strchr(display, '.');
    if (p)
    {
        size_t len = strlen(p);
        while (len > 1 && p[len - 1] == '0') { p[--len] = '\0'; }
        if (len == 1) *p = '\0';
    }
    /* Main-screen feedback: show this number as result (e.g. "Result: -5") */
    snprintf(last_result, MAX_FEEDBACK, "Result: %s", display);
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
            update_feedback(hwnd);
            if (op == '=')
            {
                char hist[MAX_HIST_LINE];
                const char *opc = (pending_op == '*') ? "*" : (pending_op == '/') ? "/" :
                    (pending_op == '%') ? "%" : (pending_op == '+') ? "+" :
                    (pending_op == '-') ? "-" : "^";
                snprintf(hist, sizeof(hist), "%.10g %s %.10g = %.12g", operand1, opc, b, result);
                add_history(hist);
            }
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

static const char* unary_func_name(int op_id)
{
    switch (op_id)
    {
        case IDC_BTN_SQRT: return "sqrt";
        case IDC_BTN_X2:   return "x^2";
        case IDC_BTN_1X:   return "1/x";
        case IDC_BTN_SIN:  return "sin";
        case IDC_BTN_COS:  return "cos";
        case IDC_BTN_TAN:  return "tan";
        case IDC_BTN_LN:   return "ln";
        case IDC_BTN_LOG:  return "log";
        case IDC_BTN_EXP:  return "exp";
        case IDC_BTN_ABS:  return "abs";
        case IDC_BTN_PI:   return "pi";
        case IDC_BTN_EE:   return "e";
        case IDC_BTN_INV:  return "asin";
        default: return "?";
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
    {
        char hist[MAX_HIST_LINE];
        const char *fn = unary_func_name(op_id);
        if (op_id == IDC_BTN_PI || op_id == IDC_BTN_EE)
            snprintf(hist, sizeof(hist), "%s = %.12g", fn, result);
        else
            snprintf(hist, sizeof(hist), "%s(%.10g) = %.12g", fn, x, result);
        add_history(hist);
    }
    update_display(hwnd);
    update_expression(hwnd);
    update_feedback(hwnd);
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
    last_result[0] = '\0';
    fresh_display = 1;
    pending_op = 0;
    operand1 = 0;
    update_display(hwnd);
    update_expression(hwnd);
    update_feedback(hwnd);
}

static LRESULT CALLBACK HistoryDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    (void)lParam;
    switch (msg)
    {
        case WM_COMMAND:
            if (LOWORD(wParam) == 2)
                DestroyWindow(hwnd);
            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    return 0;
}

static void show_history(HWND hwndParent)
{
    static int class_reg = 0;
    if (!class_reg)
    {
        WNDCLASSEXA wc = { 0 };
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = HistoryDlgProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = "CalculatorHistory";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        if (!RegisterClassExA(&wc)) return;
        class_reg = 1;
    }

    const int dlgW = 420, dlgH = 320;
    HWND hDlg = CreateWindowExA(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, "CalculatorHistory",
        "History", WS_POPUP | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, dlgW, dlgH,
        hwndParent, NULL, GetModuleHandle(NULL), NULL);
    if (!hDlg) return;

    CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", NULL,
        WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_VSCROLL,
        10, 10, dlgW - 40, dlgH - 70, hDlg, (HMENU)1, GetModuleHandle(NULL), NULL);
    CreateWindowExA(0, "BUTTON", "Close",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        dlgW/2 - 40, dlgH - 50, 80, 28, hDlg, (HMENU)2, GetModuleHandle(NULL), NULL);

    HWND hList = GetDlgItem(hDlg, 1);
    for (int i = s_history_count - 1; i >= 0; i--)
        SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)s_history[i]);
    if (s_history_count == 0)
        SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)"(No calculations yet)");

    RECT wr;
    GetWindowRect(hwndParent, &wr);
    int x = wr.left + (wr.right - wr.left - dlgW) / 2;
    int y = wr.top + (wr.bottom - wr.top - dlgH) / 2;
    SetWindowPos(hDlg, HWND_TOP, x, y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
    ShowWindow(hDlg, SW_SHOW);
    SetForegroundWindow(hDlg);

    MSG msg;
    while (IsWindow(hDlg) && GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
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

/* Button type for coloring: 0=digit, 1=operator, 2=equals, 3=action(C/BS), 4=mod, 5=scientific */
static int get_button_type(int id)
{
    if (id >= IDC_BTN_0 && id <= IDC_BTN_9) return 0;
    if (id == IDC_BTN_DOT) return 0;
    if (id == IDC_BTN_PLUS || id == IDC_BTN_MINUS || id == IDC_BTN_MUL || id == IDC_BTN_DIV) return 1;
    if (id == IDC_BTN_EQ) return 2;
    if (id == IDC_BTN_C || id == IDC_BTN_BS) return 3;
    if (id == IDC_BTN_MOD || id == IDC_BTN_POW) return 4;
    if (id == IDC_BTN_SQRT || id == IDC_BTN_X2 || id == IDC_BTN_1X || id == IDC_BTN_ABS ||
        id == IDC_BTN_SIN || id == IDC_BTN_COS || id == IDC_BTN_TAN || id == IDC_BTN_INV ||
        id == IDC_BTN_LN || id == IDC_BTN_LOG || id == IDC_BTN_EXP ||
        id == IDC_BTN_PI || id == IDC_BTN_EE) return 5;
    return 4;
}

static HWND make_button(HWND parent, const char *text, int id, int row, int col)
{
    HWND btn = CreateWindowExA(0, "BUTTON", text,
                               WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                               0, 0, 1, 1, parent, (HMENU)(INT_PTR)id,
                               GetModuleHandle(NULL), NULL);
    if (hFontBtn)
        SendMessage(btn, WM_SETFONT, (WPARAM)hFontBtn, TRUE);
    if (s_button_count < MAX_BUTTONS)
    {
        s_buttons[s_button_count].h = btn;
        s_buttons[s_button_count].row = row;
        s_buttons[s_button_count].col = col;
        s_button_count++;
    }
    return btn;
}

#define FONT_RATIO_DISPLAY  0.55   /* display font height = dispH * this */
#define FONT_RATIO_BTN      0.55   /* button/expr font height = cellH * this */

static void layout_children(HWND hwnd, int clientW, int clientH)
{
    const int pad = 10;
    const int gap = 5;
    if (clientW < minClientW) clientW = minClientW;
    if (clientH < minClientH) clientH = minClientH;

    /* Proportional heights: header ~25% of client, keypad rest */
    int exprH = clientH / 20;     if (exprH < 18) exprH = 18;
    int dispH = clientH / 10;     if (dispH < 36) dispH = 36;
    int feedbackH = clientH / 25; if (feedbackH < 16) feedbackH = 16;

    /* Expression, display, and result-feedback line */
    MoveWindow(hExpr, pad, pad, clientW - 2 * pad, exprH, TRUE);
    MoveWindow(hDisplay, pad, pad + exprH + 2, clientW - 2 * pad, dispH, TRUE);
    if (hFeedback)
        MoveWindow(hFeedback, pad, pad + exprH + 2 + dispH + 4, clientW - 2 * pad, feedbackH, TRUE);

    /* Keypad grid below display */
    int keypadTop = pad + exprH + 2 + dispH + 4 + feedbackH + 6;
    int keypadW = clientW - 2 * pad;
    int keypadH = clientH - keypadTop - pad;
    if (keypadH < 20) keypadH = 20;

    int cellW = (keypadW - (KEYPAD_COLS - 1) * gap) / KEYPAD_COLS;
    int cellH = (keypadH - (KEYPAD_ROWS - 1) * gap) / KEYPAD_ROWS;
    if (cellW < 24) cellW = 24;
    if (cellH < 22) cellH = 22;

    /* Font sizes as ratio of box height */
    int dispFontH = (int)(dispH * FONT_RATIO_DISPLAY);
    int btnFontH  = (int)(cellH * FONT_RATIO_BTN);
    if (dispFontH < 10) dispFontH = 10;
    if (btnFontH  < 9) btnFontH  = 9;

    /* Recreate fonts with scaled sizes */
    if (hFontDisplay) DeleteObject(hFontDisplay);
    if (hFontBtn) DeleteObject(hFontBtn);
    hFontDisplay = CreateFontA(dispFontH, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");
    if (!hFontDisplay)
        hFontDisplay = (HFONT)GetStockObject(SYSTEM_FIXED_FONT);
    hFontBtn = CreateFontA(btnFontH, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                          CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

    SendMessage(hExpr, WM_SETFONT, (WPARAM)hFontBtn, TRUE);
    SendMessage(hDisplay, WM_SETFONT, (WPARAM)hFontDisplay, TRUE);
    if (hFeedback) SendMessage(hFeedback, WM_SETFONT, (WPARAM)hFontBtn, TRUE);
    for (int i = 0; i < s_button_count; i++)
        SendMessage(s_buttons[i].h, WM_SETFONT, (WPARAM)hFontBtn, TRUE);

    for (int i = 0; i < s_button_count; i++)
    {
        int x = pad + s_buttons[i].col * (cellW + gap);
        int y = keypadTop + s_buttons[i].row * (cellH + gap);
        MoveWindow(s_buttons[i].h, x, y, cellW, cellH, TRUE);
    }
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            hBrushBg = CreateSolidBrush(COL_BG);
            hBrushDisplay = CreateSolidBrush(COL_DISPLAY_BG);
            hFontDisplay = NULL;
            hFontBtn = NULL;

            s_button_count = 0;
            hExpr = CreateWindowExA(0, "STATIC", "",
                                    WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                    0, 0, 100, 22, hwnd, (HMENU)(INT_PTR)IDC_EXPR,
                                    GetModuleHandle(NULL), NULL);

            hDisplay = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "0",
                                       WS_CHILD | WS_VISIBLE | ES_RIGHT | ES_READONLY,
                                       0, 0, 100, 44, hwnd, (HMENU)(INT_PTR)IDC_DISPLAY,
                                       GetModuleHandle(NULL), NULL);

            last_result[0] = '\0';
            hFeedback = CreateWindowExA(0, "STATIC", "",
                                        WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                        0, 0, 100, 20, hwnd, (HMENU)(INT_PTR)IDC_FEEDBACK,
                                        GetModuleHandle(NULL), NULL);

            /* Proper layout: 4 columns. Scientific block on top, then standard numpad (7-8-9/, 4-5-6*, 1-2-3-, 0.=+), then C/BS */
            /* Row 0-2: Scientific */
            make_button(hwnd, "sqrt", IDC_BTN_SQRT, 0, 0);
            make_button(hwnd, "x^2", IDC_BTN_X2, 0, 1);
            make_button(hwnd, "1/x", IDC_BTN_1X, 0, 2);
            make_button(hwnd, "|x|", IDC_BTN_ABS, 0, 3);

            make_button(hwnd, "sin", IDC_BTN_SIN, 1, 0);
            make_button(hwnd, "cos", IDC_BTN_COS, 1, 1);
            make_button(hwnd, "tan", IDC_BTN_TAN, 1, 2);
            make_button(hwnd, "asin", IDC_BTN_INV, 1, 3);

            make_button(hwnd, "ln", IDC_BTN_LN, 2, 0);
            make_button(hwnd, "log", IDC_BTN_LOG, 2, 1);
            make_button(hwnd, "exp", IDC_BTN_EXP, 2, 2);
            make_button(hwnd, "pi", IDC_BTN_PI, 2, 3);

            make_button(hwnd, "e", IDC_BTN_EE, 3, 0);
            make_button(hwnd, "%", IDC_BTN_MOD, 3, 1);
            make_button(hwnd, "^", IDC_BTN_POW, 3, 2);
            /* col 3 empty for row 3 */

            /* Row 4-7: Standard numpad - numbers left 3 cols, operator right */
            make_button(hwnd, "7", IDC_BTN_0 + 7, 4, 0);
            make_button(hwnd, "8", IDC_BTN_0 + 8, 4, 1);
            make_button(hwnd, "9", IDC_BTN_0 + 9, 4, 2);
            make_button(hwnd, "/", IDC_BTN_DIV, 4, 3);

            make_button(hwnd, "4", IDC_BTN_0 + 4, 5, 0);
            make_button(hwnd, "5", IDC_BTN_0 + 5, 5, 1);
            make_button(hwnd, "6", IDC_BTN_0 + 6, 5, 2);
            make_button(hwnd, "*", IDC_BTN_MUL, 5, 3);

            make_button(hwnd, "1", IDC_BTN_0 + 1, 6, 0);
            make_button(hwnd, "2", IDC_BTN_0 + 2, 6, 1);
            make_button(hwnd, "3", IDC_BTN_0 + 3, 6, 2);
            make_button(hwnd, "-", IDC_BTN_MINUS, 6, 3);

            make_button(hwnd, "0", IDC_BTN_0, 7, 0);
            make_button(hwnd, ".", IDC_BTN_DOT, 7, 1);
            make_button(hwnd, "=", IDC_BTN_EQ, 7, 2);
            make_button(hwnd, "+", IDC_BTN_PLUS, 7, 3);

            make_button(hwnd, "C", IDC_BTN_C, 8, 0);
            make_button(hwnd, "BS", IDC_BTN_BS, 8, 1);

            {
                RECT rc;
                GetClientRect(hwnd, &rc);
                layout_children(hwnd, rc.right - rc.left, rc.bottom - rc.top);
            }
            break;
        }
        case WM_SIZE:
            if (hDisplay && hExpr)
            {
                int cw = LOWORD(lParam), ch = HIWORD(lParam);
                layout_children(hwnd, cw, ch);
            }
            break;
        case WM_ERASEBKGND:
            if (hBrushBg)
            {
                RECT rc;
                GetClientRect(hwnd, &rc);
                FillRect((HDC)wParam, &rc, hBrushBg);
            }
            return 1;
        case WM_CTLCOLOREDIT:
            if ((HWND)lParam == hDisplay && hBrushDisplay)
            {
                SetTextColor((HDC)wParam, COL_DISPLAY_FG);
                SetBkColor((HDC)wParam, COL_DISPLAY_BG);
                return (LRESULT)hBrushDisplay;
            }
            break;
        case WM_CTLCOLORSTATIC:
            if (((HWND)lParam == hExpr || (HWND)lParam == hFeedback) && hBrushDisplay)
            {
                SetTextColor((HDC)wParam, COL_EXPR_FG);
                SetBkColor((HDC)wParam, COL_DISPLAY_BG);
                return (LRESULT)hBrushDisplay;
            }
            break;
        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT p = (LPDRAWITEMSTRUCT)lParam;
            if (p->CtlType != ODT_BUTTON) break;
            int id = (int)p->CtlID;
            int type = get_button_type(id);
            COLORREF bg = COL_BTN_DIGIT;
            if (type == 1) bg = COL_BTN_OP;
            else if (type == 2) bg = COL_BTN_EQ;
            else if (type == 3) bg = COL_BTN_ACTION;
            else if (type == 4) bg = COL_BTN_MOD;
            else if (type == 5) bg = COL_BTN_FUNC;
            if (p->itemState & ODS_SELECTED)
            {
                int r = (int)GetRValue(bg) + 35; if (r > 255) r = 255;
                int g = (int)GetGValue(bg) + 35; if (g > 255) g = 255;
                int b = (int)GetBValue(bg) + 35; if (b > 255) b = 255;
                bg = RGB(r, g, b);
            }
            HBRUSH hb = CreateSolidBrush(bg);
            FillRect(p->hDC, &p->rcItem, hb);
            DeleteObject(hb);
            SetBkMode(p->hDC, TRANSPARENT);
            SetTextColor(p->hDC, COL_BTN_TEXT);
            if (hFontBtn) SelectObject(p->hDC, hFontBtn);
            {
                char buf[24];
                GetWindowTextA(p->hwndItem, buf, (int)sizeof(buf));
                DrawTextA(p->hDC, buf, -1, &p->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }
            return TRUE;
        }
        case WM_COMMAND:
        {
            int id = LOWORD(wParam);
            /* Menu commands */
            if (id == IDM_FILE_EXIT)
            {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                break;
            }
            if (id == IDM_EDIT_CLEAR)
            {
                on_clear(hwnd);
                break;
            }
            if (id == IDM_EDIT_HISTORY)
            {
                show_history(hwnd);
                break;
            }
            if (id == IDM_HELP_ABOUT)
            {
                MessageBoxA(hwnd,
                    "Scientific Calculator\n\n"
                    "Basic: + - * / %% ^\n"
                    "Functions: sqrt, sin, cos, tan, ln, log, exp, etc.\n"
                    "Keys: 0-9, Enter (=), Esc (clear), Backspace",
                    "About", MB_OK | MB_ICONINFORMATION);
                break;
            }
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
            if (hBrushBg) DeleteObject(hBrushBg);
            if (hBrushDisplay) DeleteObject(hBrushDisplay);
            PostQuitMessage(0);
            break;
        case WM_GETMINMAXINFO:
        {
            LPMINMAXINFO p = (LPMINMAXINFO)lParam;
            p->ptMinTrackSize.x = minClientW + 16;
            p->ptMinTrackSize.y = minClientH + 38;
            break;
        }
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
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);  /* overridden by WM_ERASEBKGND */
    wc.lpszClassName = "CalculatorGUI";

    if (!RegisterClassExA(&wc))
        return 1;

    HWND hwnd = CreateWindowExA(0, "CalculatorGUI", "Scientific Calculator",
                                WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT, 320, 520,
                                NULL, NULL, hInstance, NULL);
    if (!hwnd)
        return 1;

    /* Menu bar: File, Edit, Help */
    {
        HMENU hMenu = CreateMenu();
        HMENU hFile = CreatePopupMenu();
        AppendMenuA(hFile, MF_STRING, IDM_FILE_EXIT, "E&xit");
        AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hFile, "&File");

        HMENU hEdit = CreatePopupMenu();
        AppendMenuA(hEdit, MF_STRING, IDM_EDIT_CLEAR, "&Clear\tEsc");
        AppendMenuA(hEdit, MF_STRING, IDM_EDIT_HISTORY, "&History");
        AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hEdit, "&Edit");

        HMENU hHelp = CreatePopupMenu();
        AppendMenuA(hHelp, MF_STRING, IDM_HELP_ABOUT, "&About");
        AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hHelp, "&Help");

        SetMenu(hwnd, hMenu);
    }

    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
