/**
 * @brief Orbital simulation
 * @author Marc S. Ressl
 *
 * @copyright Copyright (c) 2022-2023
 */

#define _USE_MATH_DEFINES
#define GRAVITATIONAL_CONSTANT 6.6743E-11F
#define ASTEROIDS_MEAN_RADIUS 4E11F

#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#include "orbitalSim.h"
#include "ephemerides.h"

static float getRandomFloat(float min, float max);
static void configureAsteroid(OrbitalBody* body, float centerMass, DispersionType dispersion, int index);
static void ComputeGravitationalAccelerations(OrbitalSim* sim, OrbitalBody* bodies, Vector3* accelerations, int n);
static void ComputeBlackHoleAcceleration(BlackHole* blackHole, OrbitalBody* bodies, Vector3* accelerations, int n);
static void HandleBlackHoleCollision(BlackHole* blackHole, OrbitalBody* body, int n);
static Vector3 ComputeBlackHoleSelfAcceleration(BlackHole* blackHole, OrbitalBody* bodies, int n);
static void initializeSolarSystem(OrbitalSim* sim);
static void initializeAlphaCentauriSystem(OrbitalSim* sim);
static void initializeAsteroids(OrbitalSim* sim, int count, DispersionType dispersion);

void createBlackHole(OrbitalSim* sim, Vector3 position) {
    if (sim->blackHole.isActive) return; // Ya hay un agujero
    sim->blackHole.position = position;
    sim->blackHole.velocity = { 0.0f, 0.0f, 0.0f };
    sim->blackHole.mass = 10.0f * 1.989E30f; // Aproximadamente 10 masas solares
    sim->blackHole.eventHorizonRadius = 2.95f * (sim->blackHole.mass / 1.989E30f) * 1E6f; // Radio de Schwarzschild
    sim->blackHole.radius = 200.0f * sim->blackHole.eventHorizonRadius;
    sim->blackHole.isActive = true;
    sim->blackHole.growthRate = 1E3f; // Crece rápidamente al consumir materia
}

/**
 * @brief Constructs an orbital simulation with configurable parameters
 */
OrbitalSim* constructOrbitalSim(float timeStep, const SimConfig* config) {
    OrbitalSim* sim = (OrbitalSim*)malloc(sizeof(OrbitalSim));
    if (!sim) return NULL;

    sim->timeStep = timeStep;
    sim->config = *config; // Store configuration
    sim->asteroidCount = config->asteroidCount;

    // Determine system bodies count
    sim->systemBodies = (config->systemType == SYSTEM_TYPE_SOLAR) ? SOLARSYSTEM_BODYNUM : ALPHACENTAURISYSTEM_BODYNUM;
    sim->numBodies = sim->systemBodies + sim->asteroidCount;

    // Allocate memory for all bodies
    sim->bodies = (OrbitalBody*)malloc(sizeof(OrbitalBody) * sim->numBodies);
    if (!sim->bodies) {
        free(sim);
        return NULL;
    }

    sim->blackHole.isActive = false;
    sim->aliveBodies = sim->numBodies;

    // Initialize system
    if (config->systemType == SYSTEM_TYPE_SOLAR) {
        initializeSolarSystem(sim);
    }
    else {
        initializeAlphaCentauriSystem(sim);
    }

    // Initialize asteroids if any
    if (sim->asteroidCount > 0) {
        initializeAsteroids(sim, sim->asteroidCount, config->dispersion);
    }

    return sim;
}

/**
 * @brief Resets the orbital simulation with new configuration
 */
void resetOrbitalSim(OrbitalSim* sim, const SimConfig* config) {
    if (!sim) return;

    // Store old values
    float timeStep = sim->timeStep;

    // Free old bodies array
    if (sim->bodies) {
        free(sim->bodies);
    }

    // Reset black hole
    sim->blackHole.isActive = false;

    // Update configuration
    sim->config = *config;
    sim->asteroidCount = config->asteroidCount;
    sim->systemBodies = (config->systemType == SYSTEM_TYPE_SOLAR) ? SOLARSYSTEM_BODYNUM : ALPHACENTAURISYSTEM_BODYNUM;
    sim->numBodies = sim->systemBodies + sim->asteroidCount;
    sim->timeStep = timeStep;

    // Allocate new memory
    sim->bodies = (OrbitalBody*)malloc(sizeof(OrbitalBody) * sim->numBodies);
    if (!sim->bodies) {
        sim->numBodies = 0;
        return;
    }

    sim->aliveBodies = sim->numBodies;

    // Initialize system
    if (config->systemType == SYSTEM_TYPE_SOLAR) {
        initializeSolarSystem(sim);
    }
    else {
        initializeAlphaCentauriSystem(sim);
    }

    // Initialize asteroids if any
    if (sim->asteroidCount > 0) {
        initializeAsteroids(sim, sim->asteroidCount, config->dispersion);
    }

    if (config->easterEgg == EASTER_EGG_JUPITER_1000X)
    {
        if (sim->config.systemType == SYSTEM_TYPE_SOLAR && sim->numBodies > 5) {
            sim->bodies[5].mass *= 1000.0;
        }
    }
}

/**
 * @brief Destroys an orbital simulation
 */
void destroyOrbitalSim(OrbitalSim* sim) {
    if (!sim) return;
    if (sim->bodies) free(sim->bodies);
    free(sim);
}

/**
 * @brief Simulates a timestep
 */
void updateOrbitalSim(OrbitalSim* sim) {
    int n = sim->numBodies;
    float dt = sim->timeStep;
    OrbitalBody* bodies = sim->bodies;

    Vector3* accelerations = (Vector3*)malloc(n * sizeof(Vector3));
    if (!accelerations) return;

    ComputeGravitationalAccelerations(sim, bodies, accelerations, n);

    if (sim->blackHole.isActive) {
        ComputeBlackHoleAcceleration(&sim->blackHole, bodies, accelerations, n);
        // Actualizar posición del agujero
        Vector3 accBH = ComputeBlackHoleSelfAcceleration(&sim->blackHole, bodies, n);
        accBH = Vector3Scale(accBH, 0.1f);
        sim->blackHole.velocity = Vector3Add(sim->blackHole.velocity,
            Vector3Scale(accBH, dt));
        sim->blackHole.position = Vector3Add(sim->blackHole.position,
            Vector3Scale(sim->blackHole.velocity, dt));
        HandleBlackHoleCollision(&sim->blackHole, bodies, n);
    }

    // Actualizar velocidades y posiciones
    for (int i = 0; i < n; i++) {
        if (!bodies[i].isAlive) continue; // Solo actualizar cuerpos vivos
        bodies[i].velocity = Vector3Add(bodies[i].velocity,
            Vector3Scale(accelerations[i], dt));

        bodies[i].position = Vector3Add(bodies[i].position,
            Vector3Scale(bodies[i].velocity, dt));
    }

    free(accelerations);
}

//***** SYSTEM INITIALIZATION FUNCTIONS *****//

/**
 * @brief Initialize Solar System
 */
static void initializeSolarSystem(OrbitalSim* sim) {
    for (int i = 0; i < SOLARSYSTEM_BODYNUM && i < sim->numBodies; i++) {
        sim->bodies[i].mass = solarSystem[i].mass;
        sim->bodies[i].radius = solarSystem[i].radius;
        sim->bodies[i].position = solarSystem[i].position;
        sim->bodies[i].velocity = solarSystem[i].velocity;
        sim->bodies[i].color = solarSystem[i].color;
        sim->bodies[i].isAlive = true;
    }
}

/**
 * @brief Initialize Alpha Centauri System
 */
static void initializeAlphaCentauriSystem(OrbitalSim* sim) {
    // Use Alpha Centauri data if available in ephemerides
    // For now, use modified solar system as placeholder
    for (int i = 0; i < ALPHACENTAURISYSTEM_BODYNUM && i < sim->numBodies; i++) {
        if (i < SOLARSYSTEM_BODYNUM) {
            sim->bodies[i].mass = solarSystem[i].mass;
            sim->bodies[i].radius = solarSystem[i].radius;
            sim->bodies[i].position = solarSystem[i].position;
            sim->bodies[i].velocity = solarSystem[i].velocity;
            sim->bodies[i].color = solarSystem[i].color;
            sim->bodies[i].isAlive = true;

            // Modify for Alpha Centauri characteristics
            if (i == 0) { // Primary star
                sim->bodies[i].color = ORANGE;
                sim->bodies[i].mass *= 1.1f; // Slightly more massive
            }
            else if (i == 1) { // Secondary star (replace Mercury)
                sim->bodies[i].color = RED;
                sim->bodies[i].mass = solarSystem[0].mass * 0.9f; // Binary companion
                sim->bodies[i].position = Vector3Scale(sim->bodies[i].position, 20.0f); // Further out
            }
        }
    }
}

/**
 * @brief Initialize asteroids with specified count and dispersion
 */
static void initializeAsteroids(OrbitalSim* sim, int count, DispersionType dispersion) {
    float centerMass = (sim->systemBodies > 0) ? sim->bodies[0].mass : 1.989E30f;

    for (int i = sim->systemBodies; i < sim->systemBodies + count && i < sim->numBodies; i++) {
        if (sim->config.easterEgg == EASTER_EGG_PHI) {
            configureAsteroid(&sim->bodies[i], centerMass, dispersion, 1);
        }
        else {
            configureAsteroid(&sim->bodies[i], centerMass, dispersion, 0);
        }
    }
}

//***** CONFIGURATION HELPER FUNCTIONS *****//

/**
 * @brief Get dispersion range based on type
 */
float getDispersionRange(DispersionType dispersion) {
    switch (dispersion) {
    case DISPERSION_TIGHT:  return 6E11F;
    case DISPERSION_NORMAL: return 12E11F;
    case DISPERSION_WIDE:   return 18E11F;
    case DISPERSION_EXTREME: return 20E12F;
    default: return 12E11F;
    }
}

/**
 * @brief Get dispersion name
 */
const char* getDispersionName(DispersionType dispersion) {
    switch (dispersion) {
    case DISPERSION_TIGHT:  return "Tight";
    case DISPERSION_NORMAL: return "Normal";
    case DISPERSION_WIDE:   return "Wide";
    case DISPERSION_EXTREME: return "Extreme";
    default: return "Normal";
    }
}

/**
 * @brief Get system name
 */
const char* getSystemName(SystemType system) {
    switch (system) {
    case SYSTEM_TYPE_SOLAR: return "Solar System";
    case SYSTEM_TYPE_ALPHA_CENTAURI: return "Alpha Centauri";
    default: return "Solar System";
    }
}

/**
 * @brief Get easter egg name
 */
const char* getEasterEggName(EasterEggType easterEgg) {
    switch (easterEgg) {
    case EASTER_EGG_NONE: return "None";
    case EASTER_EGG_PHI: return "Phi Effect";
    case EASTER_EGG_JUPITER_1000X: return "Jupiter 1000x";
    default: return "None";
    }
}

//***** STATIC HELPERS *****//

/**
 * @brief Gets a uniform random value in a range
 */
static float getRandomFloat(float min, float max) {
    return min + (max - min) * rand() / (float)RAND_MAX;
}

/**
 * @brief Configures a regular asteroid
 */
static void configureAsteroid(OrbitalBody* body, float centerMass, DispersionType dispersion, int easterEgg) {
    float minDistance = 2E11F;
    float maxDistance = getDispersionRange(dispersion);

    float r = getRandomFloat(minDistance, maxDistance);
    float phi = getRandomFloat(0, 2.0F * (float)M_PI);

    // Para órbitas elípticas, la velocidad en el afelio es menor
    float v_circular = sqrtf(GRAVITATIONAL_CONSTANT * centerMass / r);

    // Simular diferentes excentricidades
    float eccentricity = getRandomFloat(0.1F, 0.8F);  // 0 = circular, 1 = parabólica
    float v = v_circular * sqrtf((1 - eccentricity) / (1 + eccentricity));

    float vy = getRandomFloat(-25.0F, 25.0F);

    if (easterEgg)
    {
        phi = 0;
    }

    body->mass = 1E12F;
    body->radius = 2E3F;
    body->position = { r * cosf(phi), 0, r * sinf(phi) };
    body->velocity = { -v * sinf(phi), vy, v * cosf(phi) };
    body->color = GRAY;
    body->isAlive = true;
}

//***** PHYSICS COMPUTATION FUNCTIONS *****//

static void ComputeGravitationalAccelerations(OrbitalSim *sim, OrbitalBody* bodies, Vector3* accelerations, int n) {
    const double MIN_DISTANCE_CUBED = 1E29;   // Minimum distance cubed to avoid singularities
    const double INFLUENCE_DISTANCE_SQ = 1E15; // Threshold for planet-asteroid interactions

    // 1. Initialize all accelerations to zero
    for (int i = 0; i < n; i++) {
        accelerations[i] = { 0.0f, 0.0f, 0.0f };
    }

    // 2. Compute gravitational interactions between system bodies
    int systemBodies = (n > 100) ? 9 : n; // Assume first 9 are system bodies if many total bodies
    for (int i = 0; i < systemBodies; i++) {
        if (!bodies[i].isAlive) continue;

        for (int j = i + 1; j < systemBodies; j++) {
            if (!bodies[j].isAlive) continue;
            Vector3 r_vec = Vector3Subtract(bodies[j].position, bodies[i].position);
            double r_squared = Vector3LengthSqr(r_vec);
            double r_cubed = r_squared * sqrt(r_squared);

            double force_magnitude;
            Vector3 accel_j;
            Vector3 accel_i;

            if (r_cubed > MIN_DISTANCE_CUBED) {
                force_magnitude = GRAVITATIONAL_CONSTANT / r_cubed;
                accel_j = Vector3Scale(r_vec, -force_magnitude * bodies[i].mass);
                accelerations[j] = Vector3Add(accelerations[j], accel_j);
                accel_i = Vector3Scale(r_vec, force_magnitude * bodies[j].mass);
                accelerations[i] = Vector3Add(accelerations[i], accel_i);
            }
            else {
                force_magnitude = GRAVITATIONAL_CONSTANT / MIN_DISTANCE_CUBED;
                accel_j = Vector3Scale(r_vec, -force_magnitude * bodies[i].mass);
                accelerations[j] = Vector3Add(accelerations[j], accel_j);
                accel_i = Vector3Scale(r_vec, force_magnitude * bodies[j].mass);
                accelerations[i] = Vector3Add(accelerations[i], accel_i);
            }
        }
    }

    // 3. Compute gravitational acceleration from primary star to asteroids
    if (n > systemBodies && bodies[0].isAlive) {
        for (int i = systemBodies; i < n; i++) {
            if (!bodies[i].isAlive) continue;

            Vector3 r_vec = Vector3Subtract(bodies[i].position, bodies[0].position);
            double r_squared = Vector3LengthSqr(r_vec);
            double r_cubed = r_squared * sqrt(r_squared);
            
            double force_magnitude;
            Vector3 accel_asteroid;

            if (r_cubed > MIN_DISTANCE_CUBED) {
                force_magnitude = GRAVITATIONAL_CONSTANT * bodies[0].mass / r_cubed;
                accel_asteroid = Vector3Scale(r_vec, -force_magnitude);
                accelerations[i] = Vector3Add(accelerations[i], accel_asteroid);
            }
            else {
                force_magnitude = GRAVITATIONAL_CONSTANT * bodies[0].mass / MIN_DISTANCE_CUBED;
                accel_asteroid = Vector3Scale(r_vec, -force_magnitude);
                accelerations[i] = Vector3Add(accelerations[i], accel_asteroid);
            }

            if (sim->config.easterEgg == EASTER_EGG_JUPITER_1000X)
            {
                if (sim->config.systemType == SYSTEM_TYPE_SOLAR && sim->numBodies > 5) {
                    r_vec = Vector3Subtract(bodies[i].position, bodies[0].position);
                    r_squared = Vector3LengthSqr(r_vec);
                    r_cubed = r_squared * sqrt(r_squared);

                    if (r_cubed > MIN_DISTANCE_CUBED) {
                        force_magnitude = GRAVITATIONAL_CONSTANT * bodies[0].mass / r_cubed;
                        accel_asteroid = Vector3Scale(r_vec, -force_magnitude);
                        accelerations[i] = Vector3Add(accelerations[i], accel_asteroid);
                    }
                    else {
                        force_magnitude = GRAVITATIONAL_CONSTANT * bodies[0].mass / MIN_DISTANCE_CUBED;
                        accel_asteroid = Vector3Scale(r_vec, -force_magnitude);
                        accelerations[i] = Vector3Add(accelerations[i], accel_asteroid);
                    }
                }
            }
        }
    }

    // 4. Compute gravitational acceleration from planets to asteroids (if within influence distance)
    for (int i = 1; i < systemBodies; i++) { // Skip primary star (index 0)
        if (!bodies[i].isAlive) continue;

        for (int j = systemBodies; j < n; j++) {
            if (!bodies[j].isAlive) continue;

            Vector3 r_vec = Vector3Subtract(bodies[j].position, bodies[i].position);
            double r_squared = Vector3LengthSqr(r_vec);

            if (r_squared < INFLUENCE_DISTANCE_SQ && r_squared > MIN_DISTANCE_CUBED) {
                double r_cubed = r_squared * sqrt(r_squared);
                double force_magnitude = GRAVITATIONAL_CONSTANT / r_cubed;
                Vector3 accel_asteroid = Vector3Scale(r_vec, -force_magnitude * bodies[i].mass);
                accelerations[j] = Vector3Add(accelerations[j], accel_asteroid);
            }
            else if (r_squared < INFLUENCE_DISTANCE_SQ) {
                double force_magnitude = GRAVITATIONAL_CONSTANT / MIN_DISTANCE_CUBED;
                Vector3 accel_asteroid = Vector3Scale(r_vec, -force_magnitude * bodies[i].mass);
                accelerations[j] = Vector3Add(accelerations[j], accel_asteroid);
            }
        }
    }
}

static void ComputeBlackHoleAcceleration(BlackHole* blackHole, OrbitalBody* bodies, Vector3* accelerations, int n) {
    const double MIN_DISTANCE_CUBED = 1E29;
    for (int i = 0; i < n; i++) {
        if (!bodies[i].isAlive) continue;

        double dx = (double)bodies[i].position.x - (double)blackHole->position.x;
        double dy = (double)bodies[i].position.y - (double)blackHole->position.y;
        double dz = (double)bodies[i].position.z - (double)blackHole->position.z;

        double r_squared = dx * dx + dy * dy + dz * dz;
        double r_cubed = r_squared * sqrt(r_squared);

        if (r_cubed > MIN_DISTANCE_CUBED) {
            double force_magnitude_body = (double)GRAVITATIONAL_CONSTANT * blackHole->mass / r_cubed;
            double fx = -force_magnitude_body * dx;
            double fy = -force_magnitude_body * dy;
            double fz = -force_magnitude_body * dz;

            accelerations[i].x += (float)fx;
            accelerations[i].y += (float)fy;
            accelerations[i].z += (float)fz;

            double force_magnitude_blackHole = (double)GRAVITATIONAL_CONSTANT * bodies[i].mass / r_cubed;
            fx = -force_magnitude_blackHole * dx;
            fy = -force_magnitude_blackHole * dy;
            fz = -force_magnitude_blackHole * dz;

			blackHole->acceleration.x -= (float)fx;
			blackHole->acceleration.y -= (float)fy;
			blackHole->acceleration.z -= (float)fz;
        }
        else {
            double force_magnitude_body = (double)GRAVITATIONAL_CONSTANT * blackHole->mass / MIN_DISTANCE_CUBED;
            double fx = -force_magnitude_body * dx;
            double fy = -force_magnitude_body * dy;
            double fz = -force_magnitude_body * dz;
            accelerations[i].x += (float)fx;
            accelerations[i].y += (float)fy;
            accelerations[i].z += (float)fz;

            double force_magnitude_blackHole = (double)GRAVITATIONAL_CONSTANT * bodies[i].mass / r_cubed;
            fx = -force_magnitude_blackHole * dx;
            fy = -force_magnitude_blackHole * dy;
            fz = -force_magnitude_blackHole * dz;

			blackHole->acceleration.x -= (float)fx;
			blackHole->acceleration.y -= (float)fy;
			blackHole->acceleration.z -= (float)fz;
        }
    }
}

static void HandleBlackHoleCollision(BlackHole* blackHole, OrbitalBody* body, int n) {
    for (int i = 0; i < n; i++) {
        if (!body[i].isAlive) continue;

        double ACCRETION_RADIUS = fmaxf(blackHole->radius, 0.05f * Vector3Length(body[i].position));
        double dx = (double)body[i].position.x - (double)blackHole->position.x;
        double dy = (double)body[i].position.y - (double)blackHole->position.y;
        double dz = (double)body[i].position.z - (double)blackHole->position.z;

        double distance = sqrt(dx * dx + dy * dy + dz * dz);

        if (distance < ACCRETION_RADIUS) {
            body[i].isAlive = false;
            blackHole->mass += body[i].mass;
            blackHole->radius += blackHole->growthRate;
            blackHole->eventHorizonRadius = 2.95 * (blackHole->mass / 1.989E30) * 1E3;
        }
    }
}

static Vector3 ComputeBlackHoleSelfAcceleration(BlackHole* blackHole, OrbitalBody* bodies, int n) {
    Vector3 acceleration = { 0.0f, 0.0f, 0.0f };

    for (int i = 0; i < n; i++) {
        if (!bodies[i].isAlive) continue;

        double dx = (double)bodies[i].position.x - (double)blackHole->position.x;
        double dy = (double)bodies[i].position.y - (double)blackHole->position.y;
        double dz = (double)bodies[i].position.z - (double)blackHole->position.z;

        double r_squared = dx * dx + dy * dy + dz * dz;
        double r_cubed = r_squared * sqrt(r_squared);

        if (r_cubed > 0.0) {
            double force_magnitude = (double)GRAVITATIONAL_CONSTANT * (double)bodies[i].mass / r_cubed;

            acceleration.x += (float)(force_magnitude * dx);
            acceleration.y += (float)(force_magnitude * dy);
            acceleration.z += (float)(force_magnitude * dz);
        }
    }

    return acceleration;
}