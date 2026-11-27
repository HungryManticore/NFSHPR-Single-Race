// Single Race overlay (event mode, time of day, weather, weapons), drawn with Dear ImGui over the game's frames.
// Adapted from NFSHPR Event Randomizer 1.0 (src/Overlay.cpp):
// * Direct3D 11: IDXGISwapChain::Present / Present1 are hooked by patching DXGI's swap-chain vtable
//   (found through a throw-away swap chain), and the menu is drawn into the back buffer.
// * Keyboard / mouse: the game window is subclassed and DirectInput 8 GetDeviceState/GetDeviceData are
//   hooked (vtable), so the game reads nothing while the menu is open.
// New for Single Race:
// * Only on the Single Race event picker (SR_PickerVisible).
// * Controller: XInputGetState / XInputGetStateEx of xinput1_3.dll (the game's XInput) are redirected
//   to the same functions of xinput1_4.dll (or xinput9_1_0.dll); the plugin sees the real pad state
//   and the game gets an empty pad while the menu is open. v35: the game's Menu/Start input opens the menu (the
//   picker's footer prompt, sr_info.lua -> SR.OpenSettings); Start also closes it. Select does nothing on the picker.
// * Its own controller-friendly menu in two tabs (Race setup, Weapons): up/down picks the row (row 0 is the
//   tab bar), left/right changes it (on the tab bar: switches tabs), LB / RB switch tabs from any row,
//   A / Enter saves and closes, B / Esc cancels and closes.
// v40 ("hook later"): no hooks at start-up; the DirectInput, XInput and Direct3D 11 hooks are installed the first time
//   the Single Race picker is shown (OverlayStarter), with the other threads paused while XInput's code is patched.
// v28: the game's own fonts (traced from UI\FONTS: 535957 italic for tab and option names, 368865 for values and
//   prompts) and the game's button-prompt glyphs, embedded in SR_Assets.h. Prompts follow the last device used:
//   a newly pressed keyboard key switches them to keyboard keycaps, a pad button / trigger / stick to the controller.

#include "SR_Shared.h"
#include <d3d11.h>
#include <dxgi1_2.h>
#include <tlhelp32.h>
#include <float.h>
#include <stdio.h>
#include <string.h>

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "SR_Assets.h"

namespace {

const GUID kIID_ID3D11Device = {0xdb6f6ddb, 0xac77, 0x4e88, {0x82, 0x53, 0x81, 0x9d, 0xf9, 0xbb, 0xf1, 0x40}};
const GUID kIID_ID3D11Texture2D = {0x6f15aaf2, 0xd208, 0x4e89, {0x9a, 0xb4, 0x48, 0x95, 0x35, 0xd3, 0x4f, 0x9c}};
const GUID kIID_IDXGISwapChain1 = {0x790a45f7, 0x0d42, 0x4876, {0x98, 0x3a, 0x0a, 0x55, 0xcf, 0xe6, 0xf4, 0xaa}};
const GUID kIID_IDirectInput8A = {0xBF798030, 0x483A, 0x4DA2, {0xAA, 0x99, 0x5D, 0x64, 0xED, 0x36, 0x97, 0x00}};
const GUID kIID_IDirectInput8W = {0xBF798031, 0x483A, 0x4DA2, {0xAA, 0x99, 0x5D, 0x64, 0xED, 0x36, 0x97, 0x00}};
const GUID kGUID_SysKeyboard = {0x6F1D2B61, 0xD5A0, 0x11CF, {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};

// ====================================================================== state

CRITICAL_SECTION g_imguiLock;
bool g_imguiReady;
HWND g_hwnd;
WNDPROC g_origWndProc;
ID3D11Device* g_device;  // the device ImGui's DX11 backend was initialised with (not owned)
float g_fontPx = 20.0f;   // layout unit ("em"); the fonts are sized from it by their cap heights
ImFont* g_fontReg;          // values, prompts (368865)
ImFont* g_fontIt;           // tab names, option names, window title (535957)
float g_regCap, g_itCap;    // cap heights in pixels
struct IconRect { float w, h; ImVec2 uv0, uv1; };
IconRect g_icons[kIconCount];
bool g_iconsReady;
// Layout, measured each frame from the texts (the label column fits the widest option name).
float g_valueX, g_rowW;

// The device whose prompts are shown: the last one used.
enum InputDevice { kDevKeyboard, kDevPad };
volatile int g_inputDev = kDevKeyboard;
volatile LONG g_padActivity;  // bumped when a pad shows new input (a button, trigger or stick leaving rest)

volatile bool g_menuOpen;
ULONGLONG g_blockUntil;     // keep the game's input blocked shortly after closing
volatile bool g_waitPadRelease;  // after closing: the pad stays blocked until every button is released
bool g_waitKeyRelease;           // after closing: the keyboard stays blocked until the closing key is released

Conditions M;   // the menu's working copy
int g_row;      // index into the visible rows of the current tab (VisibleRows); 0 = the tab bar
// Two tabs: Race setup (mode, time of day, weather, difficulty, AI count) and Weapons (weapons mode, then the Custom
// charges). Row ids; the charge rows (enum Weapon + kRowFirstCharge) can be selected only for Custom; with Classic
// they are shown greyed out with the Classic Hot Pursuit preset (v27) and the cursor skips them.
enum Tab { kTabSetup, kTabWeapons, kTabCount };
const char* const kTabNames[kTabCount] = {"Race setup", "Weapons"};
int g_tab;
enum { kRowTabs, kRowMode, kRowTime, kRowTimeProgression, kRowWeather, kRowDifficulty, kRowRacers, kRowCops, kRowWeaponLevel, kRowWeapons, kRowFirstCharge };
int VisibleRows(int* ids) {
    int n = 0;
    ids[n++] = kRowTabs;
    if (g_tab == kTabSetup) {
        ids[n++] = kRowMode;
        ids[n++] = kRowTime;
        ids[n++] = kRowTimeProgression;
        ids[n++] = kRowWeather;
        ids[n++] = kRowDifficulty;
        ids[n++] = kRowRacers;
        if (M.eventMode == kModeHotPursuit) ids[n++] = kRowCops;  // Race and Cop Hot Pursuit: racers only
    } else {
        ids[n++] = kRowWeaponLevel;
        ids[n++] = kRowWeapons;
        if (M.weaponsMode == kWeaponsCustom)
            for (int w = 0; w < kWeaponCount; ++w) ids[n++] = kRowFirstCharge + w;
    }
    return n;
}
int RowCount() {
    int ids[kRowFirstCharge + kWeaponCount];
    return VisibleRows(ids);
}
int RowId(int index) {
    int ids[kRowFirstCharge + kWeaponCount];
    int n = VisibleRows(ids);
    return index >= 0 && index < n ? ids[index] : -1;
}

ULONGLONG Now() { return GetTickCount64(); }
bool InputBlocked() { return g_menuOpen || Now() < g_blockUntil; }

// v34: set by the game thread when sr_info.lua's footer prompt input is pressed (SR.OpenSettings); taken by
// HandleInput on the next frame.
volatile LONG g_menuRequest;

// ====================================================================== XInput

struct XGamepad { WORD buttons; BYTE lt, rt; SHORT lx, ly, rx, ry; };
struct XState { DWORD packet; XGamepad pad; };
typedef DWORD(WINAPI* XGetState_t)(DWORD, XState*);
const WORD kPadUp = 0x0001, kPadDown = 0x0002, kPadLeft = 0x0004, kPadRight = 0x0008, kPadStart = 0x0010, kPadBack = 0x0020, kPadLB = 0x0100,
           kPadRB = 0x0200, kPadA = 0x1000, kPadB = 0x2000;

XGetState_t g_realGetState, g_realGetStateEx;
volatile LONG g_padButtons[4];  // the real buttons of each pad, as last read by the game

bool PadActive(const XGamepad& p) {
    const int kStick = 12000;  // well past XInput's stick dead zones (7849 / 8689)
    return p.buttons || p.lt > 40 || p.rt > 40 || p.lx > kStick || p.lx < -kStick || p.ly > kStick || p.ly < -kStick ||
           p.rx > kStick || p.rx < -kStick || p.ry > kStick || p.ry < -kStick;
}

DWORD Filter(DWORD idx, DWORD r, XState* s) {
    if (r != 0 || !s || idx > 3) return r;
    g_padButtons[idx] = s->pad.buttons;
    static bool wasActive[4];
    bool active = PadActive(s->pad);
    if (active && !wasActive[idx]) InterlockedIncrement(&g_padActivity);
    wasActive[idx] = active;
    if (InputBlocked() || g_waitPadRelease) memset(&s->pad, 0, sizeof(s->pad));
    else if (SR_PickerVisible()) s->pad.buttons &= (WORD)~kPadBack;  // Select does nothing on the picker (v35)
    return r;
}
DWORD WINAPI Hook_XInputGetState(DWORD idx, XState* s) { return Filter(idx, g_realGetState(idx, s), s); }
DWORD WINAPI Hook_XInputGetStateEx(DWORD idx, XState* s) { return Filter(idx, g_realGetStateEx(idx, s), s); }

// Overwrites the start of fn with "mov rax, hook; jmp rax". The original is never called (the real state
// comes from another XInput DLL), so no instructions need to be relocated.
const int kRedirectLen = 12;
bool Redirect(void* fn, void* hook) {
    if (!fn) return false;
    uint8_t p[kRedirectLen] = {0x48, 0xB8, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xE0};
    uintptr_t h = (uintptr_t)hook;
    memcpy(p + 2, &h, 8);
    DWORD old;
    if (!VirtualProtect(fn, sizeof(p), PAGE_EXECUTE_READWRITE, &old)) return false;
    memcpy(fn, p, sizeof(p));
    VirtualProtect(fn, sizeof(p), old, &old);
    FlushInstructionCache(GetCurrentProcess(), fn, sizeof(p));
    return true;
}

// v40: the XInput functions are patched while the game is running (on the picker) and may be polling the pad on
// another thread, so every other thread is suspended for the patch (as MinHook does), and the patch is retried if a
// thread was stopped inside the bytes being replaced. Nothing that takes a lock (no logging, no heap) runs while the
// threads are suspended. Returns the number of attempts used, 0 if no safe moment was found.
HANDLE g_frozen[1024];
int g_frozenCount;
void ThawAll() {
    for (int i = 0; i < g_frozenCount; ++i) {
        ResumeThread(g_frozen[i]);
        CloseHandle(g_frozen[i]);
    }
    g_frozenCount = 0;
}
// Suspends every other thread of the process; false if one of them is executing inside [a, a+len) or [b, b+len).
bool FreezeOthers(uintptr_t a, uintptr_t b) {
    g_frozenCount = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;
    DWORD pid = GetCurrentProcessId(), self = GetCurrentThreadId();
    THREADENTRY32 te;
    te.dwSize = sizeof(te);
    bool clear = true;
    for (BOOL ok = Thread32First(snap, &te); ok && clear; ok = Thread32Next(snap, &te)) {
        if (te.th32OwnerProcessID != pid || te.th32ThreadID == self) continue;
        if (g_frozenCount >= (int)(sizeof(g_frozen) / sizeof(g_frozen[0]))) { clear = false; break; }
        HANDLE t = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);
        if (!t) continue;
        if (SuspendThread(t) == (DWORD)-1) { CloseHandle(t); continue; }
        g_frozen[g_frozenCount++] = t;
        alignas(16) CONTEXT ctx;
        memset(&ctx, 0, sizeof(ctx));
        ctx.ContextFlags = CONTEXT_CONTROL;
        if (GetThreadContext(t, &ctx)) {
            uintptr_t ip = (uintptr_t)ctx.Rip;
            if ((a && ip >= a && ip < a + kRedirectLen) || (b && ip >= b && ip < b + kRedirectLen)) clear = false;
        }
    }
    CloseHandle(snap);
    if (!clear) ThawAll();
    return clear;
}

void HookXInput() {
    HMODULE game = LoadLibraryW(L"xinput1_3.dll");
    if (!game) { SR_Log("overlay: xinput1_3.dll not found - controller not available for the menu"); return; }
    HMODULE real = LoadLibraryW(L"xinput1_4.dll");
    if (real) {
        g_realGetState = (XGetState_t)GetProcAddress(real, "XInputGetState");
        g_realGetStateEx = (XGetState_t)GetProcAddress(real, (LPCSTR)100);
    } else if ((real = LoadLibraryW(L"xinput9_1_0.dll")) != nullptr) {
        g_realGetState = (XGetState_t)GetProcAddress(real, "XInputGetState");
    }
    if (!g_realGetState) { SR_Log("overlay: no second XInput DLL - controller not available for the menu"); return; }
    if (!g_realGetStateEx) g_realGetStateEx = g_realGetState;
    void* fa = (void*)GetProcAddress(game, "XInputGetState");
    void* fb = (void*)GetProcAddress(game, (LPCSTR)100);
    int tries = 0;
    bool frozen = false;
    while (tries < 50 && !(frozen = FreezeOthers((uintptr_t)fa, (uintptr_t)fb))) { ++tries; Sleep(1); }
    if (!frozen) {
        SR_Log("overlay: XInput not hooked (no safe moment to patch it) - controller not available for the menu");
        return;
    }
    int threads = g_frozenCount;
    bool a = Redirect(fa, (void*)Hook_XInputGetState);
    bool b = Redirect(fb, (void*)Hook_XInputGetStateEx);
    ThawAll();
    SR_Log("overlay: XInput hooked (XInputGetState %s, XInputGetStateEx %s; %d other threads paused for the patch, %d retries)",
           a ? "yes" : "no", b ? "yes" : "no", threads, tries);
}

// ====================================================================== the menu

void OpenMenu() {
    M = SR_GetConditions();
    g_tab = kTabSetup;
    g_row = 1;  // the first setting (Mode); up reaches the tab bar
    g_menuOpen = true;
    SR_Log("conditions menu opened (%s, %s, %s, %s, weapons %s, AI: race %d racers, hot pursuit %d racers + %d cops, "
           "cop hot pursuit %d racers)",
           kEventModeNames[M.eventMode], kDifficultyNames[M.difficulty], kTimeNames[M.time], kWeatherNames[M.weather],
           kWeaponsModeNames[M.weaponsMode], M.raceRacers, M.hpRacers, M.hpCops, M.copRacers);
}

void CloseMenu(bool save) {
    if (save) SR_SetConditions(M);
    g_menuOpen = false;
    g_blockUntil = Now() + 400;
    g_waitPadRelease = true;
    g_waitKeyRelease = true;
    Conditions c = SR_GetConditions();
    SR_Log("conditions menu %s: %s, %s, %s (time progression %s), %s, weapons %s level %d (racers %d %d %d %d, cops %d %d %d %d), AI: race %d racers, "
           "hot pursuit %d racers + %d cops, cop hot pursuit %d racers", save ? "saved" : "cancelled",
           kEventModeNames[c.eventMode], kDifficultyNames[c.difficulty], kTimeNames[c.time], c.timeProgression ? "on" : "off", kWeatherNames[c.weather], kWeaponsModeNames[c.weaponsMode], c.weaponLevel, c.charges[0], c.charges[1],
           c.charges[2], c.charges[3], c.charges[4], c.charges[5], c.charges[6], c.charges[7], c.raceRacers, c.hpRacers, c.hpCops,
           c.copRacers);
}

// LB / RB, or left / right on the tab bar. The cursor stays on the tab bar, or goes to the tab's first setting.
void SwitchTab(int dir) {
    g_tab = (g_tab + dir + kTabCount) % kTabCount;
    if (g_row != 0) g_row = 1;
}

// AI count rows. Race: 1-7 racers. Hot Pursuit: 1-6 racers and 1-6 cops, 7 in all at most: a step up that would
// go past 7 does nothing. Cop Hot Pursuit: 1-7 racers (the player is the only cop).
void StepCount(int id, int dir) {
    bool hp = M.eventMode == kModeHotPursuit;
    if (M.eventMode == kModeCopPursuit) {
        int n = M.copRacers + dir;
        if (n >= kCopRacersMin && n <= kCopRacersMax) M.copRacers = n;
        return;
    }
    if (!hp) {
        int n = M.raceRacers + dir;
        if (n >= kRaceRacersMin && n <= kRaceRacersMax) M.raceRacers = n;
        return;
    }
    int& v = id == kRowCops ? M.hpCops : M.hpRacers;
    int n = v + dir, mx = id == kRowCops ? kHpCopsMax : kHpRacersMax;
    if (n < kHpMin || n > mx) return;
    if (dir > 0 && M.hpRacers + M.hpCops + 1 > kMaxAI) return;
    v = n;
}

void Step(int dir) {
    int id = RowId(g_row);
    if (id == kRowTabs) SwitchTab(dir);
    else if (id == kRowMode) M.eventMode = (M.eventMode + dir + kEventModeCount) % kEventModeCount;
    else if (id == kRowDifficulty) M.difficulty = (M.difficulty + dir + kDifficultyCount) % kDifficultyCount;
    else if (id == kRowTime) M.time = (M.time + dir + kTimeCount) % kTimeCount;
    else if (id == kRowTimeProgression) M.timeProgression = !M.timeProgression;
    else if (id == kRowWeather) M.weather = (M.weather + dir + kWeatherCount) % kWeatherCount;
    else if (id == kRowRacers || id == kRowCops) StepCount(id, dir);
    else if (id == kRowWeaponLevel)
        M.weaponLevel = (M.weaponLevel - kWeaponLevelMin + dir + kWeaponLevelMax - kWeaponLevelMin + 1) %
                            (kWeaponLevelMax - kWeaponLevelMin + 1) + kWeaponLevelMin;
    else if (id == kRowWeapons) M.weaponsMode = (M.weaponsMode + dir + kWeaponsModeCount) % kWeaponsModeCount;
    else if (id >= kRowFirstCharge) {
        int& n = M.charges[id - kRowFirstCharge];
        n += dir;
        if (n < 0) n = 0;
        if (n > kMaxCharges) n = kMaxCharges;
    }
}

void MoveRow(int dir) {
    g_row += dir;
    if (g_row < 0) g_row = 0;
    if (g_row >= RowCount()) g_row = RowCount() - 1;
}

const char* KeyName(int vk, char* out) {
    if (vk >= VK_F1 && vk <= VK_F24) sprintf(out, "F%d", vk - VK_F1 + 1);
    else sprintf(out, "key %d", vk);
    return out;
}

// ---------------------------------------------------------------------- text and prompt drawing

const char* Upper(const char* in, char* out, int size) {
    int i = 0;
    for (; in[i] && i < size - 1; ++i) out[i] = (in[i] >= 'a' && in[i] <= 'z') ? (char)(in[i] - 32) : in[i];
    out[i] = 0;
    return out;
}
float TextW(ImFont* f, const char* t, float size = 0) {
    return f->CalcTextSizeA(size > 0 ? size : f->FontSize, FLT_MAX, 0, t).x;
}
// Draws t with its baseline at y (so the italic and regular fonts line up).
void DrawAt(ImDrawList* dl, ImFont* f, float x, float baseline, ImU32 col, const char* t, float size = 0) {
    float sz = size > 0 ? size : f->FontSize;
    dl->AddText(f, sz, ImVec2(x, baseline - f->Ascent * sz / f->FontSize), col, t);
}

void DrawIcon(ImDrawList* dl, int id, float x, float cy, float w = 0) {
    const IconRect& r = g_icons[id];
    float ww = w > 0 ? w : r.w;
    dl->AddImage(ImGui::GetIO().Fonts->TexID, ImVec2(x, cy - r.h * 0.5f), ImVec2(x + ww, cy + r.h * 0.5f), r.uv0, r.uv1);
}

// A keyboard key with a name on it, built like the game builds it: keycap left half + stretched middle + right half,
// with the key name written on the cap face.
float KeycapTextSize() { return g_icons[IconKeycapLeft].h * 0.36f / kFontRegularCapEm; }
float KeycapWidth(const char* text) {
    const IconRect &l = g_icons[IconKeycapLeft], &r = g_icons[IconKeycapRight];
    float inner = TextW(g_fontReg, text, KeycapTextSize()) + l.w * 0.84f - l.w - r.w;
    return l.w + (inner > 0 ? inner : 0) + r.w;
}
void DrawKeycap(ImDrawList* dl, const char* text, float x, float cy) {
    const IconRect &l = g_icons[IconKeycapLeft], &r = g_icons[IconKeycapRight];
    float w = KeycapWidth(text), inner = w - l.w - r.w, size = KeycapTextSize();
    DrawIcon(dl, IconKeycapLeft, x, cy);
    if (inner > 0) DrawIcon(dl, IconKeycapMiddle, x + l.w, cy, inner);
    DrawIcon(dl, IconKeycapRight, x + l.w + inner, cy);
    float top = cy - l.h * 0.5f, cap = size * kFontRegularCapEm;
    DrawAt(dl, g_fontReg, x + (w - TextW(g_fontReg, text, size)) * 0.5f, top + l.h * 0.47f + cap * 0.5f, IM_COL32(255, 255, 255, 255), text, size);
}

// One prompt: up to two icons (or a named keycap) and its label.
struct Prompt { int icon1, icon2; const char* keycap; const char* label; };
float PromptIconsW(const Prompt& p, float gap) {
    float w = 0;
    if (p.keycap) w += KeycapWidth(p.keycap);
    if (p.icon1 >= 0) w += g_icons[p.icon1].w;
    if (p.icon2 >= 0) w += gap + g_icons[p.icon2].w;
    return w;
}
float PromptH(const Prompt& p) {
    float h = p.keycap ? g_icons[IconKeycapLeft].h : 0;
    if (p.icon1 >= 0 && g_icons[p.icon1].h > h) h = g_icons[p.icon1].h;
    if (p.icon2 >= 0 && g_icons[p.icon2].h > h) h = g_icons[p.icon2].h;
    return h;
}
// A row of prompts, like the game's footer; reserves its space in the window.
void PromptBar(const Prompt* ps, int n, ImU32 col) {
    const float em = g_fontPx, gapIcons = em * 0.12f, gapText = em * 0.35f, gapItems = em * 1.1f;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float h = g_regCap * 1.3f, w = 0;
    char up[64];
    for (int i = 0; i < n; ++i) {
        if (PromptH(ps[i]) > h) h = PromptH(ps[i]);
        w += (i ? gapItems : 0) + PromptIconsW(ps[i], gapIcons) + gapText + TextW(g_fontReg, Upper(ps[i].label, up, 64));
    }
    float cy = p.y + h * 0.5f, x = p.x;
    for (int i = 0; i < n; ++i) {
        const Prompt& q = ps[i];
        if (i) x += gapItems;
        if (q.keycap) { DrawKeycap(dl, q.keycap, x, cy); x += KeycapWidth(q.keycap); }
        if (q.icon1 >= 0) { DrawIcon(dl, q.icon1, x, cy); x += g_icons[q.icon1].w; }
        if (q.icon2 >= 0) { x += gapIcons; DrawIcon(dl, q.icon2, x, cy); x += g_icons[q.icon2].w; }
        x += gapText;
        Upper(q.label, up, 64);
        DrawAt(dl, g_fontReg, x, cy + g_regCap * 0.5f, col, up);
        x += TextW(g_fontReg, up);
    }
    ImGui::Dummy(ImVec2(w, h));
}

// ---------------------------------------------------------------------- the menu

void MeasureLayout() {
    const float em = g_fontPx;
    char up[64], v[64];
    const char* labels[] = {"Mode", "Time of day", "Time progression", "Weather", "Difficulty", "Racers", "Cops", "Weapon level", "Weapons"};
    float lw = 0, vw = 0;
    for (const char* l : labels) { float t = TextW(g_fontIt, Upper(l, up, 64)); if (t > lw) lw = t; }
    for (int w = 0; w < kWeaponCount; ++w) {
        char l[48];
        sprintf(l, "    %s", kWeaponLabels[w]);
        float t = TextW(g_fontIt, Upper(l, up, 64));
        if (t > lw) lw = t;
    }
    auto value = [&](const char* s) { sprintf(v, "<   %s   >", s); float t = TextW(g_fontReg, v); if (t > vw) vw = t; };
    for (int i = 0; i < kEventModeCount; ++i) value(kEventModeLabels[i]);
    for (int i = 0; i < kTimeCount; ++i) value(kTimeNames[i]);
    for (int i = 0; i < kWeatherCount; ++i) value(kWeatherLabels[i]);
    for (int i = 0; i < kDifficultyCount; ++i) value(kDifficultyNames[i]);
    value("Classic Hot Pursuit");
    value("Custom");
    value("10");
    g_valueX = em * 0.6f + lw + em * 1.5f;
    g_rowW = g_valueX + vw + em * 0.6f;
    float tabs = em * 0.6f + em * 3.6f;  // the tab bar: names, gaps and the "<  >" marker
    for (int t = 0; t < kTabCount; ++t) tabs += TextW(g_fontIt, Upper(kTabNames[t], up, 64)) + em * 1.6f;
    if (tabs > g_rowW) g_rowW = tabs;
}

// The vertical centre of a row's text is shared by both fonts: same baseline, caps centred on the regular font.
float RowBaseline(float top) { return top + g_fontPx * 0.9f + g_regCap * 0.5f; }

void Row(int id, const char* label, const char* value) {
    const float em = g_fontPx;
    bool sel = RowId(g_row) == id;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (sel) dl->AddRectFilled(p, ImVec2(p.x + g_rowW, p.y + em * 1.8f), IM_COL32(255, 158, 31, 60), em * 0.25f);
    float by = RowBaseline(p.y);
    ImU32 col = ImGui::GetColorU32(ImGuiCol_Text);
    char up[64], v[64];
    DrawAt(dl, g_fontIt, p.x + em * 0.6f, by, col, Upper(label, up, 64));
    sprintf(v, sel ? "<   %s   >" : "    %s", value);
    DrawAt(dl, g_fontReg, p.x + g_valueX, by, sel ? IM_COL32(255, 158, 31, 255) : col, v);
    ImGui::Dummy(ImVec2(g_rowW, em * 1.8f));
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + em * 2.0f));
}

// The tab bar: both tab names, the active one filled; highlighted like a row when the cursor is on it.
void TabBar() {
    const float em = g_fontPx;
    bool sel = g_row == 0;
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = g_rowW;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (sel) dl->AddRectFilled(p, ImVec2(p.x + w, p.y + em * 1.8f), IM_COL32(255, 158, 31, 60), em * 0.25f);
    float x = p.x + em * 0.6f, by = RowBaseline(p.y);
    char up[64];
    for (int t = 0; t < kTabCount; ++t) {
        const char* name = Upper(kTabNames[t], up, 64);
        float tw = TextW(g_fontIt, name);
        ImVec2 a(x - em * 0.5f, p.y + em * 0.15f), b(x + tw + em * 0.5f, p.y + em * 1.65f);
        if (t == g_tab) {
            dl->AddRectFilled(a, b, IM_COL32(255, 158, 31, sel ? 200 : 140), em * 0.2f);
            DrawAt(dl, g_fontIt, x, by, IM_COL32(20, 20, 20, 255), name);
        } else {
            dl->AddRect(a, b, IM_COL32(160, 160, 160, 160), em * 0.2f);
            DrawAt(dl, g_fontIt, x, by, IM_COL32(200, 200, 200, 255), name);
        }
        x += tw + em * 1.6f;
    }
    if (sel) DrawAt(dl, g_fontReg, p.x + w - em * 3.2f, by, IM_COL32(255, 158, 31, 255), "<  >");
    ImGui::Dummy(ImVec2(w, em * 1.8f));
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + em * 2.0f));
}

// A small heading inside the Weapons tab ("Racers" / "Cops").
void SubHeading(const char* text) {
    const float em = g_fontPx;
    ImVec2 p = ImGui::GetCursorScreenPos();
    char up[64];
    DrawAt(ImGui::GetWindowDrawList(), g_fontIt, p.x + em * 0.6f, p.y + em * 0.6f + g_itCap * 0.5f,
           ImGui::GetColorU32(ImGuiCol_TextDisabled), Upper(text, up, 64));
    ImGui::Dummy(ImVec2(g_rowW, em * 1.2f));
}

void DrawMenu() {
    ImGuiIO& io = ImGui::GetIO();
    const float em = g_fontPx;
    MeasureLayout();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::PushFont(g_fontIt);  // the window title
    ImGui::Begin("SINGLE RACE###sr", nullptr,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove);
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(g_rowW, em * 0.3f));
    TabBar();
    ImGui::Dummy(ImVec2(0, em * 0.3f));
    if (g_tab == kTabSetup) {
        Row(kRowMode, "Mode", kEventModeLabels[M.eventMode]);
        Row(kRowTime, "Time of day", kTimeNames[M.time]);
        Row(kRowTimeProgression, "Time progression", M.timeProgression ? "On" : "Off");
        Row(kRowWeather, "Weather", kWeatherLabels[M.weather]);
        Row(kRowDifficulty, "Difficulty", kDifficultyNames[M.difficulty]);
        char r[8], c[8];
        if (M.eventMode == kModeHotPursuit) {
            sprintf(r, "%d", M.hpRacers);
            sprintf(c, "%d", M.hpCops);
            Row(kRowRacers, "Racers", r);
            Row(kRowCops, "Cops", c);
        } else {
            sprintf(r, "%d", M.eventMode == kModeCopPursuit ? M.copRacers : M.raceRacers);
            Row(kRowRacers, "Racers", r);
        }
    } else {
        char lv[16];
        sprintf(lv, "Level %d", M.weaponLevel);
        Row(kRowWeaponLevel, "Weapon level", lv);
        Row(kRowWeapons, "Weapons", M.weaponsMode == kWeaponsCustom ? "Custom" : "Classic Hot Pursuit");
        // Custom: the ini charges, adjustable. Classic: the Classic Hot Pursuit preset, greyed out and not
        // selectable (the Custom values are kept and come back when Custom is chosen again).
        bool custom = M.weaponsMode == kWeaponsCustom;
        if (!custom) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        for (int w = 0; w < kWeaponCount; ++w) {
            if (w == kRacerSpikeStrip || w == kCopSpikeStrip) {
                ImGui::Dummy(ImVec2(0, em * 0.2f));
                SubHeading(w == kRacerSpikeStrip ? "Racers" : "Cops");
            }
            char v[8], label[40];
            sprintf(v, "%d", custom ? M.charges[w] : kClassicCharges[w]);
            sprintf(label, "    %s", kWeaponLabels[w]);
            Row(kRowFirstCharge + w, label, v);
        }
        if (!custom) ImGui::PopStyleColor();
    }
    ImGui::Dummy(ImVec2(0, em * 0.4f));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, em * 0.2f));
    ImU32 col = ImGui::GetColorU32(ImGuiCol_Text);
    if (!g_iconsReady) {
        ImGui::TextDisabled("D-pad / arrows: choose     LB / RB: tabs     A / Enter: save     B / Esc: cancel");
    } else if (g_inputDev == kDevPad) {
        const Prompt ps[] = {{IconDpadUpDown, -1, nullptr, "Choose"}, {IconDpadLeftRight, -1, nullptr, "Change"},
                             {IconPadLB, IconPadRB, nullptr, "Tabs"}, {IconPadA, -1, nullptr, "Save"},
                             {IconPadB, -1, nullptr, "Cancel"}};
        PromptBar(ps, 5, col);
    } else {
        // Keyboard: tabs are switched with left / right on the tab bar.
        const Prompt ps[] = {{IconKeyUp, IconKeyDown, nullptr, "Choose"}, {IconKeyLeft, IconKeyRight, nullptr, "Change"},
                             {IconKeyEnter, -1, nullptr, "Save"}, {-1, -1, "ESC", "Cancel"}};
        PromptBar(ps, 4, col);
    }
    ImGui::End();
}

// v34: the top-right "Race Settings" hint (v26-v33) is removed: the picker's footer prompt bar shows it
// natively (WALL v12, sr_info.lua).

// ====================================================================== window procedure

bool IsInputMessage(UINT m) {
    return (m >= WM_KEYFIRST && m <= WM_KEYLAST) || (m >= WM_MOUSEFIRST && m <= WM_MOUSELAST) || m == WM_INPUT;
}

LRESULT CALLBACK OverlayWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (InputBlocked() && IsInputMessage(m)) return 0;
    return CallWindowProcW(g_origWndProc, h, m, w, l);
}

// ====================================================================== DirectInput (keyboard / mouse blocking)

typedef HRESULT(WINAPI* GetDeviceState_t)(void* self, DWORD cb, void* data);
typedef HRESULT(WINAPI* GetDeviceData_t)(void* self, DWORD cbObj, void* rgdod, DWORD* inOut, DWORD flags);

struct DiVtable { void** vt; GetDeviceState_t state; GetDeviceData_t data; };
DiVtable g_diVt[2];
int g_diVtCount;

const DiVtable* FindDiVt(void* self) {
    void** vt = *(void***)self;
    for (int i = 0; i < g_diVtCount; ++i)
        if (g_diVt[i].vt == vt) return &g_diVt[i];
    return nullptr;
}

HRESULT WINAPI Hook_GetDeviceState(void* self, DWORD cb, void* data) {
    const DiVtable* v = FindDiVt(self);
    if (!v) return E_FAIL;
    HRESULT hr = v->state(self, cb, data);
    if (SUCCEEDED(hr) && data && InputBlocked()) memset(data, 0, cb);
    return hr;
}

HRESULT WINAPI Hook_GetDeviceData(void* self, DWORD cbObj, void* rgdod, DWORD* inOut, DWORD flags) {
    const DiVtable* v = FindDiVt(self);
    if (!v) return E_FAIL;
    HRESULT hr = v->data(self, cbObj, rgdod, inOut, flags);
    if (SUCCEEDED(hr) && inOut && rgdod && InputBlocked()) *inOut = 0;  // events are consumed and dropped
    return hr;
}

void* PatchVtable(void** vt, int index, void* fn) {
    DWORD old;
    if (!VirtualProtect(&vt[index], sizeof(void*), PAGE_READWRITE, &old)) return nullptr;
    void* prev = vt[index];
    vt[index] = fn;
    VirtualProtect(&vt[index], sizeof(void*), old, &old);
    return prev;
}

void HookDirectInput() {
    typedef HRESULT(WINAPI * Create_t)(HINSTANCE, DWORD, REFIID, void**, void*);
    HMODULE di = GetModuleHandleW(L"dinput8.dll");
    if (!di) di = LoadLibraryW(L"dinput8.dll");
    Create_t create = di ? (Create_t)GetProcAddress(di, "DirectInput8Create") : nullptr;
    if (!create) { SR_Log("overlay: DirectInput8Create not found - keyboard is not blocked"); return; }
    const GUID* iids[2] = {&kIID_IDirectInput8W, &kIID_IDirectInput8A};
    for (int k = 0; k < 2; ++k) {
        void* dinput = nullptr;
        if (FAILED(create(GetModuleHandleW(nullptr), 0x0800, *iids[k], &dinput, nullptr)) || !dinput) continue;
        void** divt = *(void***)dinput;
        typedef HRESULT(WINAPI * CreateDevice_t)(void*, REFGUID, void**, void*);
        void* dev = nullptr;
        if (SUCCEEDED(((CreateDevice_t)divt[3])(dinput, kGUID_SysKeyboard, &dev, nullptr)) && dev) {
            void** vt = *(void***)dev;
            bool known = false;
            for (int i = 0; i < g_diVtCount; ++i) known |= g_diVt[i].vt == vt;
            if (!known && g_diVtCount < 2) {
                DiVtable& d = g_diVt[g_diVtCount];
                d.vt = vt;
                d.state = (GetDeviceState_t)vt[9];
                d.data = (GetDeviceData_t)vt[10];
                ++g_diVtCount;  // visible before the patch makes the hooks reachable
                PatchVtable(vt, 9, (void*)Hook_GetDeviceState);
                PatchVtable(vt, 10, (void*)Hook_GetDeviceData);
            }
            ((ULONG(WINAPI*)(void*))(*(void***)dev)[2])(dev);  // Release
        }
        ((ULONG(WINAPI*)(void*))divt[2])(dinput);  // Release
    }
    SR_Log("overlay: DirectInput hooked (%d device interface%s)", g_diVtCount, g_diVtCount == 1 ? "" : "s");
}

// ====================================================================== Direct3D 11

typedef HRESULT(WINAPI* Present_t)(IDXGISwapChain*, UINT, UINT);
typedef HRESULT(WINAPI* Present1_t)(IDXGISwapChain1*, UINT, UINT, const DXGI_PRESENT_PARAMETERS*);
Present_t g_origPresent;
Present1_t g_origPresent1;

// Area-average resample of an embedded icon (straight alpha, averaged premultiplied) into the atlas.
void ResampleIcon(const IconData& src, unsigned char* dst, int pitch, int dw, int dh) {
    float sx = (float)src.w / dw, sy = (float)src.h / dh;
    for (int y = 0; y < dh; ++y)
        for (int x = 0; x < dw; ++x) {
            float x0 = x * sx, x1 = x0 + (sx > 1 ? sx : 1), y0 = y * sy, y1 = y0 + (sy > 1 ? sy : 1);
            if (sx < 1) { x0 = (x + 0.5f) * sx - 0.5f; if (x0 < 0) x0 = 0; x1 = x0 + 1; }
            if (sy < 1) { y0 = (y + 0.5f) * sy - 0.5f; if (y0 < 0) y0 = 0; y1 = y0 + 1; }
            float acc[4] = {0, 0, 0, 0}, wsum = 0;
            for (int yy = (int)y0; yy < src.h && yy < y1; ++yy) {
                float wy = (yy + 1 < y1 ? yy + 1 : y1) - (yy > y0 ? yy : y0);
                for (int xx = (int)x0; xx < src.w && xx < x1; ++xx) {
                    float wx = (xx + 1 < x1 ? xx + 1 : x1) - (xx > x0 ? xx : x0);
                    const uint8_t* q = src.rgba + (yy * src.w + xx) * 4;
                    float wgt = wx * wy, a = q[3] * wgt;
                    acc[0] += q[0] * a; acc[1] += q[1] * a; acc[2] += q[2] * a; acc[3] += a;
                    wsum += wgt;
                }
            }
            unsigned char* o = dst + y * pitch + x * 4;
            if (acc[3] <= 0 || wsum <= 0) { o[0] = o[1] = o[2] = o[3] = 0; continue; }
            for (int c = 0; c < 3; ++c) { float v = acc[c] / acc[3]; o[c] = (unsigned char)(v > 255 ? 255 : v + 0.5f); }
            float al = acc[3] / wsum;
            o[3] = (unsigned char)(al > 255 ? 255 : al + 0.5f);
        }
}

// The game's fonts (embedded TTFs) and its button-prompt glyphs (embedded RGBA, put into ImGui's font atlas so they
// draw with the same texture). em = the layout unit: regular caps are 0.70 em (as tall as Segoe UI's at em),
// italic caps 0.60 em (the game's option names are a little smaller than its values), the Xbox A button 1.3x the
// regular caps (as in the game's footer).
void LoadFonts(float em) {
    ImGuiIO& io = ImGui::GetIO();
    static const ImWchar ranges[] = {0x0020, 0x017F, 0};
    ImFontConfig cfg;
    cfg.FontDataOwnedByAtlas = false;  // static data
    cfg.OversampleH = 2;
    cfg.OversampleV = 2;
    float regPx = (float)(int)(em * 0.70f / kFontRegularCapEm + 0.5f), itPx = (float)(int)(em * 0.60f / kFontItalicCapEm + 0.5f);
    g_fontReg = io.Fonts->AddFontFromMemoryTTF((void*)kFontRegular, (int)sizeof(kFontRegular), regPx, &cfg, ranges);
    g_fontIt = io.Fonts->AddFontFromMemoryTTF((void*)kFontItalic, (int)sizeof(kFontItalic), itPx, &cfg, ranges);
    if (!g_fontReg || !g_fontIt) {
        SR_Log("overlay: the game fonts could not be loaded - using ImGui's default font");
        io.Fonts->Clear();
        ImFontConfig d;
        d.SizePixels = em;
        g_fontReg = g_fontIt = io.Fonts->AddFontDefault(&d);
        g_regCap = g_itCap = em * 0.7f;
        return;
    }
    g_regCap = regPx * kFontRegularCapEm;
    g_itCap = itPx * kFontItalicCapEm;
    float unit = g_regCap * 1.3f;  // the Xbox A button's height
    int ids[kIconCount];
    for (int i = 0; i < kIconCount; ++i) {
        const IconData& d = kIcons[i];
        // Keyboard keycaps are drawn a third larger than the game's ratio, so the key names on them stay readable
        // at the overlay's text sizes.
        bool key = i >= IconKeyUp && i <= IconKeycapRight;
        int h = (int)(unit * d.rel * (key ? 1.35f : 1.0f) + 0.5f);
        if (h < 1) h = 1;
        int w = i == IconKeycapMiddle ? d.w : (int)((float)d.w * h / d.h + 0.5f);
        if (w < 1) w = 1;
        ids[i] = io.Fonts->AddCustomRectRegular(w, h);
        g_icons[i].w = (float)w;
        g_icons[i].h = (float)h;
    }
    if (!io.Fonts->Build()) { SR_Log("overlay: font atlas build failed"); return; }
    unsigned char* px = nullptr;
    int tw = 0, th = 0;
    io.Fonts->GetTexDataAsRGBA32(&px, &tw, &th);  // cached by the atlas; the DX11 backend uploads this buffer
    if (!px) return;
    for (int i = 0; i < kIconCount; ++i) {
        ImFontAtlasCustomRect* r = io.Fonts->GetCustomRectByIndex(ids[i]);
        if (!r || !r->IsPacked()) return;
        ResampleIcon(kIcons[i], px + (r->Y * tw + r->X) * 4, tw * 4, r->Width, r->Height);
        io.Fonts->CalcCustomRectUV(r, &g_icons[i].uv0, &g_icons[i].uv1);
        if (i == IconKeycapMiddle) {  // stretched: sample the middle column only, so the edges don't bleed in
            float du = (g_icons[i].uv1.x - g_icons[i].uv0.x) * 0.5f;
            g_icons[i].uv0.x += du * 0.9f;
            g_icons[i].uv1.x -= du * 0.9f;
        }
    }
    g_iconsReady = true;
}

bool InitImGui(IDXGISwapChain* sc) {
    DXGI_SWAP_CHAIN_DESC desc;
    if (FAILED(sc->GetDesc(&desc)) || !desc.OutputWindow) return false;
    ID3D11Device* dev = nullptr;
    if (FAILED(sc->GetDevice(kIID_ID3D11Device, (void**)&dev)) || !dev) return false;
    ID3D11DeviceContext* ctx = nullptr;
    dev->GetImmediateContext(&ctx);

    g_hwnd = desc.OutputWindow;
    RECT rc;
    GetClientRect(g_hwnd, &rc);
    float h = (float)(rc.bottom - rc.top);
    if (h < 100) h = (float)desc.BufferDesc.Height;
    int fontSize = SR_GetConditions().fontSize;
    g_fontPx = fontSize > 0 ? (float)fontSize : h * 0.024f;
    if (g_fontPx < 16) g_fontPx = 16;
    if (g_fontPx > 44) g_fontPx = 44;

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding = 8;
    st.FrameRounding = 5;
    st.WindowBorderSize = 1;
    st.ScaleAllSizes(g_fontPx / 16.0f);
    ImVec4* c = st.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.07f, 0.09f, 0.94f);
    c[ImGuiCol_TitleBg] = c[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.22f, 0.45f, 1.0f);
    LoadFonts(g_fontPx);
    // Prompts start with the controller if one is connected, else the keyboard; then they follow the last input.
    if (g_realGetState) {
        XState xs;
        for (DWORD i = 0; i < 4; ++i)
            if (g_realGetState(i, &xs) == 0) { g_inputDev = kDevPad; break; }
    }

    bool ok = ImGui_ImplWin32_Init(g_hwnd) && ImGui_ImplDX11_Init(dev, ctx);
    g_device = dev;
    ctx->Release();
    dev->Release();  // the backend keeps its own references
    if (!ok) {
        SR_Log("overlay: ImGui initialisation failed");
        return false;
    }
    bool unicode = IsWindowUnicode(g_hwnd) != 0;
    g_origWndProc = (WNDPROC)(unicode ? SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, (LONG_PTR)OverlayWndProc)
                                      : SetWindowLongPtrA(g_hwnd, GWLP_WNDPROC, (LONG_PTR)OverlayWndProc));
    char k[16];
    SR_Log("overlay: ready on window %p (%dx%d back buffer, font %d px, game fonts %s, prompts %s, starting with %s), "
           "menu key %s or the footer prompt (Menu/Start)", (void*)g_hwnd, (int)desc.BufferDesc.Width, (int)desc.BufferDesc.Height, (int)g_fontPx,
           g_fontReg != g_fontIt ? "yes" : "no", g_iconsReady ? "yes" : "no", g_inputDev == kDevPad ? "controller" : "keyboard",
           KeyName(SR_GetConditions().menuKey, k));
    return true;
}

// The swap chain's device changed (e.g. the game recreated it): rebind the DX11 backend.
void RebindDevice(IDXGISwapChain* sc) {
    ID3D11Device* dev = nullptr;
    if (FAILED(sc->GetDevice(kIID_ID3D11Device, (void**)&dev)) || !dev) return;
    if (dev != g_device) {
        ID3D11DeviceContext* ctx = nullptr;
        dev->GetImmediateContext(&ctx);
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplDX11_Init(dev, ctx);
        g_device = dev;
        ctx->Release();
        SR_Log("overlay: device changed, renderer rebound");
    }
    dev->Release();
}

DXGI_FORMAT ViewFormat(DXGI_FORMAT f) {
    switch (f) {
        case DXGI_FORMAT_R8G8B8A8_TYPELESS: return DXGI_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_TYPELESS: return DXGI_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS: return DXGI_FORMAT_R10G10B10A2_UNORM;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS: return DXGI_FORMAT_R16G16B16A16_FLOAT;
        default: return f;
    }
}

// Edge-triggered input: keyboard (polled, only while the game window has focus) and the pad buttons.
struct Keys { bool menuKey, up, down, left, right, enter, esc, back; };
Keys g_prevKeys;
WORD g_prevPad;
bool g_keyWasDown[256];
LONG g_seenPadActivity;

// Which device the prompts show: a keyboard key going down (game window focused) switches to the keyboard, new pad
// input (counted in the XInput hook) to the controller. Mouse buttons are not counted.
void TrackInputDevice(bool focused) {
    bool keyPressed = false;
    for (int vk = 0x08; vk < 0xFF; ++vk) {
        bool d = focused && (GetAsyncKeyState(vk) & 0x8000) != 0;
        if (d && !g_keyWasDown[vk]) keyPressed = true;
        g_keyWasDown[vk] = d;
    }
    LONG pad = g_padActivity;
    bool padPressed = pad != g_seenPadActivity;
    g_seenPadActivity = pad;
    int dev = g_inputDev;
    if (padPressed) dev = kDevPad;
    if (keyPressed) dev = kDevKeyboard;
    if (dev != g_inputDev) {
        g_inputDev = dev;
        if (g_menuOpen || SR_PickerVisible()) SR_Log("overlay: prompts switched to %s", dev == kDevPad ? "controller" : "keyboard");
    }
}

void HandleInput() {
    bool focused = GetForegroundWindow() == g_hwnd;
    TrackInputDevice(focused);
    auto down = [&](int vk) { return focused && (GetAsyncKeyState(vk) & 0x8000) != 0; };
    Keys k = {down(SR_GetConditions().menuKey), down(VK_UP), down(VK_DOWN), down(VK_LEFT), down(VK_RIGHT),
              down(VK_RETURN), down(VK_ESCAPE), down(VK_BACK)};
    WORD pad = 0;
    for (int i = 0; i < 4; ++i) pad |= (WORD)g_padButtons[i];
    auto key = [&](bool Keys::*m) { return k.*m && !(g_prevKeys.*m); };
    auto btn = [&](WORD b) { return (pad & b) && !(g_prevPad & b); };

    if (!pad) g_waitPadRelease = false;
    bool picker = SR_PickerVisible();
    bool requested = InterlockedExchange(&g_menuRequest, 0) != 0;  // footer prompt input (sr_info.lua)
    if (!g_menuOpen) {
        if (picker && (key(&Keys::menuKey) || (requested && !InputBlocked()))) OpenMenu();
    } else if (!picker) {
        CloseMenu(false);
    } else if (key(&Keys::enter) || btn(kPadA)) {
        CloseMenu(true);
    } else if (key(&Keys::esc) || key(&Keys::back) || key(&Keys::menuKey) || btn(kPadB) || btn(kPadStart)) {
        CloseMenu(false);
    } else {
        if (key(&Keys::up) || btn(kPadUp)) MoveRow(-1);
        if (key(&Keys::down) || btn(kPadDown)) MoveRow(1);
        if (key(&Keys::left) || btn(kPadLeft)) Step(-1);
        if (key(&Keys::right) || btn(kPadRight)) Step(1);
        if (btn(kPadLB)) SwitchTab(-1);
        if (btn(kPadRB)) SwitchTab(1);
        if (g_row >= RowCount()) g_row = RowCount() - 1;  // Custom -> Classic hides the charge rows
    }
    if (g_waitKeyRelease) {
        if (k.menuKey || k.esc || k.enter || k.back) g_blockUntil = Now() + 150;  // until released
        else g_waitKeyRelease = false;
    }
    g_prevKeys = k;
    g_prevPad = pad;
}

void OnPresent(IDXGISwapChain* sc) {
    EnterCriticalSection(&g_imguiLock);
    if (!g_imguiReady) {
        g_imguiReady = InitImGui(sc);
        if (!g_imguiReady) {
            LeaveCriticalSection(&g_imguiLock);
            return;
        }
    }
    RebindDevice(sc);
    HandleInput();
    if (!g_menuOpen) {  // v34: no hint on the picker (the prompt is in the game's footer)
        LeaveCriticalSection(&g_imguiLock);
        return;
    }

    ImGui::GetIO().MouseDrawCursor = false;
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    DrawMenu();
    ImGui::Render();

    ID3D11Texture2D* bb = nullptr;
    if (SUCCEEDED(sc->GetBuffer(0, kIID_ID3D11Texture2D, (void**)&bb)) && bb) {
        D3D11_TEXTURE2D_DESC td;
        bb->GetDesc(&td);
        D3D11_RENDER_TARGET_VIEW_DESC rd = {};
        rd.Format = ViewFormat(td.Format);
        rd.ViewDimension = td.SampleDesc.Count > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
        ID3D11RenderTargetView* rtv = nullptr;
        ID3D11Device* dev = nullptr;
        bb->GetDevice(&dev);
        if (dev && SUCCEEDED(dev->CreateRenderTargetView(bb, &rd, &rtv)) && rtv) {
            ID3D11DeviceContext* ctx = nullptr;
            dev->GetImmediateContext(&ctx);
            ID3D11RenderTargetView* oldRtv = nullptr;
            ID3D11DepthStencilView* oldDsv = nullptr;
            ctx->OMGetRenderTargets(1, &oldRtv, &oldDsv);
            ctx->OMSetRenderTargets(1, &rtv, nullptr);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            ctx->OMSetRenderTargets(1, &oldRtv, oldDsv);
            if (oldRtv) oldRtv->Release();
            if (oldDsv) oldDsv->Release();
            ctx->Release();
            rtv->Release();
        }
        if (dev) dev->Release();
        bb->Release();
    }
    LeaveCriticalSection(&g_imguiLock);
}

HRESULT WINAPI Hook_Present(IDXGISwapChain* sc, UINT sync, UINT flags) {
    if (!(flags & DXGI_PRESENT_TEST)) OnPresent(sc);
    return g_origPresent(sc, sync, flags);
}

HRESULT WINAPI Hook_Present1(IDXGISwapChain1* sc, UINT sync, UINT flags, const DXGI_PRESENT_PARAMETERS* p) {
    if (!(flags & DXGI_PRESENT_TEST)) OnPresent(sc);
    return g_origPresent1(sc, sync, flags, p);
}

bool HookD3D() {
    typedef HRESULT(WINAPI * CreateDevSc_t)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL*, UINT, UINT,
                                            const DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**, ID3D11Device**,
                                            D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);
    HMODULE d3d = LoadLibraryW(L"d3d11.dll");
    CreateDevSc_t create = d3d ? (CreateDevSc_t)GetProcAddress(d3d, "D3D11CreateDeviceAndSwapChain") : nullptr;
    if (!create) return false;

    WNDCLASSEXW wc = {sizeof(wc)};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"NFSHPR_SingleRace_Dummy";
    RegisterClassExW(&wc);
    HWND w = CreateWindowExW(0, wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr, wc.hInstance, nullptr);
    if (!w) return false;

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 64;
    sd.BufferDesc.Height = 64;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = w;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    IDXGISwapChain* sc = nullptr;
    ID3D11Device* dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    HRESULT hr = E_FAIL;
    D3D_DRIVER_TYPE types[2] = {D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP};
    for (int i = 0; i < 2 && FAILED(hr); ++i)
        hr = create(nullptr, types[i], nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd, &sc, &dev, nullptr, &ctx);
    bool ok = false;
    if (SUCCEEDED(hr) && sc) {
        void** vt = *(void***)sc;
        g_origPresent = (Present_t)vt[8];
        IDXGISwapChain1* sc1 = nullptr;
        if (SUCCEEDED(sc->QueryInterface(kIID_IDXGISwapChain1, (void**)&sc1)) && sc1) {
            void** vt1 = *(void***)sc1;
            g_origPresent1 = (Present1_t)vt1[22];
            PatchVtable(vt1, 22, (void*)Hook_Present1);
            sc1->Release();
        }
        PatchVtable(vt, 8, (void*)Hook_Present);
        ok = true;
    }
    if (sc) sc->Release();
    if (ctx) ctx->Release();
    if (dev) dev->Release();
    DestroyWindow(w);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return ok;
}

}  // namespace

void SR_RequestMenu() { InterlockedExchange(&g_menuRequest, 1); }

// v40 ("hook later"): nothing is hooked at start-up. The input and Direct3D hooks (and the throw-away D3D11 device and
// window used to find the Present vtable) are installed by this thread the first time the Single Race picker is on
// screen, so the game boots, plays its splash screens and builds the main menu with no overlay code in its rendering
// or input. ImGui itself starts at the first frame after that (on the picker).
DWORD WINAPI OverlayStarter(LPVOID) {
    while (!SR_PickerVisible()) Sleep(100);
    SR_Log("overlay: Single Race picker on screen - installing the input and Direct3D hooks");
    HookDirectInput();
    HookXInput();
    if (HookD3D()) SR_Log("overlay: Direct3D 11 Present hooked, waiting for the next frame");
    else SR_Log("overlay: could not hook Direct3D 11 - the conditions menu is unavailable (the ini still works)");
    return 0;
}

void Overlay_Start() {
    InitializeCriticalSection(&g_imguiLock);
    HANDLE t = CreateThread(nullptr, 0, OverlayStarter, nullptr, 0, nullptr);
    if (!t) {
        SR_Log("overlay: could not start - the conditions menu is unavailable (the ini still works)");
        return;
    }
    CloseHandle(t);
    SR_Log("overlay: waiting for the Single Race picker (no input or Direct3D hooks until then)");
}
