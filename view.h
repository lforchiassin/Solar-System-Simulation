/**
 * @brief Implements an orbital simulation view
 * @author Marc S. Ressl
 *
 * @copyright Copyright (c) 2022-2023
 */

#ifndef ORBITALSIMVIEW_H
#define ORBITALSIMVIEW_H

#include <raylib.h>
#include "orbitalSim.h"
#define UPDATEPERFRAME 10

/**
 * The view data
 */
struct View
{
    Camera3D camera;
};

View *constructView(int fps);
void destroyView(View *view);

bool isViewRendering(View *view);
void renderView(View *view, OrbitalSim *sim);

#endif
