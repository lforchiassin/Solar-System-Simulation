/**
 * @brief Orbital simulation main module
 * @author Marc S. Ressl
 *
 * @copyright Copyright (c) 2022-2023
 */

#include "orbitalSim.h"
#include "view.h"

#define SECONDS_PER_DAY 86400

int main() {
    int fps = 60;
    float timeMultiplier = 5 * SECONDS_PER_DAY; // Simulation speed: 5 days per simulation second
    float timeStep = timeMultiplier / fps;

    // Default simulation configuration
    SimConfig defaultConfig = {
        SYSTEM_TYPE_SOLAR,      // Solar system
        EASTER_EGG_NONE,        // No easter egg
        DISPERSION_NORMAL,      // Normal asteroid dispersion
        1000                    // 1000 asteroids
    };

    OrbitalSim* sim = constructOrbitalSim(timeStep, &defaultConfig);
    View* view = constructView(fps);

    while (isViewRendering(view)) {
        for (int i = 0; i < UPDATEPERFRAME; i++) // Accelerates simulation 
            updateOrbitalSim(sim);
        renderView(view, sim, 0);
    }

    destroyView(view);
    destroyOrbitalSim(sim);

    return 0;
}