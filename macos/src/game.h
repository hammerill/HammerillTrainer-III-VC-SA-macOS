// Access to the reVC (GTA Vice City for macOS, arm64) game executable.
//
// Every function is resolved by its exported symbol name with dlsym(), so no
// addresses are hard-coded. Struct field offsets cannot be resolved that way;
// they were taken from the game's own code and Init() re-checks each one
// against the instruction that uses it, refusing to run on a different build.
#pragma once

#include <cstdint>
#include <string>

struct CVector
{
    float x, y, z;
};

namespace game
{
// Resolves symbols and validates offsets. Returns false (and a reason) if the
// running executable isn't the build this plugin was made for.
bool Init(std::string &error);
bool IsInitialised();

// World state. All of these return nullptr while no game is loaded.
void *PlayerPed();
void *PlayerVehicle();
bool IsGamePaused();

// Player
float &PedHealth(void *ped);
float &PedArmour(void *ped);
uint8_t MaxHealth();
uint8_t MaxArmour();
int32_t &Money();
void SetPlayerInvulnerable(bool on);
bool InfiniteSprint();
void SetInfiniteSprint(bool on);
int WantedChaos();
void SetWantedLevel(int level);
void ClearWanted();

// Weapons
bool GiveWeaponByModel(int modelId, int ammo);
void RefillCurrentWeapon();
void RemoveAllWeapons();

// Vehicles
bool SpawnVehicle(int modelId, bool warpInto);
void RepairVehicle(void *veh);
void FlipVehicle(void *veh);
float &VehicleHealth(void *veh);
void SetVehicleInvulnerable(void *veh, bool on);

// Teleport
CVector PlayerPosition();
bool TeleportTo(CVector pos, bool findGround);
bool GetMapMarker(CVector &pos);

// World
uint8_t ClockHours();
uint8_t ClockMinutes();
void SetClock(int hours, int minutes);
int ForcedWeather();
void ForceWeather(int type);
void ReleaseWeather();
float *TimeScale();
float *PedDensity();
float *CarDensity();
bool *DrawHud();

// Calls one of the game's own cheat functions, e.g. "HealthCheat".
bool RunCheat(const char *name);
} // namespace game
