/* Copyright (c) 2025 Otto Link. Distributed under the terms of the GNU General Public
   License. The full license is in the file LICENSE, distributed with this software. */
#pragma once
#include <glm/vec3.hpp>
#include <map>
#include <string>

namespace qtr
{

// Water presets: shallow / deep colors
static std::map<std::string, std::pair<glm::vec3, glm::vec3>> water_colors = {
    // Tropical / Clear
    {"caribbean", {glm::vec3(0.25f, 0.85f, 0.80f), glm::vec3(0.00f, 0.15f, 0.35f)}},
    {"mediterranean", {glm::vec3(0.20f, 0.65f, 0.75f), glm::vec3(0.05f, 0.20f, 0.40f)}},
    {"coral_reef", {glm::vec3(0.30f, 0.95f, 0.90f), glm::vec3(0.00f, 0.25f, 0.50f)}},

    // Cold Waters
    {"arctic_sea", {glm::vec3(0.35f, 0.70f, 0.85f), glm::vec3(0.02f, 0.10f, 0.25f)}},
    {"icelandic_lake", {glm::vec3(0.70f, 0.90f, 0.95f), glm::vec3(0.25f, 0.45f, 0.55f)}},
    {"fjord", {glm::vec3(0.20f, 0.45f, 0.55f), glm::vec3(0.02f, 0.12f, 0.18f)}},
    {"north_sea", {glm::vec3(0.25f, 0.45f, 0.50f), glm::vec3(0.05f, 0.10f, 0.20f)}},

    // Lakes
    {"alpine_lake", {glm::vec3(0.30f, 0.65f, 0.55f), glm::vec3(0.05f, 0.20f, 0.15f)}},
    {"great_lakes", {glm::vec3(0.20f, 0.50f, 0.60f), glm::vec3(0.05f, 0.12f, 0.20f)}},
    {"volcanic_lake", {glm::vec3(0.40f, 0.70f, 0.65f), glm::vec3(0.10f, 0.20f, 0.25f)}},

    // Freshwater / Rivers
    {"river", {glm::vec3(0.30f, 0.50f, 0.35f), glm::vec3(0.05f, 0.15f, 0.10f)}},
    {"amazon_river", {glm::vec3(0.45f, 0.35f, 0.20f), glm::vec3(0.15f, 0.10f, 0.05f)}},

    // Swamp / Muddy
    {"swamp", {glm::vec3(0.40f, 0.35f, 0.20f), glm::vec3(0.10f, 0.12f, 0.05f)}},

    // Desert
    {"desert_oasis", {glm::vec3(0.45f, 0.80f, 0.65f), glm::vec3(0.10f, 0.30f, 0.30f)}},

    // Fantasy / Industrial
    {"toxic_sludge", {glm::vec3(0.60f, 1.00f, 0.20f), glm::vec3(0.10f, 0.30f, 0.05f)}},
    {"molten_lava", {glm::vec3(1.00f, 0.40f, 0.00f), glm::vec3(0.30f, 0.05f, 0.00f)}},
    {"arcane_pool", {glm::vec3(0.50f, 0.20f, 1.00f), glm::vec3(0.10f, 0.00f, 0.30f)}},
    {"industrial_sewage",
     {glm::vec3(0.40f, 0.35f, 0.15f), glm::vec3(0.10f, 0.08f, 0.02f)}},
    {"cyberpunk_pool", {glm::vec3(0.00f, 1.00f, 1.00f), glm::vec3(0.00f, 0.20f, 0.40f)}},
    {"blood_pool", {glm::vec3(0.80f, 0.10f, 0.10f), glm::vec3(0.20f, 0.00f, 0.00f)}},
    {"alien_lake", {glm::vec3(0.20f, 1.00f, 0.80f), glm::vec3(0.00f, 0.20f, 0.25f)}}

    //
};

} // namespace qtr