/**
 * @brief Orbital simulation
 * @author Marc S. Ressl
 *
 * @copyright Copyright (c) 2022-2023
 */

// Enables M_PI #define in Windows
//falta ver la superfuncion el estilo!!!!

#define _USE_MATH_DEFINES
#define GRAVITATIONAL_CONSTANT 6.6743E-11F
#define ASTEROIDS_MEAN_RADIUS 4E11F
#define NUM_ASTEROIDS 1000
#define NUM_BODIES (NUM_ASTEROIDS + SOLARSYSTEM_BODYNUM)

#include <stdlib.h>
#include <math.h>

#include <stdio.h>

#include "orbitalSim.h"
#include "ephemerides.h"

static float getRandomFloat(float min, float max);
static void configureAsteroid(OrbitalBody* body, float centerMass);
static void ComputeGravitationalAccelerations(OrbitalBody *bodies, Vector3 *accelerations, int n);
static void ComputeBlackHoleAcceleration(BlackHole* blackHole, OrbitalBody *bodies, Vector3* accelerations, int n);
static void HandleBlackHoleCollision(BlackHole* blackHole, OrbitalBody* body, int n);
static Vector3 ComputeBlackHoleSelfAcceleration(BlackHole* blackHole, OrbitalBody* bodies, int n);


void createBlackHole(OrbitalSim* sim, Vector3 position)
{
	if (sim->blackHole.isActive) return; // Ya hay un agujero
	sim->blackHole.position = position;
	sim->blackHole.velocity = { 0.0f, 0.0f, 0.0f };
	sim->blackHole.mass = 10.0f * 1.989E30f; // Aproximadamente 5 masas solares
	sim->blackHole.eventHorizonRadius = 2.95f * (sim->blackHole.mass / 1.989E30f) * 1E6f; // Radio de Schwarzschild
    sim->blackHole.radius = 200.0f * sim->blackHole.eventHorizonRadius;
	sim->blackHole.isActive = true;
	sim->blackHole.growthRate = 1E3f; // Crece rápidamente al consumir materia
}

/**
 * @brief Constructs an orbital simulation
 *
 * @param float The time step
 * @return The orbital simulation
 */

OrbitalSim *constructOrbitalSim(float timeStep){
    OrbitalSim *sim = (OrbitalSim *)malloc(sizeof(OrbitalSim));
    if (!sim) return NULL;

    sim->timeStep = timeStep;
    sim->numBodies = NUM_BODIES;
    sim->bodies = (OrbitalBody *)malloc(sizeof(OrbitalBody) * sim->numBodies);
    if (!sim->bodies) {
        free(sim);
        return NULL;
    }

    sim->blackHole.isActive = false;

    // Copia los datos de los cuerpos del sistema solar
	int i, j;
    for (i = 0; i < SOLARSYSTEM_BODYNUM; i++) {
        sim->bodies[i].mass = solarSystem[i].mass;
        sim->bodies[i].radius = solarSystem[i].radius;
        
        sim->bodies[i].position = solarSystem[i].position;
        sim->bodies[i].velocity = solarSystem[i].velocity;

        sim->bodies[i].color = solarSystem[i].color;
        sim->bodies[i].isAlive = true;
    }
    sim->bodies[5].mass = solarSystem[5].mass * 1000.0;
    /*for (i = 0; i < ALPHACENTAURISYSTEM_BODYNUM; i++) {
        sim->bodies[i].mass = solarSystem[i].mass;
        sim->bodies[i].radius = solarSystem[i].radius;

        sim->bodies[i].position = solarSystem[i].position;
        sim->bodies[i].velocity = solarSystem[i].velocity;

        sim->bodies[i].color = solarSystem[i].color;
    }*/

	for (; i < sim->numBodies; i++) {
        configureAsteroid(&sim->bodies[i], solarSystem[0].mass);
    }

    return sim;
}

/**
 * @brief Destroys an orbital simulation
 */

void destroyOrbitalSim(OrbitalSim *sim){
    if (!sim) return;
    if (sim->bodies) free(sim->bodies);
    free(sim);
}

/**
 * @brief Simulates a timestep
 *
 * @param sim The orbital simulation
 */

void updateOrbitalSim(OrbitalSim* sim){
    int n = sim->numBodies;
    float dt = sim->timeStep;
    OrbitalBody *bodies = sim->bodies;
    
    Vector3 * accelerations= (Vector3 *)malloc(n * sizeof(Vector3));
    if (!accelerations) return;

    ComputeGravitationalAccelerations(bodies, accelerations, n);

    if (sim->blackHole.isActive)
    {
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

//***** STATIC HELPERS *****//

/**
 * @brief Gets a uniform random value in a range
 *
 * @param min Minimum value
 * @param max Maximum value
 * @return The random value
 */

static float getRandomFloat(float min, float max) {
    return min + (max - min) * rand() / (float)RAND_MAX;
}

/**
 * @brief Configures an asteroid
 *
 * @param body An orbital body
 * @param centerMass The mass of the most massive object in the star system
 */

static void configureAsteroid(OrbitalBody* body, float centerMass) {
    float r = getRandomFloat(2E11F, 1E12F);
    float phi = getRandomFloat(0, 2.0F * (float)M_PI);

    // Para  rbitas el pticas, la velocidad en el afelio es menor
    float v_circular = sqrtf(GRAVITATIONAL_CONSTANT * centerMass / r);

    // Simular diferentes excentricidades
    float eccentricity = getRandomFloat(0.1F, 0.8F);  // 0 = circular, 1 = parab lica
    float v = v_circular * sqrtf((1 - eccentricity) / (1 + eccentricity));

    float vy = getRandomFloat(-25.0F, 25.0F);

    body->mass = 1E12F;
    body->radius = 2E3F;
    body->position = { r * cosf(phi), 0, r * sinf(phi) };
    body->velocity = { -v * sinf(phi), vy, v * cosf(phi) };
    body->isAlive = true;
}


static void ComputeGravitationalAccelerations(OrbitalBody* bodies, Vector3* accelerations, int n) {
    const double MIN_DISTANCE_CUBED = 1E29;   // Minimum distance cubed to avoid singularities
    const double INFLUENCE_DISTANCE_SQ = 1E15; // Threshold for planet-asteroid interactions

    // 1. Initialize all accelerations to zero
    for (int i = 0; i < n; i++) {
        accelerations[i] = { 0.0f, 0.0f, 0.0f };
    }

    // 2. Compute gravitational interactions between Sun and planets, and between planets
    for (int i = 0; i < SOLARSYSTEM_BODYNUM; i++) {
        if (!bodies[i].isAlive) continue;

        for (int j = i + 1; j < SOLARSYSTEM_BODYNUM; j++) {
            if (!bodies[j].isAlive) continue;
            Vector3 r_vec = Vector3Subtract(bodies[j].position, bodies[i].position);
            double r_squared = Vector3LengthSqr(r_vec);
            double r_cubed = r_squared * sqrt(r_squared);

            // Only apply force if distance is above minimum threshold
            if (r_cubed > MIN_DISTANCE_CUBED) {
                // Compute gravitational "magnitude" factor
                double force_magnitude = GRAVITATIONAL_CONSTANT / r_cubed;

                // Acceleration on body j due to body i
                Vector3 accel_j = Vector3Scale(r_vec, -force_magnitude * bodies[i].mass);
                accelerations[j] = Vector3Add(accelerations[j], accel_j);

                // Acceleration on body i due to body j (Newton's third law)
                Vector3 accel_i = Vector3Scale(r_vec, force_magnitude * bodies[j].mass);
                accelerations[i] = Vector3Add(accelerations[i], accel_i);
            }
            else {
                printf("Cuerpo %d menor que el minimo\n", i);
                // Compute gravitational "magnitude" factor
                double force_magnitude = GRAVITATIONAL_CONSTANT / MIN_DISTANCE_CUBED;

                // Acceleration on body j due to body i
                Vector3 accel_j = Vector3Scale(r_vec, -force_magnitude * bodies[i].mass);
                accelerations[j] = Vector3Add(accelerations[j], accel_j);

                // Acceleration on body i due to body j (Newton's third law)
                Vector3 accel_i = Vector3Scale(r_vec, force_magnitude * bodies[j].mass);
                accelerations[i] = Vector3Add(accelerations[i], accel_i);
            }
               
        }
    }

    // 3. Compute gravitational acceleration from Sun to asteroids
    for (int i = SOLARSYSTEM_BODYNUM; i < n; i++) {
        if (!bodies[i].isAlive) continue;

        // Displacement vector from asteroid to Sun
        Vector3 r_vec = Vector3Subtract(bodies[i].position, bodies[0].position);
        double r_squared = Vector3LengthSqr(r_vec);
        double r_cubed = r_squared * sqrt(r_squared);

        if (r_cubed > MIN_DISTANCE_CUBED) {
            double force_magnitude = GRAVITATIONAL_CONSTANT * bodies[0].mass / r_cubed;
            Vector3 accel_asteroid = Vector3Scale(r_vec, -force_magnitude);
            accelerations[i] = Vector3Add(accelerations[i], accel_asteroid);
        }
        else
        {
            double force_magnitude = GRAVITATIONAL_CONSTANT * bodies[0].mass / MIN_DISTANCE_CUBED;
            Vector3 accel_asteroid = Vector3Scale(r_vec, -force_magnitude);
            accelerations[i] = Vector3Add(accelerations[i], accel_asteroid);
        }
    }

    // 4. Compute gravitational acceleration from planets to asteroids (if within influence distance)
    for (int i = 1; i < SOLARSYSTEM_BODYNUM; i++) { // Skip Sun (index 0)
        if (!bodies[i].isAlive) continue;

        for (int j = SOLARSYSTEM_BODYNUM; j < n; j++) {
            if (!bodies[j].isAlive) continue;

            // Vector from asteroid to planet
            Vector3 r_vec = Vector3Subtract(bodies[j].position, bodies[i].position);
            double r_squared = Vector3LengthSqr(r_vec);

            // Only apply if asteroid is close enough and above minimum distance
            if (r_squared < INFLUENCE_DISTANCE_SQ && r_squared > MIN_DISTANCE_CUBED) {
                double r_cubed = r_squared * sqrt(r_squared);
                double force_magnitude = GRAVITATIONAL_CONSTANT / r_cubed;

                // Acceleration on asteroid due to planet
                Vector3 accel_asteroid = Vector3Scale(r_vec, -force_magnitude * bodies[i].mass);
                accelerations[j] = Vector3Add(accelerations[j], accel_asteroid);
            }
            else if(r_squared < INFLUENCE_DISTANCE_SQ)
            {
                double force_magnitude = GRAVITATIONAL_CONSTANT / MIN_DISTANCE_CUBED;
                // Acceleration on asteroid due to planet
                Vector3 accel_asteroid = Vector3Scale(r_vec, -force_magnitude * bodies[i].mass);
                accelerations[j] = Vector3Add(accelerations[j], accel_asteroid);
            }
        }
    }
}

static void ComputeBlackHoleAcceleration(BlackHole* blackHole, OrbitalBody* bodies, Vector3* accelerations, int n)
{
    const double MIN_DISTANCE_CUBED = 1E29;   // Minimum distance cubed to avoid singularities
    for (int i = 0; i < n; i++) {
        if (!bodies[i].isAlive) continue;

        // Calcular diferencias en double
        double dx = (double)bodies[i].position.x - (double)blackHole->position.x;
        double dy = (double)bodies[i].position.y - (double)blackHole->position.y;
        double dz = (double)bodies[i].position.z - (double)blackHole->position.z;

        double r_squared = dx * dx + dy * dy + dz * dz;
        double r_cubed = r_squared * sqrt(r_squared);

        if (r_cubed > MIN_DISTANCE_CUBED) { // evitar división por cero
            double force_magnitude = (double)GRAVITATIONAL_CONSTANT * (double)blackHole->mass / r_cubed;

            // Fuerza sobre el cuerpo en double
            double fx = -force_magnitude * dx;
            double fy = -force_magnitude * dy;
            double fz = -force_magnitude * dz;

            // Convertir a float para el Vector3 final
            accelerations[i].x += (float)fx;
            accelerations[i].y += (float)fy;
            accelerations[i].z += (float)fz;
        }
        else
        {
            double force_magnitude = (double)GRAVITATIONAL_CONSTANT * (double)blackHole->mass / MIN_DISTANCE_CUBED;
            // Fuerza sobre el cuerpo en double
            double fx = -force_magnitude * dx;
            double fy = -force_magnitude * dy;
            double fz = -force_magnitude * dz;
            // Convertir a float para el Vector3 final
            accelerations[i].x += (float)fx;
            accelerations[i].y += (float)fy;
			accelerations[i].z += (float)fz;
        }
    }
}

static void HandleBlackHoleCollision(BlackHole* blackHole, OrbitalBody* body, int n)
{
    for (int i = 0; i < n; i++)
    {
        if (!body[i].isAlive) continue;

        double ACCRETION_RADIUS = fmaxf(blackHole->radius, 0.05f * Vector3Length(body[i].position));  // 1 UA en metros (double)
        double dx = (double)body[i].position.x - (double)blackHole->position.x;
        double dy = (double)body[i].position.y - (double)blackHole->position.y;
        double dz = (double)body[i].position.z - (double)blackHole->position.z;

        double distance = sqrt(dx * dx + dy * dy + dz * dz);

        if (distance < ACCRETION_RADIUS) {
            body[i].isAlive = false;
            blackHole->mass += body[i].mass;
            blackHole->radius += blackHole->growthRate;
            blackHole->eventHorizonRadius =
                2.95 * (blackHole->mass / 1.989E30) * 1E3;
        }
    }
}

static Vector3 ComputeBlackHoleSelfAcceleration(BlackHole* blackHole, OrbitalBody* bodies, int n)
{
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
