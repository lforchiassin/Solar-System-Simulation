/**
 * @brief Implements an orbital simulation view
 * @author Marc S. Ressl
 *
 * @copyright Copyright (c) 2022-2023
 */

#include <time.h>
#include <stdio.h>
#include "view.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

Model ship;
Vector3 shipPos = { 0.0f, 0.0f, 1.0f }; // posición inicial de la nave



/**
 * @brief Converts a timestamp (number of seconds since 1/1/2022)
 *        to an ISO date ("YYYY-MM-DD")
 *
 * @param timestamp the timestamp
 * @return The ISO date (a raylib string)
 */

const char *getISODate(float timestamp){
    // Timestamp epoch: 1/1/2022
    struct tm unichEpochTM = {0, 0, 0, 1, 0, 122};

    // Convert timestamp to UNIX timestamp (number of seconds since 1/1/1970)
    time_t unixEpoch = mktime(&unichEpochTM);
    time_t unixTimestamp = unixEpoch + (time_t)timestamp;

    // Returns ISO date
    struct tm *localTM = localtime(&unixTimestamp);
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
    View* view = new View();

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "EDA Orbital Simulation");
    SetTargetFPS(fps);
    DisableCursor();

    view->camera.position = { 10.0f, 10.0f, 10.0f };
    view->camera.target = { 0.0f, 0.0f, 0.0f };
    view->camera.up = { 0.0f, 1.0f, 0.0f };
    view->camera.fovy = 45.0f;
    view->camera.projection = CAMERA_PERSPECTIVE;

    // 🚀 Cargar modelo de la nave
    ship = LoadModel("assets/ship.obj");

    printf("Material count: %d\n", ship.materialCount);

    // ASIGNAR COLORES SEGÚN LOS MATERIALES DEL MTL
    for (int i = 0; i < ship.materialCount; i++) {
        // Configurar cada material basado en el nombre o índice
        if (i == 0) { // default (primer material)
            ship.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;    // Naranja (Kd 1 0.501961 0)
        }
        else if (i == 1) { // default_propulsion
            ship.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = ORANGE;    // Cian (Kd 0 1 1)
        }
        else if (i == 2) { // default_windows
            ship.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
            ship.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = ORANGE;    // Azul transparente
        }
        else if (i == 3) { // default_saucer
            ship.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = { 255, 255, 255, 255 };  // Blanco (Kd 1 1 1)
        }
        else if (i == 4) { // default_interiors
            ship.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = { 200, 150, 100, 255 };  // Marrón claro
        }
        else {
            // Para cualquier material adicional
            ship.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = { 255, 0, 255, 255 };    // Magenta (error)
        }
    }

    printf("✅ Nave cargada con %d meshes y %d materiales\n", ship.meshCount, ship.materialCount);

    // Debug: mostrar información de materiales
    for (int i = 0; i < ship.materialCount; i++) {
        Color diffuseColor = ship.materials[i].maps[MATERIAL_MAP_DIFFUSE].color;
        printf("Material %d: Color (%d, %d, %d, %d)\n",
            i, diffuseColor.r, diffuseColor.g, diffuseColor.b, diffuseColor.a);
    }

    return view;
}



/**
 * @brief Destroys an orbital simulation view
 *
 * @param view The view
 */

void destroyView(View *view){
    CloseWindow();
    UnloadModel(ship);
    delete view;
}

/**
 * @brief Should the view still render?
 *
 * @return Should rendering continue?
 */

bool isViewRendering(View *view){
    return !WindowShouldClose();
}

#include <stdio.h>

/**
 * Renders an orbital simulation
 *
 * @param view The view
 * @param sim The orbital sim
 */

void renderView(View *view, OrbitalSim *sim){
    UpdateCamera(&view->camera, CAMERA_FREE);

    BeginDrawing();

    ClearBackground(BLACK);
    BeginMode3D(view->camera);
    

    for (int i = 0; i < sim->numBodies; i++) {
        OrbitalBody &body = sim->bodies[i];
       
        if (i < 9) {
             DrawSphere(body.position, body.radius, body.color);

        }
        else {
            DrawSphereEx(body.position, body.radius, 5, 5, body.color);
        }
       
        //DrawPoint3D(body.position, body.color);
    }

    // 🚀 Dibujar nave
    DrawModel(ship, shipPos, 0.01f, WHITE);
    DrawGrid(10, 10.0f);
    EndMode3D();

    static float timestamp = 0.0f; // O lleva un acumulador de tiempo en tu simulación
    timestamp += sim->timeStep;
    DrawText(getISODate(timestamp), WINDOW_WIDTH * 0.03, WINDOW_HEIGHT * 0.03, 20, RAYWHITE);
    DrawFPS(WINDOW_WIDTH * 0.03, WINDOW_HEIGHT * (1 - 0.05));

    
    EndDrawing();
}

