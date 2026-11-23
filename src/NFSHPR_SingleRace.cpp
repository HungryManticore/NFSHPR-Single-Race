// NFSHPR_SingleRace.asi - Need for Speed Hot Pursuit Remastered (PC, x64), build 613F368C.
//
// Single Race helper.
//  1. Event remap. The Single Race page (the multiplayer event picker, opened offline) launches events
//     through the native SendCommandLink("PlayIt?...&res_id=<online event>"). An online Race / Hot Pursuit
//     event is replaced by the Single Race version of its route (generated events 2100000+n Race /
//     2200000+n Hot Pursuit in update\GAMELOGIC\CHALLENGEINSTANCES.BNDL: offline script, AI, every car);
//     Race or Hot Pursuit is the mode chosen in the overlay ([Event] Mode). Other online modes are ignored. Links for any other event (career map, Autolog) pass through unchanged.
//  2. Start positions. The race scripts place competitor i on intro slot i via GetCutsceneStartPosition(i) /
//     GetCutsceneStartDirection(i); competitor 0 is the player, slot 0 the pole. For Single Race events the
//     competitor index is remapped: Race - the player starts last of 8; Hot Pursuit - the 4 racers take the
//     front 4 slots with the player last of them, the 4 cops the back 4.
//  3. Opponents and cops (taken over from NFSHPR Event Randomizer 1.0, Single Race events only): each AI racer
//     and cop gets a random car of the player's class (GetOpponentVehicleId), a racer stays a racer and a cop
//     a cop. StopTraffic, the first step of every event script, starts a new event.
//     AI racers also get a random paint colour of their car (GetOpponentColourId; colours per car in
//     PaintTable.h, taken from the cars' VEH_<id>_MS.BIN). Cops keep their liveries.
//     Settings: NFSHPR_SingleRace.ini [Opponents] (re-read at every event start).
//  4. Conditions (time of day, weather) for Single Race events, chosen in an overlay on the event picker
//     (SR_Overlay.cpp, F5 or Select) and saved in NFSHPR_SingleRace.ini [Conditions]. Applied through the
//     arguments of SetTimeOfDay, as NFSHPR Event Randomizer 1.0 does. The picker is on screen while the
//     offline menu's page is "wall" (OverrideDefaultPageForContext) and its location / event list layout
//     (646187 / 646284) is the current layout set (SwitchLayoutSet).
//  5. Main menu label: the Wall item's text (localised string 1263478, "WALL") is replaced in the loaded
//     language table by the ini text ([MenuLabel] Text, default "SINGLE RACE"; see the label section below).
//  5. Weapons (Hot Pursuit only; the offline Race script sets up no weapons). The event's weapon availability
//     sets are replaced by the Single Race sets in update\GAMELOGIC\WEAPONS.BNDL: 2300101 for the player and
//     2300100 for every AI. The charges chosen in the overlay (Classic or Custom) are written into them before
//     the game hands out the weapons. The AI set gets all 8 charges; the player set only those of the player's
//     own side (racer weapons on the racer side, cop weapons in a cop pursuit), as in the career events. With
//     no charges of its own the player never "has had a weapon", so the HUD hides the weapons panel (v30).
//  6. Picker lists (v21). Each online location's event list is a GameDB record in GAMEPLAY.BNDL (schema 321665,
//     hash 69322e6b, ids 744417-744428: one u32 array of event ids), which the picker reads through
//     Location.Events / NumEvents. While the Single Race picker is in use, the helper writes the location's
//     Single Race Race ids (kPickerLists in RouteMap.h: the multiplayer routes and the Career events, each at
//     its Career starting point) over the array in memory and sets the count; from the main menu on (and so
//     in the online lobby) the lists are restored exactly. A picked Single Race id is launched as the Race or
//     Hot Pursuit of that route, as chosen in the overlay.
//  7. Intro name tags. The multiplayer intros show "<name> (<rank>)" ("AIAI (1)") over every car: the
//     intro tag layout (UI/LAYOUTS 698864, text "Gamer Tag") is bound to CompetitorInfoVM.FullRank, a wide
//     string at VM+0xE8 (pointer at VM+0x150) that the per-competitor VM update 0x2C4850 formats as
//     "{name} ({rank})" whenever the competitor's rank changes (cache at VM+0x78). For Single Race events
//     the helper empties that string after each update, so the intro shows no tag; the in-race tags
//     (position, cop health bars) are untouched. In any other event a blanked VM is re-formatted.
//  8. Loading screen event info. The pre-race loading screen (ChallengeReporter.Info) shows the event
//     record's start hour and weather icons and the career tier that lists the event (none for ours, so
//     "HOT PURSUIT - " stayed blank). For Single Race events the time and icons come from the ini
//     conditions and the tier is the class of the selected car (see the loading screen section).
//  9. Picker event details (v23; v25 adds SR.Summary, the first tab's mode / difficulty / AI count text). The picker's info panel tab "details" (UI layout 1227266 in WALL.BNDL)
//     shows the overlay's settings instead of the listed online event's: its time and weather are bound to
//     ChallengeReporter.Info.GetStartTime / WeatherIcon, which the helper answers from the ini conditions
//     while the picker is on screen (as on the loading screen); its weapon counts and grid size are bound to
//     Autolog.Controls.SRInfo.*, set every frame by the bundle's sr_info.lua from
//     ResolveStringBinding("SR.<name>"), which the helper answers (see the picker details section).
//     v34: the "RACE SETTINGS" prompt is in the picker's own footer prompt bar (WALL v12, sr_info.lua); its input
//     (to_main_menu: Menu/Start, or the key the game binds to it; v35: Select no longer opens it) calls ResolveStringBinding("SR.OpenSettings"),
//     which opens the overlay. The overlay's top-right hint is gone.
//
// 10. AI count (v24). The overlay sets the number of AI racers (Race 1-7; Hot Pursuit 1-6) and cops (Hot Pursuit
//     1-6; racers + cops <= 7), kept per mode in the ini ([Race] Racers, [HotPursuit] Racers / Cops). The event
//     records are resident (GameChanger 411487, pointers already absolute, read-write memory); at launch the
//     helper writes the launched record's racer count (+0xFC) and cop count (+0x74) and the lengths of its
//     racer (+0x100) and cop (+0x78) lists. The lists hold the most (bundle v8: Race 7 racers; Hot Pursuit 6 racers
//     and 6 cops in the order start, start, 35 %, 66 %, start, start), so the first N cops follow the spawn rules.
//     Start slots and the Hot Pursuit toughness presets follow the counts.
//
// 11. Cop Hot Pursuit (v29). A third overlay mode launches the route's cop-side Hot Pursuit 2400000+n (bundle v9:
//     mode script HPOfflineCop, 7 AI racers, no AI cops, the generic multiplayer race intro 712428 as
//     CopOfflineIntro). The player's side is the local competitor's cop flag (competitor+0x992). An event with an
//     empty car list (all of ours) leaves it as it was, so every Single Race launch link carries IsCop=1 (cop) or
//     IsCop=0 (Race, racer Hot Pursuit): the PlayIt parser (0x26FE80) sets the flag from it. The flag the player
//     had before the first Single Race launch is put back on the main menu, so the career map keeps its side.
//     Intro: the multiplayer Hot Pursuit intro 278552 (the race intro 712428 has no actor for a cop player: crash).
//     Start: as in the racer Hot Pursuits, the cutscene start slots put the AI racers in front and the player
//     behind them. AI count 1-7
//     racers ([CopHotPursuit] Racers). Difficulty presets from career cop events (balancing incl.
//     HotPursuitCopBalancingData, LocalPlayerCopToughnessScale, racer toughness).
//
// Every address is verified before use; on any mismatch the plugin does nothing.
// Log: NFSHPR_SingleRace.log next to the asi.

#include <windows.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "RouteMap.h"
#include "SR_Shared.h"
#include "VehicleTable.h"
#include "PaintTable.h"

namespace {

const uint32_t kExpectedTimestamp = 0x613F368C;

struct lua_State;
typedef int (*lua_CFunction)(lua_State*);

// Lua natives: the jmp thunk registered for Lua and its registration site
// ("lea r8,[fn]; mov rcx,rbx; lea rdx,[name]; call Register").
struct Binding {
    const char* name;
    uintptr_t fnRva, regRva;
    lua_CFunction hook;
    lua_CFunction original;
};

int Hook_SendCommandLink(lua_State* L);
int Hook_CutscenePos(lua_State* L);
int Hook_CutsceneDir(lua_State* L);
int Hook_StopTraffic(lua_State* L);
int Hook_GetOpponentVehicleId(lua_State* L);
int Hook_SetTimeOfDay(lua_State* L);
int Hook_OverrideDefaultPage(lua_State* L);
int Hook_SwitchLayoutSet(lua_State* L);
int Hook_WeaponsForCompetitor(lua_State* L);
int Hook_GetChallengeParam(lua_State* L);
int Hook_GetOpponentToughness(lua_State* L);
int Hook_ResolveStringBinding(lua_State* L);
int Hook_GetOpponentColourId(lua_State* L);

Binding g_bindings[] = {
    {"SendCommandLink", 0x130170, 0x12D56E, Hook_SendCommandLink, nullptr},
    {"GetCutsceneStartPosition", 0x2A1D40, 0x2A1204, Hook_CutscenePos, nullptr},
    {"GetCutsceneStartDirection", 0x2A1DB0, 0x2A121A, Hook_CutsceneDir, nullptr},
    {"StopTraffic", 0x298400, 0x2954D8, Hook_StopTraffic, nullptr},
    {"GetOpponentVehicleId", 0x28E700, 0x28DA02, Hook_GetOpponentVehicleId, nullptr},
    {"SetTimeOfDay", 0x299700, 0x299186, Hook_SetTimeOfDay, nullptr},
    {"OverrideDefaultPageForContext", 0x12E970, 0x12CF54, Hook_OverrideDefaultPage, nullptr},
    {"SwitchLayoutSet", 0x130AD0, 0x12D5B0, Hook_SwitchLayoutSet, nullptr},
    {"SetupWeaponAvailabilityForCompetitor", 0x2A4240, 0x2C57997, Hook_WeaponsForCompetitor, nullptr},
    {"GetChallengeParam", 0x291980, 0x2901A6, Hook_GetChallengeParam, nullptr},
    {"GetOpponentToughness", 0x28EE60, 0x28DA44, Hook_GetOpponentToughness, nullptr},
};
enum { kSendCommandLink, kCutscenePos, kCutsceneDir, kStopTraffic, kGetOpponentVehicleId, kSetTimeOfDay, kOverrideDefaultPage,
       kSwitchLayoutSet, kWeaponsForCompetitor, kGetChallengeParam, kGetOpponentToughness, kBindingCount };
// Install() puts binding k in trampoline slot k: slots 0-10 (the balancing setup thunk uses 11).
// Optional (the rest of the plugin works without it): the picker details channel, trampoline slot 36.
Binding g_resolveString = {"ResolveStringBinding", 0x12F940, 0x12D14E, Hook_ResolveStringBinding, nullptr};
const int kResolveStringSlot = 36;
// AI racer paint colours: GetOpponentColourId, trampoline slot 37.
Binding g_opponentColour = {"GetOpponentColourId", 0x28E750, 0x28DA18, Hook_GetOpponentColourId, nullptr};
const int kOpponentColourSlot = 37;

// ---- weapons
// SetupChallengeWeaponAvailability (Lua) reads the event's weapon sets - field d0ed6077 for the player,
// 43c51331 for AI - plus an optional set per AI car, and passes them to the loader 0x380330
// (weapons manager, playerSet, aiSet, uint32 vehicleSets[8], isPatrolCruise), which requests them
// (handles at manager+0xBDE0 / +0xBDF0 / +0xBE00+16*i). SetupWeaponAvailabilityForCompetitor(i) then gives
// competitor i the weapons of its set. The manager is *(*(base+0x1279218)+0x239E0).
// A set (schema 618158, hash 82d52faf) holds a pointer per weapon to a Weapon (hash 5d264e2b:
// Count u32, two floats, a flag byte) plus the cop / racer weapon levels; field names hash with the game's
// CRC (CopEMP 284a9e67, CopSpikeStrip 6acaa851, Helicopter ab40bdd5, Jammer 51bd94c1, Overdrive 5f24c217,
// RacerEMP 342b4f19, RacerSpikeStrip 51986529, RoadBlock 483378f8, CopWeaponLevel ea93e8f7,
// RacerWeaponLevel 47273606, Count e7477891).
const uintptr_t kRvaWeaponSetLoader = 0x380330, kRvaWeaponSetLoaderCaller = 0x2BB5E1F;
const uint8_t kWeaponSetLoaderPrologue[18] = {0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x6C, 0x24,
                                              0x18, 0x48, 0x89, 0x74, 0x24, 0x20, 0x57, 0x41, 0x54};
const size_t kWeaponSetLoaderStolen = 15;  // mov [rsp+8],rbx; mov [rsp+18h],rbp; mov [rsp+20h],rsi
// the loader's handle stores: mov [rcx+0BDE0h],rax ... lea r9,[rbp+0BDF0h]
const uint8_t kWeaponSetLoaderBody[] = {0x48, 0x89, 0x81, 0xE0, 0xBD, 0x00, 0x00};
const uintptr_t kWeaponSetLoaderBodyAt = 0x380352;
const uint32_t kOffWeaponsManager = 0x239E0, kOffPlayerSetHandle = 0xBDE0, kOffAiSetHandle = 0xBDF0;
const uintptr_t kRvaNullResource = 0x1287E30;  // the loader's "no resource" handle value
const uint32_t kSingleRaceWeaponSet = 2300100;        // AI set
const uint32_t kSingleRacePlayerWeaponSet = 2300101;  // player set (v30)
const uint32_t kHashWeaponSet = 0x82D52FAF, kHashWeapon = 0x5D264E2B;
// data offsets of the set's Weapon pointers, by enum Weapon (layout of the set as built from 744442)
// data offsets of the set's weapon levels (u32; the game uses level - 1, 0 = the player's unlocked level /
// level 1 for AI): CopWeaponLevel ea93e8f7 and RacerWeaponLevel 47273606. Read per competitor by 0x2F0D0A0
// (player: [manager+0x88], AI: [manager+0x48+4i] cop / [+0x68+4i] racer) when it gets its weapons.
const uint32_t kOffCopWeaponLevel = 0x10, kOffRacerWeaponLevel = 0x40;
const uint32_t kWeaponFieldOffset[8] = {0x38 /*RacerSpikeStrip*/, 0x30 /*RacerEMP*/, 0x20 /*Jammer*/, 0x28 /*Overdrive*/,
                                        0x08 /*CopSpikeStrip*/, 0x00 /*CopEMP*/, 0x48 /*RoadBlock*/, 0x18 /*Helicopter*/};
typedef uint64_t (*Fn_WeaponSetLoader)(uintptr_t, uint32_t, uint32_t, uint32_t*, uint64_t);
Fn_WeaponSetLoader g_weaponSetLoaderOriginal;
volatile LONG g_weaponsWritten;  // the charges were written for this event
// called, not hooked: the player's car
const Binding kGetSelectedVehicleId = {"GetSelectedVehicleId", 0x298D40, 0x29564E, nullptr, nullptr};
lua_CFunction g_origGetSelectedVehicleId;

// Race intro setup (0x262410, one caller at 0x26191C). It binds the intro's actors (start objects at
// mgr+0x200 + n*0x120) to competitors: the actor of type 0x11 always gets the local player (competitor
// [*(mgr+0x89e8)+0x6c04]), the others get AI racers / cops from lists sorted by score. For each binding it sets
// actor+0x110 = competitor and mgr+0x1C20[competitor] = actor. Cars are then placed on (and, during the
// intro, driven along) their actor: GetCutsceneStartPosition(i) reads mgr+0x1C20[i] (0x2977380), and so
// does the competitor start transform while the intro runs (0x23FFB3). The race intro's player actor is on
// pole, so after the setup the player swaps actors with the AI whose actor is furthest from it (the back row).
const uintptr_t kRvaIntroSetup = 0x262410, kRvaIntroSetupCaller = 0x26191C;
const uint8_t kIntroSetupPrologue[18] = {0x48, 0x8B, 0xC4, 0x48, 0x89, 0x58, 0x10, 0x48, 0x89,
                                         0x70, 0x18, 0x48, 0x89, 0x78, 0x20, 0x55, 0x41, 0x54};
const size_t kIntroSetupStolen = 15;  // mov rax,rsp; mov [rax+10h],rbx; mov [rax+18h],rsi; mov [rax+20h],rdi
// the actor writes inside the setup: mov [rbx+110h],ecx ... mov [r15+rcx*8+1C20h],rbx
const uint8_t kIntroSetupWrite[] = {0x89, 0x8B, 0x10, 0x01, 0x00, 0x00};
const uintptr_t kIntroSetupWriteAt = 0x262889;
const uint8_t kIntroSetupWrite2[] = {0x49, 0x89, 0x9C, 0xCF, 0x20, 0x1C, 0x00, 0x00};
const uintptr_t kIntroSetupWrite2At = 0x2628A3;
const uint32_t kStartSlotArray = 0x1C20, kActorCompetitor = 0x110, kLocalCompetitorPtr = 0x89E8, kLocalCompetitor = 0x6C04;
typedef uint64_t (*Fn_IntroSetup)(uintptr_t, uintptr_t, uintptr_t, uintptr_t);
Fn_IntroSetup g_introSetupOriginal;

// Lua 5.1 API in the decrypted code (verified by their first bytes).
const uintptr_t kRvaGetTop = 0xC68F00, kRvaSetTop = 0xC69C20, kRvaPushString = 0xC69590;
const uint8_t kGetTopBytes[8] = {0x48, 0x8B, 0x41, 0x10, 0x48, 0x2B, 0x41, 0x18};
const uint8_t kSetTopBytes[8] = {0x85, 0xD2, 0x78, 0x40, 0x4C, 0x63, 0xC2, 0x48};
const uint8_t kPushStringBytes[8] = {0x48, 0x89, 0x6C, 0x24, 0x18, 0x56, 0x48, 0x83};

// used by the opponent randomizer (same checks as Event Randomizer 1.0)
struct ApiSig { uintptr_t rva; uint8_t bytes[8]; };
const ApiSig kToNumber   = {0xC69EA0, {0x48, 0x83, 0xEC, 0x38, 0xE8, 0x67, 0xE9, 0xFF}};
const ApiSig kPushNumber = {0xC69570, {0x48, 0x8B, 0x41, 0x10, 0xF2, 0x0F, 0x11, 0x08}};
const ApiSig kType       = {0xC69FB0, {0x48, 0x83, 0xEC, 0x28, 0xE8, 0x57, 0xE8, 0xFF}};
const ApiSig kToLString  = {0xC69E00, {0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74}};
const ApiSig kPushBoolean = {0xC69360, {0x4C, 0x8B, 0x41, 0x10, 0x33, 0xC0, 0x85, 0xD2}};
// IsInOnlineEvent (native, rva 29A7D0) reads [[[base+1279218]+239C8]+F5DA] != 0.
const uintptr_t kRvaGameManager = 0x1279218, kOffOnlineState = 0x239C8, kOffInOnlineEvent = 0xF5DA;
const ApiSig kIsInOnlineEvent = {0x29A7E8, {0x48, 0x8B, 0x05, 0x29, 0xEA, 0xFD, 0x00, 0x33}};

typedef int (*Fn_gettop)(lua_State*);
typedef void (*Fn_settop)(lua_State*, int);
typedef void (*Fn_pushstring)(lua_State*, const char*);
typedef double (*Fn_tonumber)(lua_State*, int);
typedef void (*Fn_pushnumber)(lua_State*, double);
typedef int (*Fn_type)(lua_State*, int);
typedef const char* (*Fn_tolstring)(lua_State*, int, size_t*);
typedef void (*Fn_pushboolean)(lua_State*, int);
Fn_pushboolean lua_pushboolean_;
Fn_tolstring lua_tolstring_;
Fn_gettop lua_gettop_;
Fn_settop lua_settop_;
Fn_pushstring lua_pushstring_;
Fn_tonumber lua_tonumber_;
Fn_pushnumber lua_pushnumber_;
Fn_type lua_type_;
enum { LUA_TNIL = 0, LUA_TBOOLEAN = 1, LUA_TNUMBER = 3, LUA_TSTRING = 4 };

// Intro slots of the multiplayer intros, front to back (measured on a Race and a Hot Pursuit route:
// both give 0,7,5,6,4,1,3,2 along the driving direction).
const int kSlotsFrontToBack[8] = {0, 7, 5, 6, 4, 1, 3, 2};
// competitor -> slot
int g_raceSlot[8], g_hpSlot[8];

enum Mode { kNone, kRace, kHotPursuit, kCopPursuit };
volatile LONG g_mode = kNone;  // mode of the event most recently launched through SendCommandLink
volatile LONG g_eventDifficulty = 1;  // Difficulty chosen in the overlay when that event was launched (kMedium)
// AI racers and cops of the Single Race event most recently launched (as written into its record, or as the
// record had them if it could not be changed)
volatile LONG g_eventRacers = 3, g_eventCops = 4;

uintptr_t g_base;
wchar_t g_iniPath[MAX_PATH];
HANDLE g_log = INVALID_HANDLE_VALUE;
CRITICAL_SECTION g_logLock;

void Log(const char* fmt, ...) {
    if (g_log == INVALID_HANDLE_VALUE) return;
    char buf[1024];
    SYSTEMTIME st;
    GetLocalTime(&st);
    int o = _snprintf(buf, 32, "[%02u:%02u:%02u] ", st.wHour, st.wMinute, st.wSecond);
    va_list ap;
    va_start(ap, fmt);
    int n = _vsnprintf(buf + o, sizeof(buf) - 3 - o, fmt, ap);
    va_end(ap);
    if (n < 0 || n > (int)sizeof(buf) - 3 - o) n = (int)sizeof(buf) - 3 - o;
    n += o;
    buf[n++] = '\r';
    buf[n++] = '\n';
    DWORD w;
    EnterCriticalSection(&g_logLock);
    WriteFile(g_log, buf, (DWORD)n, &w, nullptr);
    LeaveCriticalSection(&g_logLock);
}

bool SafeRead(uintptr_t a, void* dst, size_t n) {
    SIZE_T got = 0;
    return ReadProcessMemory(GetCurrentProcess(), (const void*)a, dst, n, &got) && got == n;
}

bool CheckBytes(uintptr_t rva, const uint8_t* expect, size_t n) {
    uint8_t b[32];
    return n <= sizeof(b) && SafeRead(g_base + rva, b, n) && memcmp(b, expect, n) == 0;
}

// The registration site must load this function and this exact name; a thunk must be a jmp.
bool CheckRegistration(const char* fnName, uintptr_t fnRva, uintptr_t regRva, bool thunk) {
    struct { const char* name; uintptr_t fnRva, regRva; } b = {fnName, fnRva, regRva};
    uint8_t s[22];
    if (!SafeRead(g_base + b.regRva, s, sizeof(s))) return false;
    if (s[0] != 0x4C || s[1] != 0x8D || s[2] != 0x05 || s[10] != 0x48 || s[11] != 0x8D || s[12] != 0x15) return false;
    int32_t d1, d2;
    memcpy(&d1, s + 3, 4);
    memcpy(&d2, s + 13, 4);
    if (b.regRva + 7 + (intptr_t)d1 != b.fnRva) return false;
    char name[48] = {};
    size_t len = strlen(b.name) + 1;
    if (len > sizeof(name) || !SafeRead(g_base + b.regRva + 17 + (intptr_t)d2, name, len) || memcmp(name, b.name, len) != 0)
        return false;
    if (!thunk) return true;
    uint8_t op;
    return SafeRead(g_base + b.fnRva, &op, 1) && op == 0xE9;
}

bool CheckBinding(const Binding& b) { return CheckRegistration(b.name, b.fnRva, b.regRva, true); }

bool AllChecksPass() {
    if (!CheckBytes(kRvaGetTop, kGetTopBytes, 8) || !CheckBytes(kRvaSetTop, kSetTopBytes, 8) ||
        !CheckBytes(kRvaPushString, kPushStringBytes, 8))
        return false;
    for (const Binding& b : g_bindings)
        if (!CheckBinding(b)) return false;
    const ApiSig* apis[] = {&kToNumber, &kPushNumber, &kType, &kToLString, &kPushBoolean, &kIsInOnlineEvent};
    for (const ApiSig* a : apis)
        if (!CheckBytes(a->rva, a->bytes, 8)) return false;
    return CheckBinding(kGetSelectedVehicleId);
}

// Lua 5.1 x64: L->top at +0x10, L->base at +0x18; TValue 16 bytes (value, int tag at +8: 3 number, 4 string);
// TString: length at +0x10, characters at +0x18.
uint8_t* Base(lua_State* L) { return *(uint8_t**)((uint8_t*)L + 0x18); }
uint8_t* Top(lua_State* L) { return *(uint8_t**)((uint8_t*)L + 0x10); }

const char* ArgString(lua_State* L, size_t* len) {
    uint8_t* b = Base(L);
    if (Top(L) <= b || *(int*)(b + 8) != 4) return nullptr;
    const uint8_t* ts = *(const uint8_t* const*)b;
    if (!ts) return nullptr;
    *len = *(const size_t*)(ts + 0x10);
    return (const char*)(ts + 0x18);
}

bool ResId(const char* s, size_t len, uint32_t* out) {
    for (size_t i = 0; i + 7 <= len; ++i) {
        if (memcmp(s + i, "res_id=", 7) != 0) continue;
        uint64_t v = 0;
        size_t k = i + 7;
        for (; k < len && s[k] >= '0' && s[k] <= '9' && v < 0xFFFFFFFFull; ++k) v = v * 10 + (s[k] - '0');
        if (k == i + 7) return false;
        *out = (uint32_t)v;
        return true;
    }
    return false;
}

uint32_t MapEvent(uint32_t id) {
    for (const RouteMapEntry& e : kRouteMap)
        if (e.online == id) return e.single;
    return 0;
}

bool Blocked(uint32_t id) {
    for (uint32_t b : kBlocked)
        if (b == id) return true;
    return false;
}

Mode ModeOf(uint32_t eventId) {
    if (eventId >= 2100000 && eventId < 2200000) return kRace;
    if (eventId >= 2200000 && eventId < 2300000) return kHotPursuit;
    if (eventId >= 2400000 && eventId < 2500000) return kCopPursuit;   // v29 (2300100 is the weapon set)
    return kNone;
}

const char* ModeLabel(LONG m) {
    return m == kRace ? "Race" : m == kHotPursuit ? "Hot Pursuit" : m == kCopPursuit ? "Cop Hot Pursuit" : "none";
}

// ---- event picker tracking (for the conditions overlay)
volatile LONG g_offlinePageIsWall;  // the offline menu opens on page "wall" (the Single Race picker)
volatile LONG g_layoutSet;          // the last layout set switched to
const LONG kLayoutLocations = 646187, kLayoutEvents = 646284;

// string argument i (1-based) without touching the Lua stack
bool ArgStr(lua_State* L, int i, const char** s, size_t* len) {
    uint8_t* t = Base(L) + (size_t)(i - 1) * 16;
    if (Top(L) <= t || *(int*)(t + 8) != 4) return false;
    const uint8_t* ts = *(const uint8_t* const*)t;
    if (!ts) return false;
    *len = *(const size_t*)(ts + 0x10);
    *s = (const char*)(ts + 0x18);
    return true;
}
bool StrIs(const char* s, size_t len, const char* lit) { return len == strlen(lit) && memcmp(s, lit, len) == 0; }

// ---- intro name tags (see 7.)
const uintptr_t kRvaVmUpdate = 0x2C4850, kRvaVmUpdateCaller = 0x2CB33D;
const uint8_t kVmUpdatePrologue[15] = {0x40, 0x56, 0x57, 0x41, 0x56, 0x48, 0x81, 0xEC, 0xC0, 0x00, 0x00, 0x00, 0x49, 0x63, 0xF0};
// ... cmp eax,[rdi+78h]; je ...  (rank cache)
const uintptr_t kVmRankCacheAt = 0x2C4A02;
const uint8_t kVmRankCache[] = {0x3B, 0x47, 0x78, 0x0F, 0x84};
// ... lea rdx,[rdi+0E8h]  (FullRank string, the target of the formatter)
const uintptr_t kVmFullRankAt = 0x2C4A8C;
const uint8_t kVmFullRank[] = {0x48, 0x8D, 0x97, 0xE8, 0x00, 0x00, 0x00};
const uintptr_t kVmRankCacheOff = 0x78, kVmFullRankPtrOff = 0x150;
typedef uint64_t (*Fn_VmUpdate)(uintptr_t, uintptr_t, uintptr_t);
Fn_VmUpdate g_vmUpdateOriginal;
uintptr_t g_blankedVm[16];  // VMs whose FullRank the helper emptied (game thread only)
volatile LONG g_tagsBlankedLogged;

bool SingleRaceEvent();

uint64_t Hook_VmUpdate(uintptr_t vm, uintptr_t gm, uintptr_t idx) {
    bool single = SingleRaceEvent();
    int slot = -1;
    for (int k = 0; k < 16; ++k)
        if (g_blankedVm[k] == vm) { slot = k; break; }
    if (!single && slot >= 0) {
        *(int32_t*)(vm + kVmRankCacheOff) = 0;  // the game formats FullRank again
        g_blankedVm[slot] = 0;
    }
    uint64_t r = g_vmUpdateOriginal(vm, gm, idx);
    if (single) {
        uintptr_t str = *(uintptr_t*)(vm + kVmFullRankPtrOff);
        uint16_t* text = str ? *(uint16_t**)str : nullptr;
        if (text && *text) {
            *text = 0;
            if (slot < 0)
                for (int k = 0; k < 16; ++k)
                    if (!g_blankedVm[k]) { g_blankedVm[k] = vm; break; }
            if (!InterlockedExchange(&g_tagsBlankedLogged, 1)) Log("intro name tags hidden (Single Race event)");
        }
    } else {
        InterlockedExchange(&g_tagsBlankedLogged, 0);
    }
    return r;
}

// ---- the picker's event lists
const uint32_t kHashEventList = 0x69322E6B, kFirstOnlineList = 744417, kOnlineListCount = 12;
const LONG kLayoutMainMenu = 461008;
struct EventList {
    const PickerList* single;  // the Single Race entries for this location
    uintptr_t array;  // u32 event ids
    uint32_t* countAt;
    uint32_t count;   // original
    uint32_t ids[64]; // original order
    uint32_t keep;    // Race / Hot Pursuit entries
};
EventList g_lists[kOnlineListCount];
volatile LONG g_listsFound;    // 0 = not searched, 1 = found, -1 = searching / not found
volatile LONG g_listsFiltered;
CRITICAL_SECTION g_listLock;

// The picker lists each event once, as its Single Race Race id (kPickerLists in RouteMap.h: the multiplayer routes
// and the Career events, each at its Career starting point); the overlay's mode decides whether the Race or the
// Hot Pursuit version is launched.
const PickerList* PickerListFor(uint32_t listId) {
    for (const PickerList& P : kPickerLists)
        if (P.list == listId) return &P;
    return nullptr;
}

// Reads the list instance at p (validated) into L.
bool ReadList(uintptr_t p, uint32_t listId, EventList& L) {
    uintptr_t data = 0, arr = 0;
    uint32_t cnt = 0, es = 0;
    if (!SafeRead(p + 0x10, &data, 8) || data != p + 0x30) return false;
    if (!SafeRead(data, &arr, 8) || !SafeRead(data + 8, &cnt, 4) || !SafeRead(data + 12, &es, 4)) return false;
    if (!arr || es != 4 || cnt == 0 || cnt > 64 || !SafeRead(arr, L.ids, 4 * cnt)) return false;
    L.array = arr;
    L.countAt = (uint32_t*)(data + 8);
    L.count = cnt;
    L.single = PickerListFor(listId);
    // the Single Race entries are written over the array in place, so they must fit in it
    L.keep = L.single && L.single->count <= cnt ? L.single->count : 0;
    return true;
}

// Searches the game's memory once for the 12 list records ([P+0x18] = hash, [P+0x1C] = id, [P+0x10] = P+0x30).
DWORD WINAPI FindListsThread(LPVOID) {
    ULONGLONG t0 = GetTickCount64();
    EventList found[kOnlineListCount];
    memset(found, 0, sizeof(found));
    int n = 0;
    const size_t kChunk = 1 << 20;
    uint8_t* buf = (uint8_t*)VirtualAlloc(nullptr, kChunk + 64, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCWSTR)&FindListsThread, &self);
    MEMORY_BASIC_INFORMATION mbi;
    for (uintptr_t a = 0x10000; buf && n < (int)kOnlineListCount && VirtualQuery((void*)a, &mbi, sizeof(mbi)) == sizeof(mbi);
         a = (uintptr_t)mbi.BaseAddress + mbi.RegionSize) {
        if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS | PAGE_NOCACHE | PAGE_WRITECOMBINE)) ||
            mbi.Type == MEM_IMAGE)
            continue;
        if (!(mbi.Protect & (PAGE_READWRITE | PAGE_READONLY | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE))) continue;
        uintptr_t base = (uintptr_t)mbi.BaseAddress, end = base + mbi.RegionSize;
        for (uintptr_t c = base; c < end && n < (int)kOnlineListCount; c += kChunk) {
            size_t len = end - c < kChunk ? end - c : kChunk;
            SIZE_T got = 0;
            if (!ReadProcessMemory(GetCurrentProcess(), (void*)c, buf, len, &got) || got < 8) continue;
            for (size_t i = 0; i + 8 <= got; i += 8) {  // the record may start in the previous chunk: read via SafeRead
                if (*(uint32_t*)(buf + i) != kHashEventList) continue;
                uint32_t id = *(uint32_t*)(buf + i + 4);
                if (id < kFirstOnlineList || id >= kFirstOnlineList + kOnlineListCount) continue;
                EventList& L = found[id - kFirstOnlineList];
                if (L.array) continue;
                if (ReadList(c + i - 0x18, id, L)) ++n;
            }
        }
    }
    if (buf) VirtualFree(buf, 0, MEM_RELEASE);
    EnterCriticalSection(&g_listLock);
    if (n == (int)kOnlineListCount) {
        memcpy(g_lists, found, sizeof(found));
        InterlockedExchange(&g_listsFound, 1);
    } else {
        InterlockedExchange(&g_listsFound, 0);  // try again later
    }
    LeaveCriticalSection(&g_listLock);
    int keep = 0, total = 0;
    for (auto& L : found) { keep += L.keep; total += L.count; }
    Log("event lists: %d of %u found in %llu ms (%d Single Race entries, %d online entries)", n, kOnlineListCount,
        (unsigned long long)(GetTickCount64() - t0), keep, total);
    return 0;
}

void StartListSearch() {
    if (InterlockedCompareExchange(&g_listsFound, -1, 0) != 0) return;  // found, or a search is running
    HANDLE t = CreateThread(nullptr, 0, FindListsThread, nullptr, 0, nullptr);
    if (t) CloseHandle(t);
    else InterlockedExchange(&g_listsFound, 0);
}

bool WriteMem(void* dst, const void* src, size_t n) {
    SIZE_T done = 0;
    return WriteProcessMemory(GetCurrentProcess(), dst, src, n, &done) && done == n;
}

// filter = true: the kept Races first, count shortened. false: the original list. Only acts on a state change.
void SetListsFiltered(bool filter) {
    if (g_listsFound != 1) {
        if (filter) Log("event lists: not found yet - the picker shows every mode");
        return;
    }
    EnterCriticalSection(&g_listLock);
    if ((g_listsFiltered != 0) != filter) {
        bool ok = true;
        for (EventList& L : g_lists) {
            // still the list we read? (count and first entry unchanged)
            uint32_t cnt = 0, first = 0;
            uint32_t expectCnt = g_listsFiltered ? L.keep : L.count;
            if (!SafeRead((uintptr_t)L.countAt, &cnt, 4) || cnt != expectCnt || !SafeRead(L.array, &first, 4)) {
                ok = false;
                continue;
            }
            if (filter) {
                if (!L.keep) { ok = false; continue; }   // no layout for this list (never: checked at build time)
                ok &= WriteMem((void*)L.array, L.single->ids, 4 * L.keep) && WriteMem(L.countAt, &L.keep, 4);
            } else {
                ok &= WriteMem((void*)L.array, L.ids, 4 * L.count) && WriteMem(L.countAt, &L.count, 4);
            }
        }
        InterlockedExchange(&g_listsFiltered, filter ? 1 : 0);
        Log("event lists %s%s", filter ? "replaced by the Single Race events (Career starting points)" : "restored", ok ? "" : " (some lists changed - skipped)");
    }
    LeaveCriticalSection(&g_listLock);
}

void StartEventRootSearch();
void BuildHpSlots(int racers, int cops);
void RestorePlayerSide();

// OverrideDefaultPageForContext(context, page)
int Hook_OverrideDefaultPage(lua_State* L) {
    const char *ctx, *page;
    size_t cl, pl;
    if (ArgStr(L, 1, &ctx, &cl) && ArgStr(L, 2, &page, &pl) && StrIs(ctx, cl, "offline_menu")) {
        LONG wall = StrIs(page, pl, "wall") ? 1 : 0;
        if (InterlockedExchange(&g_offlinePageIsWall, wall) != wall)
            Log("offline menu page: %.*s%s", (int)pl, page, wall ? " (Single Race picker)" : "");
        SetListsFiltered(wall != 0);  // before the picker page loads
    }
    return g_bindings[kOverrideDefaultPage].original(L);
}

// SwitchLayoutSet(id)
int Hook_SwitchLayoutSet(lua_State* L) {
    if (lua_gettop_(L) >= 1) {
        int t = lua_type_(L, 1);
        if (t == LUA_TNUMBER || t == LUA_TSTRING) {
            LONG id = (LONG)lua_tonumber_(L, 1);
            InterlockedExchange(&g_layoutSet, id);
            if (id == kLayoutMainMenu) {
                StartListSearch();         // first visit: find the lists in the background
                StartEventRootSearch();    // and the event records (AI count)
                SetListsFiltered(false);   // main menu: online lobby and career see the full lists
                RestorePlayerSide();       // and the career map its side
            } else if (g_offlinePageIsWall && (id == kLayoutLocations || id == kLayoutEvents)) {
                SetListsFiltered(true);    // back on the picker after an event
            }
        }
    }
    return g_bindings[kSwitchLayoutSet].original(L);
}

// ---- AI count (see 10.)
// Root of the event records: GameChanger 411487 (hash 3f409a86), data = {pointer array, count, 8}, sorted by id.
// Event = hash 885312e1; OpponentData lists = {pointer array, count, 8}.
const uint32_t kHashEventRoot = 0x3F409A86, kEventRootId = 411487, kHashEvent = 0x885312E1;
const uint32_t kOffCopCount = 0x74, kOffCopList = 0x78, kOffRacerCount = 0xFC, kOffRacerList = 0x100;
// MedalData[] (field 7c7a9566): gold / silver / bronze, each {bounty, busts needed, 99, 0, time}. The cop Hot
// Pursuit events (bundle v11) carry Crimewave's list (7 / 6 / 5 busts); the helper fits it to the racer count.
const uint32_t kOffMedalList = 0xE8, kHashMedalData = 0x63D64900, kOffMedalBusts = 4;

volatile uintptr_t g_eventRoot;   // the root instance, once found
volatile LONG g_rootSearch;       // 1 while a background search runs

bool Instance(uintptr_t p, uint32_t hash, uint32_t* id) {
    uintptr_t d = 0;
    uint32_t h[2] = {};
    if (!p || (p & 7) || !SafeRead(p + 0x10, &d, 8) || d != p + 0x30 || !SafeRead(p + 0x18, h, 8)) return false;
    if (hash && h[0] != hash) return false;
    if (id) *id = h[1];
    return true;
}

bool RootList(uintptr_t root, uintptr_t* arr, uint32_t* count) {
    uint32_t id = 0, es = 0;
    if (!Instance(root, kHashEventRoot, &id) || id != kEventRootId) return false;
    if (!SafeRead(root + 0x30, arr, 8) || !SafeRead(root + 0x38, count, 4) || !SafeRead(root + 0x3C, &es, 4)) return false;
    return *arr && es == 8 && *count > 0 && *count < 8192;
}

// Searches readable private memory for the root ([P+0x18] = hash, [P+0x1C] = 411487).
uintptr_t ScanForEventRoot() {
    const size_t kChunk = 1 << 20;
    uint8_t* buf = (uint8_t*)VirtualAlloc(nullptr, kChunk, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    uintptr_t found = 0;
    MEMORY_BASIC_INFORMATION mbi;
    for (uintptr_t a = 0x10000; buf && !found && VirtualQuery((void*)a, &mbi, sizeof(mbi)) == sizeof(mbi);
         a = (uintptr_t)mbi.BaseAddress + mbi.RegionSize) {
        if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS | PAGE_NOCACHE | PAGE_WRITECOMBINE)) ||
            mbi.Type == MEM_IMAGE || !(mbi.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE)))
            continue;
        uintptr_t base = (uintptr_t)mbi.BaseAddress, end = base + mbi.RegionSize;
        for (uintptr_t c = base; c < end && !found; c += kChunk) {
            size_t len = end - c < kChunk ? end - c : kChunk;
            SIZE_T got = 0;
            if (!ReadProcessMemory(GetCurrentProcess(), (void*)c, buf, len, &got) || got < 8) continue;
            for (size_t i = 0; i + 8 <= got; i += 8) {
                if (*(uint32_t*)(buf + i) != kHashEventRoot || *(uint32_t*)(buf + i + 4) != kEventRootId) continue;
                uintptr_t arr;
                uint32_t n;
                if (c + i >= 0x18 && RootList(c + i - 0x18, &arr, &n)) { found = c + i - 0x18; break; }
            }
        }
    }
    if (buf) VirtualFree(buf, 0, MEM_RELEASE);
    return found;
}

uintptr_t EventRoot(bool searchNow) {
    uintptr_t r = g_eventRoot, arr;
    uint32_t n;
    if (r && RootList(r, &arr, &n)) return r;
    if (!searchNow) return 0;
    ULONGLONG t0 = GetTickCount64();
    r = ScanForEventRoot();
    g_eventRoot = r;
    Log("event records: %s (%llu ms)", r ? "found" : "not found", (unsigned long long)(GetTickCount64() - t0));
    return r;
}

DWORD WINAPI FindEventRootThread(LPVOID) {
    EventRoot(true);
    InterlockedExchange(&g_rootSearch, 0);
    return 0;
}

void StartEventRootSearch() {   // main menu: find the records in the background before the first launch
    if (g_eventRoot || InterlockedCompareExchange(&g_rootSearch, 1, 0) != 0) return;
    HANDLE t = CreateThread(nullptr, 0, FindEventRootThread, nullptr, 0, nullptr);
    if (t) CloseHandle(t);
    else InterlockedExchange(&g_rootSearch, 0);
}

// The event record (instance address) by id: binary search of the root's sorted list.
uintptr_t FindEventRecord(uint32_t want) {
    for (int attempt = 0; attempt < 2; ++attempt) {
        while (g_rootSearch) Sleep(5);   // a background search is running
        uintptr_t root = EventRoot(true), arr;
        uint32_t n;
        if (!root || !RootList(root, &arr, &n)) return 0;
        int lo = 0, hi = (int)n - 1;
        bool broken = false;
        while (lo <= hi) {
            int mid = (lo + hi) / 2;
            uintptr_t p = 0;
            uint32_t id = 0;
            if (!SafeRead(arr + 8 * (uintptr_t)mid, &p, 8) || !Instance(p, kHashEvent, &id)) { broken = true; break; }
            if (id == want) return p;
            if (id < want) lo = mid + 1;
            else hi = mid - 1;
        }
        if (!broken) return 0;
        g_eventRoot = 0;   // stale: search again
    }
    return 0;
}

// How many entries of the list at field address f are valid OpponentData instances (same hash as the first),
// up to cap. The game fixed up every entry of the list's full (bundle) length.
int ListRoom(uintptr_t f, int cap, uint32_t* countNow) {
    uintptr_t arr = 0;
    uint32_t es = 0, hash = 0, id;
    if (!SafeRead(f, &arr, 8) || !SafeRead(f + 8, countNow, 4) || !SafeRead(f + 12, &es, 4) || !arr || es != 8) return 0;
    int k = 0;
    for (; k < cap; ++k) {
        uintptr_t p = 0;
        if (!SafeRead(arr + 8 * (uintptr_t)k, &p, 8) || !Instance(p, hash, &id)) break;
        if (k == 0) SafeRead(p + 0x18, &hash, 4);
    }
    return k;
}

// Cop Hot Pursuit: the busts needed for gold / silver / bronze = the racers, one fewer, two fewer (at least 1).
void FitCopMedals(uint32_t eventId, uintptr_t d, int racers) {
    uintptr_t arr = 0;
    uint32_t cnt = 0, es = 0;
    if (!SafeRead(d + kOffMedalList, &arr, 8) || !SafeRead(d + kOffMedalList + 8, &cnt, 4) ||
        !SafeRead(d + kOffMedalList + 12, &es, 4) || !arr || es != 8 || cnt == 0 || cnt > 3) {
        Log("medals: event %u has no medal list (count %u) - an older CHALLENGEINSTANCES.BNDL?", eventId, cnt);
        return;
    }
    char text[64];
    int o = 0;
    for (uint32_t k = 0; k < cnt; ++k) {
        uintptr_t p = 0;
        if (!SafeRead(arr + 8 * (uintptr_t)k, &p, 8) || !Instance(p, kHashMedalData, nullptr)) {
            Log("medals: event %u: medal %u is not MedalData - not changed", eventId, k);
            return;
        }
        int want = racers - (int)k;
        uint32_t busts = (uint32_t)(want < 1 ? 1 : want);
        bool ok = WriteMem((void*)(p + 0x30 + kOffMedalBusts), &busts, 4);
        o += _snprintf(text + o, sizeof(text) - o, "%s%u%s", k ? " / " : "", busts, ok ? "" : "!");
    }
    Log("medals: event %u: busts needed %s (gold / silver / bronze, %d racers)", eventId, text, racers);
}

// Writes the AI count chosen in the overlay into the launched Single Race event; sets g_eventRacers / g_eventCops
// and the Hot Pursuit start slots to what the event will have.
void ApplyAICount(uint32_t eventId, Mode mode, const Conditions& c) {
    bool hp = mode == kHotPursuit;   // AI cops only in the racer-side Hot Pursuit (the cop side has the player)
    int wantR = hp ? c.hpRacers : mode == kCopPursuit ? c.copRacers : c.raceRacers, wantC = hp ? c.hpCops : 0;
    int haveR = hp ? kDefaultHpRacers : mode == kCopPursuit ? kDefaultCopRacers : kDefaultRaceRacers;
    int haveC = hp ? kDefaultHpCops : 0;
    uintptr_t p = FindEventRecord(eventId);
    if (!p) {
        Log("AI count: event %u not found in memory - it keeps its own count", eventId);
    } else {
        uintptr_t d = p + 0x30;
        uint32_t fieldR = 0, fieldC = 0, listR = 0, listC = 0;
        SafeRead(d + kOffRacerCount, &fieldR, 4);
        SafeRead(d + kOffCopCount, &fieldC, 4);
        int roomR = ListRoom(d + kOffRacerList, hp ? kHpRacersMax : mode == kCopPursuit ? kCopRacersMax : kRaceRacersMax, &listR);
        int roomC = hp ? ListRoom(d + kOffCopList, kHpCopsMax, &listC) : 0;
        haveR = (int)fieldR;
        haveC = (int)fieldC;
        int r = wantR < 1 ? 1 : wantR > roomR ? roomR : wantR;
        int k = !hp ? 0 : wantC < 1 ? 1 : wantC > roomC ? roomC : wantC;
        if (hp && r + k > kMaxAI) k = kMaxAI - r;
        if (r < 1 || (hp && k < 1)) {
            Log("AI count: event %u has no room (racers %d, cops %d) - not changed", eventId, roomR, roomC);
        } else {
            uint32_t ur = (uint32_t)r, uk = (uint32_t)k;
            bool ok = WriteMem((void*)(d + kOffRacerCount), &ur, 4) && WriteMem((void*)(d + kOffRacerList + 8), &ur, 4);
            if (hp) ok = ok && WriteMem((void*)(d + kOffCopCount), &uk, 4) && WriteMem((void*)(d + kOffCopList + 8), &uk, 4);
            if (ok) {
                haveR = r;
                haveC = hp ? k : (int)fieldC;
            }
            Log("AI count: event %u: %d racers%s (was %u, list %u, room %d)%s", eventId, r,
                r != wantR ? " (limited)" : "", fieldR, listR, roomR, ok ? "" : " - WRITE FAILED");
            if (hp)
                Log("AI count: event %u: %d cops%s (was %u, list %u, room %d)%s", eventId, k, k != wantC ? " (limited)" : "",
                    fieldC, listC, roomC, ok ? "" : " - WRITE FAILED");
        }
    }
    if (p && mode == kCopPursuit) FitCopMedals(eventId, p + 0x30, haveR);
    InterlockedExchange(&g_eventRacers, haveR);
    InterlockedExchange(&g_eventCops, haveC);
    if (hp || mode == kCopPursuit) BuildHpSlots(haveR, haveC);   // the cop side: AI racers in front, the player behind
}

// ---- the player's side (see 11.)
// The local competitor: G = [base+0x1276650], index [G+0x6C04], competitor G+0x1180+index*0xB50; cop flag +0x992
// (set by 0x241970 = jmp 0x290E1A0, which the PlayIt parser calls for "IsCop=<n>").
const uintptr_t kRvaRaceManager = 0x1276650;
const uint32_t kOffLocalIndex = 0x6C04, kOffCompetitors = 0x1180, kCompetitorSize = 0xB50, kOffIsCop = 0x992;
// the parser's "IsCop" case: mov r8,[rip+..]; ... movsxd rax,[r8+6C04h]; add r8,1180h; imul rcx,rax,0B50h
const uintptr_t kIsCopParseAt = 0x26FEBE;
const uint8_t kIsCopParse[] = {0x4C, 0x8B, 0x05, 0x8B, 0x67, 0x00, 0x01, 0x85, 0xC0, 0x0F, 0x95, 0xC2, 0x49, 0x63,
                               0x80, 0x04, 0x6C, 0x00, 0x00, 0x49, 0x81, 0xC0, 0x80, 0x11, 0x00, 0x00, 0x48, 0x69,
                               0xC8, 0x50, 0x0B, 0x00, 0x00};
const uint8_t kSetIsCopBody[] = {0x3A, 0x91, 0x92, 0x09, 0x00, 0x00, 0x74, 0x06, 0x88, 0x91, 0x92, 0x09, 0x00, 0x00, 0xC3};
const uintptr_t kRvaSetIsCop = 0x241970, kRvaSetIsCopBody = 0x290E1A0;
volatile LONG g_sideCodeOk = -1;   // the code above checked: 1 ok, 0 differs
volatile LONG g_savedSide = -1;    // the player's flag before the first Single Race launch (-1 = none saved)

bool SideCodeOk() {
    if (g_sideCodeOk < 0) {
        // Only the parser site is required: it holds the global and the offsets the helper uses. The setter
        // (thunk 0x241970 -> 0x290E1A0) is in the protected code and is only logged.
        bool ok = CheckBytes(kIsCopParseAt, kIsCopParse, sizeof(kIsCopParse));
        uint8_t j[5] = {}, body[sizeof(kSetIsCopBody)] = {}, parse[sizeof(kIsCopParse)] = {};
        int32_t rel = 0;
        SafeRead(g_base + kRvaSetIsCop, j, 5);
        memcpy(&rel, j + 1, 4);
        SafeRead(g_base + kRvaSetIsCopBody, body, sizeof(body));
        SafeRead(g_base + kIsCopParseAt, parse, sizeof(parse));
        char hb[3 * sizeof(kIsCopParse) + 1] = {}, hs[3 * sizeof(kSetIsCopBody) + 1] = {};
        for (size_t i = 0; i < sizeof(parse); ++i) _snprintf(hb + 3 * i, 4, "%02X ", parse[i]);
        for (size_t i = 0; i < sizeof(body); ++i) _snprintf(hs + 3 * i, 4, "%02X ", body[i]);
        bool setterOk = j[0] == 0xE9 && kRvaSetIsCop + 5 + (intptr_t)rel == kRvaSetIsCopBody &&
                        memcmp(body, kSetIsCopBody, sizeof(body)) == 0;
        InterlockedExchange(&g_sideCodeOk, ok ? 1 : 0);
        Log("player side: parser site %s, setter %s (thunk %02X -> rva %llX)", ok ? "as expected" : "DIFFERS",
            setterOk ? "as expected" : "differs", j[0], (unsigned long long)(kRvaSetIsCop + 5 + (intptr_t)rel));
        if (!ok) Log("player side: parser bytes %s- the side is not saved / restored", hb);
        if (!setterOk) Log("player side: setter bytes %s", hs);
    }
    return g_sideCodeOk == 1;
}

uintptr_t LocalCompetitorSideFlag() {
    uintptr_t g = 0;
    int idx = -1;
    if (!SideCodeOk() || !SafeRead(g_base + kRvaRaceManager, &g, 8) || !g || !SafeRead(g + kOffLocalIndex, &idx, 4) ||
        idx < 0 || idx > 7)
        return 0;
    return g + kOffCompetitors + (uintptr_t)idx * kCompetitorSize + kOffIsCop;
}

int ReadPlayerSide() {   // 1 cop, 0 racer, -1 unknown
    uintptr_t f = LocalCompetitorSideFlag();
    uint8_t v = 2;
    return f && SafeRead(f, &v, 1) && v <= 1 ? v : -1;
}

// Main menu: the side the player had before Single Race.
void RestorePlayerSide() {
    LONG saved = InterlockedExchange(&g_savedSide, -1);
    if (saved < 0) return;
    int now = ReadPlayerSide();
    uintptr_t f = LocalCompetitorSideFlag();
    if (now < 0 || !f) { Log("player side: not readable - not restored"); return; }
    if (now == saved) return;
    uint8_t v = (uint8_t)saved;
    bool ok = WriteMem((void*)f, &v, 1);
    Log("player side: back to %s (as before Single Race)%s", saved ? "cop" : "racer", ok ? "" : " - WRITE FAILED");
}

int Hook_SendCommandLink(lua_State* L) {
    size_t len = 0;
    const char* s = ArgString(L, &len);
    uint32_t id = 0;
    if (s && ResId(s, len, &id)) {
        uint32_t single = ModeOf(id) != kNone ? id : MapEvent(id);   // picker entries are Single Race ids (v7+)
        if (single) {
            // same route, mode from the overlay (Race 2100000+n, Hot Pursuit 2200000+n, Cop Hot Pursuit 2400000+n)
            uint32_t route = single % 100000;
            int chosen = SR_GetConditions().eventMode;
            single = (chosen == kModeHotPursuit ? 2200000 : chosen == kModeCopPursuit ? 2400000 : 2100000) + route;
            bool cop = chosen == kModeCopPursuit;
            if (g_savedSide < 0) {
                int side = ReadPlayerSide();
                if (side >= 0) InterlockedExchange(&g_savedSide, side);
            }
            char link[112];
            // IsCop: the player's side (the PlayIt parser sets the local competitor's flag; see 11.)
            _snprintf(link, sizeof(link), "PlayIt?event=Mission&res_id=%u&IsCop=%d", single, cop ? 1 : 0);
            link[sizeof(link) - 1] = 0;
            lua_pushstring_(L, link);
            memcpy(Base(L), Top(L) - 16, 16);  // argument 1 = the new string (kept alive by the stack)
            lua_settop_(L, -2);
            InterlockedExchange(&g_mode, ModeOf(single));
            Conditions cond = SR_GetConditions();
            int difficulty = cond.difficulty;
            InterlockedExchange(&g_eventDifficulty, difficulty);
            char ai[40];
            if (chosen == kModeHotPursuit) _snprintf(ai, sizeof(ai), "%d racers + %d cops", cond.hpRacers, cond.hpCops);
            else _snprintf(ai, sizeof(ai), "%d racers", cop ? cond.copRacers : cond.raceRacers);
            ai[sizeof(ai) - 1] = 0;
            Log("event %u -> Single Race %s %u (mode chosen in the overlay), difficulty %s, AI %s; link \"%s\"", id,
                ModeLabel(ModeOf(single)), single, kDifficultyNames[difficulty], ai, link);
            ApplyAICount(single, ModeOf(single), cond);
        } else if (Blocked(id)) {
            Log("event %u: mode not offered in Single Race, ignored", id);
            return 0;
        } else {
            InterlockedExchange(&g_mode, ModeOf(id));  // career events etc.: no remapping of start slots
        }
    }
    return g_bindings[kSendCommandLink].original(L);
}

int CompetitorArg(lua_State* L) {
    uint8_t* b = Base(L);
    if (Top(L) <= b || *(int*)(b + 8) != 3) return -1;
    double v;
    memcpy(&v, b, 8);
    int c = (int)v;
    return c >= 0 && c <= 7 && (double)c == v ? c : -1;
}

// After the intro setup of a Single Race Race: the player swaps actors with the AI racer furthest from it.
void PlacePlayerLast(uintptr_t mgr) {
    int local = -1;
    uintptr_t comps = 0;
    if (!SafeRead(mgr + kLocalCompetitorPtr, &comps, 8) || !comps || !SafeRead(comps + kLocalCompetitor, &local, 4) ||
        local < 0 || local > 7) {
        Log("intro setup: local competitor unknown - not changed");
        return;
    }
    uintptr_t* slots = (uintptr_t*)(mgr + kStartSlotArray);
    uintptr_t actor[8];
    if (!SafeRead((uintptr_t)slots, actor, sizeof(actor)) || !actor[local]) {
        Log("intro setup: no actor for the player - not changed");
        return;
    }
    // translation of each actor's start transform (row 3 of the matrix at actor+0x10)
    float pos[8][3];
    int bound[8];
    for (int c = 0; c < 8; ++c) {
        bound[c] = -1;
        if (!actor[c]) continue;
        if (!SafeRead(actor[c] + 0x40, pos[c], sizeof(pos[c])) || !SafeRead(actor[c] + kActorCompetitor, &bound[c], 4))
            actor[c] = 0;
    }
    Log("intro setup: player = competitor %d at (%.1f, %.1f, %.1f)", local, pos[local][0], pos[local][1], pos[local][2]);
    int back = -1;
    float best = -1;
    for (int c = 0; c < 8; ++c) {
        if (c == local || !actor[c] || bound[c] != c) continue;
        float dx = pos[c][0] - pos[local][0], dy = pos[c][1] - pos[local][1], dz = pos[c][2] - pos[local][2];
        float d = dx * dx + dy * dy + dz * dz;
        Log("intro setup: competitor %d at (%.1f, %.1f, %.1f), %.1f m from the player", c, pos[c][0], pos[c][1], pos[c][2],
            __builtin_sqrtf(d));
        if (d > best) { best = d; back = c; }
    }
    if (back < 0 || bound[local] != local) {
        Log("intro setup: unexpected actor layout - not changed");
        return;
    }
    uintptr_t a = actor[local], b = actor[back];
    int cl = back, cf = local;
    SIZE_T n1 = 0, n2 = 0, n3 = 0, n4 = 0;
    bool ok = WriteProcessMemory(GetCurrentProcess(), (void*)(a + kActorCompetitor), &cl, 4, &n1) &&
              WriteProcessMemory(GetCurrentProcess(), (void*)(b + kActorCompetitor), &cf, 4, &n2) &&
              WriteProcessMemory(GetCurrentProcess(), &slots[local], &b, 8, &n3) &&
              WriteProcessMemory(GetCurrentProcess(), &slots[back], &a, 8, &n4);
    Log("intro setup: player (competitor %d) swapped actors with competitor %d (%.1f m back)%s", local, back,
        __builtin_sqrtf(best), ok ? "" : " - WRITE FAILED");
}

uint64_t Hook_IntroSetup(uintptr_t mgr, uintptr_t a2, uintptr_t a3, uintptr_t a4) {
    uint64_t r = g_introSetupOriginal(mgr, a2, a3, a4);
    if (g_mode == kRace) PlacePlayerLast(mgr);
    return r;
}

// Argument 1 = competitor index (Lua number). Hot Pursuit (either side, intro 278552): replaced by the intro slot.
void RemapCompetitor(lua_State* L, const char* what) {
    if (g_mode != kHotPursuit && g_mode != kCopPursuit) return;
    int c = CompetitorArg(L);
    if (c < 0 || c > 7) return;
    int slot = g_hpSlot[c];
    double nv = (double)slot;
    memcpy(Base(L), &nv, 8);
    if (what) Log("%s: competitor %d -> slot %d", what, c, slot);
}

int Hook_CutscenePos(lua_State* L) {
    RemapCompetitor(L, "start position");
    return g_bindings[kCutscenePos].original(L);
}

int Hook_CutsceneDir(lua_State* L) {
    RemapCompetitor(L, nullptr);
    return g_bindings[kCutsceneDir].original(L);
}

// ====================================================================== opponents and cops
// From NFSHPR Event Randomizer 1.0 (src/NFSHPR_EventRandomizer.cpp), opponent part only; applied to Single Race
// events only (career events are left to the Event Randomizer, if installed).

struct OpponentSettings {
    int racers, cops, matchClass, avoidDuplicates, avoidPlayerModel, racerColours;
    uint32_t exclude[64];
    int excludeCount;
};
OpponentSettings S;
// ini [HotPursuit] CopRespawnTime (v33): seconds for every difficulty, or -1 = the difficulty's own (key empty)
volatile LONG g_copRespawnOverride = -1;
const int kCopRespawnMax = 3600;

const char kDefaultIni[] =
    "; NFSHPR Single Race - re-read at the start of every event.\r\n"
    "\r\n"
    "[Opponents]\r\n"
    "; Single Race events only: AI racers and cops get random cars\r\n"
    "RandomizeRacers=1\r\n"
    "RandomizeCops=1\r\n"
    "; 1 = opponents and cops come from the class of the player's car\r\n"
    "; 0 = each opponent keeps its own class (only the car changes)\r\n"
    "MatchPlayerClass=1\r\n"
    "; 1 = no two opponents get the same car while the class has enough cars\r\n"
    "AvoidDuplicates=1\r\n"
    "; 1 = opponents never get the same model as the player\r\n"
    "AvoidPlayerModel=0\r\n"
    "; 1 = AI racers get a random paint colour of their car (cops keep their liveries)\r\n"
    "RandomizeRacerColours=1\r\n"
    "; vehicle ids never to pick, comma separated (ids are in the log)\r\n"
    "Exclude=\r\n"
    "\r\n"
    "[Event]\r\n"
    "; Single Race events. Set in the overlay on the event picker (F5 or Select on the controller).\r\n"
    "; Mode: Race, HotPursuit (racer side), CopHotPursuit (cop side)\r\n"
    "Mode=Race\r\n"
    "; Difficulty: Easy, Medium, Hard (AI balancing and car toughness from early / mid / late career events)\r\n"
    "Difficulty=Medium\r\n"
    "\r\n"
    "[Race]\r\n"
    "; Single Race Races: AI racers, 1-7. Set in the overlay.\r\n"
    "Racers=7\r\n"
    "\r\n"
    "[HotPursuit]\r\n"
    "; Single Race Hot Pursuits: AI racers 1-6 and cops 1-6, 7 in all at most. Set in the overlay.\r\n"
    "; Cops: all start behind the pack.\r\n"
    "Racers=3\r\n"
    "Cops=4\r\n"
    "; Seconds until a wrecked cop comes back (0-3600). Empty = the difficulty's own (Easy 60, Medium 60, Hard 90);\r\n"
    "; a number here is used for all three difficulties.\r\n"
    "CopRespawnTime=\r\n"
    "\r\n"
    "[CopHotPursuit]\r\n"
    "; Single Race cop-side Hot Pursuits (you are the cop): AI racers 1-7. Set in the overlay.\r\n"
    "Racers=7\r\n"
    "\r\n"
    "[Conditions]\r\n"
    "; Single Race events. Set in the overlay on the event picker (F5 or Select on the controller).\r\n"
    "; TimeOfDay: Dawn, Morning, Noon, Afternoon, Sunset, Night\r\n"
    "TimeOfDay=Noon\r\n"
    "; Weather: Dry, Wet, Rain, Storm\r\n"
    "Weather=Dry\r\n"
    "; TimeProgression: On = the clock runs as in the event, Off = it stays at the chosen time\r\n"
    "TimeProgression=On\r\n"
    "\r\n"
    "[Weapons]\r\n"
    "; Single Race Hot Pursuit events. Classic = no weapons for racers, cops have spike strips, roadblocks and\r\n"
    "; the helicopter. Custom = the charges below (0-5) for racers (the player included) and cops.\r\n"
    "; Level: weapon level 1-3, for all weapons of the player, the AI racers and the cops (both Modes)\r\n"
    "Level=1\r\n"
    "Mode=Classic\r\n"
    "RacerSpikeStrip=2\r\n"
    "RacerEMP=2\r\n"
    "Jammer=1\r\n"
    "Turbo=1\r\n"
    "CopSpikeStrip=2\r\n"
    "CopEMP=2\r\n"
    "RoadBlock=2\r\n"
    "Helicopter=1\r\n"
    "\r\n"
    "[Overlay]\r\n"
    "; 0 = no overlay (the [Conditions] above still apply)\r\n"
    "Enabled=1\r\n"
    "; key that opens the overlay on the event picker: F1-F12, or a virtual-key code number\r\n"
    "Key=F5\r\n"
    "; overlay text size in pixels, 0 = automatic\r\n"
    "FontSize=0\r\n"
    "\r\n"
    "[MenuLabel]\r\n"
    "; Text of the main menu item that opens Single Race (the game's \"WALL\"). Read at game start.\r\n"
    "; \\n = line break (e.g. SINGLE\\nRACE). Empty = keep WALL.\r\n"
    "Text=SINGLE RACE\r\n"
    "\r\n"
    "[Log]\r\n"
    "; 1 = write NFSHPR_SingleRace.log next to the asi, 0 = no log. Read at game start.\r\n"
    "Enabled=1\r\n";

CRITICAL_SECTION g_condLock;
Conditions g_cond = {kModeRace, kNoon, kDry, kWeaponsClassic, {2, 2, 1, 1, 2, 2, 2, 1}, VK_F5, 0, kMedium,
                     kDefaultRaceRacers, kDefaultHpRacers, kDefaultHpCops, kDefaultCopRacers, 1, kDefaultWeaponLevel};
const int kDefaultCharges[kWeaponCount] = {2, 2, 1, 1, 2, 2, 2, 1};

int NameIndex(const wchar_t* text, const char* const* names, int count, int def) {
    char a[32];
    WideCharToMultiByte(CP_ACP, 0, text, -1, a, sizeof(a), nullptr, nullptr);
    for (int i = 0; i < count; ++i)
        if (lstrcmpiA(a, names[i]) == 0) return i;
    return def;
}

int IniInt(const wchar_t* sec, const wchar_t* key, int def);

void LoadConditions() {
    Conditions c;
    wchar_t t[64];
    // an ini from an older helper: add the new sections with their defaults
    if (!GetPrivateProfileStringW(L"Overlay", L"Key", L"", t, 64, g_iniPath)) {
        WritePrivateProfileStringW(L"Conditions", L"TimeOfDay", L"Noon", g_iniPath);
        WritePrivateProfileStringW(L"Conditions", L"Weather", L"Dry", g_iniPath);
        WritePrivateProfileStringW(L"Overlay", L"Enabled", L"1", g_iniPath);
        WritePrivateProfileStringW(L"Overlay", L"Key", L"F5", g_iniPath);
        WritePrivateProfileStringW(L"Overlay", L"FontSize", L"0", g_iniPath);
    }
    // an ini from before the mode option: add it (Race)
    if (!GetPrivateProfileStringW(L"Event", L"Mode", L"", t, 64, g_iniPath))
        WritePrivateProfileStringW(L"Event", L"Mode", L"Race", g_iniPath);
    GetPrivateProfileStringW(L"Event", L"Mode", L"Race", t, 64, g_iniPath);
    c.eventMode = NameIndex(t, kEventModeNames, kEventModeCount, kModeRace);
    // an ini from before the difficulty option: add it (Medium = the balancing of helper 18 and earlier)
    if (!GetPrivateProfileStringW(L"Event", L"Difficulty", L"", t, 64, g_iniPath))
        WritePrivateProfileStringW(L"Event", L"Difficulty", L"Medium", g_iniPath);
    GetPrivateProfileStringW(L"Event", L"Difficulty", L"Medium", t, 64, g_iniPath);
    c.difficulty = NameIndex(t, kDifficultyNames, kDifficultyCount, kMedium);
    GetPrivateProfileStringW(L"Conditions", L"TimeOfDay", L"Noon", t, 64, g_iniPath);
    c.time = NameIndex(t, kTimeNames, kTimeCount, kNoon);
    GetPrivateProfileStringW(L"Conditions", L"Weather", L"Dry", t, 64, g_iniPath);
    c.weather = NameIndex(t, kWeatherNames, kWeatherCount, kDry);
    // time progression (v36; an older ini: add it, On = the behaviour of earlier helpers)
    if (!GetPrivateProfileStringW(L"Conditions", L"TimeProgression", L"", t, 64, g_iniPath))
        WritePrivateProfileStringW(L"Conditions", L"TimeProgression", L"On", g_iniPath);
    GetPrivateProfileStringW(L"Conditions", L"TimeProgression", L"On", t, 64, g_iniPath);
    c.timeProgression = !(_wcsicmp(t, L"Off") == 0 || _wcsicmp(t, L"0") == 0 || _wcsicmp(t, L"false") == 0);
    GetPrivateProfileStringW(L"Overlay", L"Key", L"F5", t, 64, g_iniPath);
    c.menuKey = VK_F5;
    if ((t[0] == L'F' || t[0] == L'f') && t[1] >= L'1' && t[1] <= L'9') {
        int n = _wtoi(t + 1);
        if (n >= 1 && n <= 24) c.menuKey = VK_F1 + n - 1;
    } else if (t[0] >= L'0' && t[0] <= L'9') {
        c.menuKey = _wtoi(t);
    }
    c.fontSize = (int)GetPrivateProfileIntW(L"Overlay", L"FontSize", 0, g_iniPath);
    // an ini from before the weapons option: add the section with its defaults
    if (!GetPrivateProfileStringW(L"Weapons", L"Mode", L"", t, 64, g_iniPath)) {
        WritePrivateProfileStringW(L"Weapons", L"Mode", L"Classic", g_iniPath);
        for (int w = 0; w < kWeaponCount; ++w) {
            wchar_t k[32], v[8];
            MultiByteToWideChar(CP_ACP, 0, kWeaponKeys[w], -1, k, 32);
            _snwprintf(v, 8, L"%d", kDefaultCharges[w]);
            v[7] = 0;
            WritePrivateProfileStringW(L"Weapons", k, v, g_iniPath);
        }
    }
    GetPrivateProfileStringW(L"Weapons", L"Mode", L"Classic", t, 64, g_iniPath);
    c.weaponsMode = NameIndex(t, kWeaponsModeNames, kWeaponsModeCount, kWeaponsClassic);
    for (int w = 0; w < kWeaponCount; ++w) {
        wchar_t k[32];
        MultiByteToWideChar(CP_ACP, 0, kWeaponKeys[w], -1, k, 32);
        int n = (int)GetPrivateProfileIntW(L"Weapons", k, kDefaultCharges[w], g_iniPath);
        c.charges[w] = n < 0 ? 0 : n > kMaxCharges ? kMaxCharges : n;
    }
    // weapon level (v37; an older ini: add it)
    if (!GetPrivateProfileStringW(L"Weapons", L"Level", L"", t, 64, g_iniPath))
        WritePrivateProfileStringW(L"Weapons", L"Level", L"1", g_iniPath);
    {
        int n = (int)GetPrivateProfileIntW(L"Weapons", L"Level", kDefaultWeaponLevel, g_iniPath);
        c.weaponLevel = n < kWeaponLevelMin ? kWeaponLevelMin : n > kWeaponLevelMax ? kWeaponLevelMax : n;
    }
    // AI count, one section per mode (an ini from before v24: add them with the defaults)
    if (!GetPrivateProfileStringW(L"Race", L"Racers", L"", t, 64, g_iniPath))
        WritePrivateProfileStringW(L"Race", L"Racers", L"7", g_iniPath);
    if (!GetPrivateProfileStringW(L"HotPursuit", L"Racers", L"", t, 64, g_iniPath))
        WritePrivateProfileStringW(L"HotPursuit", L"Racers", L"3", g_iniPath);
    if (!GetPrivateProfileStringW(L"HotPursuit", L"Cops", L"", t, 64, g_iniPath))
        WritePrivateProfileStringW(L"HotPursuit", L"Cops", L"4", g_iniPath);
    auto clamp = [](int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; };
    c.raceRacers = clamp(IniInt(L"Race", L"Racers", kDefaultRaceRacers), kRaceRacersMin, kRaceRacersMax);
    c.hpRacers = clamp(IniInt(L"HotPursuit", L"Racers", kDefaultHpRacers), kHpMin, kHpRacersMax);
    c.hpCops = clamp(IniInt(L"HotPursuit", L"Cops", kDefaultHpCops), kHpMin, kHpCopsMax);
    if (c.hpRacers + c.hpCops > kMaxAI) c.hpCops = kMaxAI - c.hpRacers;   // 7 AI at most: fewer cops
    // Cop Hot Pursuit (v29; an older ini: add the section)
    if (!GetPrivateProfileStringW(L"CopHotPursuit", L"Racers", L"", t, 64, g_iniPath))
        WritePrivateProfileStringW(L"CopHotPursuit", L"Racers", L"7", g_iniPath);
    c.copRacers = clamp(IniInt(L"CopHotPursuit", L"Racers", kDefaultCopRacers), kCopRacersMin, kCopRacersMax);
    EnterCriticalSection(&g_condLock);
    g_cond = c;
    LeaveCriticalSection(&g_condLock);
}

int IniInt(const wchar_t* sec, const wchar_t* key, int def) { return (int)GetPrivateProfileIntW(sec, key, def, g_iniPath); }

void LoadSettings() {
    if (GetFileAttributesW(g_iniPath) == INVALID_FILE_ATTRIBUTES) {
        HANDLE f = CreateFileW(g_iniPath, GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (f != INVALID_HANDLE_VALUE) {
            DWORD w;
            WriteFile(f, kDefaultIni, sizeof(kDefaultIni) - 1, &w, nullptr);
            CloseHandle(f);
        }
    }
    OpponentSettings s;
    memset(&s, 0, sizeof(s));
    s.racers = IniInt(L"Opponents", L"RandomizeRacers", 1);
    s.cops = IniInt(L"Opponents", L"RandomizeCops", 1);
    s.matchClass = IniInt(L"Opponents", L"MatchPlayerClass", 1);
    s.avoidDuplicates = IniInt(L"Opponents", L"AvoidDuplicates", 1);
    s.avoidPlayerModel = IniInt(L"Opponents", L"AvoidPlayerModel", 0);
    wchar_t t[512];
    if (!GetPrivateProfileStringW(L"Opponents", L"RandomizeRacerColours", L"", t, 64, g_iniPath))
        WritePrivateProfileStringW(L"Opponents", L"RandomizeRacerColours", L"1", g_iniPath);  // an older ini
    s.racerColours = IniInt(L"Opponents", L"RandomizeRacerColours", 1);
    GetPrivateProfileStringW(L"Opponents", L"Exclude", L"", t, 512, g_iniPath);
    uint32_t v = 0;
    bool have = false;
    for (const wchar_t* p = t;; ++p) {
        if (*p >= L'0' && *p <= L'9') { v = v * 10 + (uint32_t)(*p - L'0'); have = true; }
        else {
            if (have && s.excludeCount < 64) s.exclude[s.excludeCount++] = v;
            v = 0;
            have = false;
            if (!*p) break;
        }
    }
    S = s;
    // cop respawn delay override (v33; an older ini: add the key, empty)
    if (GetPrivateProfileStringW(L"HotPursuit", L"CopRespawnTime", L"\x01", t, 64, g_iniPath) == 1 && t[0] == 1) {
        WritePrivateProfileStringW(L"HotPursuit", L"CopRespawnTime", L"", g_iniPath);
        t[0] = 0;
    }
    {
        const wchar_t* q = t;
        while (*q == L' ' || *q == L'\t') ++q;
        LONG secs = -1;
        if (*q) {
            long n = 0;
            bool ok = true;
            const wchar_t* d = q;
            for (; *d >= L'0' && *d <= L'9'; ++d) n = n < 100000 ? n * 10 + (*d - L'0') : n;
            while (*d == L' ' || *d == L'\t') ++d;
            if (d == q || *d) ok = false;
            if (!ok) Log("ini [HotPursuit] CopRespawnTime=%ls is not a whole number of seconds - the difficulty's own delay is used", q);
            else secs = n > kCopRespawnMax ? kCopRespawnMax : n;
        }
        InterlockedExchange(&g_copRespawnOverride, secs);
    }
    LoadConditions();
}

uint64_t g_rng;
uint64_t Rand() {  // xorshift64*
    g_rng ^= g_rng >> 12;
    g_rng ^= g_rng << 25;
    g_rng ^= g_rng >> 27;
    return g_rng * 0x2545F4914F6CDD1DULL;
}
unsigned RandBelow(unsigned n) { return n ? (unsigned)(Rand() % n) : 0; }

const char* kRacerClass[] = {"?", "Sports", "Performance", "Super", "Exotic", "Hyper"};
const char* kCopClass[] = {"?", "Traffic Police", "Highway Patrol", "Rapid Deployment", "Speed Enforcement", "Special Response"};

const Vehicle* FindVehicle(uint32_t id) {
    for (int i = 0; i < kVehicleCount; ++i)
        if (kVehicles[i].id == id) return &kVehicles[i];
    return nullptr;
}
const char* ClassName(const Vehicle* v) {
    if (!v || v->tier < 1 || v->tier > 5) return "?";
    return v->side == 3 ? kCopClass[v->tier] : kRacerClass[v->tier];
}
const char* VName(const Vehicle* v) { return v ? v->name : "unknown"; }

bool Excluded(uint32_t id) {
    for (int i = 0; i < S.excludeCount; ++i)
        if (S.exclude[i] == id) return true;
    return false;
}

struct Slot { int competitor; uint32_t original, chosen, colour; };  // colour 0 = not chosen yet
struct EventState {
    int number;
    bool singleRace;  // a Single Race event: opponents are randomized
    Slot slots[64];
    int slotCount;
    uint32_t playerId;
    bool playerKnown;
};
EventState E;

bool InOnlineEvent() {
    uintptr_t mgr = 0, online = 0;
    uint8_t flag = 0;
    if (!SafeRead(g_base + kRvaGameManager, &mgr, 8) || !mgr) return false;
    if (!SafeRead(mgr + kOffOnlineState, &online, 8) || !online) return false;
    if (!SafeRead(online + kOffInOnlineEvent, &flag, 1)) return false;
    return flag != 0;
}

// The player's car: GetSelectedVehicleId's original, called on the current Lua state (clears the stack).
void LearnPlayerCar(lua_State* L) {
    if (E.playerKnown) return;
    int n = g_origGetSelectedVehicleId(L);
    if (n >= 1 && lua_type_(L, -1) == LUA_TNUMBER) {
        E.playerId = (uint32_t)lua_tonumber_(L, -1);
        E.playerKnown = true;
        const Vehicle* v = FindVehicle(E.playerId);
        Log("player car: %u %s [%s %s]", E.playerId, VName(v), v ? (v->side == 3 ? "cop" : "racer") : "?", ClassName(v));
    }
    lua_settop_(L, 0);
}

bool UsedThisEvent(uint32_t id) {
    for (int i = 0; i < E.slotCount; ++i)
        if (E.slots[i].chosen == id) return true;
    return false;
}

uint32_t PickReplacement(uint32_t original) {
    const Vehicle* o = FindVehicle(original);
    if (!o) return original;  // traffic / unknown vehicle: leave it
    if (o->side == 2 && !S.racers) return original;
    if (o->side == 3 && !S.cops) return original;
    const Vehicle* p = E.playerKnown ? FindVehicle(E.playerId) : nullptr;
    int tier = (S.matchClass && p) ? p->tier : o->tier;
    const Vehicle* pool[64];
    int n = 0;
    for (int pass = 0; pass < 2 && n == 0; ++pass) {  // pass 1 allows duplicates if needed
        for (int i = 0; i < kVehicleCount && n < 64; ++i) {
            const Vehicle* v = &kVehicles[i];
            if (v->side != o->side || v->tier != tier || Excluded(v->id)) continue;
            if (S.avoidPlayerModel && p && lstrcmpA(v->name, p->name) == 0) continue;
            if (pass == 0 && S.avoidDuplicates && UsedThisEvent(v->id)) continue;
            pool[n++] = v;
        }
    }
    if (!n) return original;
    return pool[RandBelow((unsigned)n)]->id;
}

int Hook_StopTraffic(lua_State* L) {
    ++E.number;
    E.slotCount = 0;
    E.playerKnown = false;
    E.playerId = 0;
    E.singleRace = g_mode != kNone;
    if (E.singleRace) {
        LoadSettings();
        Log("---- event %d loading (Single Race %s): RandomizeRacers=%d RandomizeCops=%d MatchPlayerClass=%d RandomizeRacerColours=%d", E.number,
            ModeLabel(g_mode), S.racers, S.cops, S.matchClass, S.racerColours);
    }
    return g_bindings[kStopTraffic].original(L);
}

// GetOpponentVehicleId(competitor_index) -> vehicle id
int Hook_GetOpponentVehicleId(lua_State* L) {
    int competitor = (lua_gettop_(L) >= 1 && lua_type_(L, 1) == LUA_TNUMBER) ? (int)lua_tonumber_(L, 1) : -1;
    int n = g_bindings[kGetOpponentVehicleId].original(L);
    if (!E.singleRace || n < 1 || lua_type_(L, -1) != LUA_TNUMBER) return n;
    uint32_t original = (uint32_t)lua_tonumber_(L, -1);
    if (InOnlineEvent()) return n;
    // Same competitor and same original car in this event -> same answer (spawn, cutscene, redeploy).
    for (int i = 0; i < E.slotCount; ++i) {
        Slot& sl = E.slots[i];
        if (sl.competitor == competitor && sl.original == original) {
            if (sl.chosen == original) return n;
            lua_settop_(L, 0);
            lua_pushnumber_(L, (double)sl.chosen);
            return 1;
        }
    }
    LearnPlayerCar(L);  // clears the stack
    uint32_t chosen = PickReplacement(original);
    if (E.slotCount < 64) E.slots[E.slotCount++] = Slot{competitor, original, chosen, 0};
    const Vehicle* o = FindVehicle(original);
    const Vehicle* c = FindVehicle(chosen);
    Log("competitor %d: %u %s [%s] -> %u %s [%s]", competitor, original, VName(o), ClassName(o), chosen, VName(c),
        ClassName(c));
    lua_settop_(L, 0);
    lua_pushnumber_(L, (double)chosen);
    return 1;
}

const PaintList* FindPaints(uint32_t vehicle) {
    for (int i = 0; i < kPaintCount; ++i)
        if (kPaints[i].vehicle == vehicle) return &kPaints[i];
    return nullptr;
}

// A random colour of the car, preferring one no other AI with the same car has in this event.
uint32_t PickColour(const PaintList* pl, int self) {
    uint32_t pool[16];
    int n = 0;
    for (int pass = 0; pass < 2 && n == 0; ++pass)
        for (int k = 0; k < pl->count; ++k) {
            bool taken = false;
            for (int i = 0; pass == 0 && i < E.slotCount; ++i)
                if (i != self && E.slots[i].chosen == pl->vehicle && E.slots[i].colour == pl->colours[k]) taken = true;
            if (!taken) pool[n++] = pl->colours[k];
        }
    return n ? pool[RandBelow((unsigned)n)] : 0;
}

// GetOpponentColourId(competitor_index) -> paint colour id. The scripts call it right after
// GetOpponentVehicleId for the same competitor, so the slot recorded there names the car it will drive.
int Hook_GetOpponentColourId(lua_State* L) {
    int competitor = (lua_gettop_(L) >= 1 && lua_type_(L, 1) == LUA_TNUMBER) ? (int)lua_tonumber_(L, 1) : -1;
    int n = g_opponentColour.original(L);
    if (!E.singleRace || !S.racerColours || n < 1 || lua_type_(L, -1) != LUA_TNUMBER || competitor < 0) return n;
    if (InOnlineEvent()) return n;
    int self = -1;
    for (int i = E.slotCount - 1; i >= 0; --i)  // the latest car of this competitor
        if (E.slots[i].competitor == competitor) { self = i; break; }
    if (self < 0) return n;
    Slot& sl = E.slots[self];
    const Vehicle* v = FindVehicle(sl.chosen);
    if (!v || v->side != 2) return n;  // cops keep their liveries
    if (!sl.colour) {
        const PaintList* pl = FindPaints(sl.chosen);
        if (!pl || pl->count == 0) return n;
        uint32_t original = (uint32_t)lua_tonumber_(L, -1);
        sl.colour = PickColour(pl, self);
        if (!sl.colour) return n;
        Log("competitor %d: colour %u -> %u (%d colours for %s)", competitor, original, sl.colour, pl->count, VName(v));
    }
    lua_settop_(L, 0);
    lua_pushnumber_(L, (double)sl.colour);
    return 1;
}

// ====================================================================== time of day and weather
// From NFSHPR Event Randomizer 1.0 (Hook_SetTimeOfDay, SetFlagArg, DescribeArg), with the overlay's fixed
// choices instead of its modes. Single Race events only.

uint8_t* ArgSlot(lua_State* L, int i) { return Base(L) + (size_t)(i - 1) * 16; }
int SlotType(uint8_t* t) { return *(int*)(t + 8); }

// Writes a true/false into argument i, keeping the type the game used for it.
void SetFlagArg(lua_State* L, int i, bool v) {
    if (lua_gettop_(L) < i) return;
    uint8_t* t = ArgSlot(L, i);
    switch (SlotType(t)) {
        case LUA_TNUMBER: *(double*)t = v ? 1.0 : 0.0; break;
        case LUA_TSTRING: {
            const char* str = lua_tolstring_(L, i, nullptr);
            const char* w = v ? "1" : "0";
            if (str && (str[0] == 't' || str[0] == 'f')) w = v ? "true" : "false";
            if (str && (str[0] == 'T' || str[0] == 'F')) w = v ? "True" : "False";
            lua_pushstring_(L, w);
            t = ArgSlot(L, i);  // the stack may have moved
            memcpy(t, Top(L) - 16, 16);
            lua_settop_(L, -2);
            break;
        }
        default:
            *(int*)t = v ? 1 : 0;
            *(int*)(t + 8) = LUA_TBOOLEAN;
            break;
    }
}

void DescribeArg(lua_State* L, int i, char* out, size_t cap) {
    if (lua_gettop_(L) < i) { _snprintf(out, cap, "-"); return; }
    uint8_t* t = ArgSlot(L, i);
    switch (SlotType(t)) {
        case LUA_TNIL: _snprintf(out, cap, "nil"); break;
        case LUA_TBOOLEAN: _snprintf(out, cap, "%s", *(int*)t ? "true" : "false"); break;
        case LUA_TNUMBER: _snprintf(out, cap, "%.2f", *(double*)t); break;
        case LUA_TSTRING: {
            const char* str = lua_tolstring_(L, i, nullptr);
            _snprintf(out, cap, "\"%.20s\"", str ? str : "");
            break;
        }
        default: _snprintf(out, cap, "type%d", SlotType(t)); break;
    }
    out[cap - 1] = 0;
}

void LogTimeArgs(lua_State* L, const char* label) {
    char a[9][40];
    for (int i = 0; i < 9; ++i) DescribeArg(L, i + 1, a[i], sizeof(a[i]));
    Log("%s start=%s end=%s cycle=%s wet=%s rain=%s thunder=%s altWeather=%s sunOverride=%s sunDir=%s", label, a[0],
        a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8]);
}

double WrapHour(double h) {
    while (h >= 24.0) h -= 24.0;
    while (h < 0.0) h += 24.0;
    return h;
}

// SetTimeOfDay(start, finish, cycle, isWet, isRain, isThunder, isAlternativeWeather, overrideSun, sunDir)
int Hook_SetTimeOfDay(lua_State* L) {
    if (!E.singleRace || InOnlineEvent()) return g_bindings[kSetTimeOfDay].original(L);
    Conditions c = SR_GetConditions();
    LogTimeArgs(L, "event time/weather:");
    int top = lua_gettop_(L);
    if (top >= 2 && SlotType(ArgSlot(L, 1)) == LUA_TNUMBER) {
        double start = *(double*)ArgSlot(L, 1);
        bool finishIsNumber = SlotType(ArgSlot(L, 2)) == LUA_TNUMBER;
        double span = finishIsNumber ? *(double*)ArgSlot(L, 2) - start : 0.0;  // the clock runs as in the event
        double ns = WrapHour(kTimeHours[c.time]);
        *(double*)ArgSlot(L, 1) = ns;
        // Time progression Off (v36): the finish is a moment after the start, so the clock barely moves (finish =
        // start exactly could be read as a full 24-hour lap, as a negative span wraps the long way round)
        if (!c.timeProgression) span = 0.001;
        if (finishIsNumber) *(double*)ArgSlot(L, 2) = WrapHour(ns + span);
        if (top >= 8) SetFlagArg(L, 8, false);  // drop the event's fixed sun direction
    }
    if (top >= 6) {
        SetFlagArg(L, 4, c.weather != kDry);
        SetFlagArg(L, 5, c.weather == kRain || c.weather == kStorm);
        SetFlagArg(L, 6, c.weather == kStorm);
    }
    LogTimeArgs(L, "new time/weather:  ");
    Log("conditions: %s, %s, time progression %s", kTimeNames[c.time], kWeatherLabels[c.weather],
        c.timeProgression ? "on" : "off");
    return g_bindings[kSetTimeOfDay].original(L);
}

// ---- weapons

// either side of a Hot Pursuit (weapons)
bool SingleRaceHotPursuit() { return (g_mode == kHotPursuit || g_mode == kCopPursuit) && !InOnlineEvent(); }
bool SingleRaceEvent() { return g_mode != kNone && !InOnlineEvent(); }

// ====================================================================== difficulty
// Chosen in the overlay (Easy / Medium / Hard), applied to Single Race events only. Each preset is a set taken
// from a real career event (early / mid / late career).
//  1. Balancing records. In "Post Load" the mode script calls SetupRaceBalancing() (thunk 0x292970 -> 0x2B61D90);
//     it reads 5 ids from the event (RaceBalancingData cc1a2667, RaceBalancingProfile 1c6e495f,
//     ShortcutBalancingData e8350282, CopBalancingData a745e92d, HotPursuitCopBalancingData 8381ea95) and calls
//     0x226190 (jmp 0x284C550; only caller 0x2B61F2B) = Setup(manager, the 5 ids; the last two on the stack).
//     The manager stores them (+0x118..+0x128) and loads the records from BALANCINGDATA by id, so the helper
//     swaps the ids there. The Single Race events have no shortcut set (the AI took no shortcuts) and their Hot
//     Pursuits no cop set.
//  2. Player toughness and cop respawn come from GetChallengeParam (Lua): LocalPlayerToughnessScale
//     (Primitive.SetupRaceCar: player toughness = car toughness * scale), UseDelayCopRespawn and
//     AICopRespawnAfterWreckTime (SetCopDelayRespawnAfterWrecking).
//  3. AI toughness: GetOpponentToughness(competitor) -> the event's OpponentData Toughness (0 = the car's own).
//     Hot Pursuit only: competitors 1..R are the AI racers, R+1..R+C the cops (R, C = the event's AI count).
//     Races keep theirs (1.0 in every career race).
struct DifficultyPreset {
    const char* source;
    uint32_t raceBalancing, profile, shortcuts, cops;  // BALANCINGDATA ids
    float playerToughnessScale;
    float copRespawnSeconds;   // Hot Pursuit (with UseDelayCopRespawn = true): seconds until a wrecked cop comes
                               // back (silently, parked on the route line ahead). v31 used 120 s for all three; v33
                               // is back to 60 / 60 / 90, and ini [HotPursuit] CopRespawnTime overrides all three
    float racerToughness;      // Hot Pursuit AI racers
    float copToughness[6];     // Hot Pursuit cops 1-6 (bundle v10: all start with the pack; before v10 the
                               // 3rd / 4th joined at 35 % / 66 %)
};
const DifficultyPreset kRacePresets[kDifficultyCount] = {
    {"career race 572222 (class 2)", 695386, 745009, 695374, 0, 2.25f, 0, 0, {0, 0, 0, 0, 0, 0}},
    {"career race 1070080 (class 3)", 1136012, 1136021, 1136011, 0, 3.0f, 0, 0, {0, 0, 0, 0, 0, 0}},
    {"career race 1070006 (class 5)", 1070019, 1070021, 1070023, 0, 3.0f, 0, 0, {0, 0, 0, 0, 0, 0}},
};
const DifficultyPreset kHotPursuitPresets[kDifficultyCount] = {
    {"career Hot Pursuit 572362 (class 2)", 674399, 688501, 688502, 688454, 2.25f, 60.0f, 0.17f, {0.25f, 0.25f, 0.35f, 0.35f, 0.25f, 0.25f}},
    {"career Hot Pursuit 1070090 (class 3), cops 1070087", 1136024, 1136043, 1136006, 1136048, 4.0f, 60.0f, 0.2f,
     {0.3f, 0.38f, 0.38f, 0.38f, 0.3f, 0.38f}},
    {"career Hot Pursuit 1070008 (class 5)", 1070059, 1070061, 1070063, 1070057, 4.0f, 90.0f, 0.35f, {0.5f, 0.5f, 0.45f, 0.45f, 0.5f, 0.5f}},
};

// Cop Hot Pursuit (v29): presets from career cop events (HPOfflineCop) of cop classes 2 / 3 / 5. Those events have no
// RaceBalancingData, profile or CopBalancingData; their AI racers are balanced by HotPursuitCopBalancingData (one
// entry per racer: 744065 has 6, the balancing overlay sr_v2 gives it a 7th). The player's toughness scale is
// LocalPlayerCopToughnessScale; the AI racers' toughness comes from the event's racer list (7 values).
struct CopPursuitPreset {
    const char* source;
    uint32_t shortcuts, hpCops;      // ShortcutBalancingData, HotPursuitCopBalancingData
    float playerCopToughnessScale;
    float racerToughness[7];         // AI racers 1-7
};
const CopPursuitPreset kCopPursuitPresets[kDifficultyCount] = {
    {"career cop event 572318 Point of Impact (cop class 2)", 688600, 744065, 2.25f, {0.25f, 0.25f, 0.22f, 0.2f, 0.2f, 0.2f, 0.2f}},
    {"career cop event 1070088 Crimewave (cop class 3)", 1136002, 1136036, 3.0f, {0.2f, 0.25f, 0.28f, 0.22f, 0.22f, 0.2f, 0.2f}},
    {"career cop event 1070007 Lockdown (cop class 5)", 1070032, 1070034, 3.0f, {0.36f, 0.33f, 0.33f, 0.31f, 0.31f, 0.29f, 0.29f}},
};
const CopPursuitPreset* CurrentCopPreset() {
    if (g_mode != kCopPursuit || !SingleRaceEvent()) return nullptr;
    LONG d = g_eventDifficulty;
    return &kCopPursuitPresets[d >= 0 && d < kDifficultyCount ? d : kMedium];
}

const DifficultyPreset* CurrentPreset() {
    if (!SingleRaceEvent() || g_mode == kCopPursuit) return nullptr;   // the cop side: CurrentCopPreset
    LONG d = g_eventDifficulty;
    if (d < 0 || d >= kDifficultyCount) d = kMedium;
    return g_mode == kHotPursuit ? &kHotPursuitPresets[d] : &kRacePresets[d];
}
const char* CurrentDifficultyName() {
    LONG d = g_eventDifficulty;
    return kDifficultyNames[d >= 0 && d < kDifficultyCount ? d : kMedium];
}

bool Install(Binding& b, int slot);

const uintptr_t kRvaBalancingSetup = 0x226190, kRvaBalancingSetupCaller = 0x2B61F2B;
const int kBalancingSetupSlot = 11;
typedef uint64_t (*Fn_BalancingSetup)(uintptr_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
Fn_BalancingSetup g_balancingSetupOriginal;

uint64_t Hook_BalancingSetup(uintptr_t mgr, uint32_t race, uint32_t profile, uint32_t shortcuts, uint32_t cops,
                             uint32_t hpCops) {
    if (const CopPursuitPreset* q = CurrentCopPreset()) {
        Log("difficulty %s (%s): balancing 0 / profile 0 / shortcuts %u / cops 0 / hot pursuit cops %u (event had %u / %u / "
            "%u / %u / %u)",
            CurrentDifficultyName(), q->source, q->shortcuts, q->hpCops, race, profile, shortcuts, cops, hpCops);
        return g_balancingSetupOriginal(mgr, 0, 0, q->shortcuts, 0, q->hpCops);
    }
    if (const DifficultyPreset* p = CurrentPreset()) {
        Log("difficulty %s (%s): balancing %u / profile %u / shortcuts %u / cops %u (event had %u / %u / %u / %u, "
            "hot pursuit cops %u kept)",
            CurrentDifficultyName(), p->source, p->raceBalancing, p->profile, p->shortcuts, p->cops, race, profile,
            shortcuts, cops, hpCops);
        race = p->raceBalancing;
        profile = p->profile;
        shortcuts = p->shortcuts;
        cops = p->cops;
    }
    return g_balancingSetupOriginal(mgr, race, profile, shortcuts, cops, hpCops);
}

// The thunk is a 5-byte jmp like the Lua natives' thunks; its only caller is SetupRaceBalancing.
bool InstallBalancingSetupHook() {
    uint8_t thunk[5], call[5];
    if (!SafeRead(g_base + kRvaBalancingSetup, thunk, 5) || thunk[0] != 0xE9 ||
        !SafeRead(g_base + kRvaBalancingSetupCaller, call, 5) || call[0] != 0xE8) {
        Log("balancing setup: code not as expected");
        return false;
    }
    int32_t rel;
    memcpy(&rel, call + 1, 4);
    if (kRvaBalancingSetupCaller + 5 + (intptr_t)rel != kRvaBalancingSetup) {
        Log("balancing setup: caller does not call %X", (unsigned)kRvaBalancingSetup);
        return false;
    }
    Binding b = {"race balancing setup", kRvaBalancingSetup, 0, (lua_CFunction)(void*)&Hook_BalancingSetup, nullptr};
    if (!Install(b, kBalancingSetupSlot)) return false;
    g_balancingSetupOriginal = (Fn_BalancingSetup)(void*)b.original;
    return true;
}

// GetChallengeParam(name): the original, then the preset's value in place of its result.
int Hook_GetChallengeParam(lua_State* L) {
    const char* name = nullptr;
    size_t len = 0;
    bool haveName = lua_gettop_(L) >= 1 && ArgStr(L, 1, &name, &len);
    char key[48] = {};
    if (haveName) memcpy(key, name, len < sizeof(key) - 1 ? len : sizeof(key) - 1);  // the stack is about to change
    int n = g_bindings[kGetChallengeParam].original(L);
    if (const CopPursuitPreset* q = haveName && n == 1 ? CurrentCopPreset() : nullptr) {
        if (!strcmp(key, "LocalPlayerCopToughnessScale") && lua_type_(L, -1) == LUA_TNUMBER) {
            double was = lua_tonumber_(L, -1);
            lua_settop_(L, -2);
            lua_pushnumber_(L, q->playerCopToughnessScale);
            Log("difficulty %s: LocalPlayerCopToughnessScale %.2f (event %.2f)", CurrentDifficultyName(),
                q->playerCopToughnessScale, was);
        }
        return n;
    }
    const DifficultyPreset* p = haveName && n == 1 ? CurrentPreset() : nullptr;
    if (!p) return n;
    bool hp = g_mode == kHotPursuit;
    int t = lua_type_(L, -1);
    if (!strcmp(key, "LocalPlayerToughnessScale") && t == LUA_TNUMBER) {
        double was = lua_tonumber_(L, -1);
        lua_settop_(L, -2);
        lua_pushnumber_(L, p->playerToughnessScale);
        Log("difficulty %s: LocalPlayerToughnessScale %.2f (event %.2f)", CurrentDifficultyName(), p->playerToughnessScale, was);
    } else if (hp && !strcmp(key, "AICopRespawnAfterWreckTime") && t == LUA_TNUMBER) {
        double was = lua_tonumber_(L, -1);
        lua_settop_(L, -2);
        LONG over = g_copRespawnOverride;
        double secs = over >= 0 ? (double)over : p->copRespawnSeconds;
        lua_pushnumber_(L, secs);
        Log("difficulty %s: AICopRespawnAfterWreckTime %.0f s (%s; event %.0f)", CurrentDifficultyName(), secs,
            over >= 0 ? "ini CopRespawnTime" : "difficulty default", was);
    } else if (hp && !strcmp(key, "UseDelayCopRespawn") && (t == LUA_TBOOLEAN || t == LUA_TNUMBER)) {
        lua_settop_(L, -2);
        if (t == LUA_TBOOLEAN) lua_pushboolean_(L, 1);
        else lua_pushnumber_(L, 1.0);
        Log("difficulty %s: UseDelayCopRespawn true", CurrentDifficultyName());
    }
    return n;
}

// GetOpponentToughness(competitor) -> toughness (0 = the car's own). Hot Pursuit only.
int Hook_GetOpponentToughness(lua_State* L) {
    int competitor = (lua_gettop_(L) >= 1 && lua_type_(L, 1) == LUA_TNUMBER) ? (int)lua_tonumber_(L, 1) : -1;
    int n = g_bindings[kGetOpponentToughness].original(L);
    if (const CopPursuitPreset* q = n == 1 ? CurrentCopPreset() : nullptr) {   // every AI is a racer
        if (competitor < 1 || competitor > 7 || competitor > (int)g_eventRacers || lua_type_(L, -1) != LUA_TNUMBER) return n;
        float v = q->racerToughness[competitor - 1];
        double was = lua_tonumber_(L, -1);
        lua_settop_(L, -2);
        lua_pushnumber_(L, v);
        Log("difficulty %s: competitor %d (AI racer) toughness %.2f (event %.2f)", CurrentDifficultyName(), competitor, v, was);
        return n;
    }
    const DifficultyPreset* p = n == 1 && g_mode == kHotPursuit ? CurrentPreset() : nullptr;
    int racers = (int)g_eventRacers, cops = (int)g_eventCops;
    if (!p || competitor < 1 || competitor > racers + cops || competitor > 7 || lua_type_(L, -1) != LUA_TNUMBER) return n;
    int cop = competitor - racers - 1;   // 0-based cop index, < 0 for an AI racer
    float v = cop < 0 ? p->racerToughness : p->copToughness[cop < 6 ? cop : 5];
    double was = lua_tonumber_(L, -1);
    lua_settop_(L, -2);
    lua_pushnumber_(L, v);
    Log("difficulty %s: competitor %d (%s) toughness %.2f (event %.2f)", CurrentDifficultyName(), competitor,
        cop < 0 ? "AI racer" : "cop", v, was);
    return n;
}

// Loader 0x380330: Single Race Hot Pursuit -> the player uses the Single Race player set, every AI the AI set.
uint64_t Hook_WeaponSetLoader(uintptr_t mgr, uint32_t playerSet, uint32_t aiSet, uint32_t* vehicleSets, uint64_t patrol) {
    // The HP script sets up weapons before StopTraffic, so the event state is not new yet: the mode set when the
    // event was launched decides.
    if (SingleRaceHotPursuit()) {
        Log("weapons: event sets player %u / AI %u -> Single Race sets player %u / AI %u", playerSet, aiSet,
            kSingleRacePlayerWeaponSet, kSingleRaceWeaponSet);
        playerSet = kSingleRacePlayerWeaponSet;
        aiSet = kSingleRaceWeaponSet;
        if (vehicleSets)
            for (int i = 0; i < 8; ++i) vehicleSets[i] = 0;  // no per-car sets: every AI takes the AI set
        InterlockedExchange(&g_weaponsWritten, 0);
    }
    return g_weaponSetLoaderOriginal(mgr, playerSet, aiSet, vehicleSets, patrol);
}

// The loaded set behind a loader handle (as the game resolves it: [handle+8] + [[handle]+0x28]).
uintptr_t ResolveSet(uintptr_t handle, uint32_t wantId) {
    uintptr_t res = 0, nullRes = 0, base = 0;
    uint32_t off = 0;
    if (!SafeRead(handle, &res, 8) || !SafeRead(handle + 8, &off, 4) || !res) return 0;
    if (SafeRead(g_base + kRvaNullResource, &nullRes, 8) && res == nullRes) return 0;
    if (!SafeRead(res + 0x28, &base, 8) || !base) return 0;
    uintptr_t p = base + off;
    uint32_t hid[2];
    if (!SafeRead(p + 0x18, hid, 8) || hid[0] != kHashWeaponSet || hid[1] != wantId) return 0;
    return p;
}

bool WriteCharges(uintptr_t set, const int* charges, int level) {
    uintptr_t data = 0;
    if (!SafeRead(set + 0x10, &data, 8) || !data) return false;
    uintptr_t subs[kWeaponCount];
    for (int w = 0; w < kWeaponCount; ++w) {  // check all first, then write
        uint32_t h = 0;
        if (!SafeRead(data + kWeaponFieldOffset[w], &subs[w], 8) || !subs[w] || !SafeRead(subs[w] + 0x18, &h, 4) ||
            h != kHashWeapon)
            return false;
    }
    for (int w = 0; w < kWeaponCount; ++w) {
        uintptr_t sd = 0;
        if (!SafeRead(subs[w] + 0x10, &sd, 8) || !sd) return false;
        uint32_t v = (uint32_t)charges[w];
        SIZE_T n = 0;
        if (!WriteProcessMemory(GetCurrentProcess(), (void*)sd, &v, 4, &n) || n != 4) return false;  // Count
    }
    uint32_t lv = (uint32_t)level;
    SIZE_T n1 = 0, n2 = 0;
    return WriteProcessMemory(GetCurrentProcess(), (void*)(data + kOffCopWeaponLevel), &lv, 4, &n1) && n1 == 4 &&
           WriteProcessMemory(GetCurrentProcess(), (void*)(data + kOffRacerWeaponLevel), &lv, 4, &n2) && n2 == 4;
}

// SetupWeaponAvailabilityForCompetitor(i): before the first one of a Single Race Hot Pursuit, write the charges.
int Hook_WeaponsForCompetitor(lua_State* L) {
    if (SingleRaceHotPursuit() && !InterlockedExchange(&g_weaponsWritten, 1)) {
        Conditions c = SR_GetConditions();
        const int* charges = c.weaponsMode == kWeaponsCustom ? c.charges : kClassicCharges;
        uintptr_t gm = 0, mgr = 0;
        bool ok = SafeRead(g_base + kRvaGameManager, &gm, 8) && gm && SafeRead(gm + kOffWeaponsManager, &mgr, 8) && mgr;
        uintptr_t player = ok ? ResolveSet(mgr + kOffPlayerSetHandle, kSingleRacePlayerWeaponSet) : 0;
        uintptr_t ai = ok ? ResolveSet(mgr + kOffAiSetHandle, kSingleRaceWeaponSet) : 0;
        // the player's set: own side only (weapons 0-3 racer, 4-7 cop); the other side's charges would count
        // as "had a weapon" and show the HUD's weapons panel with nothing usable in it
        bool copSide = g_mode == kCopPursuit;
        int own[kWeaponCount];
        for (int w = 0; w < kWeaponCount; ++w) own[w] = (w >= 4) == copSide ? charges[w] : 0;
        if (!player && !ai) {
            Log("weapons: Single Race sets not loaded - the event's own weapons apply");
        } else {
            bool w1 = player && WriteCharges(player, own, c.weaponLevel);
            bool w2 = ai && WriteCharges(ai, charges, c.weaponLevel);
            Log("weapons (%s, level %d): racers spike %d emp %d jammer %d turbo %d | cops spike %d emp %d roadblock %d heli %d"
                " | player (%s side) gets its own side only%s%s%s",
                kWeaponsModeNames[c.weaponsMode], c.weaponLevel, charges[0], charges[1], charges[2], charges[3], charges[4],
                charges[5], charges[6], charges[7], copSide ? "cop" : "racer", player ? "" : " - PLAYER SET NOT LOADED",
                ai ? "" : " - AI SET NOT LOADED", (!player || w1) && (!ai || w2) ? "" : " - WRITE FAILED");
        }
    }
    return g_bindings[kWeaponsForCompetitor].original(L);
}

// Hot Pursuit: the player and the AI racers take the front slots (AI first, the player last of them), the cops
// the slots behind them. racers / cops = AI counts.
void BuildHpSlots(int racers, int cops) {
    if (racers < 0) racers = 0;
    if (racers > 7) racers = 7;
    for (int c = 0; c < 8; ++c) g_hpSlot[c] = kSlotsFrontToBack[c];  // the cops (and anything else): in order
    for (int c = 1; c <= racers; ++c) g_hpSlot[c] = kSlotsFrontToBack[c - 1];
    g_hpSlot[0] = kSlotsFrontToBack[racers];
    (void)cops;   // cops c = racers+1 .. racers+cops keep slot kSlotsFrontToBack[c]
}

void BuildSlotTables() {
    // Race: AI 1..7 take the front 7 slots in order, the player (0) the last slot.
    for (int c = 1; c < 8; ++c) g_raceSlot[c] = kSlotsFrontToBack[c - 1];
    g_raceSlot[0] = kSlotsFrontToBack[7];
    // Hot Pursuit: racers 0..3 take the front 4 slots (AI 1..3 first, player last of them), cops 4..7 the back 4.
    for (int c = 1; c < 4; ++c) g_hpSlot[c] = kSlotsFrontToBack[c - 1];
    g_hpSlot[0] = kSlotsFrontToBack[3];
    for (int c = 4; c < 8; ++c) g_hpSlot[c] = kSlotsFrontToBack[c];
}

uint8_t* g_tramp;

bool AllocTrampolines() {
    uintptr_t target = g_base + 0x290000;
    MEMORY_BASIC_INFORMATION mbi;
    for (uintptr_t a = (g_base & ~(uintptr_t)0xFFFF) - 0x10000; a > 0x10000 && target - a < 0x70000000; a -= 0x10000) {
        if (VirtualQuery((void*)a, &mbi, sizeof(mbi)) != sizeof(mbi)) continue;
        if (mbi.State != MEM_FREE) continue;
        void* p = VirtualAlloc((void*)a, 0x1000, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        if (p) { g_tramp = (uint8_t*)p; return true; }
    }
    return false;
}

// Redirects the thunk's jmp to the hook; the previous jump target (the game's function, or another
// plugin's hook) becomes the original we call.
bool Install(Binding& b, int slot) {
    uint8_t* thunk = (uint8_t*)(g_base + b.fnRva);
    int32_t rel;
    memcpy(&rel, thunk + 1, 4);
    b.original = (lua_CFunction)(thunk + 5 + rel);
    Log("hook %s: thunk rva %X -> previous target rva %llX", b.name, (unsigned)b.fnRva,
        (unsigned long long)((uintptr_t)b.original - g_base));
    uint8_t* t = g_tramp + 16 * slot;  // jmp [rip+0]; dq hook
    t[0] = 0xFF;
    t[1] = 0x25;
    memset(t + 2, 0, 4);
    uintptr_t d = (uintptr_t)b.hook;
    memcpy(t + 6, &d, 8);
    int32_t newRel = (int32_t)((intptr_t)t - (intptr_t)(thunk + 5));
    DWORD old;
    if (!VirtualProtect(thunk, 5, PAGE_EXECUTE_READWRITE, &old)) return false;
    memcpy(thunk + 1, &newRel, 4);
    VirtualProtect(thunk, 5, old, &old);
    FlushInstructionCache(GetCurrentProcess(), thunk, 5);
    return true;
}

// Inline hook of the race intro setup: the first 15 bytes (no rip-relative operands) move to a trampoline.
bool InstallIntroSetupHook() {
    if (!CheckBytes(kRvaIntroSetup, kIntroSetupPrologue, sizeof(kIntroSetupPrologue)) ||
        !CheckBytes(kIntroSetupWriteAt, kIntroSetupWrite, sizeof(kIntroSetupWrite)) ||
        !CheckBytes(kIntroSetupWrite2At, kIntroSetupWrite2, sizeof(kIntroSetupWrite2))) {
        Log("race intro setup: game code differs from the analysed build");
        return false;
    }
    uint8_t call[5];
    int32_t rel;
    if (!SafeRead(g_base + kRvaIntroSetupCaller, call, 5) || call[0] != 0xE8) return false;
    memcpy(&rel, call + 1, 4);
    if (kRvaIntroSetupCaller + 5 + (intptr_t)rel != kRvaIntroSetup) return false;
    uint8_t* fn = (uint8_t*)(g_base + kRvaIntroSetup);
    // trampoline: stolen bytes, then jmp [rip+0] -> fn+15   (slots 12-13 of the trampoline page: 32 bytes)
    uint8_t* t = g_tramp + 16 * 12;
    memcpy(t, fn, kIntroSetupStolen);
    t[15] = 0xFF;
    t[16] = 0x25;
    memset(t + 17, 0, 4);
    uintptr_t back = (uintptr_t)(fn + kIntroSetupStolen);
    memcpy(t + 21, &back, 8);
    g_introSetupOriginal = (Fn_IntroSetup)t;
    // stub: jmp [rip+0] -> hook   (slot 15)
    uint8_t* stub = g_tramp + 16 * 15;
    stub[0] = 0xFF;
    stub[1] = 0x25;
    memset(stub + 2, 0, 4);
    uintptr_t h = (uintptr_t)&Hook_IntroSetup;
    memcpy(stub + 6, &h, 8);
    uint8_t patch[kIntroSetupStolen];
    memset(patch, 0x90, sizeof(patch));
    patch[0] = 0xE9;
    int32_t r2 = (int32_t)((intptr_t)stub - (intptr_t)(fn + 5));
    memcpy(patch + 1, &r2, 4);
    DWORD old;
    if (!VirtualProtect(fn, sizeof(patch), PAGE_EXECUTE_READWRITE, &old)) return false;
    memcpy(fn, patch, sizeof(patch));
    VirtualProtect(fn, sizeof(patch), old, &old);
    FlushInstructionCache(GetCurrentProcess(), fn, sizeof(patch));
    Log("race intro setup hooked (rva %X)", (unsigned)kRvaIntroSetup);
    return true;
}

// ====================================================================== main menu label
// The main menu's Wall item shows localised string 1263478 ("WALL"; the menu bundle UI/SCREENS/MAIN_MENU
// also lists GARAGE 1263476 and CAREER 1263477). Once loaded, the language table (LocalisedText resource,
// e.g. english.lang) is a heap block whose file offsets have been turned into absolute pointers:
//   +0x00 u32 1, +0x04 u32 count,
//   +0x08 u64 -> sorted u32 ids[count]   (== table + 0x18)
//   +0x10 u64 -> entry[count]             (== table + 0x18 + 4*count)
//   entry = u64 -> UTF-16 text, u64 length in chars including the terminating 0.
// The table is found by scanning the game's private read/write memory for that self-referencing header,
// from DllMain on (the menu copies the text when it builds its items, so the table has to be patched
// before that). If the label still reads "WALL" (English) its entry is pointed at the ini text
// ([MenuLabel] Text, kept in g_labelText below). The entry is watched and patched again if the game
// resets it or reloads the table.

const uint32_t kWallLabelId = 1263478;
const wchar_t kWallText[] = L"WALL";
wchar_t g_labelText[256];  // zero-padded: a reader that still has the old length stays inside it
uint64_t g_labelLen;       // chars including the terminating 0

struct LabelPatch { uintptr_t table; uintptr_t entry; bool patched; };
LabelPatch g_labels[4];
int g_labelCount;

// Reads [MenuLabel] Text= from the ini. The file is read here directly (not with GetPrivateProfileString)
// so that UTF-8 text works as well as ANSI and UTF-16. "\n" in the value is a line break.
// Returns false when the key is missing (an ini from an older helper).
bool ReadLabelText(wchar_t* out, int cap) {
    out[0] = 0;
    HANDLE f = CreateFileW(g_iniPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (f == INVALID_HANDLE_VALUE) return false;
    DWORD size = GetFileSize(f, nullptr), got = 0;
    if (size == INVALID_FILE_SIZE || size > (1u << 20)) { CloseHandle(f); return false; }
    char* raw = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size + 4);
    wchar_t* w = (wchar_t*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size + 4) * sizeof(wchar_t));
    bool found = false;
    if (raw && w && ReadFile(f, raw, size, &got, nullptr)) {
        const unsigned char* u = (const unsigned char*)raw;
        int wn = 0;
        if (got >= 2 && u[0] == 0xFF && u[1] == 0xFE) {
            wn = (int)(got - 2) / 2;
            memcpy(w, raw + 2, wn * sizeof(wchar_t));
        } else {
            int skip = (got >= 3 && u[0] == 0xEF && u[1] == 0xBB && u[2] == 0xBF) ? 3 : 0;
            wn = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, raw + skip, (int)got - skip, w, (int)size + 2);
            if (wn <= 0) wn = MultiByteToWideChar(CP_ACP, 0, raw + skip, (int)got - skip, w, (int)size + 2);
            if (wn < 0) wn = 0;
        }
        w[wn] = 0;
        bool inSection = false;
        for (wchar_t* line = w; line && *line && !found;) {
            wchar_t* next = wcschr(line, L'\n');
            if (next) *next++ = 0;
            while (*line == L' ' || *line == L'\t') ++line;
            int n = lstrlenW(line);
            while (n && (line[n - 1] == L'\r' || line[n - 1] == L' ' || line[n - 1] == L'\t')) line[--n] = 0;
            if (line[0] == L'[') {
                wchar_t* e = wcschr(line, L']');
                if (e) *e = 0;
                inSection = lstrcmpiW(line + 1, L"MenuLabel") == 0;
            } else if (inSection && line[0] != L';' && line[0] != L'#') {
                wchar_t* eq = wcschr(line, L'=');
                if (eq) {
                    wchar_t* ke = eq;
                    while (ke > line && (ke[-1] == L' ' || ke[-1] == L'\t')) --ke;
                    *ke = 0;
                    if (lstrcmpiW(line, L"Text") == 0) {
                        const wchar_t* v = eq + 1;
                        while (*v == L' ' || *v == L'\t') ++v;
                        int o = 0;
                        for (; *v && o < cap - 1; ++v) {
                            if (v[0] == L'\\' && (v[1] == L'n' || v[1] == L'N')) { out[o++] = L'\n'; ++v; }
                            else out[o++] = *v;
                        }
                        out[o] = 0;
                        found = true;
                    }
                }
            }
            line = next;
        }
    }
    if (raw) HeapFree(GetProcessHeap(), 0, raw);
    if (w) HeapFree(GetProcessHeap(), 0, w);
    CloseHandle(f);
    return found;
}

// For the log: the label text as UTF-8, line breaks shown as '|'.
void LabelTextForLog(const wchar_t* t, char* out, int cap) {
    int n = WideCharToMultiByte(CP_UTF8, 0, t, -1, out, cap, nullptr, nullptr);
    if (n <= 0) out[0] = 0;
    for (char* c = out; *c; ++c) if (*c == '\n') *c = '|';
}

void LoadLabelSettings() {
    if (!ReadLabelText(g_labelText, 200)) {
        // an ini from before this option: append the section (plain ASCII, valid in ANSI and UTF-8 files)
        static const char kSection[] =
            "\r\n[MenuLabel]\r\n"
            "; Text of the main menu item that opens Single Race (the game's \"WALL\"). Read at game start.\r\n"
            "; \\n = line break (e.g. SINGLE\\nRACE). Empty = keep WALL.\r\n"
            "Text=SINGLE RACE\r\n";
        HANDLE f = CreateFileW(g_iniPath, FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        if (f != INVALID_HANDLE_VALUE) {
            unsigned char bom[2] = {};
            DWORD r = 0;
            HANDLE rf = CreateFileW(g_iniPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
            if (rf != INVALID_HANDLE_VALUE) { ReadFile(rf, bom, 2, &r, nullptr); CloseHandle(rf); }
            if (!(r == 2 && bom[0] == 0xFF && bom[1] == 0xFE)) {  // not a UTF-16 ini
                DWORD w;
                WriteFile(f, kSection, sizeof(kSection) - 1, &w, nullptr);
                Log("menu label: [MenuLabel] added to the ini");
            }
            CloseHandle(f);
        }
        lstrcpyW(g_labelText, L"SINGLE RACE");
    }
    g_labelLen = (uint64_t)lstrlenW(g_labelText) + 1;
    char t[600];
    LabelTextForLog(g_labelText, t, sizeof(t));
    if (g_labelLen <= 1) Log("menu label: Text is empty - WALL stays");
    else Log("menu label: \"WALL\" will read \"%s\"", t);
}

bool ValidLabelTable(uintptr_t t, uint32_t* countOut, uintptr_t* idsOut, uintptr_t* entsOut) {
    uint32_t h[2];
    uint64_t p[2];
    if (!SafeRead(t, h, 8) || !SafeRead(t + 8, p, 16)) return false;
    if (h[0] != 1 || h[1] < 16 || h[1] > 1000000) return false;
    if (p[0] != t + 0x18 || p[1] != t + 0x18 + 4ull * h[1]) return false;
    *countOut = h[1];
    *idsOut = (uintptr_t)p[0];
    *entsOut = (uintptr_t)p[1];
    return true;
}

// Points the Wall label of the table at `t` to the ini text. Returns true when the table is a language
// table that holds the label (patched, or left alone because it isn't "WALL").
bool PatchLabelTable(uintptr_t t, bool track) {
    uint32_t count;
    uintptr_t ids, ents;
    if (!ValidLabelTable(t, &count, &ids, &ents)) return false;
    int lo = 0, hi = (int)count - 1, idx = -1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        uint32_t id;
        if (!SafeRead(ids + 4 * (uintptr_t)mid, &id, 4)) return false;
        if (id == kWallLabelId) { idx = mid; break; }
        if (id < kWallLabelId) lo = mid + 1;
        else hi = mid - 1;
    }
    if (idx < 0) return false;
    uintptr_t entry = ents + 16 * (uintptr_t)idx;
    uint64_t e[2];
    if (!SafeRead(entry, e, 16)) return false;
    if (e[0] == (uint64_t)(uintptr_t)g_labelText) {
        if (track && g_labelCount < 4) g_labels[g_labelCount++] = LabelPatch{t, entry, true};
        return true;
    }
    wchar_t text[8] = {};
    if (e[1] != 5 || !SafeRead((uintptr_t)e[0], text, sizeof(kWallText)) || memcmp(text, kWallText, sizeof(kWallText)) != 0) {
        if (track && g_labelCount < 4) g_labels[g_labelCount++] = LabelPatch{t, entry, false};
        Log("menu label: string %u is not \"WALL\" in this language - left as it is", kWallLabelId);
        return true;
    }
    if (track && g_labelCount < 4) g_labels[g_labelCount++] = LabelPatch{t, entry, true};
    // pointer first, then length (g_labelText is zero-padded, so either order of reads stays in it)
    InterlockedExchange64((volatile LONG64*)entry, (LONG64)(uintptr_t)g_labelText);
    InterlockedExchange64((volatile LONG64*)(entry + 8), (LONG64)g_labelLen);
    Log("menu label: patched (language table at %p, %u strings, entry %p)", (void*)t, count, (void*)entry);
    return true;
}

// One pass over the game's private read/write memory, looking for language tables.
int ScanForLanguageTables() {
    static uint8_t buf[(4 << 20) + 0x18];
    const size_t kChunk = 4 << 20;
    int found = 0;
    uintptr_t bufLo = (uintptr_t)buf, bufHi = bufLo + sizeof(buf);
    MEMORY_BASIC_INFORMATION mbi;
    for (uintptr_t a = 0x10000; a < 0x7FFFFFFF0000ull; a = (uintptr_t)mbi.BaseAddress + mbi.RegionSize) {
        if (VirtualQuery((void*)a, &mbi, sizeof(mbi)) != sizeof(mbi)) break;
        uintptr_t r = (uintptr_t)mbi.BaseAddress, end = r + mbi.RegionSize;
        if (mbi.State != MEM_COMMIT || mbi.Type != MEM_PRIVATE || mbi.Protect != PAGE_READWRITE || (r < bufHi && end > bufLo))
            continue;
        for (uintptr_t c = r; c < end; c += kChunk) {
            size_t n = end - c < sizeof(buf) ? (size_t)(end - c) : sizeof(buf);
            SIZE_T got = 0;
            if (!ReadProcessMemory(GetCurrentProcess(), (const void*)c, buf, n, &got) || got < 0x18) continue;
            for (size_t i = 0; i + 0x18 <= got && i < kChunk; i += 8) {
                const uint32_t* h = (const uint32_t*)(buf + i);
                if (h[0] != 1 || h[1] < 16 || h[1] > 1000000) continue;
                uintptr_t t = c + i;
                if (*(const uint64_t*)(buf + i + 8) != t + 0x18 || *(const uint64_t*)(buf + i + 16) != t + 0x18 + 4ull * h[1]) continue;
                bool known = false;
                for (int k = 0; k < g_labelCount; ++k) known |= g_labels[k].table == t;
                if (!known && PatchLabelTable(t, true)) ++found;
            }
        }
    }
    return found;
}

DWORD WINAPI LabelWorker(LPVOID) {
    LoadLabelSettings();
    if (g_labelLen <= 1) return 0;
    ULONGLONG started = GetTickCount64();
    int scans = 0;
    for (;;) {
        for (int k = 0; k < g_labelCount;) {
            LabelPatch l = g_labels[k];
            uint32_t count;
            uintptr_t ids, ents;
            uint64_t p = 0;
            if (!ValidLabelTable(l.table, &count, &ids, &ents)) {
                g_labels[k] = g_labels[--g_labelCount];
                Log("menu label: language table gone (reloaded?) - searching again");
                continue;
            }
            if (l.patched && SafeRead(l.entry, &p, 8) && p != (uint64_t)(uintptr_t)g_labelText) {
                Log("menu label: the game reset the label");
                PatchLabelTable(l.table, false);  // patches again if it reads "WALL" once more
            }
            ++k;
        }
        if (g_labelCount == 0) {
            ULONGLONG t0 = GetTickCount64();
            int n = ScanForLanguageTables();
            ++scans;
            if (n || scans == 1 || scans % 100 == 0)
                Log("menu label: search %d found %d language table(s) in %u ms", scans, n, (unsigned)(GetTickCount64() - t0));
        }
        // nothing found yet: search every 50 ms for the first two minutes, then every 2 s
        Sleep(g_labelCount ? 500 : (GetTickCount64() - started < 120000 ? 50 : 2000));
    }
    return 0;
}

// Inline hook of the weapon set loader (same technique as the intro setup; slots 16-17 trampoline, 19 stub).
bool InstallWeaponSetLoaderHook() {
    if (!CheckBytes(kRvaWeaponSetLoader, kWeaponSetLoaderPrologue, sizeof(kWeaponSetLoaderPrologue)) ||
        !CheckBytes(kWeaponSetLoaderBodyAt, kWeaponSetLoaderBody, sizeof(kWeaponSetLoaderBody))) {
        Log("weapon set loader: game code differs from the analysed build");
        return false;
    }
    uint8_t call[5];
    int32_t rel;
    if (!SafeRead(g_base + kRvaWeaponSetLoaderCaller, call, 5) || call[0] != 0xE8) return false;
    memcpy(&rel, call + 1, 4);
    if (kRvaWeaponSetLoaderCaller + 5 + (intptr_t)rel != kRvaWeaponSetLoader) return false;
    uint8_t* fn = (uint8_t*)(g_base + kRvaWeaponSetLoader);
    uint8_t* t = g_tramp + 16 * 16;
    memcpy(t, fn, kWeaponSetLoaderStolen);
    t[15] = 0xFF;
    t[16] = 0x25;
    memset(t + 17, 0, 4);
    uintptr_t back = (uintptr_t)(fn + kWeaponSetLoaderStolen);
    memcpy(t + 21, &back, 8);
    g_weaponSetLoaderOriginal = (Fn_WeaponSetLoader)t;
    uint8_t* stub = g_tramp + 16 * 19;
    stub[0] = 0xFF;
    stub[1] = 0x25;
    memset(stub + 2, 0, 4);
    uintptr_t h = (uintptr_t)&Hook_WeaponSetLoader;
    memcpy(stub + 6, &h, 8);
    uint8_t patch[kWeaponSetLoaderStolen];
    memset(patch, 0x90, sizeof(patch));
    patch[0] = 0xE9;
    int32_t r2 = (int32_t)((intptr_t)stub - (intptr_t)(fn + 5));
    memcpy(patch + 1, &r2, 4);
    DWORD old;
    if (!VirtualProtect(fn, sizeof(patch), PAGE_EXECUTE_READWRITE, &old)) return false;
    memcpy(fn, patch, sizeof(patch));
    VirtualProtect(fn, sizeof(patch), old, &old);
    FlushInstructionCache(GetCurrentProcess(), fn, sizeof(patch));
    Log("weapon set loader hooked (rva %X)", (unsigned)kRvaWeaponSetLoader);
    return true;
}

// Inline hook of the competitor info VM update (same technique; slots 20-21 trampoline, 23 stub).
bool InstallVmUpdateHook() {
    if (!CheckBytes(kRvaVmUpdate, kVmUpdatePrologue, sizeof(kVmUpdatePrologue)) ||
        !CheckBytes(kVmRankCacheAt, kVmRankCache, sizeof(kVmRankCache)) ||
        !CheckBytes(kVmFullRankAt, kVmFullRank, sizeof(kVmFullRank))) {
        Log("competitor info update: game code differs from the analysed build");
        return false;
    }
    uint8_t call[5];
    int32_t rel;
    if (!SafeRead(g_base + kRvaVmUpdateCaller, call, 5) || call[0] != 0xE8) return false;
    memcpy(&rel, call + 1, 4);
    if (kRvaVmUpdateCaller + 5 + (intptr_t)rel != kRvaVmUpdate) return false;
    uint8_t* fn = (uint8_t*)(g_base + kRvaVmUpdate);
    const size_t stolen = sizeof(kVmUpdatePrologue);
    uint8_t* t = g_tramp + 16 * 20;
    memcpy(t, fn, stolen);
    t[15] = 0xFF;
    t[16] = 0x25;
    memset(t + 17, 0, 4);
    uintptr_t back = (uintptr_t)(fn + stolen);
    memcpy(t + 21, &back, 8);
    g_vmUpdateOriginal = (Fn_VmUpdate)t;
    uint8_t* stub = g_tramp + 16 * 23;
    stub[0] = 0xFF;
    stub[1] = 0x25;
    memset(stub + 2, 0, 4);
    uintptr_t h = (uintptr_t)&Hook_VmUpdate;
    memcpy(stub + 6, &h, 8);
    uint8_t patch[sizeof(kVmUpdatePrologue)];
    memset(patch, 0x90, sizeof(patch));
    patch[0] = 0xE9;
    int32_t r2 = (int32_t)((intptr_t)stub - (intptr_t)(fn + 5));
    memcpy(patch + 1, &r2, 4);
    DWORD old;
    if (!VirtualProtect(fn, sizeof(patch), PAGE_EXECUTE_READWRITE, &old)) return false;
    memcpy(fn, patch, sizeof(patch));
    VirtualProtect(fn, sizeof(patch), old, &old);
    FlushInstructionCache(GetCurrentProcess(), fn, sizeof(patch));
    Log("competitor info update hooked (rva %X)", (unsigned)kRvaVmUpdate);
    return true;
}

// ====================================================================== loading screen (see 8.)
// The pre-race loading layouts (UI/LAYOUTS 1244670 / 1254268) are bound to the ChallengeViewModel
// ("ChallengeReporter.Info", one instance, updated every frame by 0x238310):
//   "{ChallengeReporter.Info.ModeName} - {ChallengeReporter.Info.TierName}"  - TierName: string id at VM+0x7C,
//      set once by SetChallenge (0x2392C0) from the career tier that lists the event - none for our events;
//   ChallengeReporter.Info.GetStartTime(TIME,OFDAY)  - getter 0x23C990: the event record's start hour (float,
//      field da6ac384) truncated to whole hours, in microseconds;
//   ChallengeReporter.Info.WeatherIcon[1..2]  - getter 0x23CCC0(vm, out, index 0/1): the record's 2 icon ids
//      (field 0194dacf; NumWeatherIcons is the schema count, always 2).
// The VM's challenge is the GameDB handle at VM+0x50 (resource) / +0x58 (offset): instance P =
// [res+0x28] + offset, type hash at P+0x18, event id at P+0x1C. For Single Race events the helper answers
// the two getters from the ini conditions and writes the class of the player's selected car into TierName.
// History: v15 crashed at start-up because of a trampoline bug (fixed in InstallInline); v16 avoided code
// patches by rewriting the event records (whole hours only); v17 is v15 with the fix - no data is rewritten.
const uintptr_t kRvaCvmUpdate = 0x238310, kRvaCvmUpdateCaller = 0x369345;
const uint8_t kCvmUpdatePrologue[16] = {0x40, 0x53, 0x48, 0x81, 0xEC, 0x80, 0x00, 0x00,
                                        0x00, 0x48, 0x8B, 0x81, 0x90, 0x07, 0x00, 0x00};
const size_t kCvmUpdateStolen = 9;  // push rbx; sub rsp,80h
// ... call SetChallenge; mov byte [rbx+70h],1
const uint8_t kCvmSetChallengeCall[9] = {0xE8, 0x39, 0x0F, 0x00, 0x00, 0xC6, 0x43, 0x70, 0x01};
const uintptr_t kCvmSetChallengeCallAt = 0x238382;
// in SetChallenge: mov [rdi+7Ch],ebp (TierName = 0) ... mov [rdi+7Ch],ecx (the tier's name)
const uint8_t kCvmTierWrite[3] = {0x89, 0x6F, 0x7C}, kCvmTierWrite2[3] = {0x89, 0x4F, 0x7C};
const uintptr_t kCvmTierWriteAt = 0x239555, kCvmTierWrite2At = 0x239598;
const uintptr_t kRvaCvmStartTime = 0x23C990;
const uint8_t kCvmStartTimePrologue[13] = {0x40, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B, 0x41, 0x50, 0x48, 0x8B, 0xFA};
const size_t kCvmStartTimeStolen = 6;  // push rdi; sub rsp,20h
const uint8_t kCvmStartTimeField[5] = {0xBA, 0x84, 0xC3, 0x6A, 0xDA};  // mov edx,0DA6AC384h
const uintptr_t kCvmStartTimeFieldAt = 0x23C9AB;
const uintptr_t kRvaCvmWeatherIcon = 0x23CCC0;
const uint8_t kCvmWeatherIconPrologue[16] = {0x48, 0x89, 0x74, 0x24, 0x10, 0x48, 0x89, 0x7C,
                                             0x24, 0x18, 0x41, 0x56, 0x48, 0x83, 0xEC, 0x20};
const size_t kCvmWeatherIconStolen = 5;  // mov [rsp+10h],rsi
const uint8_t kCvmWeatherIconField[5] = {0xBA, 0xCF, 0xDA, 0x94, 0x01};  // mov edx,194DACFh
const uintptr_t kCvmWeatherIconFieldAt = 0x23CCEE;
const uintptr_t kRvaNullChallenge = 0x126DEA0;  // the getters' "no challenge" resource
const uint32_t kOffCvmRes = 0x50, kOffCvmResOffset = 0x58, kOffCvmReady = 0x70, kOffCvmTierName = 0x7C;
const uint32_t kHashChallengeInstance = 0x885312E1;
// The selected car (GetSelectedVehicle 0x228F20): M = *(base+1279218) + 31B20,
// id = *(u32*)(M + 1D34 + 4 * *(int*)(*(M+1E90) + 6C04)).
const uintptr_t kRvaSelectedVehicle = 0x228F20;
const uint8_t kSelectedVehicleBytes[22] = {0x48, 0x8B, 0x81, 0x90, 0x1E, 0x00, 0x00, 0x4C, 0x63, 0x80, 0x04,
                                           0x6C, 0x00, 0x00, 0x42, 0x8B, 0x84, 0x81, 0x34, 0x1D, 0x00, 0x00};
const uint32_t kOffSelection = 0x31B20, kOffSelLocal = 0x1E90, kOffSelVehicles = 0x1D34;
// Tier name strings (english.lang): racer SPORTS .. HYPER, cop TRAFFIC POLICE .. SPECIAL RESPONSE (class 1-5).
const uint32_t kRacerTierName[6] = {0, 559305, 559307, 559309, 559311, 559313};
const uint32_t kCopTierName[6] = {0, 559286, 559288, 559297, 559299, 559302};
// Weather icon ids (the layout's Sun / Snow / Rain / Moon / Wet / SunSet / Lightning / Cloud images).
const uint32_t kIconSun = 823036, kIconRain = 823038, kIconMoon = 823039, kIconWet = 829342, kIconSunSet = 829343,
               kIconLightning = 829344;

typedef uint64_t (*Fn_CvmUpdate)(uintptr_t);
typedef bool (*Fn_CvmStartTime)(uintptr_t, int64_t*);
typedef bool (*Fn_CvmWeatherIcon)(uintptr_t, uint32_t*, int);
Fn_CvmUpdate g_cvmUpdateOriginal;
Fn_CvmStartTime g_cvmStartTimeOriginal;
Fn_CvmWeatherIcon g_cvmWeatherIconOriginal;
uint32_t g_loadLoggedEvent, g_loadLoggedVehicle;  // game thread only

// The id of the challenge the VM shows, or 0.
uint32_t CvmEventId(uintptr_t vm) {
    uintptr_t res = 0, nullRes = 0, base = 0;
    uint32_t off = 0, hi[2] = {};
    if (!SafeRead(vm + kOffCvmRes, &res, 8) || !res) return 0;
    if (SafeRead(g_base + kRvaNullChallenge, &nullRes, 8) && res == nullRes) return 0;
    if (!SafeRead(vm + kOffCvmResOffset, &off, 4) || !SafeRead(res + 0x28, &base, 8) || !base) return 0;
    if (!SafeRead(base + off + 0x18, hi, 8) || hi[0] != kHashChallengeInstance) return 0;
    return hi[1];
}

bool SingleRaceChallenge(uintptr_t vm, uint32_t* id) {
    uint32_t e = CvmEventId(vm);
    if (id) *id = e;
    return ModeOf(e) != kNone && !InOnlineEvent();
}

uint32_t SelectedVehicle() {
    uintptr_t g = 0, local = 0;
    int idx = -1;
    uint32_t id = 0;
    if (!SafeRead(g_base + kRvaGameManager, &g, 8) || !g) return 0;
    uintptr_t m = g + kOffSelection;
    if (!SafeRead(m + kOffSelLocal, &local, 8) || !local || !SafeRead(local + kLocalCompetitor, &idx, 4)) return 0;
    if (idx < 0 || idx > 7 || !SafeRead(m + kOffSelVehicles + 4 * (uintptr_t)idx, &id, 4)) return 0;
    return id;
}

uint32_t TimeIcon(int t) { return t == kNight ? kIconMoon : (t == kDawn || t == kSunset) ? kIconSunSet : kIconSun; }
uint32_t WeatherIconId(int w) { return w == kWet ? kIconWet : w == kRain ? kIconRain : w == kStorm ? kIconLightning : 0; }

// After each VM update: a Single Race challenge shows the class of the selected car as its tier.
uint64_t Hook_CvmUpdate(uintptr_t vm) {
    uint64_t r = g_cvmUpdateOriginal(vm);
    uint8_t ready = 0;
    uint32_t ev = 0;
    if (!SafeRead(vm + kOffCvmReady, &ready, 1) || !ready || !SingleRaceChallenge(vm, &ev)) return r;
    uint32_t car = SelectedVehicle();
    const Vehicle* v = FindVehicle(car);
    if (!v || v->tier < 1 || v->tier > 5) return r;
    uint32_t name = v->side == 3 ? kCopTierName[v->tier] : kRacerTierName[v->tier];
    *(volatile uint32_t*)(vm + kOffCvmTierName) = name;
    if (ev != g_loadLoggedEvent || car != g_loadLoggedVehicle) {
        g_loadLoggedEvent = ev;
        g_loadLoggedVehicle = car;
        Conditions c = SR_GetConditions();
        Log("event info: Single Race event %u, car %u %s -> class %s (string %u); time %s, weather %s", ev, car,
            VName(v), ClassName(v), name, kTimeNames[c.time], kWeatherLabels[c.weather]);
    }
    return r;
}

// The picker's details tab binds to these getters too (9.): there they always show the ini conditions.
bool PickerDetails() { return SR_PickerVisible() && !InOnlineEvent(); }

bool Hook_CvmStartTime(uintptr_t vm, int64_t* out) {
    if (!out || !(SingleRaceChallenge(vm, nullptr) || PickerDetails())) return g_cvmStartTimeOriginal(vm, out);
    Conditions c = SR_GetConditions();
    int64_t seconds = (int64_t)(WrapHour(kTimeHours[c.time]) * 3600.0 + 0.5);  // keeps the minutes (15:30, 19:30)
    *out = seconds * 1000000;
    return true;
}

bool Hook_CvmWeatherIcon(uintptr_t vm, uint32_t* out, int index) {
    if (!out || index < 0 || index > 1 || !(SingleRaceChallenge(vm, nullptr) || PickerDetails()))
        return g_cvmWeatherIconOriginal(vm, out, index);
    Conditions c = SR_GetConditions();
    *out = index == 0 ? TimeIcon(c.time) : WeatherIconId(c.weather);  // 0 = no second icon (dry)
    return true;
}

// Inline hook: the first `stolen` bytes (whole instructions, no rip-relative operands) move to a trampoline
// at slots tslot..tslot+1 (stolen bytes + 14-byte jump back <= 29 bytes); the function starts with a jmp to a
// stub at slot sslot that jumps to the hook.
bool InstallInline(const char* what, uintptr_t rva, const uint8_t* prologue, size_t checkLen, size_t stolen, int tslot,
                   int sslot, void* hook, void** original) {
    if (stolen < 5 || stolen > 15 || checkLen < stolen || !CheckBytes(rva, prologue, checkLen)) {
        Log("%s: game code differs from the analysed build", what);
        return false;
    }
    uint8_t* fn = (uint8_t*)(g_base + rva);
    uint8_t* t = g_tramp + 16 * tslot;
    // stolen bytes, then at once jmp [rip+0] -> fn+stolen (v15 left the jump at +15: with fewer than 15 stolen
    // bytes the zero gap ran as "add [rax],al" and crashed the game at start-up)
    memcpy(t, fn, stolen);
    uint8_t* j = t + stolen;
    j[0] = 0xFF;
    j[1] = 0x25;
    memset(j + 2, 0, 4);
    uintptr_t back = (uintptr_t)(fn + stolen);
    memcpy(j + 6, &back, 8);
    *original = t;
    uint8_t* stub = g_tramp + 16 * sslot;
    stub[0] = 0xFF;
    stub[1] = 0x25;
    memset(stub + 2, 0, 4);
    uintptr_t h = (uintptr_t)hook;
    memcpy(stub + 6, &h, 8);
    uint8_t patch[15];
    memset(patch, 0x90, sizeof(patch));
    patch[0] = 0xE9;
    int32_t r2 = (int32_t)((intptr_t)stub - (intptr_t)(fn + 5));
    memcpy(patch + 1, &r2, 4);
    DWORD old;
    if (!VirtualProtect(fn, stolen, PAGE_EXECUTE_READWRITE, &old)) return false;
    memcpy(fn, patch, stolen);
    VirtualProtect(fn, stolen, old, &old);
    FlushInstructionCache(GetCurrentProcess(), fn, stolen);
    Log("%s hooked (rva %X)", what, (unsigned)rva);
    return true;
}

// Trampoline slots 24-25 / 27, 28-29 / 31, 32-33 / 35.
bool InstallLoadingScreenHooks() {
    uint8_t call[5];
    int32_t rel;
    if (!CheckBytes(kCvmSetChallengeCallAt, kCvmSetChallengeCall, sizeof(kCvmSetChallengeCall)) ||
        !CheckBytes(kCvmTierWriteAt, kCvmTierWrite, 3) || !CheckBytes(kCvmTierWrite2At, kCvmTierWrite2, 3) ||
        !CheckBytes(kCvmStartTimeFieldAt, kCvmStartTimeField, 5) ||
        !CheckBytes(kCvmWeatherIconFieldAt, kCvmWeatherIconField, 5) ||
        !CheckBytes(kRvaSelectedVehicle, kSelectedVehicleBytes, sizeof(kSelectedVehicleBytes)) ||
        !SafeRead(g_base + kRvaCvmUpdateCaller, call, 5) || call[0] != 0xE8) {
        Log("event info: game code differs from the analysed build");
        return false;
    }
    memcpy(&rel, call + 1, 4);
    if (kRvaCvmUpdateCaller + 5 + (intptr_t)rel != kRvaCvmUpdate) {
        Log("event info: the challenge view model update is not where expected");
        return false;
    }
    bool ok = InstallInline("challenge view model update", kRvaCvmUpdate, kCvmUpdatePrologue, sizeof(kCvmUpdatePrologue),
                            kCvmUpdateStolen, 24, 27, (void*)&Hook_CvmUpdate, (void**)&g_cvmUpdateOriginal);
    ok = InstallInline("event info GetStartTime", kRvaCvmStartTime, kCvmStartTimePrologue, sizeof(kCvmStartTimePrologue),
                       kCvmStartTimeStolen, 28, 31, (void*)&Hook_CvmStartTime, (void**)&g_cvmStartTimeOriginal) && ok;
    ok = InstallInline("event info WeatherIcon", kRvaCvmWeatherIcon, kCvmWeatherIconPrologue,
                       sizeof(kCvmWeatherIconPrologue), kCvmWeatherIconStolen, 32, 35, (void*)&Hook_CvmWeatherIcon,
                       (void**)&g_cvmWeatherIconOriginal) && ok;
    return ok;
}

// ====================================================================== picker event details (see 9.)
// sr_info.lua (WALL.BNDL) calls ResolveStringBinding("SR.<name>") every frame on the picker and publishes the
// numbers as Autolog.Controls.SRInfo.<name>. Weapons follow the overlay: Race mode has none (0); Hot Pursuit
// shows the Classic Hot Pursuit charges or the Custom ones from the ini. GridSize: the racers incl. the player
// (AI count + 1, per mode).
// "SR.Time" is the details tab's time of day as text ("15:30HRS"); "SR.Weather1" / "SR.Weather2" are its two
// condition icon ids (time icon; wet / rain / lightning, 0 = none) - see sr_info.lua.
// "SR.Summary" (v25) is a string for the info panel's first tab (the event description text, 4 lines):
// mode / DIFFICULTY / OPPONENTS (AI racers, not the player) / COPS (Hot Pursuit only).
// Any other binding string goes to the game's function unchanged.
int PickerSummary(char* out, size_t cap) {
    Conditions c = SR_GetConditions();
    bool hp = c.eventMode == kModeHotPursuit;
    static const char* const kDiffUpper[kDifficultyCount] = {"EASY", "MEDIUM", "HARD"};
    const char* diff = c.difficulty >= 0 && c.difficulty < kDifficultyCount ? kDiffUpper[c.difficulty] : "?";
    if (hp)
        return _snprintf(out, cap, "HOT PURSUIT (RACER)\nDIFFICULTY: %s\nOPPONENTS: %d\nCOPS: %d", diff, c.hpRacers, c.hpCops);
    if (c.eventMode == kModeCopPursuit)
        return _snprintf(out, cap, "HOT PURSUIT (COP)\nDIFFICULTY: %s\nOPPONENTS: %d", diff, c.copRacers);
    return _snprintf(out, cap, "RACE\nDIFFICULTY: %s\nOPPONENTS: %d", diff, c.raceRacers);
}

bool PickerValue(const char* s, size_t len, int* v) {
    Conditions c = SR_GetConditions();
    bool cop = c.eventMode == kModeCopPursuit;
    bool hp = c.eventMode == kModeHotPursuit || cop;   // weapons: either side of a Hot Pursuit
    if (StrIs(s, len, "GridSize")) {
        *v = (c.eventMode == kModeHotPursuit ? c.hpRacers : cop ? c.copRacers : c.raceRacers) + 1;
        return true;
    }
    // time / weather icons for the details tab (published by sr_info.lua; before the first event the
    // ChallengeReporter data source those fields used does not exist yet)
    if (StrIs(s, len, "Weather1")) { *v = (int)TimeIcon(c.time); return true; }
    if (StrIs(s, len, "Weather2")) { *v = (int)WeatherIconId(c.weather); return true; }
    for (int w = 0; w < kWeaponCount; ++w)
        if (StrIs(s, len, kWeaponKeys[w])) {
            *v = !hp ? 0 : c.weaponsMode == kWeaponsClassic ? kClassicCharges[w] : c.charges[w];
            return true;
        }
    return false;
}

int g_pickerLoggedSig = -1;  // game thread only

int Hook_ResolveStringBinding(lua_State* L) {
    const char* s;
    size_t len;
    if (lua_gettop_(L) >= 1 && ArgStr(L, 1, &s, &len) && len > 3 && memcmp(s, "SR.", 3) == 0) {
        // v34: the picker's footer prompt "RACE SETTINGS" (sr_info.lua, input to_main_menu) opens the overlay
        if (StrIs(s + 3, len - 3, "OpenSettings")) {
            if (SR_PickerVisible()) {
                Log("picker: Race Settings prompt pressed");
                SR_RequestMenu();
            }
            lua_settop_(L, 0);
            lua_pushboolean_(L, 1);
            return 1;
        }
        int v = 0;
        bool known = PickerValue(s + 3, len - 3, &v);
        Conditions c = SR_GetConditions();
        int sig = c.eventMode | c.weaponsMode << 2 | c.time << 4 | c.weather << 8 | c.difficulty << 12;
        sig = sig * 11 + c.raceRacers;
        sig = sig * 11 + c.hpRacers;
        sig = sig * 11 + c.hpCops;
        sig = sig * 11 + c.copRacers;
        for (int w = 0; w < kWeaponCount; ++w) sig = sig * 7 + c.charges[w];
        if (sig != g_pickerLoggedSig) {
            g_pickerLoggedSig = sig;
            int r[kWeaponCount];
            for (int w = 0; w < kWeaponCount; ++w) {
                r[w] = 0;
                PickerValue(kWeaponKeys[w], strlen(kWeaponKeys[w]), &r[w]);
            }
            Log("picker details: %s, %s %s, weapons %s - racer %d/%d/%d/%d, cop %d/%d/%d/%d", kEventModeLabels[c.eventMode],
                kTimeNames[c.time], kWeatherLabels[c.weather], kWeaponsModeNames[c.weaponsMode], r[0], r[1], r[2],
                r[3], r[4], r[5], r[6], r[7]);
            char sum[128];
            PickerSummary(sum, sizeof(sum));
            for (char* q = sum; *q; ++q)
                if (*q == '\n') *q = '|';
            Log("picker details: summary \"%s\"", sum);
        }
        if (StrIs(s + 3, len - 3, "Time")) {  // "12:00HRS", as the game's (TIME,OFDAY) format shows it
            int minutes = (int)(WrapHour(kTimeHours[c.time]) * 60.0 + 0.5) % (24 * 60);
            char t[16];
            _snprintf(t, sizeof(t), "%02d:%02dHRS", minutes / 60, minutes % 60);
            t[sizeof(t) - 1] = 0;
            lua_settop_(L, 0);
            lua_pushstring_(L, t);
            return 1;
        }
        if (StrIs(s + 3, len - 3, "Summary")) {
            char sum[128];
            int n = PickerSummary(sum, sizeof(sum));
            if (n < 0 || n >= (int)sizeof(sum)) sum[sizeof(sum) - 1] = 0;
            lua_settop_(L, 0);
            lua_pushstring_(L, sum);
            return 1;
        }
        lua_settop_(L, 0);
        if (!known) return 0;  // nil
        lua_pushnumber_(L, (double)v);
        return 1;
    }
    return g_resolveString.original(L);
}

bool InstallPickerDetailsHook() {
    if (!CheckBinding(g_resolveString)) {
        Log("picker details: ResolveStringBinding not as expected");
        return false;
    }
    return Install(g_resolveString, kResolveStringSlot);
}

DWORD WINAPI Worker(LPVOID) {
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS64 nt;
    if (!SafeRead(g_base, &dos, sizeof(dos)) || !SafeRead(g_base + dos.e_lfanew, &nt, sizeof(nt))) {
        Log("cannot read the exe headers - plugin disabled");
        return 0;
    }
    if (nt.FileHeader.TimeDateStamp != kExpectedTimestamp) {
        Log("unsupported game build (PE timestamp %08X, expected %08X) - plugin disabled",
            (unsigned)nt.FileHeader.TimeDateStamp, kExpectedTimestamp);
        return 0;
    }
    // The protector decrypts the code after start-up: wait until everything checks out.
    int i = 0;
    for (; i < 1200 && !AllChecksPass(); ++i) Sleep(250);
    if (i == 1200) {
        Log("game code not as expected after 5 minutes - plugin disabled");
        return 0;
    }
    lua_settop_ = (Fn_settop)(g_base + kRvaSetTop);
    lua_pushstring_ = (Fn_pushstring)(g_base + kRvaPushString);
    lua_gettop_ = (Fn_gettop)(g_base + kRvaGetTop);
    lua_tonumber_ = (Fn_tonumber)(g_base + kToNumber.rva);
    lua_pushnumber_ = (Fn_pushnumber)(g_base + kPushNumber.rva);
    lua_type_ = (Fn_type)(g_base + kType.rva);
    lua_tolstring_ = (Fn_tolstring)(g_base + kToLString.rva);
    lua_pushboolean_ = (Fn_pushboolean)(g_base + kPushBoolean.rva);
    {
        uint8_t* sel = (uint8_t*)(g_base + kGetSelectedVehicleId.fnRva);
        int32_t rel;
        memcpy(&rel, sel + 1, 4);
        g_origGetSelectedVehicleId = (lua_CFunction)(sel + 5 + rel);
    }
    LoadSettings();
    BuildSlotTables();
    if (!AllocTrampolines()) {
        Log("could not allocate trampoline memory - plugin disabled");
        return 0;
    }
    for (int k = 0; k < kBindingCount; ++k)
        if (!Install(g_bindings[k], k)) {
            Log("could not hook %s", g_bindings[k].name);
            return 0;
        }
    if (!CheckBinding(g_opponentColour) || !Install(g_opponentColour, kOpponentColourSlot))
        Log("GetOpponentColourId not hooked - AI racers keep the event's colours");
    if (!InstallBalancingSetupHook()) Log("race balancing setup not hooked - Single Race events keep their own AI balancing");
    if (!InstallIntroSetupHook()) Log("race intro setup not hooked - Races keep the player on pole");
    if (!InstallWeaponSetLoaderHook()) Log("weapon set loader not hooked - Hot Pursuits keep their own weapons");
    if (!InstallVmUpdateHook()) Log("competitor info update not hooked - intro name tags stay");
    if (!InstallLoadingScreenHooks()) Log("event info not hooked - the loading screen keeps the event's own time, weather and class");
    if (!InstallPickerDetailsHook()) Log("picker details not hooked - the picker's details tab shows no weapon counts");
    if (GetPrivateProfileIntW(L"Overlay", L"Enabled", 1, g_iniPath)) Overlay_Start();
    else Log("overlay: disabled in the ini");
    Log("ready: %d routes mapped, %d events ignored; start positions, opponents, conditions, difficulty, weapons and event info handled for Single Race events; AI count set at launch",
        (int)(sizeof(kRouteMap) / sizeof(kRouteMap[0])), (int)(sizeof(kBlocked) / sizeof(kBlocked[0])));
    return 0;
}

}  // namespace

const char* const kTimeNames[kTimeCount] = {"Dawn", "Morning", "Noon", "Afternoon", "Sunset", "Night"};
const char* const kWeatherNames[kWeatherCount] = {"Dry", "Wet", "Rain", "Storm"};
const char* const kWeatherLabels[kWeatherCount] = {"Dry", "Wet roads", "Rain", "Storm"};
const float kTimeHours[kTimeCount] = {6.0f, 9.0f, 12.0f, 15.5f, 19.5f, 23.0f};  // Event Randomizer's presets
const char* const kWeaponsModeNames[kWeaponsModeCount] = {"Classic", "Custom"};
const char* const kEventModeNames[kEventModeCount] = {"Race", "HotPursuit", "CopHotPursuit"};
const char* const kEventModeLabels[kEventModeCount] = {"Race", "Hot Pursuit (Racer)", "Hot Pursuit (Cop)"};
const char* const kDifficultyNames[kDifficultyCount] = {"Easy", "Medium", "Hard"};
const char* const kWeaponKeys[kWeaponCount] = {"RacerSpikeStrip", "RacerEMP", "Jammer", "Turbo",
                                               "CopSpikeStrip", "CopEMP", "RoadBlock", "Helicopter"};
const char* const kWeaponLabels[kWeaponCount] = {"Spike Strip", "EMP", "Jammer", "Turbo",
                                                 "Spike Strip", "EMP", "Roadblock", "Helicopter"};
// Classic Hot Pursuit: racers have nothing; cops have spike strips, roadblocks and the helicopter
const int kClassicCharges[kWeaponCount] = {0, 0, 0, 0, 3, 0, 3, 2};

Conditions SR_GetConditions() {
    EnterCriticalSection(&g_condLock);
    Conditions c = g_cond;
    LeaveCriticalSection(&g_condLock);
    return c;
}

void SR_SetConditions(const Conditions& c) {
    EnterCriticalSection(&g_condLock);
    g_cond.eventMode = c.eventMode;
    g_cond.difficulty = c.difficulty;
    g_cond.time = c.time;
    g_cond.weather = c.weather;
    g_cond.weaponsMode = c.weaponsMode;
    for (int w = 0; w < kWeaponCount; ++w) g_cond.charges[w] = c.charges[w];
    g_cond.raceRacers = c.raceRacers;
    g_cond.hpRacers = c.hpRacers;
    g_cond.hpCops = c.hpCops;
    g_cond.copRacers = c.copRacers;
    g_cond.timeProgression = c.timeProgression;
    g_cond.weaponLevel = c.weaponLevel;
    LeaveCriticalSection(&g_condLock);
    wchar_t w[32];
    MultiByteToWideChar(CP_ACP, 0, kEventModeNames[c.eventMode], -1, w, 32);
    WritePrivateProfileStringW(L"Event", L"Mode", w, g_iniPath);
    MultiByteToWideChar(CP_ACP, 0, kDifficultyNames[c.difficulty], -1, w, 32);
    WritePrivateProfileStringW(L"Event", L"Difficulty", w, g_iniPath);
    MultiByteToWideChar(CP_ACP, 0, kTimeNames[c.time], -1, w, 32);
    WritePrivateProfileStringW(L"Conditions", L"TimeOfDay", w, g_iniPath);
    MultiByteToWideChar(CP_ACP, 0, kWeatherNames[c.weather], -1, w, 32);
    WritePrivateProfileStringW(L"Conditions", L"Weather", w, g_iniPath);
    WritePrivateProfileStringW(L"Conditions", L"TimeProgression", c.timeProgression ? L"On" : L"Off", g_iniPath);
    MultiByteToWideChar(CP_ACP, 0, kWeaponsModeNames[c.weaponsMode], -1, w, 32);
    WritePrivateProfileStringW(L"Weapons", L"Mode", w, g_iniPath);
    _snwprintf(w, 32, L"%d", c.weaponLevel);
    WritePrivateProfileStringW(L"Weapons", L"Level", w, g_iniPath);
    for (int i = 0; i < kWeaponCount; ++i) {
        wchar_t k[32], v[8];
        MultiByteToWideChar(CP_ACP, 0, kWeaponKeys[i], -1, k, 32);
        _snwprintf(v, 8, L"%d", c.charges[i]);
        v[7] = 0;
        WritePrivateProfileStringW(L"Weapons", k, v, g_iniPath);
    }
    wchar_t v[8];
    _snwprintf(v, 8, L"%d", c.raceRacers);
    WritePrivateProfileStringW(L"Race", L"Racers", v, g_iniPath);
    _snwprintf(v, 8, L"%d", c.hpRacers);
    WritePrivateProfileStringW(L"HotPursuit", L"Racers", v, g_iniPath);
    _snwprintf(v, 8, L"%d", c.hpCops);
    WritePrivateProfileStringW(L"HotPursuit", L"Cops", v, g_iniPath);
    _snwprintf(v, 8, L"%d", c.copRacers);
    WritePrivateProfileStringW(L"CopHotPursuit", L"Racers", v, g_iniPath);
}

bool SR_PickerVisible() {
    LONG l = g_layoutSet;
    return g_offlinePageIsWall && (l == kLayoutLocations || l == kLayoutEvents);
}

void SR_Log(const char* fmt, ...) {
    char buf[900];
    va_list ap;
    va_start(ap, fmt);
    _vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);
    buf[sizeof(buf) - 1] = 0;
    Log("%s", buf);
}

extern "C" BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID) {
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    DisableThreadLibraryCalls(h);
    InitializeCriticalSection(&g_logLock);
    InitializeCriticalSection(&g_condLock);
    InitializeCriticalSection(&g_listLock);
    g_base = (uintptr_t)GetModuleHandleW(nullptr);
    wchar_t path[MAX_PATH];
    DWORD len = GetModuleFileNameW(h, path, MAX_PATH);
    while (len && path[len - 1] != L'\\' && path[len - 1] != L'/') --len;
    path[len] = 0;
    lstrcpyW(g_iniPath, path);
    lstrcatW(g_iniPath, L"NFSHPR_SingleRace.ini");
    lstrcatW(path, L"NFSHPR_SingleRace.log");
    LARGE_INTEGER q;
    QueryPerformanceCounter(&q);
    g_rng = (uint64_t)q.QuadPart ^ ((uint64_t)GetCurrentProcessId() << 32) ^ 0x9E3779B97F4A7C15ULL;
    if (!g_rng) g_rng = 1;
    // ini [Log] Enabled (v37): 0 = no log file. An existing ini from an older helper gets the key; a missing ini
    // is written whole (with the key) by LoadSettings.
    bool logOn = true;
    if (GetFileAttributesW(g_iniPath) != INVALID_FILE_ATTRIBUTES) {
        wchar_t t[16];
        if (!GetPrivateProfileStringW(L"Log", L"Enabled", L"", t, 16, g_iniPath))
            WritePrivateProfileStringW(L"Log", L"Enabled", L"1", g_iniPath);
        logOn = GetPrivateProfileIntW(L"Log", L"Enabled", 1, g_iniPath) != 0;
    }
    // appended, so a test session survives the next game start (restarted when it grows past 1 MB)
    if (logOn) g_log = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (g_log != INVALID_HANDLE_VALUE) {
        if (GetFileSize(g_log, nullptr) > (1u << 20)) SetEndOfFile(g_log);
        SetFilePointer(g_log, 0, nullptr, FILE_END);
    }
    Log("---- NFSHPR Single Race helper 37 (%d cars in the class table)", kVehicleCount);
    HANDLE t = CreateThread(nullptr, 0, Worker, nullptr, 0, nullptr);
    if (t) CloseHandle(t);
    // the label search starts at once: the main menu copies its texts before the game code is unpacked
    // and the hooks are in (it checks its data itself and needs no game code)
    if (HANDLE lt = CreateThread(nullptr, 0, LabelWorker, nullptr, 0, nullptr)) CloseHandle(lt);
    return TRUE;
}
