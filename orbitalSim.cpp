/**
 * @brief Implements an orbital simulation view with enhanced UI and menu system
 * @author Dylan Frigerio, Luca Forchiassin
 *
 * @copyright Copyright (c) 2025
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
static void initializeSolarSystem(OrbitalSim* sim);
static void initializeAlphaCentauriSystem(OrbitalSim* sim);
static void initializeAsteroids(OrbitalSim* sim, int count, DispersionType dispersion);

void createBlackHole(OrbitalSim* sim, Vector3 position) {
	if (sim->blackHole.isActive) return; // There can be only one
    sim->blackHole.position = position;
    sim->blackHole.velocity = { 0.0f, 0.0f, 0.0f };
	sim->blackHole.mass = 10.0f * 1.989E30f; // Aproximately 10 solar masses
    sim->blackHole.eventHorizonRadius = 2.95f * (sim->blackHole.mass / 1.989E30f) * 1E6f; // Schwarzschild ratio
    sim->blackHole.radius = 200.0f * sim->blackHole.eventHorizonRadius;
    sim->blackHole.isActive = true;
	sim->blackHole.growthRate = 1E3f; // Grows by consuming mass
	sim->blackHole.acceleration = { 0.0f, 0.0f, 0.0f };
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
        sim->blackHole.acceleration = { 0, 0, 0 };
        ComputeBlackHoleAcceleration(&sim->blackHole, bodies, accelerations, n);
		// Updates black hole position and velocity
        Vector3 accBH = sim->blackHole.acceleration;
        sim->blackHole.velocity = Vector3Add(sim->blackHole.velocity,
            Vector3Scale(accBH, dt));
        sim->blackHole.position = Vector3Add(sim->blackHole.position,
            Vector3Scale(sim->blackHole.velocity, dt));
        HandleBlackHoleCollision(&sim->blackHole, bodies, n);
    }

    for (int i = 0; i < n; i++) {
		if (!bodies[i].isAlive) continue; // Just updates alive bodies
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
    for (int i = 0; i < ALPHACENTAURISYSTEM_BODYNUM; i++) {
        sim->bodies[i].mass = alphaCentauriSystem[i].mass;
        sim->bodies[i].radius = alphaCentauriSystem[i].radius;
        sim->bodies[i].position = alphaCentauriSystem[i].position;
        sim->bodies[i].velocity = alphaCentauriSystem[i].velocity;
        sim->bodies[i].color = alphaCentauriSystem[i].color;
        sim->bodies[i].isAlive = true;
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

    // For elliptical orbits, the speed at aphelion is lower
    float v_circular = sqrtf(GRAVITATIONAL_CONSTANT * centerMass / r);

	// Simulate eccentricity for more interesting orbits
    float eccentricity = getRandomFloat(0.1F, 0.8F);  // 0 = circular, 1 = parabolic
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

/**
 * @brief Calculates gravitational accelerations for all bodies
 */

static void ComputeGravitationalAccelerations(OrbitalSim *sim, OrbitalBody* bodies, Vector3* accelerations, int n) {
    const double MIN_DISTANCE_CUBED = 1E29;   // Minimum distance cubed to avoid singularities
    const double INFLUENCE_DISTANCE_SQ = 1E15; // Threshold for planet-asteroid interactions

    // 1. Initialize all accelerations to zero
    for (int i = 0; i < n; i++) {
        accelerations[i] = { 0.0f, 0.0f, 0.0f };
    }

    // 2. Compute gravitational interactions between system bodies
    int systemBodies = sim->systemBodies;
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
                    r_vec = Vector3Subtract(bodies[i].position, bodies[5].position);
                    r_squared = Vector3LengthSqr(r_vec);
                    r_cubed = r_squared * sqrt(r_squared);

                    if (r_cubed > MIN_DISTANCE_CUBED) {
                        force_magnitude = GRAVITATIONAL_CONSTANT * bodies[5].mass / r_cubed;
                        accel_asteroid = Vector3Scale(r_vec, -force_magnitude);
                        accelerations[i] = Vector3Add(accelerations[i], accel_asteroid);
                    }
                    else {
                        force_magnitude = GRAVITATIONAL_CONSTANT * bodies[5].mass / MIN_DISTANCE_CUBED;
                        accel_asteroid = Vector3Scale(r_vec, -force_magnitude);
                        accelerations[i] = Vector3Add(accelerations[i], accel_asteroid);
                    }
                }
            }
            if (sim->config.systemType == SYSTEM_TYPE_ALPHA_CENTAURI)
            {
                r_vec = Vector3Subtract(bodies[i].position, bodies[1].position);
                r_squared = Vector3LengthSqr(r_vec);
                r_cubed = r_squared * sqrt(r_squared);

                if (r_cubed > MIN_DISTANCE_CUBED) {
                    force_magnitude = GRAVITATIONAL_CONSTANT * bodies[1].mass / r_cubed;
                    accel_asteroid = Vector3Scale(r_vec, -force_magnitude);
                    accelerations[i] = Vector3Add(accelerations[i], accel_asteroid);
                }
                else {
                    force_magnitude = GRAVITATIONAL_CONSTANT * bodies[1].mass / MIN_DISTANCE_CUBED;
                    accel_asteroid = Vector3Scale(r_vec, -force_magnitude);
                    accelerations[i] = Vector3Add(accelerations[i], accel_asteroid);
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

        Vector3 r_vec = Vector3Subtract(bodies[i].position, blackHole->position);
        float r_squared = Vector3LengthSqr(r_vec);
        float r_cubed = r_squared * sqrtf(r_squared);

        if (r_cubed > MIN_DISTANCE_CUBED) {
			// Force on the orbital body (towards the black hole)
            float force_magnitude_body = (float)GRAVITATIONAL_CONSTANT * blackHole->mass / r_cubed;
            Vector3 accel_body = Vector3Scale(r_vec, -force_magnitude_body);
            accelerations[i] = Vector3Add(accelerations[i], accel_body);

			// Force on the black hole (towards the body)
            float force_magnitude_blackHole = (float)GRAVITATIONAL_CONSTANT * bodies[i].mass / r_cubed;
            Vector3 accel_blackHole = Vector3Scale(r_vec, force_magnitude_blackHole);
            blackHole->acceleration = Vector3Add(blackHole->acceleration, accel_blackHole);
        }
        else {
			// Force on the orbital body (minimum distance)
            float force_magnitude_body = (float)GRAVITATIONAL_CONSTANT * blackHole->mass / MIN_DISTANCE_CUBED;
            Vector3 accel_body = Vector3Scale(r_vec, -force_magnitude_body);
            accelerations[i] = Vector3Add(accelerations[i], accel_body);

			// Force on the black hole (minimum distance)
            float force_magnitude_blackHole = (float)0.01f * GRAVITATIONAL_CONSTANT * bodies[i].mass / MIN_DISTANCE_CUBED;
            Vector3 accel_blackHole = Vector3Scale(r_vec, force_magnitude_blackHole);
            blackHole->acceleration = Vector3Add(blackHole->acceleration, accel_blackHole);
        }
    }
}

static void HandleBlackHoleCollision(BlackHole * blackHole, OrbitalBody * body, int n) {
    for (int i = 0; i < n; i++) {
        if (!body[i].isAlive) continue;

		// Calculate accretion radius
        float ACCRETION_RADIUS = fmaxf(blackHole->radius, 0.05f * Vector3Length(body[i].position));

		// Calculate distance to black hole
        Vector3 distance_vec = Vector3Subtract(body[i].position, blackHole->position);
        float distance = Vector3Length(distance_vec);

        // Verify collision
        if (distance < ACCRETION_RADIUS) {
            body[i].isAlive = false;
            blackHole->mass += body[i].mass;
            blackHole->radius += blackHole->growthRate;
            blackHole->eventHorizonRadius = 2.95f * (blackHole->mass / 1.989E30f) * 1E3f;
        }
    }
}