/**
 * @brief Orbital simulation
 * @author Marc S. Ressl
 *
 * @copyright Copyright (c) 2022-2023
 */

// Enables M_PI #define in Windows
#define _USE_MATH_DEFINES

#include <stdlib.h>
#include <math.h>

#include "orbitalSim.h"
#include "ephemerides.h"

#define GRAVITATIONAL_CONSTANT 6.6743E-11F
#define ASTEROIDS_MEAN_RADIUS 4E11F

/**
 * @brief Gets a uniform random value in a range
 *
 * @param min Minimum value
 * @param max Maximum value
 * @return The random value
 */
float getRandomFloat(float min, float max)
{
    return min + (max - min) * rand() / (float)RAND_MAX;
}

/**
 * @brief Configures an asteroid
 *
 * @param body An orbital body
 * @param centerMass The mass of the most massive object in the star system
 */
void configureAsteroid(OrbitalBody *body, float centerMass)
{
    // Logit distribution
    float x = getRandomFloat(0, 1);
    float l = logf(x) - logf(1 - x) + 1;

    // https://mathworld.wolfram.com/DiskPointPicking.html
    float r = ASTEROIDS_MEAN_RADIUS * sqrtf(fabsf(l));
    float phi = getRandomFloat(0, 2.0F * (float)M_PI);

    // Surprise!
    // phi = 0;

    // https://en.wikipedia.org/wiki/Circular_orbit#Velocity
    float v = sqrtf(GRAVITATIONAL_CONSTANT * centerMass / r) * getRandomFloat(0.6F, 1.2F);
    float vy = getRandomFloat(-1E2F, 1E2F);

    // Fill in with your own fields:
    body->mass = 1E12F;  // Typical asteroid weight: 1 billion tons
    body->radius = 2E3F; // Typical asteroid radius: 2km
    body->position = {r * cosf(phi), 0, r * sinf(phi)};
    body->velocity = {-v * sinf(phi), vy, v * cosf(phi)};
}


#include <stdio.h>
/**
 * @brief Constructs an orbital simulation
 *
 * @param float The time step
 * @return The orbital simulation
 */
OrbitalSim *constructOrbitalSim(float timeStep)
{
    OrbitalSim *sim = (OrbitalSim *)malloc(sizeof(OrbitalSim));
    if (!sim) return NULL;

    sim->timeStep = timeStep;
    sim->numBodies = SOLARSYSTEM_BODYNUM;
    sim->bodies = (OrbitalBody *)malloc(sizeof(OrbitalBody) * sim->numBodies);
    if (!sim->bodies) {
        free(sim);
        return NULL;
    }

    // Copia los datos de los cuerpos del sistema solar
    for (int i = 0; i < SOLARSYSTEM_BODYNUM; i++) {
        sim->bodies[i].mass = solarSystem[i].mass;
        sim->bodies[i].radius = 0.005F * logf(solarSystem[i].radius);
        
        sim->bodies[i].position = Vector3Scale(solarSystem[i].position, 1E-11);
        sim->bodies[i].velocity = solarSystem[i].velocity;
        
        sim->bodies[i].color = solarSystem[i].color;
        
        printf("Body %d: Mass = %f, Radius = %f, Position = %f, %f, %f, Velocity = %f, %f, %f\n",
               i,
               sim->bodies[i].mass,
               sim->bodies[i].radius,
               sim->bodies[i].position.x,
               sim->bodies[i].position.y,
               sim->bodies[i].position.z,
               sim->bodies[i].velocity.x,
               sim->bodies[i].velocity.y,
               sim->bodies[i].velocity.z);
    }

    return sim;
}

/**
 * @brief Destroys an orbital simulation
 */
void destroyOrbitalSim(OrbitalSim *sim)
{
    if (!sim) return;
    if (sim->bodies) free(sim->bodies);
    free(sim);
}

/**
 * @brief Simulates a timestep
 *
 * @param sim The orbital simulation
 */
void updateOrbitalSim(OrbitalSim *sim)
{
    int n = sim->numBodies;
    float dt = sim->timeStep;
    OrbitalBody *bodies = sim->bodies;
    const float epsilon = 1e-20f;
    
    // NUEVA constante gravitatoria para posiciones escaladas pero velocidades SIN escalar
    // G_mixed = G_real × (1E-11)² = G_real × 1E-22
    const float G_mixed = GRAVITATIONAL_CONSTANT * 1E-22f;
    
    // Arreglo para almacenar las aceleraciones
    Vector3 *accelerations = (Vector3*)malloc(n * sizeof(Vector3));
    
    // Paso 1: Calcular aceleración para cada cuerpo
    for (int i = 0; i < n; i++) {
        accelerations[i] = {0.0f, 0.0f, 0.0f};
        
        for (int j = 0; j < n; j++) {
            if (i != j) {
                Vector3 r_vec = Vector3Subtract(bodies[j].position, bodies[i].position);
                float r = Vector3Length(r_vec);
                
                if (r > epsilon) {
                    float factor = G_mixed * bodies[j].mass / (r * r * r);
                    Vector3 accel_contribution = Vector3Scale(r_vec, factor);
                    accelerations[i] = Vector3Add(accelerations[i], accel_contribution);
                }
            }
        }
    }
    
    // Paso 2: Actualizar velocidades y posiciones
    for (int i = 0; i < n; i++) {
        bodies[i].velocity = Vector3Add(bodies[i].velocity, 
            Vector3Scale(accelerations[i], dt));
        
        Vector3 velocity_scaled = Vector3Scale(bodies[i].velocity, 1E-11);
        bodies[i].position = Vector3Add(bodies[i].position, 
            Vector3Scale(velocity_scaled, dt));
    }
    
    free(accelerations);
}
