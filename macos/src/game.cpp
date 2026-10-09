#include "game.h"

#include <dlfcn.h>

#include <cmath>
#include <cstring>

namespace game
{
namespace
{
// Field offsets in the arm64 reVC executable. Each one is re-checked in
// kOffsetChecks below against the instruction it was read from.
namespace off
{
constexpr size_t PlayerInfoSize = 0x1A0;
constexpr size_t PlayerInfoMoney = 0xC8;
constexpr size_t PlayerInfoInfiniteSprint = 0x16C;
constexpr size_t PlayerInfoMaxHealth = 0x16F;
constexpr size_t PlayerInfoMaxArmour = 0x170;

constexpr size_t EntityMatrix = 0x08; // CMatrix: right 0x00, forward 0x10, up 0x20, pos 0x30
constexpr size_t EntityPos = EntityMatrix + 0x30;
constexpr size_t EntityStatus = 0x60;

constexpr size_t PhysicalMoveSpeed = 0x84;
constexpr size_t PhysicalTurnSpeed = 0x90;

constexpr size_t PedHealth = 0x44C;
constexpr size_t PedArmour = 0x450;
constexpr size_t PedWeapons = 0x550;
constexpr size_t WeaponSize = 0x18;
constexpr size_t WeaponType = 0x00;
constexpr size_t WeaponAmmoTotal = 0x0C;
constexpr size_t PedCurrentWeaponSlot = 0x64C;
constexpr size_t PlayerPedWanted = 0x778;
constexpr size_t PlayerPedCanBeDamaged = 0x7D1;

constexpr size_t VehHealth = 0x2A0;
constexpr size_t VehFlags = 0x295;         // 40-bit bitfield, see kOffsetChecks
constexpr uint32_t VehFlagSpawned = 1u << 18; // set by reVC's own SpawnCar()
constexpr uint32_t VehFlagCanBeDamaged = 1u << 22;
constexpr size_t VehDoorLock = 0x2D8;
constexpr size_t VehType = 0x348;
constexpr size_t VehDamageManager = 0x34C;

constexpr size_t AutomobileSize = 0x6F0;
constexpr size_t BikeSize = 0x5E0;
constexpr size_t BoatSize = 0x5A0;

constexpr size_t StreamingInfoSize = 0x20;
constexpr size_t StreamingLoadState = 0x10;

constexpr size_t WantedChaos = 0x00;
} // namespace off

constexpr int VEHICLE_TYPE_CAR = 0;
constexpr int VEHICLE_TYPE_BIKE = 5;
constexpr int STREAMING_LOADED = 1;
constexpr int STATUS_ABANDONED = 4;
constexpr int CARLOCK_UNLOCKED = 1;
constexpr int RANDOM_VEHICLE = 1;
constexpr int WEAPONSLOT_FIRST_RANGED = 2;

struct OffsetCheck
{
    const char *symbol;
    uint32_t offset;
    uint32_t instruction;
};

// Generated with objdump from the macOS reVC build (arm64). Comments name the
// field whose offset the instruction encodes.
constexpr OffsetCheck kOffsetChecks[] = {
    {"_Z11HealthCheatv", 0x40, 0xbd044c08},                                                // CPed::m_fHealth
    {"_Z11HealthCheatv", 0x34, 0x3d45bd00},                                                // CPlayerInfo::m_nMaxHealth
    {"_Z11HealthCheatv", 0x54, 0xb902a008},                                                // CVehicle::m_fHealth
    {"_Z11HealthCheatv", 0x5c, 0xb9434808},                                                // CVehicle::m_vehType
    {"_Z11ArmourCheatv", 0x40, 0xbd045008},                                                // CPed::m_fArmour
    {"_Z11ArmourCheatv", 0x34, 0x3d45c100},                                                // CPlayerInfo::m_nMaxArmour
    {"_Z10MoneyCheatv", 0x1c, 0x5280340a},                                                 // sizeof(CPlayerInfo)
    {"_Z10MoneyCheatv", 0x24, 0xb940c909},                                                 // CPlayerInfo::m_nMoney
    {"_Z17FindPlayerVehiclev", 0x24, 0x39530109},                                          // CPed::bInVehicle
    {"_Z17FindPlayerVehiclev", 0x2c, 0xf9425d00},                                          // CPed::m_pMyVehicle
    {"_Z15FindPlayerCoorsv", 0x5c, 0x9100e128},                                            // CEntity matrix position
    {"_ZN10CPlaceable10SetHeadingEf", 0x14, 0xbd403808},                                   // SetHeading takes the CMatrix
    {"_ZN4CPed16SetCurrentWeaponEi", 0x20, 0x39593008},                                    // CPed::m_currentWeapon
    {"_ZN4CPed16SetCurrentWeaponEi", 0x2c, 0xb9455100},                                    // CPed::m_weapons
    {"_ZN4CPed7SetAmmoE11eWeaponTypej", 0x34, 0x52800309},                                 // sizeof(CWeapon)
    {"_ZN4CPed7SetAmmoE11eWeaponTypej", 0x3c, 0xb9055d13},                                 // CWeapon::m_nAmmoTotal
    {"_Z20WantedLevelDownCheatv", 0x2c, 0xf943bc00},                                       // CPlayerPed::m_pWanted
    {"_ZN7CWanted14SetWantedLevelEi", 0xb0, 0xb9000009},                                   // CWanted::m_nChaos
    {"_ZN4CPed13InflictDamageEP7CEntity11eWeaponTypef14ePedPieceTypesh", 0xe4, 0x395f4668}, // CPlayerPed::m_bCanBeDamaged
    {"_ZN8CVehicle13InflictDamageEP7CEntity11eWeaponTypef7CVector", 0x20, 0x910a5417},     // CVehicle flags at 0x295
    {"_ZN8CVehicle13InflictDamageEP7CEntity11eWeaponTypef7CVector", 0x30, 0x37b00129},     // CVehicle::bCanBeDamaged is bit 22
    {"_ZN10CPlayerPed15UseSprintEnergyEv", 0x0, 0xbd478c00},                               // CPlayerPed::m_fCurrentStamina
    {"_ZN10CPlayerPed15UseSprintEnergyEv", 0x30, 0x3945b108},                              // CPlayerInfo::m_bInfiniteSprint
    {"_Z8SpawnCari", 0x38, 0x8b081528},                                                    // sizeof(CStreamingInfo)
    {"_Z8SpawnCari", 0x3c, 0x39404108},                                                    // CStreamingInfo::m_loadState
    {"_Z8SpawnCari", 0xac, 0x5280b400},                                                    // sizeof(CBoat)
    {"_Z8SpawnCari", 0xf4, 0x5280bc00},                                                    // sizeof(CBike)
    {"_Z8SpawnCari", 0x118, 0x5280de00},                                                   // sizeof(CAutomobile)
    {"_Z8SpawnCari", 0x140, 0x320e0129},                                                   // CVehicle flag set by SpawnCar
    {"_Z8SpawnCari", 0x25c, 0xb902da88},                                                   // CVehicle::m_nDoorLock
    {"_Z8SpawnCari", 0x244, 0x9278e929},                                                   // CEntity::m_status bits
    {"_ZN9CPhysical14ApplyMoveSpeedEv", 0x14, 0xbd408c01},                                 // CPhysical::m_vecMoveSpeed.z
    {"_ZN9CPhysical14ApplyMoveSpeedEv", 0x1c, 0xfc484002},                                 // CPhysical::m_vecMoveSpeed
    {"_ZN9CPhysical14ApplyTurnSpeedEv", 0x30, 0xfd404a62},                                 // CPhysical::m_vecTurnSpeed
};

using TeleportFn = void (*)(void *entity, CVector pos);
using VehicleCtorFn = void (*)(void *self, int model, uint8_t createdBy);

struct Api
{
    // Functions
    void *(*FindPlayerPed)();
    void *(*FindPlayerVehicle)();
    void *(*FindPlayerEntity)();
    CVector (*FindPlayerCoors)();
    float (*FindPlayerHeading)();
    void (*RequestModel)(int model, int flags);
    void (*LoadAllRequestedModels)(bool onlyPriority);
    void (*LoadScene)(const CVector &pos);
    void (*LoadSceneCollision)(const CVector &pos);
    float (*FindGroundZForCoord)(float x, float y);
    float (*FindGroundZFor3DCoord)(float x, float y, float z, bool *found);
    float (*GetDistanceFromCentreOfMassToBaseOfModel)(void *entity);
    bool (*IsBoatModel)(int model);
    bool (*IsBikeModel)(int model);
    int (*WeaponForModel)(int model);
    void (*GiveWeapon)(void *ped, int type, unsigned ammo, bool unused);
    void (*SetCurrentWeapon)(void *ped, int type);
    void (*ClearWeapons)(void *ped);
    void (*WarpPedIntoCar)(void *ped, void *veh);
    void *(*VehicleNew)(size_t size);
    VehicleCtorFn AutomobileCtor;
    VehicleCtorFn BikeCtor;
    VehicleCtorFn BoatCtor;
    void (*SetHeading)(void *matrix, float heading);
    void (*WorldAdd)(void *entity);
    void (*AutomobileFix)(void *veh);
    void (*BikeFix)(void *veh);
    void (*SetEngineStatus)(void *damageManager, unsigned status);
    void (*CheatWantedLevel)(void *wanted, int level);
    void (*WantedSetLevel)(void *wanted, int level);
    void (*ClearQdCrimes)(void *wanted);
    void (*SetGameClock)(uint8_t h, uint8_t m);
    void (*ForceWeatherNow)(short type);
    void (*ReleaseWeather)();
    TeleportFn TeleportFns[4];

    // Variables
    uint8_t *Players;
    uint8_t *PlayerInFocus;
    uint8_t *StreamingInfo;
    int *TargetMarkerId;
    CVector *TargetMarkerPos;
    uint8_t *ClockHours;
    uint8_t *ClockMinutes;
    short *ForcedWeatherType;
    float *TimeScale;
    float *TimeStep;
    bool *UserPause;
    bool *CodePause;
    float *PedDensity;
    float *CarDensity;
    bool *DrawHud;
};

Api api;
bool initialised = false;

template <typename T>
void Resolve(T &out, const char *symbol, std::string &missing)
{
    void *p = dlsym(RTLD_MAIN_ONLY, symbol);
    if (!p)
        missing += std::string("  missing symbol ") + symbol + "\n";
    out = reinterpret_cast<T>(p);
}

template <typename T>
T &Field(void *base, size_t offset)
{
    return *reinterpret_cast<T *>(static_cast<uint8_t *>(base) + offset);
}

uint8_t *PlayerInfo()
{
    return api.Players + *api.PlayerInFocus * off::PlayerInfoSize;
}

void *Wanted(void *ped)
{
    return Field<void *>(ped, off::PlayerPedWanted);
}

bool LoadModel(int model, int flags)
{
    api.RequestModel(model, flags);
    api.LoadAllRequestedModels(false);
    return api.StreamingInfo[model * off::StreamingInfoSize + off::StreamingLoadState] == STREAMING_LOADED;
}

void SetVehicleFlag(void *veh, uint32_t flag, bool on)
{
    // The bitfield starts at an odd address, so go through memcpy.
    uint32_t bits;
    memcpy(&bits, static_cast<uint8_t *>(veh) + off::VehFlags, sizeof(bits));
    bits = on ? (bits | flag) : (bits & ~flag);
    memcpy(static_cast<uint8_t *>(veh) + off::VehFlags, &bits, sizeof(bits));
}
} // namespace

bool Init(std::string &error)
{
    std::string missing;
    Resolve(api.FindPlayerPed, "_Z13FindPlayerPedv", missing);
    Resolve(api.FindPlayerVehicle, "_Z17FindPlayerVehiclev", missing);
    Resolve(api.FindPlayerEntity, "_Z16FindPlayerEntityv", missing);
    Resolve(api.FindPlayerCoors, "_Z15FindPlayerCoorsv", missing);
    Resolve(api.FindPlayerHeading, "_Z17FindPlayerHeadingv", missing);
    Resolve(api.RequestModel, "_ZN10CStreaming12RequestModelEii", missing);
    Resolve(api.LoadAllRequestedModels, "_ZN10CStreaming22LoadAllRequestedModelsEb", missing);
    Resolve(api.LoadScene, "_ZN10CStreaming9LoadSceneERK7CVector", missing);
    Resolve(api.LoadSceneCollision, "_ZN10CStreaming18LoadSceneCollisionERK7CVector", missing);
    Resolve(api.FindGroundZForCoord, "_ZN6CWorld19FindGroundZForCoordEff", missing);
    Resolve(api.FindGroundZFor3DCoord, "_ZN6CWorld21FindGroundZFor3DCoordEfffPb", missing);
    Resolve(api.GetDistanceFromCentreOfMassToBaseOfModel, "_ZN7CEntity40GetDistanceFromCentreOfMassToBaseOfModelEv", missing);
    Resolve(api.IsBoatModel, "_ZN10CModelInfo11IsBoatModelEi", missing);
    Resolve(api.IsBikeModel, "_ZN10CModelInfo11IsBikeModelEi", missing);
    Resolve(api.WeaponForModel, "_ZN8CPickups14WeaponForModelEi", missing);
    Resolve(api.GiveWeapon, "_ZN4CPed10GiveWeaponE11eWeaponTypejb", missing);
    Resolve(api.SetCurrentWeapon, "_ZN4CPed16SetCurrentWeaponE11eWeaponType", missing);
    Resolve(api.ClearWeapons, "_ZN4CPed12ClearWeaponsEv", missing);
    Resolve(api.WarpPedIntoCar, "_ZN4CPed14WarpPedIntoCarEP8CVehicle", missing);
    Resolve(api.VehicleNew, "_ZN8CVehiclenwEm", missing);
    Resolve(api.AutomobileCtor, "_ZN11CAutomobileC1Eih", missing);
    Resolve(api.BikeCtor, "_ZN5CBikeC1Eih", missing);
    Resolve(api.BoatCtor, "_ZN5CBoatC1Eih", missing);
    Resolve(api.SetHeading, "_ZN10CPlaceable10SetHeadingEf", missing);
    Resolve(api.WorldAdd, "_ZN6CWorld3AddEP7CEntity", missing);
    Resolve(api.AutomobileFix, "_ZN11CAutomobile3FixEv", missing);
    Resolve(api.BikeFix, "_ZN5CBike3FixEv", missing);
    Resolve(api.SetEngineStatus, "_ZN14CDamageManager15SetEngineStatusEj", missing);
    Resolve(api.CheatWantedLevel, "_ZN7CWanted16CheatWantedLevelEi", missing);
    Resolve(api.WantedSetLevel, "_ZN7CWanted14SetWantedLevelEi", missing);
    Resolve(api.ClearQdCrimes, "_ZN7CWanted13ClearQdCrimesEv", missing);
    Resolve(api.SetGameClock, "_ZN6CClock12SetGameClockEhh", missing);
    Resolve(api.ForceWeatherNow, "_ZN8CWeather15ForceWeatherNowEs", missing);
    Resolve(api.ReleaseWeather, "_ZN8CWeather14ReleaseWeatherEv", missing);
    Resolve(api.TeleportFns[0], "_ZN4CPed8TeleportE7CVector", missing);
    Resolve(api.TeleportFns[1], "_ZN11CAutomobile8TeleportE7CVector", missing);
    Resolve(api.TeleportFns[2], "_ZN5CBike8TeleportE7CVector", missing);
    Resolve(api.TeleportFns[3], "_ZN5CBoat8TeleportE7CVector", missing);

    Resolve(api.Players, "_ZN6CWorld7PlayersE", missing);
    Resolve(api.PlayerInFocus, "_ZN6CWorld13PlayerInFocusE", missing);
    Resolve(api.StreamingInfo, "_ZN10CStreaming16ms_aInfoForModelE", missing);
    Resolve(api.TargetMarkerId, "_ZN6CRadar14TargetMarkerIdE", missing);
    Resolve(api.TargetMarkerPos, "_ZN6CRadar15TargetMarkerPosE", missing);
    Resolve(api.ClockHours, "_ZN6CClock18ms_nGameClockHoursE", missing);
    Resolve(api.ClockMinutes, "_ZN6CClock20ms_nGameClockMinutesE", missing);
    Resolve(api.ForcedWeatherType, "_ZN8CWeather17ForcedWeatherTypeE", missing);
    Resolve(api.TimeScale, "_ZN6CTimer13ms_fTimeScaleE", missing);
    Resolve(api.TimeStep, "_ZN6CTimer12ms_fTimeStepE", missing);
    Resolve(api.UserPause, "_ZN6CTimer11m_UserPauseE", missing);
    Resolve(api.CodePause, "_ZN6CTimer11m_CodePauseE", missing);
    Resolve(api.PedDensity, "_ZN11CPopulation20PedDensityMultiplierE", missing);
    Resolve(api.CarDensity, "_ZN8CCarCtrl20CarDensityMultiplierE", missing);
    Resolve(api.DrawHud, "_ZN4CHud19m_Wants_To_Draw_HudE", missing);

    for (const OffsetCheck &check : kOffsetChecks)
    {
        auto *code = static_cast<const uint8_t *>(dlsym(RTLD_MAIN_ONLY, check.symbol));
        if (!code)
        {
            missing += std::string("  missing symbol ") + check.symbol + "\n";
            continue;
        }
        uint32_t instruction;
        memcpy(&instruction, code + check.offset, sizeof(instruction));
        if (instruction != check.instruction)
        {
            char line[256];
            snprintf(line, sizeof(line), "  %s+0x%x: expected %08x, found %08x\n", check.symbol, check.offset,
                     check.instruction, instruction);
            missing += line;
        }
    }

    if (!missing.empty())
    {
        error = "This reVC build doesn't match the one the trainer was made for:\n" + missing;
        return false;
    }
    initialised = true;
    return true;
}

bool IsInitialised()
{
    return initialised;
}

void *PlayerPed()
{
    return initialised ? api.FindPlayerPed() : nullptr;
}

void *PlayerVehicle()
{
    return PlayerPed() ? api.FindPlayerVehicle() : nullptr;
}

bool IsGamePaused()
{
    return *api.UserPause || *api.CodePause;
}

float TimeStep()
{
    return *api.TimeStep;
}

float &PedHealth(void *ped)
{
    return Field<float>(ped, off::PedHealth);
}

float &PedArmour(void *ped)
{
    return Field<float>(ped, off::PedArmour);
}

uint8_t MaxHealth()
{
    return PlayerInfo()[off::PlayerInfoMaxHealth];
}

uint8_t MaxArmour()
{
    return PlayerInfo()[off::PlayerInfoMaxArmour];
}

int32_t &Money()
{
    return Field<int32_t>(PlayerInfo(), off::PlayerInfoMoney);
}

void SetPlayerInvulnerable(bool on)
{
    if (void *ped = PlayerPed())
        Field<bool>(ped, off::PlayerPedCanBeDamaged) = !on;
}

bool InfiniteSprint()
{
    return PlayerInfo()[off::PlayerInfoInfiniteSprint] != 0;
}

void SetInfiniteSprint(bool on)
{
    PlayerInfo()[off::PlayerInfoInfiniteSprint] = on;
}

int WantedChaos()
{
    void *ped = PlayerPed();
    return ped ? Field<int32_t>(Wanted(ped), off::WantedChaos) : 0;
}

void SetWantedLevel(int level)
{
    if (void *ped = PlayerPed())
        api.CheatWantedLevel(Wanted(ped), level);
}

void ClearWanted()
{
    void *ped = PlayerPed();
    if (!ped || WantedChaos() == 0)
        return;
    api.WantedSetLevel(Wanted(ped), 0);
    api.ClearQdCrimes(Wanted(ped));
}

bool GiveWeaponByModel(int modelId, int ammo)
{
    void *ped = PlayerPed();
    if (!ped)
        return false;
    int type = api.WeaponForModel(modelId);
    if (type <= 0 || !LoadModel(modelId, 1))
        return false;
    api.GiveWeapon(ped, type, ammo, true);
    api.SetCurrentWeapon(ped, type);
    return true;
}

void RefillCurrentWeapon()
{
    void *ped = PlayerPed();
    if (!ped)
        return;
    uint8_t slot = Field<uint8_t>(ped, off::PedCurrentWeaponSlot);
    if (slot < WEAPONSLOT_FIRST_RANGED)
        return;
    uint8_t *weapon = static_cast<uint8_t *>(ped) + off::PedWeapons + slot * off::WeaponSize;
    if (Field<int32_t>(weapon, off::WeaponType) == 0)
        return;
    int32_t &ammo = Field<int32_t>(weapon, off::WeaponAmmoTotal);
    if (ammo < 9000)
        ammo = 9999;
}

void RemoveAllWeapons()
{
    if (void *ped = PlayerPed())
        api.ClearWeapons(ped);
}

bool SpawnVehicle(int modelId, bool warpInto)
{
    void *ped = PlayerPed();
    if (!ped || !LoadModel(modelId, 0))
        return false;

    bool isBoat = api.IsBoatModel(modelId);
    bool isBike = api.IsBikeModel(modelId);
    bool inVehicle = api.FindPlayerVehicle() != nullptr;
    // Only warp in from on foot; otherwise park it in front of the player.
    bool warp = warpInto && !inVehicle;

    CVector pos = api.FindPlayerCoors();
    float heading = api.FindPlayerHeading();
    float distance = warp ? 0.0f : (isBoat ? 10.0f : 5.0f);
    pos.x += -sinf(heading) * distance;
    pos.y += cosf(heading) * distance;

    size_t size = isBoat ? off::BoatSize : isBike ? off::BikeSize : off::AutomobileSize;
    VehicleCtorFn ctor = isBoat ? api.BoatCtor : isBike ? api.BikeCtor : api.AutomobileCtor;
    void *veh = api.VehicleNew(size);
    if (!veh) // vehicle pool is full
        return false;
    ctor(veh, modelId, RANDOM_VEHICLE);

    if (!isBoat)
    {
        bool found = false;
        float ground = api.FindGroundZFor3DCoord(pos.x, pos.y, pos.z + 2.0f, &found);
        if (found)
            pos.z = ground;
    }
    api.SetHeading(static_cast<uint8_t *>(veh) + off::EntityMatrix, heading);
    Field<CVector>(veh, off::EntityPos) = pos;
    Field<CVector>(veh, off::EntityPos).z += api.GetDistanceFromCentreOfMassToBaseOfModel(veh);

    // Same setup reVC's debug-menu SpawnCar() does.
    uint8_t &status = Field<uint8_t>(veh, off::EntityStatus);
    status = (status & 0x07) | (STATUS_ABANDONED << 3);
    SetVehicleFlag(veh, off::VehFlagSpawned, true);
    Field<int32_t>(veh, off::VehDoorLock) = CARLOCK_UNLOCKED;
    api.WorldAdd(veh);

    if (warp)
        api.WarpPedIntoCar(ped, veh);
    return true;
}

void RepairVehicle(void *veh)
{
    VehicleHealth(veh) = 1000.0f;
    switch (Field<int32_t>(veh, off::VehType))
    {
    case VEHICLE_TYPE_CAR:
        api.SetEngineStatus(static_cast<uint8_t *>(veh) + off::VehDamageManager, 0);
        api.AutomobileFix(veh);
        break;
    case VEHICLE_TYPE_BIKE:
        api.BikeFix(veh);
        break;
    }
}

void FlipVehicle(void *veh)
{
    // Put it back on its wheels, keeping the direction it was facing.
    auto *matrix = static_cast<uint8_t *>(veh) + off::EntityMatrix;
    const CVector &forward = Field<CVector>(matrix, 0x10);
    float heading = atan2f(-forward.x, forward.y);
    api.SetHeading(matrix, heading);
    Field<CVector>(veh, off::EntityPos).z += 1.0f;
}

float &VehicleHealth(void *veh)
{
    return Field<float>(veh, off::VehHealth);
}

void SetVehicleInvulnerable(void *veh, bool on)
{
    SetVehicleFlag(veh, off::VehFlagCanBeDamaged, !on);
}

CVector Forward(void *entity)
{
    return Field<CVector>(entity, off::EntityMatrix + 0x10);
}

CVector &MoveSpeed(void *entity)
{
    return Field<CVector>(entity, off::PhysicalMoveSpeed);
}

CVector &TurnSpeed(void *entity)
{
    return Field<CVector>(entity, off::PhysicalTurnSpeed);
}

CVector PlayerPosition()
{
    return PlayerPed() ? api.FindPlayerCoors() : CVector{};
}

bool TeleportTo(CVector pos, bool findGround)
{
    void *entity = PlayerPed() ? api.FindPlayerEntity() : nullptr;
    if (!entity)
        return false;

    // Teleport() is virtual (vtable slot 12). Only call it if the slot holds
    // one of the implementations we know, so an unexpected entity type (or a
    // different vtable layout) can't send us somewhere random.
    TeleportFn teleport = (*reinterpret_cast<TeleportFn **>(entity))[12];
    bool known = false;
    for (TeleportFn fn : api.TeleportFns)
        known |= fn == teleport;
    if (!known)
        return false;

    api.LoadScene(pos);
    api.LoadSceneCollision(pos);
    if (findGround)
        pos.z = api.FindGroundZForCoord(pos.x, pos.y) + api.GetDistanceFromCentreOfMassToBaseOfModel(entity);
    teleport(entity, pos);
    return true;
}

bool GetMapMarker(CVector &pos)
{
    if (*api.TargetMarkerId == -1)
        return false;
    pos = *api.TargetMarkerPos;
    return true;
}

uint8_t ClockHours()
{
    return *api.ClockHours;
}

uint8_t ClockMinutes()
{
    return *api.ClockMinutes;
}

void SetClock(int hours, int minutes)
{
    api.SetGameClock(static_cast<uint8_t>(hours), static_cast<uint8_t>(minutes));
}

int ForcedWeather()
{
    return *api.ForcedWeatherType;
}

void ForceWeather(int type)
{
    api.ForceWeatherNow(static_cast<short>(type));
}

void ReleaseWeather()
{
    api.ReleaseWeather();
}

float *TimeScale()
{
    return api.TimeScale;
}

float *PedDensity()
{
    return api.PedDensity;
}

float *CarDensity()
{
    return api.CarDensity;
}

bool *DrawHud()
{
    return api.DrawHud;
}

bool RunCheat(const char *name)
{
    if (!PlayerPed())
        return false;
    // Cheats are plain `void Name()` functions: _Z<len><name>v
    std::string symbol = "_Z" + std::to_string(strlen(name)) + name + "v";
    auto fn = reinterpret_cast<void (*)()>(dlsym(RTLD_MAIN_ONLY, symbol.c_str()));
    if (!fn)
        return false;
    fn();
    return true;
}
} // namespace game
