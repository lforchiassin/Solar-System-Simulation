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
#define NUM_ASTEROIDS 2000
#define NUM_BODIES (NUM_ASTEROIDS + SOLARSYSTEM_BODYNUM + ALPHACENTAURISYSTEM_BODYNUM)

#include <stdlib.h>
#include <math.h>

#include "orbitalSim.h"
#include "ephemerides.h"

static void calculateAccelerationsOptimized(OrbitalBody *bodies, Vector3 *accelerations, int n, float G);

/**
 * @brief Gets a uniform random value in a range
 *
 * @param min Minimum value
 * @param max Maximum value
 * @return The random value
 */

float getRandomFloat(float min, float max){
    return min + (max - min) * rand() / (float)RAND_MAX;
}

/**
 * @brief Configures an asteroid
 *
 * @param body An orbital body
 * @param centerMass The mass of the most massive object in the star system
 */

void configureAsteroid(OrbitalBody* body, float centerMass) {
    float r = getRandomFloat(2E11F, 10E12F);
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
}

#include <stdio.h>

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

    // Copia los datos de los cuerpos del sistema solar
	int i, j;
    for (i = 0; i < SOLARSYSTEM_BODYNUM; i++) {
        sim->bodies[i].mass = solarSystem[i].mass;
        sim->bodies[i].radius = solarSystem[i].radius;
        
        sim->bodies[i].position = solarSystem[i].position;
        sim->bodies[i].velocity = solarSystem[i].velocity;

        sim->bodies[i].color = solarSystem[i].color;
        
    //     printf("Body %d: Mass = %f, Radius = %f, Position = %f, %f, %f, Velocity = %f, %f, %f\n",
    //            i,
    //            sim->bodies[i].mass,
    //            sim->bodies[i].radius,
    //            sim->bodies[i].position.x,
    //            sim->bodies[i].position.y,
    //            sim->bodies[i].position.z,
    //            sim->bodies[i].velocity.x,
    //            sim->bodies[i].velocity.y,
    //            sim->bodies[i].velocity.z);
    }
    /*for (i = 0; i < ALPHACENTAURISYSTEM_BODYNUM; i++) {
        sim->bodies[i].mass = solarSystem[i].mass;
        sim->bodies[i].radius = RADIUS_SCALE(solarSystem[i].radius);

        sim->bodies[i].position = Vector3Scale(solarSystem[i].position, SCALE_FACTOR);
        sim->bodies[i].velocity = Vector3Scale(solarSystem[i].velocity, SCALE_FACTOR);

        sim->bodies[i].color = solarSystem[i].color;

        //     printf("Body %d: Mass = %f, Radius = %f, Position = %f, %f, %f, Velocity = %f, %f, %f\n",
        //            i,
        //            sim->bodies[i].mass,
        //            sim->bodies[i].radius,
        //            sim->bodies[i].position.x,
        //            sim->bodies[i].position.y,
        //            sim->bodies[i].position.z,
        //            sim->bodies[i].velocity.x,
        //            sim->bodies[i].velocity.y,
        //            sim->bodies[i].velocity.z);
    }*/
    /*for (j = 0; j < ALPHACENTAURISYSTEM_BODYNUM; j++) {
        sim->bodies[j].mass = alphaCentauriSystem[j].mass;
        sim->bodies[j].radius = RADIUS_SCALE(alphaCentauriSystem[j].radius);

        sim->bodies[j].position = Vector3Scale(alphaCentauriSystem[j].position, SCALE_FACTOR);
        sim->bodies[j].velocity = Vector3Scale(alphaCentauriSystem[j].velocity, SCALE_FACTOR);

        sim->bodies[j].color = alphaCentauriSystem[j].color;

        printf("Body %d: Mass = %f, Radius = %f, Position = %f, %f, %f, Velocity = %f, %f, %f\n",
            j,
            sim->bodies[j].mass,
            sim->bodies[j].radius,
            sim->bodies[j].position.x,
            sim->bodies[j].position.y,
            sim->bodies[j].position.z,
            sim->bodies[j].velocity.x,
            sim->bodies[j].velocity.y,
            sim->bodies[j].velocity.z);
    }*/

	for (; i < sim->numBodies; i++) {
        configureAsteroid(&sim->bodies[i], solarSystem[0].mass);
        // printf("Body %d: Mass = %f, Radius = %f, Position = %f, %f, %f, Velocity = %f, %f, %f\n",
        //     i,
        //     sim->bodies[i].mass,
        //     sim->bodies[i].radius,
        //     sim->bodies[i].position.x,
        //     sim->bodies[i].position.y,
        //     sim->bodies[i].position.z,
        //     sim->bodies[i].velocity.x,
        //     sim->bodies[i].velocity.y,
        //     sim->bodies[i].velocity.z);
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
    
    // Inicializar aceleraciones
    Vector3 * accelerations= (Vector3 *)malloc(n * sizeof(Vector3));
    if (!accelerations) return;

    calculateAccelerationsOptimized(bodies, accelerations, n, GRAVITATIONAL_CONSTANT);

    // Paso 2: Actualizar velocidades y posiciones
    for (int i = 0; i < n; i++) {
        bodies[i].velocity = Vector3Add(bodies[i].velocity, 
            Vector3Scale(accelerations[i], dt));

        bodies[i].position = Vector3Add(bodies[i].position,
            Vector3Scale(bodies[i].velocity, dt));
        
        // printf("Body %d: Position = %f, %f, %f, Velocity = %.10f, %.10f, %.10f\n",
        //     i,
        //     sim->bodies[i].position.x,
        //     sim->bodies[i].position.y,
        //     sim->bodies[i].position.z,
        //     sim->bodies[i].velocity.x,
        //     sim->bodies[i].velocity.y,
        //     sim->bodies[i].velocity.z);
    }
    
    free(accelerations);
}

static void calculateAccelerationsOptimized(OrbitalBody* bodies, Vector3* accelerations, int n, float G) {
    // Inicializar aceleraciones
    for (int i = 0; i < n; i++) {
        accelerations[i] = { 0.0f, 0.0f, 0.0f };
    }

    // El Sol permanece fijo (no se actualiza su aceleraci n)
    // Esto es una aproximaci n v lida dado que su masa es mucho mayor

    // Calcular fuerzas Sol-Planetas
    for (int j = 1; j <= SOLARSYSTEM_BODYNUM; j++) {
        Vector3 r_vec = Vector3Subtract(bodies[j].position, bodies[0].position);
        float r_squared = Vector3LengthSqr(r_vec);
        float r_cubed = r_squared * sqrtf(r_squared);

        if (r_cubed > 0)
        {
            float force_magnitude = G * bodies[0].mass / r_cubed;
            Vector3 force_on_planet = Vector3Scale(r_vec, -force_magnitude);
            accelerations[j] = Vector3Add(accelerations[j], force_on_planet);
        }
    }
    // Calcular fuerzas Planetas-Planetas
    for (int i = 1; i <= SOLARSYSTEM_BODYNUM; i++) {
        for (int j = i + 1; j < SOLARSYSTEM_BODYNUM; j++) {  // j = i+1 evita duplicar c lculos
            Vector3 r_vec = Vector3Subtract(bodies[j].position, bodies[i].position);
            float r_squared = Vector3LengthSqr(r_vec);
            float r_cubed = r_squared * sqrtf(r_squared);

            if (r_cubed > 0) {  // Evitar divisi n por cero
                float force_magnitude = G / r_cubed;

                // Fuerza sobre el planeta j debido al planeta i
                Vector3 force_on_j = Vector3Scale(r_vec, -force_magnitude * bodies[i].mass);
                accelerations[j] = Vector3Add(accelerations[j], force_on_j);

                // Fuerza sobre el planeta i debido al planeta j (tercera ley de Newton)
                Vector3 force_on_i = Vector3Scale(r_vec, force_magnitude * bodies[j].mass);
                accelerations[i] = Vector3Add(accelerations[i], force_on_i);
            }
        }
    }

    // Calcular fuerzas Sol-Asteroides
    for (int j = SOLARSYSTEM_BODYNUM; j < n; j++) {
        Vector3 r_vec = Vector3Subtract(bodies[j].position, bodies[0].position);
        float r_squared = Vector3LengthSqr(r_vec);
        float r_cubed = r_squared * sqrtf(r_squared);

        if (r_cubed > 0) {  // Evitar divisi n por cero
            float force_magnitude = G * bodies[0].mass / r_cubed;
            Vector3 force_on_asteroid = Vector3Scale(r_vec, -force_magnitude);
            accelerations[j] = Vector3Add(accelerations[j], force_on_asteroid);
        }
    }

    // Calcular fuerzas Planetas-Asteroides (solo si est n cerca)
    const float INFLUENCE_DISTANCE_SQ = 1e15f;  // Distancia de influencia al cuadrado (ajustable)
    for (int i = 1; i < SOLARSYSTEM_BODYNUM; i++) {  // Para cada planeta
        for (int j = SOLARSYSTEM_BODYNUM; j < n; j++) {  // Para cada asteroide
            Vector3 r_vec = Vector3Subtract(bodies[j].position, bodies[i].position);
            float r_squared = Vector3LengthSqr(r_vec);

            // Solo calcular si est n suficientemente cerca
            if (r_squared < INFLUENCE_DISTANCE_SQ && r_squared > 0) {
                float r_cubed = r_squared * sqrtf(r_squared);
                float force_magnitude = G / r_cubed;

                // Fuerza sobre el asteroide debido al planeta
                Vector3 force_on_asteroid = Vector3Scale(r_vec, -force_magnitude * bodies[i].mass);
                accelerations[j] = Vector3Add(accelerations[j], force_on_asteroid);
            }
        }
    }
}

