#include "trainer.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "data.h"
#include "game.h"
#include "imgui.h"
#include "log.h"

namespace trainer
{
namespace
{
struct Item
{
    std::string name;
    int model;
};

struct ItemCategory
{
    std::string name;
    std::vector<Item> items;
};

struct Location
{
    std::string name;
    CVector pos;
};

struct LocationCategory
{
    std::string name;
    std::vector<Location> locations;
};

struct Cheat
{
    const char *function;
    const char *label;
};

// reVC's built-in cheat functions, the same ones the PC cheat codes trigger.
constexpr Cheat kCheats[] = {
    {"WeaponCheat1", "Weapon set 1 (thug tools)"},
    {"WeaponCheat2", "Weapon set 2 (professional tools)"},
    {"WeaponCheat3", "Weapon set 3 (nutter tools)"},
    {"HealthCheat", "Full health & repair car"},
    {"ArmourCheat", "Full armour"},
    {"MoneyCheat", "Add $250,000"},
    {"WantedLevelUpCheat", "Raise wanted level"},
    {"WantedLevelDownCheat", "Clear wanted level"},
    {"FastTimeCheat", "Speed up game"},
    {"SlowTimeCheat", "Slow down game"},
    {"BlowUpCarsCheat", "Blow up all cars"},
    {"ChangePlayerCheat", "Random player skin"},
    {"MayhemCheat", "Peds riot"},
    {"EverybodyAttacksPlayerCheat", "Everyone attacks you"},
    {"WeaponsForAllCheat", "Peds have weapons"},
    {"DoChicksWithGunsCheat", "Armed women"},
    {"PickUpChicksCheat", "Women follow you"},
    {"FannyMagnetCheat", "Ladies' man"},
    {"MadCarsCheat", "Aggressive drivers"},
    {"BlackCarsCheat", "All cars black"},
    {"PinkCarsCheat", "All cars pink"},
    {"TrafficLightsCheat", "Green traffic lights"},
    {"StrongGripCheat", "Better car handling"},
    {"ChittyChittyBangBangCheat", "Flying cars"},
    {"FlyingFishCheat", "Flying boats"},
    {"NoSeaBedCheat", "Cars drive on water"},
    {"OnlyRenderWheelsCheat", "Invisible cars"},
    {"KangarooCheat", "Big jumps"},
    {"SuicideCheat", "Suicide"},
};

constexpr const char *kWeatherNames[] = {"Sunny", "Cloudy", "Rainy", "Foggy", "Extra sunny", "Hurricane"};

std::string initError;
std::vector<ItemCategory> vehicles;
std::vector<Item> weapons;
std::vector<LocationCategory> locations;

// Toggles applied every frame by Update().
bool godMode = false;
bool neverWanted = false;
bool infiniteAmmo = false;
bool infiniteSprint = false;
bool sprintBefore = false; // the game's own flag, restored when the toggle is turned off
bool vehicleGodMode = false;
void *invulnerableVehicle = nullptr;
bool freezeClock = false;
int frozenHour = 12;
int frozenMinute = 0;

bool spawnInside = true;
int weaponAmmo = 500;
int moneyInput = 0;
int wantedLevel = 0;
float customPos[3] = {};

std::string status;
double statusUntil = 0.0;

void Notify(const std::string &message)
{
    Log("%s", message.c_str());
    status = message;
    statusUntil = ImGui::GetTime() + 3.0;
}

void LoadData()
{
    for (const data::Category &cat : data::ParseToml(data::File("vehicles.toml")))
    {
        ItemCategory out{cat.name, {}};
        for (const data::Entry &e : cat.entries)
            out.items.push_back({e.key, atoi(e.value.c_str())});
        std::sort(out.items.begin(), out.items.end(), [](const Item &a, const Item &b) { return a.name < b.name; });
        vehicles.push_back(std::move(out));
    }

    for (const data::Category &cat : data::ParseToml(data::File("weapons.toml")))
        for (const data::Entry &e : cat.entries)
            weapons.push_back({e.value, atoi(e.key.c_str())});

    for (const data::Category &cat : data::ParseToml(data::File("locations.toml")))
    {
        LocationCategory out{cat.name, {}};
        for (const data::Entry &e : cat.entries)
        {
            int interior;
            CVector pos;
            if (sscanf(e.value.c_str(), "%d , %f , %f , %f", &interior, &pos.x, &pos.y, &pos.z) == 4)
                out.locations.push_back({e.key, pos});
        }
        locations.push_back(std::move(out));
    }
}

bool FilterMatches(const ImGuiTextFilter &filter, const std::string &name)
{
    return filter.PassFilter(name.c_str());
}

// Lays out equally sized buttons left to right, wrapping at the window edge.
class ButtonGrid
{
  public:
    ButtonGrid()
        : width_(ImGui::GetFontSize() * 8.5f), right_(ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x)
    {
    }

    bool Button(const char *label)
    {
        if (count_++ > 0 && ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + width_ <= right_)
            ImGui::SameLine();
        return ImGui::Button(label, ImVec2(width_, 0));
    }

  private:
    float width_;
    float right_;
    int count_ = 0;
};

void HelpMarker(const char *text)
{
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void DrawPlayerTab(void *ped)
{
    ImGui::Text("Health %.0f  Armour %.0f  Money $%d", game::PedHealth(ped), game::PedArmour(ped), game::Money());
    ImGui::Separator();

    if (ImGui::Checkbox("God mode", &godMode) && !godMode)
        game::SetPlayerInvulnerable(false);
    HelpMarker("You take no damage and your health stays full.");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x * 0.5f);
    ImGui::Checkbox("Never wanted", &neverWanted);

    ImGui::Checkbox("Infinite ammo", &infiniteAmmo);
    ImGui::SameLine(ImGui::GetContentRegionAvail().x * 0.5f);
    if (ImGui::Checkbox("Infinite sprint", &infiniteSprint))
    {
        if (infiniteSprint)
            sprintBefore = game::InfiniteSprint();
        else
            game::SetInfiniteSprint(sprintBefore);
    }

    ImGui::Spacing();
    if (ImGui::Button("Heal"))
        game::PedHealth(ped) = game::MaxHealth();
    ImGui::SameLine();
    if (ImGui::Button("Full armour"))
        game::PedArmour(ped) = game::MaxArmour();

    ImGui::SeparatorText("Wanted level");
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10.0f);
    ImGui::SliderInt("##wanted", &wantedLevel, 0, 6);
    ImGui::SameLine();
    if (ImGui::Button("Set wanted level"))
        game::SetWantedLevel(wantedLevel);

    ImGui::SeparatorText("Money");
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10.0f);
    ImGui::InputInt("##money", &moneyInput, 1000, 100000);
    ImGui::SameLine();
    if (ImGui::Button("Set"))
        game::Money() = moneyInput;
    ImGui::SameLine();
    if (ImGui::Button("Add"))
        game::Money() += moneyInput;
}

void DrawWeaponsTab()
{
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
    ImGui::InputInt("Ammo", &weaponAmmo, 50, 500);
    weaponAmmo = std::clamp(weaponAmmo, 1, 99999);
    ImGui::SameLine();
    if (ImGui::Button("Remove all weapons"))
        game::RemoveAllWeapons();

    ImGui::SeparatorText("Give weapon");
    ButtonGrid grid;
    for (const Item &weapon : weapons)
    {
        ImGui::PushID(weapon.model);
        if (grid.Button(weapon.name.c_str()))
            Notify(game::GiveWeaponByModel(weapon.model, weaponAmmo) ? "Given " + weapon.name : "Couldn't give " + weapon.name);
        ImGui::PopID();
    }
}

void DrawVehiclesTab()
{
    void *veh = game::PlayerVehicle();
    ImGui::SeparatorText("Current vehicle");
    if (veh)
    {
        ImGui::Text("Health %.0f", game::VehicleHealth(veh));
        if (ImGui::Button("Repair"))
            game::RepairVehicle(veh);
        ImGui::SameLine();
        if (ImGui::Button("Flip upright"))
            game::FlipVehicle(veh);
    }
    else
    {
        ImGui::TextDisabled("You're not in a vehicle.");
    }
    if (ImGui::Checkbox("Vehicle god mode", &vehicleGodMode) && !vehicleGodMode && invulnerableVehicle)
    {
        if (invulnerableVehicle == veh)
            game::SetVehicleInvulnerable(veh, false);
        invulnerableVehicle = nullptr;
    }

    ImGui::SeparatorText("Spawn vehicle");
    ImGui::Checkbox("Spawn inside", &spawnInside);
    HelpMarker("Puts you in the driver's seat when spawning on foot. When you're already in a vehicle, the new one is placed in front of you.");
    static ImGuiTextFilter filter;
    filter.Draw("Search", ImGui::GetFontSize() * 12.0f);

    for (const ItemCategory &cat : vehicles)
    {
        bool any = std::any_of(cat.items.begin(), cat.items.end(), [](const Item &i) { return FilterMatches(filter, i.name); });
        if (!any)
            continue;
        if (filter.IsActive())
            ImGui::SetNextItemOpen(true);
        if (!ImGui::CollapsingHeader(cat.name.c_str()))
            continue;
        ButtonGrid grid;
        for (const Item &item : cat.items)
        {
            if (!FilterMatches(filter, item.name))
                continue;
            ImGui::PushID(item.model);
            if (grid.Button(item.name.c_str()))
                Notify(game::SpawnVehicle(item.model, spawnInside) ? "Spawned " + item.name : "Couldn't spawn " + item.name);
            ImGui::PopID();
        }
    }
}

void DrawTeleportTab()
{
    CVector pos = game::PlayerPosition();
    ImGui::Text("Position  %.1f, %.1f, %.1f", pos.x, pos.y, pos.z);

    if (ImGui::Button("Teleport to map marker"))
    {
        CVector marker;
        if (!game::GetMapMarker(marker))
            Notify("Place a marker on the map first (pause menu > Map).");
        else if (!game::TeleportTo(marker, true))
            Notify("Can't teleport right now.");
    }

    ImGui::SeparatorText("Coordinates");
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 16.0f);
    ImGui::InputFloat3("##coords", customPos, "%.1f");
    ImGui::SameLine();
    if (ImGui::Button("Go"))
        game::TeleportTo({customPos[0], customPos[1], customPos[2]}, false);
    ImGui::SameLine();
    if (ImGui::Button("Use current"))
    {
        customPos[0] = pos.x;
        customPos[1] = pos.y;
        customPos[2] = pos.z;
    }

    ImGui::SeparatorText("Locations");
    static ImGuiTextFilter filter;
    filter.Draw("Search", ImGui::GetFontSize() * 12.0f);
    for (const LocationCategory &cat : locations)
    {
        bool any = std::any_of(cat.locations.begin(), cat.locations.end(),
                               [](const Location &l) { return FilterMatches(filter, l.name); });
        if (!any)
            continue;
        if (filter.IsActive())
            ImGui::SetNextItemOpen(true);
        if (!ImGui::CollapsingHeader(cat.name.c_str()))
            continue;
        for (const Location &loc : cat.locations)
        {
            if (!FilterMatches(filter, loc.name))
                continue;
            if (ImGui::Selectable(loc.name.c_str()))
                game::TeleportTo(loc.pos, false);
        }
    }
}

void DrawWorldTab()
{
    ImGui::SeparatorText("Time");
    int hour = game::ClockHours();
    int minute = game::ClockMinutes();
    ImGui::Text("Current time %02d:%02d", hour, minute);
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
    ImGui::SliderInt("Hour", &frozenHour, 0, 23);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.0f);
    ImGui::SliderInt("Minute", &frozenMinute, 0, 59);
    if (ImGui::Button("Set time"))
        game::SetClock(frozenHour, frozenMinute);
    ImGui::SameLine();
    ImGui::Checkbox("Freeze time", &freezeClock);

    ImGui::SeparatorText("Weather");
    int forced = game::ForcedWeather();
    ImGui::Text("Forced weather: %s", forced >= 0 && forced < (int)IM_ARRAYSIZE(kWeatherNames) ? kWeatherNames[forced] : "none");
    for (int i = 0; i < (int)IM_ARRAYSIZE(kWeatherNames); ++i)
    {
        if (i > 0)
            ImGui::SameLine();
        if (ImGui::Button(kWeatherNames[i]))
            game::ForceWeather(i);
    }
    if (ImGui::Button("Release weather"))
        game::ReleaseWeather();

    ImGui::SeparatorText("Simulation");
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12.0f);
    ImGui::SliderFloat("Game speed", game::TimeScale(), 0.1f, 4.0f, "%.2fx");
    ImGui::SameLine();
    if (ImGui::Button("Reset##speed"))
        *game::TimeScale() = 1.0f;
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12.0f);
    ImGui::SliderFloat("Ped density", game::PedDensity(), 0.0f, 3.0f, "%.2f");
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12.0f);
    ImGui::SliderFloat("Car density", game::CarDensity(), 0.0f, 3.0f, "%.2f");
    ImGui::Checkbox("Show HUD", game::DrawHud());
}

void DrawCheatsTab()
{
    ImGui::TextWrapped("The game's own cheats. Toggle cheats switch off when used again.");
    float width = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
    for (size_t i = 0; i < IM_ARRAYSIZE(kCheats); ++i)
    {
        if (i % 2)
            ImGui::SameLine();
        if (ImGui::Button(kCheats[i].label, ImVec2(width, 0)))
            game::RunCheat(kCheats[i].function);
    }
}

void DrawAboutTab()
{
    ImGui::Text("Hammerill Trainer for GTA Vice City (macOS) v%s", HAMMERILL_TRAINER_VC_MACOS_VERSION);
    ImGui::Spacing();
    ImGui::TextWrapped("A standalone trainer for the macOS reVC build. Open and close it with "
                       "Left Ctrl + T, or X + A on a gamepad (Square + Cross on PlayStation).");
    ImGui::Spacing();
    ImGui::TextWrapped("Based on Grinch Trainer by Grinch_, whose data files and font it uses. "
                       "UI by Dear ImGui (Omar Cornut). Runs on top of reVC by the re3 team.");
}
} // namespace

void Init(const std::string &gameError)
{
    initError = gameError;
    LoadData();
}

void Update()
{
    if (!game::IsInitialised() || game::IsGamePaused())
        return;
    void *ped = game::PlayerPed();
    if (!ped)
        return;

    if (godMode)
    {
        game::SetPlayerInvulnerable(true);
        game::PedHealth(ped) = std::max<float>(game::PedHealth(ped), game::MaxHealth());
    }
    if (neverWanted)
        game::ClearWanted();
    if (infiniteAmmo)
        game::RefillCurrentWeapon();
    if (infiniteSprint)
        game::SetInfiniteSprint(true);
    if (freezeClock)
        game::SetClock(frozenHour, frozenMinute);

    if (vehicleGodMode)
    {
        if (void *veh = game::PlayerVehicle())
        {
            game::SetVehicleInvulnerable(veh, true);
            game::VehicleHealth(veh) = std::max(game::VehicleHealth(veh), 1000.0f);
            invulnerableVehicle = veh;
        }
    }
}

void Draw(bool *open)
{
    const float em = ImGui::GetFontSize();
    ImGui::SetNextWindowPos(ImVec2(40, 40), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(em * 30, em * 34), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Hammerill Trainer###HammerillTrainerVC", open, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }

    void *ped = game::PlayerPed();
    if (!initError.empty())
    {
        ImGui::TextWrapped("The trainer is disabled because this version of the game isn't supported.");
        ImGui::Spacing();
        ImGui::TextDisabled("%s", initError.c_str());
    }
    else if (!ped)
    {
        ImGui::TextWrapped("Start or load a game to use the trainer.");
    }
    else if (ImGui::BeginTabBar("tabs"))
    {
        auto tab = [](const char *name, auto draw) {
            if (ImGui::BeginTabItem(name))
            {
                ImGui::BeginChild("content", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
                draw();
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
        };
        tab("Player", [&] { DrawPlayerTab(ped); });
        tab("Weapons", DrawWeaponsTab);
        tab("Vehicles", DrawVehiclesTab);
        tab("Teleport", DrawTeleportTab);
        tab("World", DrawWorldTab);
        tab("Cheats", DrawCheatsTab);
        tab("About", DrawAboutTab);
        ImGui::EndTabBar();

        if (ImGui::GetTime() < statusUntil)
            ImGui::TextUnformatted(status.c_str());
    }
    ImGui::End();
}
} // namespace trainer
