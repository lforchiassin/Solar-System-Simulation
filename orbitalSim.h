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

struct OrbitalBody{
    // Fill in your code here...
    Vector3 position;
    Vector3 velocity;
    double mass;
    double radius;
	CLITERAL(Color) color;
    bool isAlive;
};

/**
 * @brief Black hole definition
 */
struct BlackHole {
    Vector3 position;
    Vector3 velocity;
    double mass;
    double radius;
    double eventHorizonRadius;
    bool isActive;
    double growthRate; // Qué tan rápido crece cuando consume materia
};

/**
 * @brief Orbital simulation definition
 */

struct OrbitalSim{
    // Fill in your code here...
   float timeStep; // Time step in seconds
   OrbitalBody *bodies; // Array of orbital bodies
   int numBodies; // Number of orbital bodies
   float centerRadius; // Radius of the most massive object in the star system
   BlackHole blackHole; // El agujero negro
   int aliveBodies; // Contador de cuerpos vivos
};

OrbitalSim *constructOrbitalSim(float timeStep);
void destroyOrbitalSim(OrbitalSim *sim);
void updateOrbitalSim(OrbitalSim *sim);
void createBlackHole(OrbitalSim* sim, Vector3 position);

#endif
