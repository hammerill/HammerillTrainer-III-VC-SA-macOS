#pragma once

#include <string>

#define HAMMERILL_TRAINER_VC_MACOS_VERSION "1.0.0"

namespace trainer
{
// Loads the embedded data lists. `gameError` is empty if game::Init succeeded.
void Init(const std::string &gameError);

// Called every frame, menu open or not, to keep toggled cheats applied.
void Update();

// Draws the menu. Sets *open to false when the user closes the window.
void Draw(bool *open);
} // namespace trainer
