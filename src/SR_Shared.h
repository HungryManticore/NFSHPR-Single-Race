// Shared between the Single Race helper core (NFSHPR_SingleRace.cpp) and its conditions overlay (SR_Overlay.cpp).
#pragma once
#include <windows.h>
#include <stdint.h>

// Time of day presets (start hours as in NFSHPR Event Randomizer's preset buttons) and weather.
enum TimeOfDay { kDawn, kMorning, kNoon, kAfternoon, kSunset, kNight, kTimeCount };
enum Weather { kDry, kWet, kRain, kStorm, kWeatherCount };
// Event mode chosen in the overlay: the picker lists each route once (its Race), and the helper launches
// the route's Race, racer-side Hot Pursuit or (v29) cop-side Hot Pursuit.
enum EventMode { kModeRace, kModeHotPursuit, kModeCopPursuit, kEventModeCount };
// Weapons for Single Race Hot Pursuit events.
enum WeaponsMode { kWeaponsClassic, kWeaponsCustom, kWeaponsModeCount };
// AI difficulty for Single Race events: balancing and toughness sets taken from career events.
enum Difficulty { kEasy, kMedium, kHard, kDifficultyCount };
// The 8 weapons, in menu order: racers first, then cops.
enum Weapon { kRacerSpikeStrip, kRacerEMP, kJammer, kTurbo, kCopSpikeStrip, kCopEMP, kRoadBlock, kHelicopter, kWeaponCount };
const int kMaxCharges = 5;

struct Conditions {
    int eventMode; // EventMode
    int time;      // TimeOfDay
    int weather;   // Weather
    int weaponsMode;              // WeaponsMode
    int charges[kWeaponCount];    // Custom: 0..kMaxCharges per weapon
    int menuKey;   // virtual-key code that opens the overlay (ini [Overlay] Key)
    int fontSize;  // 0 = automatic
    int difficulty;  // Difficulty
    // AI count (v24), kept separately per mode (ini [Race] Racers, [HotPursuit] Racers / Cops)
    int raceRacers;  // Race: AI racers, kRaceRacersMin..kRaceRacersMax
    int hpRacers;    // Hot Pursuit: AI racers, kHpMin..kHpRacersMax
    int hpCops;      // Hot Pursuit: cops, kHpMin..kHpCopsMax; hpRacers + hpCops <= kMaxAI
    int copRacers;   // Cop Hot Pursuit (v29, ini [CopHotPursuit] Racers): AI racers, kCopRacersMin..kCopRacersMax
};
// AI count limits: 8 cars in an event (the player + 7). The Hot Pursuit events hold 6 racers and 6 cops.
const int kMaxAI = 7, kRaceRacersMin = 1, kRaceRacersMax = 7, kHpMin = 1, kHpRacersMax = 6, kHpCopsMax = 6;
const int kDefaultRaceRacers = 7, kDefaultHpRacers = 3, kDefaultHpCops = 4;
// Cop Hot Pursuit: the player is the only cop; the events hold 7 AI racers.
const int kCopRacersMin = 1, kCopRacersMax = 7, kDefaultCopRacers = 7;

extern const char* const kEventModeNames[kEventModeCount];   // "Race", "HotPursuit", "CopHotPursuit" (ini values)
extern const char* const kEventModeLabels[kEventModeCount];  // "Race", "Hot Pursuit (Racer)", "Hot Pursuit (Cop)" (menu text)
extern const char* const kTimeNames[kTimeCount];      // "Dawn", ... (ini values)
extern const char* const kWeatherNames[kWeatherCount];  // "Dry", "Wet", "Rain", "Storm" (ini values)
extern const char* const kWeatherLabels[kWeatherCount]; // "Dry", "Wet roads", "Rain", "Storm" (menu text)
extern const float kTimeHours[kTimeCount];
extern const char* const kWeaponsModeNames[kWeaponsModeCount];  // "Classic", "Custom" (ini values)
extern const char* const kWeaponKeys[kWeaponCount];    // ini keys
extern const char* const kWeaponLabels[kWeaponCount];  // menu text
extern const int kClassicCharges[kWeaponCount];        // Classic Hot Pursuit
extern const char* const kDifficultyNames[kDifficultyCount];  // "Easy", "Medium", "Hard" (ini values and menu text)

Conditions SR_GetConditions();
void SR_SetConditions(const Conditions& c);  // also writes the ini
bool SR_PickerVisible();                      // the Single Race event picker is on screen
void SR_RequestMenu();  // sr_info.lua asked for Race Settings (picker footer prompt, v34); opened on the next frame
void SR_Log(const char* fmt, ...);

void Overlay_Start();  // called once the game code is verified and the game hooks are in
