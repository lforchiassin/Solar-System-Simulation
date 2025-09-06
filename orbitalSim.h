/**
 * @brief Implements an orbital simulation view with enhanced UI and menu system
 * @author Dylan Frigerio, Luca Forchiassin
 *
 * @copyright Copyright (c) 2025
 */

#ifndef ORBITALSIM_H
#define ORBITALSIM_H
#include "raylib.h"

 /**
  * @brief System type enumeration
  */
typedef enum {
    SYSTEM_TYPE_SOLAR,
    SYSTEM_TYPE_ALPHA_CENTAURI
} SystemType;

/**
 * @brief Easter egg type enumeration
 */
typedef enum {
    EASTER_EGG_NONE,
    EASTER_EGG_PHI,
    EASTER_EGG_JUPITER_1000X
} EasterEggType;

/**
 * @brief Dispersion type enumeration
 */
typedef enum {
    DISPERSION_TIGHT,    // 2E11F to 6E11F
    DISPERSION_NORMAL,   // 2E11F to 12E11F
    DISPERSION_WIDE,     // 2E11F to 18E11F
    DISPERSION_EXTREME   // 2E11F to 20E12F
} DispersionType;

/**
 * @brief Orbital body definition
 */
struct OrbitalBody {
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
	Vector3 acceleration;
    double mass;
    double radius;
    double eventHorizonRadius;
    bool isActive;
    double growthRate; // Qué tan rápido crece cuando consume materia
};

/**
 * @brief Simulation configuration structure
 */
struct SimConfig {
    SystemType systemType;
    EasterEggType easterEgg;
    DispersionType dispersion;
    int asteroidCount;
};

/**
 * @brief Orbital simulation definition
 */
struct OrbitalSim {
    float timeStep; // Time step in seconds
    OrbitalBody* bodies; // Array of orbital bodies
    int numBodies; // Number of orbital bodies
    int systemBodies; // Number of system bodies (planets/stars)
    int asteroidCount; // Number of asteroids
    float centerRadius; // Radius of the most massive object in the star system
    BlackHole blackHole; // El agujero negro
    int aliveBodies; // Contador de cuerpos vivos
    SimConfig config; // Configuration used for this simulation
};

// Main simulation functions
OrbitalSim* constructOrbitalSim(float timeStep, const SimConfig* config);
void destroyOrbitalSim(OrbitalSim* sim);
void updateOrbitalSim(OrbitalSim* sim);
void resetOrbitalSim(OrbitalSim* sim, const SimConfig* config);

// Black hole functions
void createBlackHole(OrbitalSim* sim, Vector3 position);

// Configuration helper functions
float getDispersionRange(DispersionType dispersion);
const char* getDispersionName(DispersionType dispersion);
const char* getSystemName(SystemType system);
const char* getEasterEggName(EasterEggType easterEgg);

#endif