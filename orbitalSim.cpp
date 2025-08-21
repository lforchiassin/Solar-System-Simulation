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

static 

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
    body->radius = 0.005F * logf(2E3F); // Typical asteroid radius: 2km
    body->position = {r * cosf(phi) * 1E-11f, 0, r * sinf(phi) * 1E-11f};
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
    sim->numBodies = SOLARSYSTEM_BODYNUM + 500;
    sim->bodies = (OrbitalBody *)malloc(sizeof(OrbitalBody) * sim->numBodies);
    if (!sim->bodies) {
        free(sim);
        return NULL;
    }

    // Copia los datos de los cuerpos del sistema solar
	int i;
    for (i = 0; i < SOLARSYSTEM_BODYNUM; i++) {
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

	for(; i < sim->numBodies; i++) {
        configureAsteroid(&sim->bodies[i], solarSystem[0].mass);
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

void calculateAccelerationsOptimized(OrbitalBody *bodies, Vector3 *accelerations, int n, float G)
{
    const float epsilon = 1e-20f;
    
    // Inicializar aceleraciones
    for (int i = 0; i < n; i++) {
        accelerations[i] = {0.0f, 0.0f, 0.0f};
    }
    
    // Calcular fuerzas entre pares únicos (aprovecha 3ª ley de Newton)
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            Vector3 r_vec = Vector3Subtract(bodies[j].position, bodies[i].position);
            float r_squared = Vector3LengthSqr(r_vec);
            
            if (r_squared > epsilon) {
                float r = sqrtf(r_squared);
                float r_cubed = r_squared * r;
                
                // Calcular fuerza una sola vez
                float force_magnitude = G * bodies[i].mass * bodies[j].mass / r_cubed;
                Vector3 force_direction = r_vec; // Ya tiene la dirección correcta
                
                // Aplicar fuerzas iguales y opuestas
                Vector3 force_on_i = Vector3Scale(force_direction, force_magnitude / bodies[i].mass);
                Vector3 force_on_j = Vector3Scale(force_direction, -force_magnitude / bodies[j].mass);
                
                accelerations[i] = Vector3Add(accelerations[i], force_on_i);
                accelerations[j] = Vector3Add(accelerations[j], force_on_j);
            }
        }
    }
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
    
    // Inicializar aceleraciones
    Vector3 * accelerations= (Vector3 *)malloc(n * sizeof(Vector3));
    if (!accelerations) return;

    calculateAccelerationsOptimized(bodies, accelerations, n, GRAVITATIONAL_CONSTANT * 1E-22);

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
