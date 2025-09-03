/**
 * @brief Implements an orbital simulation view
 * @author Marc S. Ressl
 *
 * @copyright Copyright (c) 2022-2023
 */

#include <time.h>
#include <stdio.h>
#include "view.h"
#include "raymath.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define SCALE_FACTOR 1E-11F
#define RADIUS_SCALE(r) (0.005F * logf(r))

 /**
  * @brief Converts a timestamp (number of seconds since 1/1/2022)
  *        to an ISO date ("YYYY-MM-DD")
  *
  * @param timestamp the timestamp
  * @return The ISO date (a raylib string)
  */

const char* getISODate(float timestamp) {
    // Timestamp epoch: 1/1/2022
    struct tm unichEpochTM = { 0, 0, 0, 1, 0, 122 };

    // Convert timestamp to UNIX timestamp (number of seconds since 1/1/1970)
    time_t unixEpoch = mktime(&unichEpochTM);
    time_t unixTimestamp = unixEpoch + (time_t)timestamp;

    // Returns ISO date
    struct tm* localTM = localtime(&unixTimestamp);
    return TextFormat("%04d-%02d-%02d",
        1900 + localTM->tm_year, localTM->tm_mon + 1, localTM->tm_mday);
}

/**
 * @brief Constructs an orbital simulation view
 *
 * @param fps Frames per second for the view
 * @return The view
 */

View* constructView(int fps) {
    // Variables estáticas para el modelo de la nave
    static Model ship;
    static bool shipLoaded = false;

    View* view = new View();

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "EDA Orbital Simulation");
    SetTargetFPS(fps);
    DisableCursor();

    view->camera.position = { 10.0f, 10.0f, 10.0f };
    view->camera.target = { 0.0f, 0.0f, 0.0f };
    view->camera.up = { 0.0f, 1.0f, 0.0f };
    view->camera.fovy = 45.0f;
    view->camera.projection = CAMERA_PERSPECTIVE;

    // 🚀 Cargar modelo de la nave solo una vez
    if (!shipLoaded) {
        ship = LoadModel("assets/ship.obj");
        shipLoaded = true;
    }

    return view;
}

/**
 * @brief Destroys an orbital simulation view
 *
 * @param view The view
 */

void destroyView(View* view) {
    // Variables estáticas para controlar la limpieza
    static bool shipUnloaded = false;

    if (!shipUnloaded) {
        // Solo descargar el modelo una vez
        // Nota: El modelo está en una variable estática en constructView,
        // así que necesitamos una forma de acceder a él para descargarlo
        shipUnloaded = true;
    }

    CloseWindow();
    delete view;
}

/**
 * @brief Should the view still render?
 *
 * @return Should rendering continue?
 */

bool isViewRendering(View* view) {
    return !WindowShouldClose();
}

/**
 * Renders an orbital simulation
 *
 * @param view The view
 * @param sim The orbital sim
 */

void renderView(View* view, OrbitalSim* sim) {
    static float lodMultiplier = 1.0f;
    if (IsKeyPressed(KEY_E)) lodMultiplier *= 1.2f;     // + para ver más asteroides
    if (IsKeyPressed(KEY_X)) lodMultiplier *= 0.8f;     // - para ver menos asteroides
    if (IsKeyPressed(KEY_R)) lodMultiplier = 1.0f;      // R para resetear

    // Para la rotación y posición de la nave
    static float shipRotationAngle = 0.0f;
    static float shipRotationSpeed = 90.0f; // grados por segundo
    static Vector3 shipPos;
    // Variable estática para acceder al modelo de la nave
    static Model* shipPtr = nullptr;
    static bool shipInitialized = false;

    UpdateCamera(&view->camera, CAMERA_FREE);

    // Inicializar referencia al modelo (solo la primera vez)
    if (!shipInitialized) {
        // Aquí necesitaríamos una forma de obtener la referencia al modelo
        // Por ahora, cargaremos el modelo aquí también
        static Model ship = LoadModel("assets/ship.obj");
        shipPtr = &ship;
        shipInitialized = true;

        // Configurar materiales (copiado de constructView)
        for (int i = 0; i < ship.materialCount; i++) {
            if (i == 0) {
                ship.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
            }
            else if (i == 1) {
                ship.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = BLUE;
            }
            else if (i == 2) {
                ship.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
            }
            else if (i == 3) {
                ship.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
            }
            else if (i == 4) {
                ship.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
            }
            else {
                ship.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
            }
        }
    }

    // Actualizar rotación de la nave
    shipRotationAngle += shipRotationSpeed * GetFrameTime();
    if (shipRotationAngle >= 360.0f) {
        shipRotationAngle -= 360.0f; // Resetear para evitar números muy grandes
    }

    // Calcular posición de la nave relativa a la cámara
    Vector3 cameraForward = Vector3Normalize(Vector3Subtract(view->camera.target, view->camera.position));
    Vector3 cameraRight = Vector3Normalize(Vector3CrossProduct(cameraForward, view->camera.up));
    Vector3 cameraUp = Vector3CrossProduct(cameraRight, cameraForward);



    // Posicionar la nave un poco adelante y abajo de la cámara
    float distanceAhead = 7.0f;    // Distancia hacia adelante
    float distanceDown = -1.0f;    // Distancia hacia abajo
    float distanceRight = 0.0f;    // Distancia hacia la derecha



    shipPos = Vector3Add(view->camera.position,
        Vector3Add(Vector3Add(Vector3Scale(cameraForward, distanceAhead),
            Vector3Scale(cameraUp, distanceDown)),
            Vector3Scale(cameraRight, distanceRight)));

    BeginDrawing();
    ClearBackground(BLACK);
    BeginMode3D(view->camera);

    // LOD dinámico ajustable
    float baseLOD = (10.0f / tanf(view->camera.fovy * 0.5f * DEG2RAD)) * lodMultiplier;
    const float LOD_HIGH = baseLOD * 0.3f;
    const float LOD_MEDIUM = baseLOD * 0.8f;
    const float LOD_LOW = baseLOD * 2.0f;
    const float LOD_CULL = baseLOD * 5.0f;

    // Planetas con distancias aún más generosas
    const float PLANET_LOD_CULL = baseLOD * 15.0f;

    int rendered_planets = 0;
    int rendered_asteroids = 0;

    for (int i = 0; i < sim->numBodies; i++) {
        OrbitalBody& body = sim->bodies[i];
        Vector3 scaledPosition = Vector3Scale(body.position, SCALE_FACTOR);
        float distance = Vector3Distance(view->camera.position, scaledPosition);

        if (i < 9) {
            // PLANETAS - 6 niveles de LOD
            if (distance > PLANET_LOD_CULL) continue;

            float radius = RADIUS_SCALE(body.radius);
            float relativeDistance = distance / PLANET_LOD_CULL; // 0.0 = muy cerca, 1.0 = muy lejos

            if (relativeDistance < 0.1f) {
                // Nivel 0 - Máximo detalle
                DrawSphere(scaledPosition, radius, body.color);
            }
            else if (relativeDistance < 0.2f) {
                // Nivel 1 - Alta resolución
                DrawSphereEx(scaledPosition, radius, 20, 20, body.color);
            }
            else if (relativeDistance < 0.4f) {
                // Nivel 2 - Resolución media-alta
                DrawSphereEx(scaledPosition, radius * 0.95f, 16, 16, body.color);
            }
            else if (relativeDistance < 0.6f) {
                // Nivel 3 - Resolución media
                DrawSphereEx(scaledPosition, radius * 0.9f, 12, 12, body.color);
            }
            else if (relativeDistance < 0.8f) {
                // Nivel 4 - Resolución baja
                DrawSphereEx(scaledPosition, radius * 0.8f, 8, 8, body.color);
            }
            else {
                // Nivel 5 - Resolución mínima
                DrawSphereEx(scaledPosition, radius * 0.7f, 6, 6, body.color);
            }
            rendered_planets++;
        }
        else {
            // ASTEROIDES - 5 niveles de LOD
            if (distance > LOD_CULL) continue;

            float lodFactor = 1.0f;
            float relativeDistance = distance / LOD_CULL;

            // Calcular factor LOD basado en distancia
            if (relativeDistance > 0.8f) lodFactor = 0.05f;      // 1/20
            else if (relativeDistance > 0.6f) lodFactor = 0.1f;  // 1/10
            else if (relativeDistance > 0.4f) lodFactor = 0.25f; // 1/4
            else if (relativeDistance > 0.2f) lodFactor = 0.5f;  // 1/2
            // else lodFactor = 1.0f (todos)

            if (((i * 73 + 17) % 1000) < (int)(lodFactor * 1000)) {
                float asteroidRadius = RADIUS_SCALE(body.radius) * 0.3f;

                if (relativeDistance < 0.1f) {
                    // Nivel 0 - Esfera completa
                    DrawSphere(scaledPosition, asteroidRadius, body.color);
                }
                else if (relativeDistance < 0.3f) {
                    // Nivel 1 - Esfera media resolución
                    DrawSphereEx(scaledPosition, asteroidRadius, 10, 10, body.color);
                }
                else if (relativeDistance < 0.5f) {
                    // Nivel 2 - Esfera baja resolución
                    DrawSphereEx(scaledPosition, asteroidRadius * 0.8f, 6, 6, body.color);
                }
                else if (relativeDistance < 0.7f) {
                    // Nivel 3 - Esfera muy simple
                    DrawSphereEx(scaledPosition, asteroidRadius * 0.6f, 4, 4, body.color);
                }
                else {
                    // Nivel 4 - Solo punto
                    DrawPoint3D(scaledPosition, body.color);
                }
                rendered_asteroids++;
            }
        }
    }

    //Dibujar nave CON ROTACIÓN (si está inicializada)
    //Dibujar nave CON ROTACIÓN Y PIVOT PERSONALIZADO
    if (shipPtr != nullptr) {
        // Definir el offset del pivot (relativo al centro del modelo)
        Vector3 pivotOffset = { 0.0f, -2.0f, -1.5f }; // Por ejemplo, rotar desde 2 unidades abajo

        // 1. Trasladar al pivot
        Vector3 pivotPosition = Vector3Add(shipPos, pivotOffset);

        // 2. Aplicar rotación alrededor del pivot
        Matrix rotationMatrix = MatrixRotateY(shipRotationAngle * DEG2RAD);

        // 3. Calcular la posición final (pivot + rotación del offset)
        Vector3 rotatedOffset = Vector3Transform(Vector3Negate(pivotOffset), rotationMatrix);
        Vector3 finalPosition = Vector3Add(pivotPosition, rotatedOffset);

        Vector3 rotationAxis = { 0.0f, 1.0f, 0.0f };
        Vector3 shipScale = { 0.01f, 0.01f, 0.01f };

        DrawModelEx(*shipPtr, finalPosition, rotationAxis, shipRotationAngle, shipScale, WHITE);
    }

    DrawGrid(10, 10.0f);
    EndMode3D();

    static float timestamp = 0.0f;
    timestamp += sim->timeStep;
    DrawText(getISODate(timestamp), WINDOW_WIDTH * 0.03, WINDOW_HEIGHT * 0.03, 20, RAYWHITE);
    DrawFPS(WINDOW_WIDTH * 0.03, WINDOW_HEIGHT * (1 - 0.05));
    DrawText(TextFormat("Planets: %d/9", rendered_planets),
        WINDOW_WIDTH * 0.03, WINDOW_HEIGHT * 0.08, 16, RAYWHITE);
    DrawText(TextFormat("Asteroids: %d/%d", rendered_asteroids, sim->numBodies - 9),
        WINDOW_WIDTH * 0.03, WINDOW_HEIGHT * 0.11, 16, RAYWHITE);
    DrawText(TextFormat("LOD Multiplier: %.2f", lodMultiplier),
        WINDOW_WIDTH * 0.03, WINDOW_HEIGHT * 0.14, 16, RAYWHITE);
    DrawText("Controls: +(E)/-(X) adjust LOD, R reset",
        WINDOW_WIDTH * 0.03, WINDOW_HEIGHT * 0.17, 14, GRAY);

    EndDrawing();
}