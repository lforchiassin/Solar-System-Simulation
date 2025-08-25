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
#define NUM_ASTEROIDS 1000
#define SCALE_FACTOR 1E-11F

#define NUM_BODIES (NUM_ASTEROIDS + SOLARSYSTEM_BODYNUM + ALPHACENTAURISYSTEM_BODYNUM)
#define RADIUS_SCALE(r) (0.005F * logf(r))

static void calculateAccelerationsHybrid(OrbitalBody *bodies, Vector3 *accelerations, int n, float G);
static void calculateAccelerationsOptimized(OrbitalBody *bodies, Vector3 *accelerations, int n, float G);

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
    body->radius = RADIUS_SCALE(2E3F); // Typical asteroid radius: 2km
    body->position = {r * cosf(phi) * SCALE_FACTOR, 0, r * sinf(phi) * SCALE_FACTOR};
    body->velocity = {-v * sinf(phi) * SCALE_FACTOR, vy * SCALE_FACTOR, v * cosf(phi) * SCALE_FACTOR};
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
        sim->bodies[i].radius = RADIUS_SCALE(solarSystem[i].radius);
        
        sim->bodies[i].position = Vector3Scale(solarSystem[i].position, SCALE_FACTOR);
        sim->bodies[i].velocity = Vector3Scale(solarSystem[i].velocity, SCALE_FACTOR);

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

    /*for (j = 0; i < SOLARSYSTEM_BODYNUM + ALPHACENTAURISYSTEM_BODYNUM; i++, j++) {
        sim->bodies[i].mass = alphaCentauriSystem[j].mass;
        sim->bodies[i].radius = RADIUS_SCALE(alphaCentauriSystem[j].radius);

        sim->bodies[i].position = Vector3Scale(alphaCentauriSystem[j].position, SCALE_FACTOR);
        sim->bodies[i].velocity = Vector3Scale(alphaCentauriSystem[j].velocity, SCALE_FACTOR);

        sim->bodies[i].color = alphaCentauriSystem[j].color;

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
    }*/

	for (; i < sim->numBodies; i++) {
        configureAsteroid(&sim->bodies[i], solarSystem[0].mass);
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
    
    // Inicializar aceleraciones
    Vector3 * accelerations= (Vector3 *)malloc(n * sizeof(Vector3));
    if (!accelerations) return;

    calculateAccelerationsOptimized(bodies, accelerations, n, GRAVITATIONAL_CONSTANT * SCALE_FACTOR * SCALE_FACTOR * SCALE_FACTOR);

    // Paso 2: Actualizar velocidades y posiciones
    for (int i = 0; i < n; i++) {
        bodies[i].velocity = Vector3Add(bodies[i].velocity, 
            Vector3Scale(accelerations[i], dt));

        bodies[i].position = Vector3Add(bodies[i].position,
            Vector3Scale(bodies[i].velocity, dt));
        
        printf("Body %d: Position = %f, %f, %f, Velocity = %.10f, %.10f, %.10f\n",
            i,
            sim->bodies[i].position.x,
            sim->bodies[i].position.y,
            sim->bodies[i].position.z,
            sim->bodies[i].velocity.x,
            sim->bodies[i].velocity.y,
            sim->bodies[i].velocity.z);
    }
    
    free(accelerations);
}

// ========== FMM HYBRID IMPLEMENTATION ==========
// Agregar al final de orbitalSim.cpp, antes de calculateAccelerationsOptimized

#include <math.h>
#include <stdlib.h>
#include <string.h>

// FMM Tree Node
typedef struct FMMNode {
    Vector3 center;          // Center of the cell
    float size;              // Size of the cell
    float total_mass;        // Total mass in this cell
    Vector3 center_of_mass;  // Center of mass
    
    // Multipole expansion coefficients
    float monopole;          // q0 = total mass
    Vector3 dipole;          // q1 = first moment
    
    // Tree structure
    struct FMMNode *children[8]; // Octree children
    int is_leaf;
    OrbitalBody **bodies;    // Bodies in this leaf
    int num_bodies;
    int capacity;
} FMMNode;

// FMM Parameters - Más conservadores para mayor precisión
#define MAX_BODIES_PER_LEAF 6
#define THETA_THRESHOLD 0.4f
#define MIN_DISTANCE 1e-15f
#define HYBRID_THRESHOLD 50  // Cambiar a método directo si hay menos cuerpos

// Helper function to find maximum of three values
static float fmax3(float a, float b, float c) {
    float max_ab = (a > b) ? a : b;
    return (max_ab > c) ? max_ab : c;
}

// Create new FMM node
static FMMNode* create_node(Vector3 center, float size) {
    FMMNode *node = (FMMNode*)calloc(1, sizeof(FMMNode));
    if (!node) return NULL;
    
    node->center = center;
    node->size = size;
    node->is_leaf = 1;
    node->capacity = MAX_BODIES_PER_LEAF;
    node->bodies = (OrbitalBody**)malloc(sizeof(OrbitalBody*) * node->capacity);
    if (!node->bodies) {
        free(node);
        return NULL;
    }
    return node;
}

// Find bounding box for all bodies
static void find_bounds(OrbitalBody *bodies, int n, Vector3 *min_bound, Vector3 *max_bound) {
    if (n == 0) return;
    
    *min_bound = bodies[0].position;
    *max_bound = bodies[0].position;
    
    for (int i = 1; i < n; i++) {
        if (bodies[i].position.x < min_bound->x) min_bound->x = bodies[i].position.x;
        if (bodies[i].position.y < min_bound->y) min_bound->y = bodies[i].position.y;
        if (bodies[i].position.z < min_bound->z) min_bound->z = bodies[i].position.z;
        
        if (bodies[i].position.x > max_bound->x) max_bound->x = bodies[i].position.x;
        if (bodies[i].position.y > max_bound->y) max_bound->y = bodies[i].position.y;
        if (bodies[i].position.z > max_bound->z) max_bound->z = bodies[i].position.z;
    }
}

// Check if point is inside node
static int point_in_node(Vector3 point, FMMNode *node) {
    float half_size = node->size / 2.0f;
    return (point.x >= node->center.x - half_size && point.x < node->center.x + half_size &&
            point.y >= node->center.y - half_size && point.y < node->center.y + half_size &&
            point.z >= node->center.z - half_size && point.z < node->center.z + half_size);
}

// Forward declarations
static void insert_body(FMMNode *node, OrbitalBody *body);
static Vector3 compute_acceleration_recursive(OrbitalBody *target, FMMNode *node, float G);

// Subdivide node into 8 children
static void subdivide(FMMNode *node) {
    if (!node->is_leaf) return;
    
    node->is_leaf = 0;
    float quarter_size = node->size / 4.0f;
    float half_size = node->size / 2.0f;
    
    // Create 8 children (octree)
    for (int i = 0; i < 8; i++) {
        Vector3 child_center = node->center;
        child_center.x += (i & 1) ? quarter_size : -quarter_size;
        child_center.y += (i & 2) ? quarter_size : -quarter_size;
        child_center.z += (i & 4) ? quarter_size : -quarter_size;
        
        node->children[i] = create_node(child_center, half_size);
        if (!node->children[i]) return;
    }
    
    // Redistribute bodies to children
    for (int i = 0; i < node->num_bodies; i++) {
        for (int j = 0; j < 8; j++) {
            if (point_in_node(node->bodies[i]->position, node->children[j])) {
                if (node->children[j]->num_bodies < node->children[j]->capacity) {
                    node->children[j]->bodies[node->children[j]->num_bodies++] = node->bodies[i];
                }
                break;
            }
        }
    }
    
    // Recursively subdivide children if needed
    for (int i = 0; i < 8; i++) {
        if (node->children[i] && node->children[i]->num_bodies > MAX_BODIES_PER_LEAF) {
            subdivide(node->children[i]);
        }
    }
    
    free(node->bodies);
    node->bodies = NULL;
}

// Insert body into tree
static void insert_body(FMMNode *node, OrbitalBody *body) {
    if (!point_in_node(body->position, node)) return;
    
    if (node->is_leaf) {
        if (node->num_bodies < MAX_BODIES_PER_LEAF) {
            node->bodies[node->num_bodies++] = body;
        } else {
            subdivide(node);
            insert_body(node, body);
        }
    } else {
        for (int i = 0; i < 8; i++) {
            if (node->children[i]) {
                insert_body(node->children[i], body);
            }
        }
    }
}

// Build FMM tree
static FMMNode* build_tree(OrbitalBody *bodies, int n) {
    if (n == 0) return NULL;
    
    Vector3 min_bound, max_bound;
    find_bounds(bodies, n, &min_bound, &max_bound);
    
    Vector3 center = {
        (min_bound.x + max_bound.x) / 2.0f,
        (min_bound.y + max_bound.y) / 2.0f,
        (min_bound.z + max_bound.z) / 2.0f
    };
    
    float size = fmax3(max_bound.x - min_bound.x, 
                       max_bound.y - min_bound.y, 
                       max_bound.z - min_bound.z) * 1.2f;
    
    FMMNode *root = create_node(center, size);
    if (!root) return NULL;
    
    for (int i = 0; i < n; i++) {
        insert_body(root, &bodies[i]);
    }
    
    return root;
}

// Compute multipole moments for a node
static void compute_moments(FMMNode *node) {
    if (!node) return;
    
    if (node->is_leaf) {
        node->total_mass = 0.0f;
        node->center_of_mass = (Vector3){0, 0, 0};
        
        for (int i = 0; i < node->num_bodies; i++) {
            float mass = node->bodies[i]->mass;
            node->total_mass += mass;
            node->center_of_mass = Vector3Add(node->center_of_mass, 
                                            Vector3Scale(node->bodies[i]->position, mass));
        }
        
        if (node->total_mass > 0) {
            node->center_of_mass = Vector3Scale(node->center_of_mass, 1.0f / node->total_mass);
        }
        
        node->monopole = node->total_mass;
        node->dipole = (Vector3){0, 0, 0};
        for (int i = 0; i < node->num_bodies; i++) {
            Vector3 r = Vector3Subtract(node->bodies[i]->position, node->center_of_mass);
            node->dipole = Vector3Add(node->dipole, Vector3Scale(r, node->bodies[i]->mass));
        }
    } else {
        node->total_mass = 0.0f;
        node->center_of_mass = (Vector3){0, 0, 0};
        
        for (int i = 0; i < 8; i++) {
            if (node->children[i] && node->children[i]->total_mass > 0) {
                compute_moments(node->children[i]);
                
                float child_mass = node->children[i]->total_mass;
                node->total_mass += child_mass;
                node->center_of_mass = Vector3Add(node->center_of_mass,
                                                Vector3Scale(node->children[i]->center_of_mass, child_mass));
            }
        }
        
        if (node->total_mass > 0) {
            node->center_of_mass = Vector3Scale(node->center_of_mass, 1.0f / node->total_mass);
        }
        
        node->monopole = node->total_mass;
        node->dipole = (Vector3){0, 0, 0};
    }
}

// Compute force from multipole expansion
static Vector3 compute_multipole_force(Vector3 target_pos, FMMNode *node, float G) {
    Vector3 r = Vector3Subtract(target_pos, node->center_of_mass);
    float r_mag_sq = Vector3LengthSqr(r);
    
    if (r_mag_sq < MIN_DISTANCE) return (Vector3){0, 0, 0};
    
    float r_mag = sqrtf(r_mag_sq);
    Vector3 r_unit = Vector3Scale(r, 1.0f / r_mag);
    
    Vector3 force = Vector3Scale(r_unit, -G * node->monopole / r_mag_sq);
    return force;
}

// Traverse tree and compute acceleration for a single body
static Vector3 compute_acceleration_recursive(OrbitalBody *target, FMMNode *node, float G) {
    Vector3 acceleration = {0, 0, 0};
    
    if (!node || node->total_mass == 0) return acceleration;
    
    float distance = Vector3Distance(target->position, node->center_of_mass);
    
    // Criterio adaptativo más estricto para cuerpos masivos
    float adaptive_theta = THETA_THRESHOLD;
    if (node->total_mass > 1e29f) {  // Cuerpos muy masivos como el Sol
        adaptive_theta *= 0.2f;
    }
    
    if (node->is_leaf || (node->size / (distance + MIN_DISTANCE)) < adaptive_theta) {
        if (node->is_leaf) {
            // Direct computation for leaf
            const float epsilon = 1e-20f;
            
            for (int i = 0; i < node->num_bodies; i++) {
                if (node->bodies[i] != target) {
                    Vector3 r_vec = Vector3Subtract(target->position, node->bodies[i]->position);
                    float r_sq = Vector3LengthSqr(r_vec);
                    
                    if (r_sq > epsilon) {
                        float r_cubed = r_sq * sqrtf(r_sq);
                        float force_magnitude = G * node->bodies[i]->mass / r_cubed;
                        Vector3 acc = Vector3Scale(r_vec, -force_magnitude);
                        acceleration = Vector3Add(acceleration, acc);
                    }
                }
            }
        } else {
            Vector3 multipole_acc = compute_multipole_force(target->position, node, G);
            acceleration = Vector3Add(acceleration, multipole_acc);
        }
    } else {
        for (int i = 0; i < 8; i++) {
            if (node->children[i]) {
                Vector3 child_acc = compute_acceleration_recursive(target, node->children[i], G);
                acceleration = Vector3Add(acceleration, child_acc);
            }
        }
    }
    
    return acceleration;
}

// Free FMM tree
static void free_tree(FMMNode *node) {
    if (!node) return;
    
    if (!node->is_leaf) {
        for (int i = 0; i < 8; i++) {
            free_tree(node->children[i]);
        }
    } else {
        if (node->bodies) free(node->bodies);
    }
    
    free(node);
}

// ========== IMPLEMENTACIÓN HÍBRIDA ==========
static void calculateAccelerationsHybrid(OrbitalBody *bodies, Vector3 *accelerations, int n, float G) {
    const float epsilon = 1e-20f;
    
    // Inicializar todas las aceleraciones a cero
    for (int i = 0; i < n; i++) {
        accelerations[i] = (Vector3){0.0f, 0.0f, 0.0f};
    }
    
    // PASO 1: Encontrar el Sol (cuerpo más masivo - típicamente el primero)
    int sun_index = 0;
    float max_mass = bodies[0].mass;
    for (int i = 1; i < n && i < SOLARSYSTEM_BODYNUM; i++) { // Solo buscar en planetas
        if (bodies[i].mass > max_mass) {
            max_mass = bodies[i].mass;
            sun_index = i;
        }
    }
    
    // PASO 2: Sol interactúa con TODOS los cuerpos (método directo - máxima precisión)
    for (int i = 0; i < n; i++) {
        if (i != sun_index) {
            Vector3 r_vec = Vector3Subtract(bodies[i].position, bodies[sun_index].position);
            float r_sq = Vector3LengthSqr(r_vec);
            
            if (r_sq > epsilon) {
                float r_cubed = r_sq * sqrtf(r_sq);
                float force_magnitude = G * bodies[sun_index].mass / r_cubed;
                
                // Aceleración sobre el cuerpo i debido al Sol
                Vector3 acc_from_sun = Vector3Scale(r_vec, -force_magnitude);
                accelerations[i] = Vector3Add(accelerations[i], acc_from_sun);
                
                // Newton's 3rd law: aceleración sobre el Sol
                Vector3 acc_on_sun = Vector3Scale(r_vec, force_magnitude * bodies[i].mass / bodies[sun_index].mass);
                accelerations[sun_index] = Vector3Add(accelerations[sun_index], acc_on_sun);
            }
        }
    }
    
    // PASO 3: Interacciones no-Sol
    if (n < HYBRID_THRESHOLD) {
        // MÉTODO DIRECTO para sistemas pequeños
        for (int i = 0; i < n; i++) {
            if (i == sun_index) continue; // Ya procesado
            
            for (int j = i + 1; j < n; j++) {
                if (j == sun_index) continue; // Ya procesado
                
                Vector3 r_vec = Vector3Subtract(bodies[j].position, bodies[i].position);
                float r_sq = Vector3LengthSqr(r_vec);
                
                if (r_sq > epsilon) {
                    float r_cubed = r_sq * sqrtf(r_sq);
                    float force_magnitude = G * bodies[i].mass * bodies[j].mass / r_cubed;
                    
                    Vector3 force_on_i = Vector3Scale(r_vec, force_magnitude / bodies[i].mass);
                    Vector3 force_on_j = Vector3Scale(r_vec, -force_magnitude / bodies[j].mass);

                    accelerations[i] = Vector3Add(accelerations[i], force_on_i);
                    accelerations[j] = Vector3Add(accelerations[j], force_on_j);
                }
            }
        }
    } else {
        // MÉTODO FMM para sistemas grandes (asteroides entre sí)
        
        // Separar planetas y asteroides
        int num_planets = SOLARSYSTEM_BODYNUM;
        int num_asteroids = n - num_planets;
        
        // Interacciones planeta-planeta (método directo)
        for (int i = 0; i < num_planets; i++) {
            if (i == sun_index) continue;
            
            for (int j = i + 1; j < num_planets; j++) {
                if (j == sun_index) continue;
                
                Vector3 r_vec = Vector3Subtract(bodies[j].position, bodies[i].position);
                float r_sq = Vector3LengthSqr(r_vec);
                
                if (r_sq > epsilon) {
                    float r_cubed = r_sq * sqrtf(r_sq);
                    float force_magnitude = G * bodies[i].mass * bodies[j].mass / r_cubed;
                    
                    Vector3 force_on_i = Vector3Scale(r_vec, force_magnitude / bodies[i].mass);
                    Vector3 force_on_j = Vector3Scale(r_vec, -force_magnitude / bodies[j].mass);

                    accelerations[i] = Vector3Add(accelerations[i], force_on_i);
                    accelerations[j] = Vector3Add(accelerations[j], force_on_j);
                }
            }
        }
        
        // Interacciones planeta-asteroide (método directo)
        for (int i = 0; i < num_planets; i++) {
            if (i == sun_index) continue;
            
            for (int j = num_planets; j < n; j++) {
                Vector3 r_vec = Vector3Subtract(bodies[j].position, bodies[i].position);
                float r_sq = Vector3LengthSqr(r_vec);
                
                if (r_sq > epsilon) {
                    float r_cubed = r_sq * sqrtf(r_sq);
                    float force_magnitude = G * bodies[i].mass * bodies[j].mass / r_cubed;
                    
                    Vector3 force_on_i = Vector3Scale(r_vec, force_magnitude / bodies[i].mass);
                    Vector3 force_on_j = Vector3Scale(r_vec, -force_magnitude / bodies[j].mass);

                    accelerations[i] = Vector3Add(accelerations[i], force_on_i);
                    accelerations[j] = Vector3Add(accelerations[j], force_on_j);
                }
            }
        }
        
        // Interacciones asteroide-asteroide (FMM)
        if (num_asteroids > 8) {
            OrbitalBody *asteroids = &bodies[num_planets];
            FMMNode *root = build_tree(asteroids, num_asteroids);
            
            if (root) {
                compute_moments(root);
                
                for (int i = 0; i < num_asteroids; i++) {
                    Vector3 fmm_acc = compute_acceleration_recursive(&asteroids[i], root, G);
                    accelerations[num_planets + i] = Vector3Add(accelerations[num_planets + i], fmm_acc);
                }
                
                free_tree(root);
            } else {
                // Fallback a método directo si falla FMM
                for (int i = num_planets; i < n; i++) {
                    for (int j = i + 1; j < n; j++) {
                        Vector3 r_vec = Vector3Subtract(bodies[j].position, bodies[i].position);
                        float r_sq = Vector3LengthSqr(r_vec);
                        
                        if (r_sq > epsilon) {
                            float r_cubed = r_sq * sqrtf(r_sq);
                            float force_magnitude = G * bodies[i].mass * bodies[j].mass / r_cubed;
                            
                            Vector3 force_on_i = Vector3Scale(r_vec, force_magnitude / bodies[i].mass);
                            Vector3 force_on_j = Vector3Scale(r_vec, -force_magnitude / bodies[j].mass);

                            accelerations[i] = Vector3Add(accelerations[i], force_on_i);
                            accelerations[j] = Vector3Add(accelerations[j], force_on_j);
                        }
                    }
                }
            }
        }
    }
}

static void calculateAccelerationsOptimized(OrbitalBody *bodies, Vector3 *accelerations, int n, float G)
{
    const float epsilon = 1e-20f;
    
    // Inicializar aceleraciones
    for (int i = 0; i < n; i++) {
        accelerations[i] = {0.0f, 0.0f, 0.0f};
    }
    
    // Calcular fuerzas entre pares únicos
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            Vector3 r_vec = Vector3Subtract(bodies[j].position, bodies[i].position);
            float r_squared = Vector3LengthSqr(r_vec);

            if (r_squared > epsilon) {
                float r_cubed = r_squared * sqrtf(r_squared);

                float force_magnitude = G * bodies[i].mass * bodies[j].mass / r_cubed;
                
                // Aplicar fuerzas iguales y opuestas
                Vector3 force_on_i = Vector3Scale(r_vec, force_magnitude / bodies[i].mass);
                Vector3 force_on_j = Vector3Scale(r_vec, -force_magnitude / bodies[j].mass);
                
                accelerations[i] = Vector3Add(accelerations[i], force_on_i);
                accelerations[j] = Vector3Add(accelerations[j], force_on_j);
            }
        }
    }
}