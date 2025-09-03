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

    // Dibujar cuerpos celestes
    for (int i = 0; i < sim->numBodies; i++) {
        OrbitalBody& body = sim->bodies[i];
        if (i < 9) {
            DrawSphere(body.position, body.radius, body.color);
        }
        else {
            DrawSphereEx(body.position, body.radius, 5, 5, body.color);
        }
    }

    // 🚀 Dibujar nave CON ROTACIÓN (si está inicializada)
    // 🚀 Dibujar nave CON ROTACIÓN Y PIVOT PERSONALIZADO
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

    EndDrawing();
}