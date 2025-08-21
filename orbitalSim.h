/**
 * @brief Orbital simulation
 * @author Marc S. Ressl
 *
 * @copyright Copyright (c) 2022-2023
 */

#ifndef ORBITALSIM_H
#define ORBITALSIM_H
#include "raylib.h"

/**
 * @brief Orbital body definition
 */
struct OrbitalBody
{
    // Fill in your code here...
    Vector3 position;
    Vector3 velocity;
    float mass;
	float radius;
	CLITERAL(Color) color;
};


/**
 * @brief Orbital simulation definition
 */
struct OrbitalSim
{
    // Fill in your code here...
   float timeStep; // Time step in seconds
   OrbitalBody *bodies; // Array of orbital bodies
   int numBodies; // Number of orbital bodies
   float centerMass; // Mass of the most massive object in the star system
   float centerRadius; // Radius of the most massive object in the star system

};

OrbitalSim *constructOrbitalSim(float timeStep);
void destroyOrbitalSim(OrbitalSim *sim);

void updateOrbitalSim(OrbitalSim *sim);

#endif
