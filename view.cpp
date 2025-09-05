/**
 * @brief Implements an orbital simulation view with enhanced UI and menu system
 * @author Marc S. Ressl (Enhanced by Claude)
 *
 * @copyright Copyright (c) 2022-2023
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "view.h"
#include "raymath.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define SCALE_FACTOR 1E-11F
#define RADIUS_SCALE(r) (0.005F * logf(r))

 // UI Colors and styling
#define UI_PRIMARY_COLOR Color{0, 255, 255, 255}      // Cyan
#define UI_SECONDARY_COLOR Color{0, 128, 255, 255}    // Blue
#define UI_ACCENT_COLOR Color{255, 255, 255, 255}     // White
#define UI_BACKGROUND Color{10, 25, 47, 240}          // Dark blue with transparency
#define UI_PANEL_BG Color{15, 25, 40, 220}            // Panel background
#define UI_TEXT_PRIMARY Color{255, 255, 255, 255}     // White text
#define UI_TEXT_SECONDARY Color{200, 200, 200, 180}   // Gray text
#define UI_SUCCESS_COLOR Color{0, 255, 0, 255}        // Green
#define UI_WARNING_COLOR Color{255, 255, 0, 255}      // Yellow
#define UI_ERROR_COLOR Color{255, 100, 100, 255}      // Light red

// UI Layout constants
#define PANEL_PADDING 20
#define PANEL_MARGIN 30
#define BUTTON_HEIGHT 35
#define BUTTON_SPACING 8
#define STAT_BOX_SIZE 120

// Menu state structure
typedef struct {
    bool isOpen;
    SystemType selectedSystem;
    EasterEggType selectedEasterEgg;
    DispersionType selectedDispersion;
    bool showConfirmReset;
    float animationTime;
    float confirmDialogTimer;

    // Asteroid controls
    char asteroidCountText[8];  // String for text input
    int asteroidCount;          // Actual number
    bool asteroidInputActive;   // Is text field being edited
    int cursorPosition;         // Cursor position in text field
    float cursorBlinkTimer;     // Cursor blink animation
} MenuState;

// UI Animation state
typedef struct {
    float rotation;
    float pulse;
    float glow;
    float uiTime;
} UIAnimationState;

static UIAnimationState uiAnim = { 0 };
static MenuState menuState = {
    false,                    // isOpen
    SYSTEM_TYPE_SOLAR,       // selectedSystem
    EASTER_EGG_NONE,         // selectedEasterEgg
    DISPERSION_NORMAL,       // selectedDispersion
    false,                   // showConfirmReset
    0.0f,                    // animationTime
    0.0f,                    // confirmDialogTimer
    "1000",                  // asteroidCountText
    1000,                    // asteroidCount
    false,                   // asteroidInputActive
    4,                       // cursorPosition
    0.0f                     // cursorBlinkTimer
};

typedef struct {
    Model model;
    bool isLoaded;
    Vector3 localRotation;     // local spaceship rotation
    Vector3 scale;
	Vector3 relativePosition;  // relative position to camera
    float rotationSpeed;
    bool isInitialized;
} ShipRenderer;

// global ship renderer instance
static ShipRenderer shipRenderer = { 0 };

// Forward declarations for UI functions
static void DrawEnhancedTopHUD(OrbitalSim* sim, float timestamp);
static void DrawEnhancedLeftPanel(OrbitalSim* sim, float lodMultiplier, int rendered_planets, int rendered_asteroids);
static void DrawEnhancedRightPanel(void);
static void DrawEnhancedBottomHUD(int fps);
static void DrawPanelBackground(Rectangle rect, Color color);
static void DrawStatBox(Rectangle rect, const char* value, const char* label, Color accentColor);
static void DrawButton(Rectangle rect, const char* text, bool isPressed, Color color);
static void DrawTextInput(Rectangle rect, const char* text, bool isActive, const char* label);
static Rectangle GetCenteredRect(float x, float y, float width, float height);
static bool IsMouseInside(Rectangle rect);
static void DrawMainMenu(OrbitalSim* sim);
static void HandleMenuInput(OrbitalSim* sim);
static void HandleTextInput(void);
static void InitializeSystem(OrbitalSim* sim);
static void InitializeShip(void);
static void UpdateShipRotation(float deltaTime);
static Vector3 CalculateShipWorldPosition(Camera3D* camera);
static void RenderShip(Camera3D* camera);
static void CleanupShip(void);

/**
 * @brief Converts a timestamp to an ISO date
 */
const char* getISODate(float timestamp) {
    struct tm unichEpochTM = { 0, 0, 0, 1, 0, 122 };
    time_t unixEpoch = mktime(&unichEpochTM);
    time_t unixTimestamp = unixEpoch + (time_t)timestamp;
    struct tm* localTM = localtime(&unixTimestamp);
    return TextFormat("%04d-%02d-%02d",
        1900 + localTM->tm_year, localTM->tm_mon + 1, localTM->tm_mday);
}

/**
 * @brief Constructs an orbital simulation view
 */
View* constructView(int fps) {
    static Model ship;
    static bool shipLoaded = false;

    View* view = new View();

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "EDA Orbital Simulation - Enhanced");
    SetTargetFPS(fps);
    DisableCursor();

    view->camera.position = { 15.0f, 15.0f, 15.0f };
    view->camera.target = { 0.0f, 0.0f, 0.0f };
    view->camera.up = { 0.0f, 1.0f, 0.0f };
    view->camera.fovy = 45.0f;
    view->camera.projection = CAMERA_PERSPECTIVE;

    if (!shipLoaded) {
        ship = LoadModel("assets/Ufo.obj");
        shipLoaded = true;
    }

    return view;
}

/**
 * @brief Destroys an orbital simulation view
 */
void destroyView(View* view) {
    CleanupShip();  
    CloseWindow();
    delete view;
}

/**
 * @brief Should the view still render?
 */
bool isViewRendering(View* view) {
    return !WindowShouldClose();
}

/**
 * @brief Main render function with enhanced UI
 */
void renderView(View* view, OrbitalSim* sim, int reset) {
    static float timestamp = 0.0f;

    static bool beamActive = false;
    static float beamTimer = 0.0f;
    static Vector3 beamStartPos = { 0 };
    static Vector3 beamEndPos = { 0 };

    if (reset)
    {
        timestamp = 0.0f;
        return;
    }

    // Update UI animations
    uiAnim.uiTime = GetTime();
    uiAnim.rotation += 45.0f * GetFrameTime();
    uiAnim.pulse = (sinf(uiAnim.uiTime * 2.0f) + 1.0f) * 0.5f;

    menuState.animationTime += GetFrameTime();
    menuState.cursorBlinkTimer += GetFrameTime();

	// Update simulation timestamp
    if (menuState.showConfirmReset) {
        menuState.confirmDialogTimer += GetFrameTime();
    }

    static float lodMultiplier = 0.8f;

    // Handle menu input
    HandleMenuInput(sim);

    // Handle text input when menu is open
    if (menuState.isOpen && menuState.asteroidInputActive) {
        HandleTextInput();
    }

    // Handle input only when menu is not open
    if (!menuState.isOpen) {
        if (IsKeyPressed(KEY_ONE)) lodMultiplier *= 1.2f;
        if (IsKeyPressed(KEY_TWO)) lodMultiplier *= 0.8f;
        if (IsKeyPressed(KEY_R)) lodMultiplier = 1.0f;

        if (IsKeyPressed(KEY_K) && !sim->blackHole.isActive) {
			Vector3 shipPos = CalculateShipWorldPosition(&view->camera);
            beamActive = true;
            beamTimer = 0.0f;
            beamStartPos = shipPos;
            beamEndPos = Vector3{ shipPos.x, 0.0f, shipPos.z };
        }
    }

	// initialize and update ship
    InitializeShip();
    UpdateShipRotation(GetFrameTime());

    if (!menuState.isOpen && !beamActive) {
        UpdateCamera(&view->camera, CAMERA_FREE);
    }


    BeginDrawing();
    ClearBackground(BLACK);

    BeginMode3D(view->camera);

    // LOD calculations
    float baseLOD = (10.0f / tanf(view->camera.fovy * 0.5f * DEG2RAD)) * lodMultiplier;
    const float PLANET_LOD_CULL = baseLOD * 15.0f;
    const float LOD_CULL = baseLOD * 5.0f;

    int rendered_planets = 0;
    int rendered_asteroids = 0;

    // Render celestial bodies with LOD (código existente...)
    for (int i = 0; i < sim->numBodies; i++) {
        OrbitalBody& body = sim->bodies[i];
        if (!body.isAlive) continue;

        Vector3 scaledPosition = Vector3Scale(body.position, SCALE_FACTOR);
        float distance = Vector3Distance(view->camera.position, scaledPosition);

        if (i < sim->systemBodies) { // System bodies (planets/stars)
            if (distance > PLANET_LOD_CULL) continue;
            float radius = RADIUS_SCALE(body.radius);
            float relativeDistance = distance / PLANET_LOD_CULL;

            if (relativeDistance < 0.1f) {
                DrawSphere(scaledPosition, radius, body.color);
            }
            else if (relativeDistance < 0.4f) {
                DrawSphereEx(scaledPosition, radius * 0.95f, 16, 16, body.color);
            }
            else if (relativeDistance < 0.8f) {
                DrawSphereEx(scaledPosition, radius * 0.8f, 8, 8, body.color);
            }
            else {
                DrawSphereEx(scaledPosition, radius * 0.7f, 6, 6, body.color);
            }
            rendered_planets++;
        }
        else { // Asteroids
            if (distance > LOD_CULL) continue;
            float relativeDistance = distance / LOD_CULL;
            float lodFactor = (relativeDistance > 0.8f) ? 0.05f :
                (relativeDistance > 0.4f) ? 0.25f : 1.0f;

            if (((i * 73 + 17) % 1000) < (int)(lodFactor * 1000)) {
                float asteroidRadius = RADIUS_SCALE(body.radius) * 0.3f;
                if (relativeDistance < 0.3f) {
                    DrawSphereEx(scaledPosition, asteroidRadius, 10, 10, body.color);
                }
                else if (relativeDistance < 0.7f) {
                    DrawSphereEx(scaledPosition, asteroidRadius * 0.6f, 4, 4, body.color);
                }
                else {
                    DrawPoint3D(scaledPosition, body.color);
                }
                rendered_asteroids++;
            }
        }
    }

    // Enhanced Black Hole Rendering (código existente...)
    if (sim->blackHole.isActive) {
        Vector3 blackHoleScaledPos = Vector3Scale(sim->blackHole.position, SCALE_FACTOR);
        double eventHorizonScaledRadius = RADIUS_SCALE(sim->blackHole.radius) * 2;

        // Accretion disk
        for (int layer = 0; layer < 3; layer++) {
            float layerRadius = eventHorizonScaledRadius * (2.0f + layer * 0.8f);
            Color layerColor = (layer == 0) ? Color{ 255, 255, 255, 200 } :
                (layer == 1) ? Color{ 255, 200, 100, 180 } :
                Color{ 255, 100, 0, 140 };

            int particleCount = 32 / (layer + 1);
            for (int i = 0; i < particleCount; i++) {
                float angle = (uiAnim.rotation + i * 360.0f / particleCount) * DEG2RAD;
                Vector3 particlePos = {
                    blackHoleScaledPos.x + layerRadius * cosf(angle),
                    blackHoleScaledPos.y + sinf(angle * 3.0f + uiAnim.uiTime) * layerRadius * 0.1f,
                    blackHoleScaledPos.z + layerRadius * sinf(angle)
                };
                DrawSphere(particlePos, 0.05f, layerColor);
            }
        }

        // Event horizon
        DrawSphere(blackHoleScaledPos, eventHorizonScaledRadius, BLACK);
    }

	// spaceship rendering
    RenderShip(&view->camera);

    if (beamActive) {
        beamTimer += GetFrameTime();

        // Color violeta pulsante
        float pulse = (sinf(GetTime() * 20.0f) + 1.0f) * 0.5f;
        Color violet = { 200, (unsigned char)(pulse * 100), 255, 200 };

        DrawCylinderEx(
            beamStartPos,
            beamEndPos,
            0.2f,  // radio superior
            0.2f,  // radio inferior
            16,
            violet
        );

        // Cuando pasa 1 segundo, spawnea el agujero negro
        if (beamTimer > 1.0f) {
            Vector3 blackHolePos = Vector3Scale(beamEndPos, 1.0f / SCALE_FACTOR);
            createBlackHole(sim, blackHolePos);
            beamActive = false;
        }
    }

    DrawGrid(10, 10.0f);
    EndMode3D();

    // Update timestamp
    timestamp += sim->timeStep * UPDATEPERFRAME;

	static bool f3PressedLastFrame = true;
    if (IsKeyPressed(KEY_F3)) f3PressedLastFrame = !f3PressedLastFrame;

    // Draw Enhanced UI Elements (resto del código existente...)
    if (!menuState.isOpen) {
        DrawEnhancedTopHUD(sim, timestamp);

        if (f3PressedLastFrame)
        {
            DrawEnhancedLeftPanel(sim, lodMultiplier, rendered_planets, rendered_asteroids);
            DrawEnhancedRightPanel();
        }
        DrawEnhancedBottomHUD(GetFPS());
    }

    // Draw main menu if open
    if (menuState.isOpen) {
        DrawMainMenu(sim);
    }

    EndDrawing();
}

/**
 * @brief Handle text input for asteroid count
 */
static void HandleTextInput(void) {
    int key = GetCharPressed();

    // Handle character input
    while (key > 0) {
        if (key >= 48 && key <= 57 && strlen(menuState.asteroidCountText) < 6) { // Numbers 0-9
            int len = strlen(menuState.asteroidCountText);
            if (menuState.cursorPosition <= len) {
                // Insert character at cursor position
                for (int i = len; i >= menuState.cursorPosition; i--) {
                    menuState.asteroidCountText[i + 1] = menuState.asteroidCountText[i];
                }
                menuState.asteroidCountText[menuState.cursorPosition] = (char)key;
                menuState.cursorPosition++;
            }
        }
        key = GetCharPressed();
    }

    // Handle special keys
    if (IsKeyPressed(KEY_BACKSPACE) && menuState.cursorPosition > 0) {
        menuState.cursorPosition--;
        int len = strlen(menuState.asteroidCountText);
        for (int i = menuState.cursorPosition; i < len; i++) {
            menuState.asteroidCountText[i] = menuState.asteroidCountText[i + 1];
        }
    }

    if (IsKeyPressed(KEY_DELETE)) {
        int len = strlen(menuState.asteroidCountText);
        if (menuState.cursorPosition < len) {
            for (int i = menuState.cursorPosition; i < len; i++) {
                menuState.asteroidCountText[i] = menuState.asteroidCountText[i + 1];
            }
        }
    }

    if (IsKeyPressed(KEY_LEFT) && menuState.cursorPosition > 0) {
        menuState.cursorPosition--;
    }

    if (IsKeyPressed(KEY_RIGHT)) {
        int len = strlen(menuState.asteroidCountText);
        if (menuState.cursorPosition < len) {
            menuState.cursorPosition++;
        }
    }

    if (IsKeyPressed(KEY_HOME)) {
        menuState.cursorPosition = 0;
    }

    if (IsKeyPressed(KEY_END)) {
        menuState.cursorPosition = strlen(menuState.asteroidCountText);
    }

    // Update asteroid count from text
    int newCount = atoi(menuState.asteroidCountText);
    if (newCount < 0) newCount = 0;
    if (newCount > 5000) {
        newCount = 5000;
        strcpy(menuState.asteroidCountText, "5000");
        menuState.cursorPosition = 4;
    }
    menuState.asteroidCount = newCount;

    // Deactivate input if Enter is pressed or clicked outside
    if (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        menuState.asteroidInputActive = false;
    }
}

/**
 * @brief Inicialize the ship model and settings
 */
static void InitializeShip(void) {
    if (shipRenderer.isInitialized) return;

    // Loading space model
    shipRenderer.model = LoadModel("assets/Ufo.obj");
    shipRenderer.isLoaded = IsModelValid(shipRenderer.model);

    if (shipRenderer.isLoaded) {
		// Setting material properties
        for (int i = 0; i < shipRenderer.model.materialCount; i++) {
           
            shipRenderer.model.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = (i == 1) ? BLUE : WHITE;

            
            shipRenderer.model.materials[i].shader = LoadShader(NULL, NULL); 
        }
        if (shipRenderer.isLoaded) {
            printf("✅ Modelo cargado correctamente\n");
        }
        else {
            printf("❌ Error: No se pudo cargar el modelo ship.obj\n");
            printf("   Verifica que el archivo existe en: assets/ship.obj\n");
        }

        for (int i = 0; i < shipRenderer.model.meshCount; i++) {
            if (shipRenderer.model.meshes[i].tangents == NULL) {
                GenMeshTangents(&shipRenderer.model.meshes[i]);
            }
        }
    }

    // Initial settings
    shipRenderer.localRotation = Vector3{ 0.0f, 0.0f, 0.0f };
    shipRenderer.scale = Vector3{ 0.08f, 0.08f, 0.08f };
    shipRenderer.relativePosition = Vector3{ 2, -0.2, 0 }; 
    shipRenderer.rotationSpeed = 150.0f; 
    shipRenderer.isInitialized = true;
}

/**
 * @brief Updates the ship's rotation over time
 */
static void UpdateShipRotation(float deltaTime) {
    if (!shipRenderer.isInitialized || !shipRenderer.isLoaded) return;

	// Rotation around local Y axis
    shipRenderer.localRotation.y += shipRenderer.rotationSpeed * deltaTime;

	// Keep angle within 0-360 degrees
    if (shipRenderer.localRotation.y >= 360.0f) {
        shipRenderer.localRotation.y -= 360.0f;
    }
}

/**
 * @brief Calculates the ship's world position based on camera orientation
 */
static Vector3 CalculateShipWorldPosition(Camera3D* camera) {
	// Calculate camera basis vectors
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera->target, camera->position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera->up));
    Vector3 up = Vector3Normalize(Vector3CrossProduct(right, forward));

    // Calculate world position
    Vector3 worldPos = camera->position;
    worldPos = Vector3Add(worldPos, Vector3Scale(forward, shipRenderer.relativePosition.x));
    worldPos = Vector3Add(worldPos, Vector3Scale(right, shipRenderer.relativePosition.z));
    worldPos = Vector3Add(worldPos, Vector3Scale(up, shipRenderer.relativePosition.y));

    return worldPos;
}

/**
 * @brief renders the ship model
 */
static void RenderShip(Camera3D* camera) {
    if (!shipRenderer.isInitialized || !shipRenderer.isLoaded) return;

    Vector3 worldPosition = CalculateShipWorldPosition(camera);
	Vector3 rotationAxis = { 0.0f, 1.0f, 0.0f }; // Y axis for local rotation
    float rotationAngle = shipRenderer.localRotation.y; 

    DrawModelEx(
        shipRenderer.model,
        worldPosition,
        rotationAxis,
        rotationAngle,
        shipRenderer.scale,
        WHITE
    );
}

/**
 * @brief Cleans up ship resources
 */
static void CleanupShip(void) {
    if (shipRenderer.isLoaded) {
        UnloadModel(shipRenderer.model);
        shipRenderer.isLoaded = false;
    }
    shipRenderer.isInitialized = false;
}

/**
 * @brief Handle menu input
 */
static void HandleMenuInput(OrbitalSim* sim) {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_M)) {
        menuState.isOpen = !menuState.isOpen;
        menuState.showConfirmReset = false;
        menuState.asteroidInputActive = false;
        if (menuState.isOpen) {
            EnableCursor();
        }
        else {
            DisableCursor();
        }
    }

    if (IsKeyPressed(KEY_F5)) {
        menuState.showConfirmReset = true;
        menuState.isOpen = true;
        EnableCursor();
    }
}

/**
 * @brief Draw text input field
 */
static void DrawTextInput(Rectangle rect, const char* text, bool isActive, const char* label) {
    Color bgColor = isActive ? ColorAlpha(UI_PRIMARY_COLOR, 0.1f) : ColorAlpha(UI_SECONDARY_COLOR, 0.1f);
    Color borderColor = isActive ? UI_PRIMARY_COLOR : UI_SECONDARY_COLOR;
    Color textColor = UI_TEXT_PRIMARY;

    DrawRectangleRounded(rect, 0.1f, 4, bgColor);
    DrawRectangleRoundedLines(rect, 0.1f, 4, borderColor);

    // Draw label
    DrawText(label, rect.x, rect.y - 20, 12, UI_TEXT_SECONDARY);

    // Draw text
    Vector2 textPos = { rect.x + 10, rect.y + rect.height / 2 - 6 };
    DrawText(text, textPos.x, textPos.y, 12, textColor);

    // Draw cursor if active
    if (isActive && (int)(menuState.cursorBlinkTimer * 2) % 2 == 0) {
        const char* beforeCursor = TextFormat("%.*s", menuState.cursorPosition, text);
        Vector2 beforeSize = MeasureTextEx(GetFontDefault(), beforeCursor, 12, 0);
        Vector2 cursorPos = { textPos.x + beforeSize.x, textPos.y };
        DrawLine(cursorPos.x, cursorPos.y, cursorPos.x, cursorPos.y + 12, UI_PRIMARY_COLOR);
    }
}

/**
 * @brief Draw main menu
 */
static void DrawMainMenu(OrbitalSim* sim) {
    // Semi-transparent overlay
    DrawRectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, Color{ 0, 0, 0, 180 });

    Rectangle menuPanel = GetCenteredRect(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, 600, 650);
    DrawPanelBackground(menuPanel, UI_PANEL_BG);

    // Menu title
    DrawText("SIMULATION CONTROL PANEL", menuPanel.x + 120, menuPanel.y + 30, 24, UI_PRIMARY_COLOR);

    float yPos = menuPanel.y + 80;

    // System selection
    DrawText("SELECT SYSTEM:", menuPanel.x + 50, yPos, 18, UI_TEXT_PRIMARY);
    yPos += 40;

    Rectangle solarBtn = { menuPanel.x + 50, yPos, 200, 40 };
    Rectangle centauriBtn = { menuPanel.x + 300, yPos, 200, 40 };

    bool solarPressed = IsMouseInside(solarBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    bool centauriPressed = IsMouseInside(centauriBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    if (solarPressed) menuState.selectedSystem = SYSTEM_TYPE_SOLAR;
    if (centauriPressed) menuState.selectedSystem = SYSTEM_TYPE_ALPHA_CENTAURI;

    Color solarColor = (menuState.selectedSystem == SYSTEM_TYPE_SOLAR) ? UI_SUCCESS_COLOR : UI_SECONDARY_COLOR;
    Color centauriColor = (menuState.selectedSystem == SYSTEM_TYPE_ALPHA_CENTAURI) ? UI_SUCCESS_COLOR : UI_SECONDARY_COLOR;

    DrawButton(solarBtn, "Solar System", menuState.selectedSystem == SYSTEM_TYPE_SOLAR, solarColor);
    DrawButton(centauriBtn, "Alpha Centauri", menuState.selectedSystem == SYSTEM_TYPE_ALPHA_CENTAURI, centauriColor);

    yPos += 80;

    // Asteroid Configuration Section
    DrawText("ASTEROID CONFIGURATION:", menuPanel.x + 50, yPos, 18, UI_TEXT_PRIMARY);
    yPos += 40;

    // Asteroid count input
    Rectangle asteroidInput = { menuPanel.x + 50, yPos, 120, 35 };
    bool inputClicked = IsMouseInside(asteroidInput) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    if (inputClicked && !menuState.asteroidInputActive) {
        menuState.asteroidInputActive = true;
        menuState.cursorPosition = strlen(menuState.asteroidCountText);
    }
    else if (!inputClicked && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        menuState.asteroidInputActive = false;
    }

    DrawTextInput(asteroidInput, menuState.asteroidCountText, menuState.asteroidInputActive, "Count (0-5000)");

    // Dispersion selection
    DrawText("Dispersion:", menuPanel.x + 200, yPos - 15, 14, UI_TEXT_SECONDARY);

    Rectangle tightBtn = { menuPanel.x + 200, yPos, 80, 35 };
    Rectangle normalBtn = { menuPanel.x + 290, yPos, 80, 35 };
    Rectangle wideBtn = { menuPanel.x + 380, yPos, 80, 35 };
    Rectangle extremeBtn = { menuPanel.x + 470, yPos, 80, 35 };

    bool tightPressed = IsMouseInside(tightBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    bool normalPressed = IsMouseInside(normalBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    bool widePressed = IsMouseInside(wideBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    bool extremePressed = IsMouseInside(extremeBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    if (tightPressed) menuState.selectedDispersion = DISPERSION_TIGHT;
    if (normalPressed) menuState.selectedDispersion = DISPERSION_NORMAL;
    if (widePressed) menuState.selectedDispersion = DISPERSION_WIDE;
    if (extremePressed) menuState.selectedDispersion = DISPERSION_EXTREME;

    Color tightColor = (menuState.selectedDispersion == DISPERSION_TIGHT) ? UI_SUCCESS_COLOR : UI_SECONDARY_COLOR;
    Color normalColor = (menuState.selectedDispersion == DISPERSION_NORMAL) ? UI_SUCCESS_COLOR : UI_SECONDARY_COLOR;
    Color wideColor = (menuState.selectedDispersion == DISPERSION_WIDE) ? UI_WARNING_COLOR : UI_SECONDARY_COLOR;
    Color extremeColor = (menuState.selectedDispersion == DISPERSION_EXTREME) ? UI_ERROR_COLOR : UI_SECONDARY_COLOR;

    DrawButton(tightBtn, "Tight", menuState.selectedDispersion == DISPERSION_TIGHT, tightColor);
    DrawButton(normalBtn, "Normal", menuState.selectedDispersion == DISPERSION_NORMAL, normalColor);
    DrawButton(wideBtn, "Wide", menuState.selectedDispersion == DISPERSION_WIDE, wideColor);
    DrawButton(extremeBtn, "Extreme", menuState.selectedDispersion == DISPERSION_EXTREME, extremeColor);

    // Display dispersion info
    yPos += 45;
    DrawText(TextFormat("Range: 2E11 to %.1fE%s",
        getDispersionRange(menuState.selectedDispersion) >= 1E12F ?
        getDispersionRange(menuState.selectedDispersion) / 1E12F : getDispersionRange(menuState.selectedDispersion) / 1E11F,
        getDispersionRange(menuState.selectedDispersion) >= 1E12F ? "12" : "11"),
        menuPanel.x + 200, yPos, 12, UI_TEXT_SECONDARY);

    yPos += 40;

    // Easter egg selection
    DrawText("EASTER EGGS:", menuPanel.x + 50, yPos, 18, UI_TEXT_PRIMARY);
    yPos += 40;

    Rectangle noneBtn = { menuPanel.x + 50, yPos, 150, 35 };
    Rectangle phiBtn = { menuPanel.x + 220, yPos, 150, 35 };
    Rectangle jupiterBtn = { menuPanel.x + 390, yPos, 150, 35 };

    bool nonePressed = IsMouseInside(noneBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    bool phiPressed = IsMouseInside(phiBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    bool jupiterPressed = IsMouseInside(jupiterBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    if (nonePressed) menuState.selectedEasterEgg = EASTER_EGG_NONE;
    if (phiPressed) menuState.selectedEasterEgg = EASTER_EGG_PHI;
    if (jupiterPressed) menuState.selectedEasterEgg = EASTER_EGG_JUPITER_1000X;

    Color noneColor = (menuState.selectedEasterEgg == EASTER_EGG_NONE) ? UI_SUCCESS_COLOR : UI_SECONDARY_COLOR;
    Color phiColor = (menuState.selectedEasterEgg == EASTER_EGG_PHI) ? UI_WARNING_COLOR : UI_SECONDARY_COLOR;
    Color jupiterColor = (menuState.selectedEasterEgg == EASTER_EGG_JUPITER_1000X) ? UI_ERROR_COLOR : UI_SECONDARY_COLOR;

    DrawButton(noneBtn, "None", menuState.selectedEasterEgg == EASTER_EGG_NONE, noneColor);
    DrawButton(phiBtn, "Phi Effect", menuState.selectedEasterEgg == EASTER_EGG_PHI, phiColor);
    DrawButton(jupiterBtn, "Jupiter 1000x", menuState.selectedEasterEgg == EASTER_EGG_JUPITER_1000X, jupiterColor);

    yPos += 70;

    // Action buttons
    Rectangle applyBtn = { menuPanel.x + 80, yPos, 120, 45 };
    Rectangle resetBtn = { menuPanel.x + 220, yPos, 120, 45 };
    Rectangle closeBtn = { menuPanel.x + 360, yPos, 120, 45 };

    bool applyPressed = IsMouseInside(applyBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    bool resetPressed = IsMouseInside(resetBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    bool closePressed = IsMouseInside(closeBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    DrawButton(applyBtn, "APPLY", applyPressed, UI_SUCCESS_COLOR);
    DrawButton(resetBtn, "RESET", resetPressed, UI_ERROR_COLOR);
    DrawButton(closeBtn, "CLOSE", closePressed, UI_SECONDARY_COLOR);

    if (applyPressed) {
        InitializeSystem(sim);
        menuState.isOpen = false;
        menuState.asteroidInputActive = false;
		renderView(0, sim, 1); // Reset timestamp
        DisableCursor();
    }

    if (resetPressed || menuState.showConfirmReset) {
		// Inicialize timer if just pressed reset
        if (resetPressed && !menuState.showConfirmReset) {
            menuState.showConfirmReset = true;
            menuState.confirmDialogTimer = 0.0f;  // Resetear el timer
        }

        // Confirmation dialog
        Rectangle confirmPanel = GetCenteredRect(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 100, 400, 150);
        DrawPanelBackground(confirmPanel, UI_ERROR_COLOR);

        DrawText("CONFIRM RESET?", confirmPanel.x + 120, confirmPanel.y + 30, 18, WHITE);
        DrawText("This will restart the simulation", confirmPanel.x + 80, confirmPanel.y + 60, 14, WHITE);

        Rectangle yesBtn = { confirmPanel.x + 80, confirmPanel.y + 90, 80, 35 };
        Rectangle noBtn = { confirmPanel.x + 200, confirmPanel.y + 90, 80, 35 };

		// Only allow clicking after 0.3 seconds
        bool canClick = menuState.confirmDialogTimer > 0.3f;

        bool yesPressed = canClick && IsMouseInside(yesBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        bool noPressed = canClick && IsMouseInside(noBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

		// Change button color based on clickability
        Color yesColor = canClick ? UI_SECONDARY_COLOR : ColorAlpha(UI_SECONDARY_COLOR, 0.5f);
        Color noColor = canClick ? UI_SECONDARY_COLOR : ColorAlpha(UI_SECONDARY_COLOR, 0.5f);

        DrawButton(yesBtn, "YES", yesPressed, yesColor);
        DrawButton(noBtn, "NO", noPressed, noColor);

		// Show countdown if not clickable yet
        if (!canClick) {
            int countdown = (int)((0.3f - menuState.confirmDialogTimer) * 10) + 1;
            DrawText(TextFormat("Wait %d...", countdown), confirmPanel.x + 180, confirmPanel.y + 130, 12, UI_TEXT_SECONDARY);
        }

        if (yesPressed) {
            InitializeSystem(sim);
            menuState.isOpen = false;
            menuState.showConfirmReset = false;
            menuState.asteroidInputActive = false;
            menuState.confirmDialogTimer = 0.0f;  // Reset timer
            renderView(0, sim, 1); // Reset timestamp
            DisableCursor();
        }

        if (noPressed) {
            menuState.showConfirmReset = false;
            menuState.confirmDialogTimer = 0.0f;  // Resetear timer
        }
    }

    if (closePressed) {
        menuState.isOpen = false;
        menuState.asteroidInputActive = false;
        DisableCursor();
    }

    // Instructions
    DrawText("Press M to open/close menu | F5 for quick reset", menuPanel.x + 50, menuPanel.y + 600, 12, UI_TEXT_SECONDARY);
    DrawText("Click on asteroid count field to edit | Use arrow keys to move cursor", menuPanel.x + 50, menuPanel.y + 615, 12, UI_TEXT_SECONDARY);
}

/**
 * @brief Initialize system based on menu selection
 */
static void InitializeSystem(OrbitalSim* sim) {
    SimConfig newConfig = {
        menuState.selectedSystem,
        menuState.selectedEasterEgg,
        menuState.selectedDispersion,
        menuState.asteroidCount
    };

    resetOrbitalSim(sim, &newConfig);
}

/**
 * @brief Draw enhanced top HUD
 */
static void DrawEnhancedTopHUD(OrbitalSim* sim, float timestamp) {
    Rectangle topHUD = { 0, 0, WINDOW_WIDTH, 80 };
    DrawPanelBackground(topHUD, UI_BACKGROUND);

    // Logo section
    Vector2 logoPos = { 30, 15 };

    // Animated orbital icon
    Vector2 orbitCenter = { logoPos.x + 20, logoPos.y + 25 };
    DrawCircleLines(orbitCenter.x, orbitCenter.y, 18, UI_PRIMARY_COLOR);

    float orbitX = orbitCenter.x + 15 * cosf(uiAnim.rotation * DEG2RAD);
    float orbitY = orbitCenter.y + 15 * sinf(uiAnim.rotation * DEG2RAD);
    DrawCircle(orbitX, orbitY, 3, WHITE);
    DrawCircle(orbitCenter.x, orbitCenter.y, 2, UI_PRIMARY_COLOR);

    // Title
    DrawText("EDA ORBITAL SIMULATION", logoPos.x + 60, logoPos.y + 10, 24, UI_PRIMARY_COLOR);
    DrawText("Advanced Physics Engine", logoPos.x + 60, logoPos.y + 35, 12, UI_TEXT_SECONDARY);

    // Date display
    const char* dateStr = getISODate(timestamp);
    Vector2 dateSize = MeasureTextEx(GetFontDefault(), dateStr, 28, 0);
    Vector2 datePos = { WINDOW_WIDTH / 2 - dateSize.x / 2, 20 };
    DrawText(dateStr, datePos.x, datePos.y, 28, UI_PRIMARY_COLOR);
    DrawText("SIMULATION DATE", datePos.x + 20, datePos.y + 30, 10, UI_TEXT_SECONDARY);

    // FPS counter and menu hint
    int fps = GetFPS();
    Color fpsColor = (fps >= 55) ? UI_SUCCESS_COLOR : (fps >= 30) ? UI_WARNING_COLOR : UI_ERROR_COLOR;
    DrawText(TextFormat("%d FPS", fps), WINDOW_WIDTH - 160, 15, 20, fpsColor);
    DrawText("Press M for Menu", WINDOW_WIDTH - 160, 45, 12, UI_TEXT_SECONDARY);
}

/**
 * @brief Draw enhanced left panel
 */
static void DrawEnhancedLeftPanel(OrbitalSim* sim, float lodMultiplier, int rendered_planets, int rendered_asteroids) {
    Rectangle panel = { PANEL_MARGIN, 100, 320, 500 };
    DrawPanelBackground(panel, UI_PANEL_BG);

    // Panel header
    Vector2 headerPos = { panel.x + PANEL_PADDING, panel.y + PANEL_PADDING };
    DrawText("SYSTEM STATUS", headerPos.x + 60, headerPos.y, 18, UI_PRIMARY_COLOR);

    // Stats grid
    float statY = headerPos.y + 40;
    float statSpacing = 80;

    // Planet count
    Rectangle planetStat = { panel.x + 20, statY, STAT_BOX_SIZE, 60 };
    DrawStatBox(planetStat, TextFormat("%d/%d", rendered_planets, sim->systemBodies), "PLANETS", UI_SUCCESS_COLOR);

    // Asteroid count  
    Rectangle asteroidStat = { panel.x + 160, statY, STAT_BOX_SIZE, 60 };
    DrawStatBox(asteroidStat, TextFormat("%d", rendered_asteroids), "RENDERED", UI_WARNING_COLOR);

    statY += statSpacing;

    // Total bodies
    Rectangle totalStat = { panel.x + 20, statY, STAT_BOX_SIZE, 60 };
    DrawStatBox(totalStat, TextFormat("%d", sim->numBodies), "TOTAL", UI_SECONDARY_COLOR);

    // Black holes
    Rectangle bhStat = { panel.x + 160, statY, STAT_BOX_SIZE, 60 };
    int bhCount = sim->blackHole.isActive ? 1 : 0;
    Color bhColor = bhCount > 0 ? UI_ERROR_COLOR : UI_TEXT_SECONDARY;
    DrawStatBox(bhStat, TextFormat("%d", bhCount), "BLACK HOLES", bhColor);

    // Current Configuration Info
    statY += 80;
    DrawText("CURRENT CONFIG", panel.x + PANEL_PADDING, statY, 14, UI_PRIMARY_COLOR);

    Rectangle configPanel = { panel.x + 20, statY + 25, 280, 120 };
    DrawPanelBackground(configPanel, Color{ 0, 0, 0, 100 });

    DrawText(TextFormat("System: %s", getSystemName(sim->config.systemType)),
        configPanel.x + 10, configPanel.y + 10, 14, UI_TEXT_PRIMARY);
    DrawText(TextFormat("Asteroids: %d", sim->asteroidCount),
        configPanel.x + 10, configPanel.y + 30, 14, UI_TEXT_PRIMARY);
    DrawText(TextFormat("Dispersion: %s", getDispersionName(sim->config.dispersion)),
        configPanel.x + 10, configPanel.y + 50, 14, UI_TEXT_PRIMARY);
    DrawText(TextFormat("Easter Egg: %s", getEasterEggName(sim->config.easterEgg)),
        configPanel.x + 10, configPanel.y + 70, 14, UI_TEXT_PRIMARY);
    DrawText("Open menu (M) to modify", configPanel.x + 10, configPanel.y + 90, 12, UI_TEXT_SECONDARY);

    // LOD Control section
    statY += 160;
    DrawText("LOD CONTROL", panel.x + PANEL_PADDING, statY, 14, UI_PRIMARY_COLOR);

    Rectangle lodPanel = { panel.x + 20, statY + 25, 280, 80 };
    DrawPanelBackground(lodPanel, Color{ 0, 0, 0, 100 });

    // LOD value display
    DrawText(TextFormat("Multiplier: %.2f", lodMultiplier), lodPanel.x + 10, lodPanel.y + 10, 16, UI_TEXT_PRIMARY);

    // LOD buttons
    float btnY = lodPanel.y + 35;
    float btnWidth = 60;
    float btnSpacing = 70;

    Rectangle btnInc = { lodPanel.x + 15, btnY, btnWidth, BUTTON_HEIGHT };
    Rectangle btnDec = { lodPanel.x + 15 + btnSpacing, btnY, btnWidth, BUTTON_HEIGHT };
    Rectangle btnRst = { lodPanel.x + 15 + btnSpacing * 2, btnY, btnWidth, BUTTON_HEIGHT };

    bool key1Pressed = IsKeyPressed(KEY_ONE);
    bool key2Pressed = IsKeyPressed(KEY_TWO);
    bool keyRPressed = IsKeyPressed(KEY_R);

    DrawButton(btnInc, "+(1)", key1Pressed, UI_SUCCESS_COLOR);
    DrawButton(btnDec, "-(2)", key2Pressed, UI_WARNING_COLOR);
    DrawButton(btnRst, "RST(R)", keyRPressed, UI_SECONDARY_COLOR);
}

/**
 * @brief Draw enhanced right panel
 */
static void DrawEnhancedRightPanel(void) {
    Rectangle panel = { WINDOW_WIDTH - 280 - PANEL_MARGIN, 100, 280, 320 };
    DrawPanelBackground(panel, UI_PANEL_BG);

    DrawText("CONTROLS", panel.x + 90, panel.y + 20, 18, UI_PRIMARY_COLOR);

    float yPos = panel.y + 60;
    float lineHeight = 30;

    struct {
        const char* action;
        const char* key;
        Color keyColor;
    } controls[] = {
        {"Increase LOD", "1", UI_SUCCESS_COLOR},
        {"Decrease LOD", "2", UI_WARNING_COLOR},
        {"Reset LOD", "R", UI_SECONDARY_COLOR},
        {"Create Black Hole", "K", UI_ERROR_COLOR},
        {"Open Menu", "M/ESC", UI_PRIMARY_COLOR},
        {"Quick Reset", "F5", UI_ERROR_COLOR},
        {"Free Camera", "WASD", UI_TEXT_PRIMARY},
        {"Camera Look", "Mouse", UI_TEXT_PRIMARY},
        {"Show/Hide Interface", "F3", UI_TEXT_PRIMARY }
    };

    int n = sizeof(controls) / sizeof(controls[0]);

    for (int i = 0; i < n; i++) {
        DrawText(controls[i].action, panel.x + 20, yPos, 13, UI_TEXT_PRIMARY);

        Rectangle keyRect = { panel.x + 180, yPos - 3, 70, 18 };
        DrawPanelBackground(keyRect, Color{ 30, 40, 60, 255 });
        DrawText(controls[i].key, keyRect.x + 8, keyRect.y + 2, 12, controls[i].keyColor);

        yPos += lineHeight;
    }
}

/**
 * @brief Draw enhanced bottom HUD
 */
static void DrawEnhancedBottomHUD(int fps) {
    Rectangle bottomHUD = { 0, WINDOW_HEIGHT - 60, WINDOW_WIDTH, 60 };
    DrawPanelBackground(bottomHUD, UI_BACKGROUND);

    Vector2 centerPos = { WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT - 40 };

    // Status indicators
    struct {
        const char* label;
        Color color;
        bool active;
    } indicators[] = {
        {"Simulation Active", UI_SUCCESS_COLOR, true},
        {"Physics Engine", UI_WARNING_COLOR, fps > 30},
        {"Rendering", UI_PRIMARY_COLOR, true}
    };

    for (int i = 0; i < 3; i++) {
        Vector2 pos = { centerPos.x + i * 150, centerPos.y };

        // Animated dot
        float dotSize = 4 + uiAnim.pulse * 2;
        Color dotColor = indicators[i].active ? indicators[i].color : UI_TEXT_SECONDARY;
        DrawCircle(pos.x, pos.y + 5, dotSize, dotColor);

        DrawText(indicators[i].label, pos.x + 15, pos.y, 12, UI_TEXT_SECONDARY);
    }
}

/**
 * @brief Draw panel background with border
 */
static void DrawPanelBackground(Rectangle rect, Color color) {
    DrawRectangleRounded(rect, 0.1f, 6, color);
    DrawRectangleRoundedLines(rect, 0.1f, 6, ColorAlpha(UI_PRIMARY_COLOR, 0.3f));
}

/**
 * @brief Draw statistic box
 */
static void DrawStatBox(Rectangle rect, const char* value, const char* label, Color accentColor) {
    DrawPanelBackground(rect, ColorAlpha(accentColor, 0.1f));

    Vector2 valueSize = MeasureTextEx(GetFontDefault(), value, 24, 0);
    Vector2 labelSize = MeasureTextEx(GetFontDefault(), label, 10, 0);

    DrawText(value, rect.x + rect.width / 2 - valueSize.x / 2, rect.y + 8, 24, UI_TEXT_PRIMARY);
    DrawText(label, rect.x + rect.width / 2 - labelSize.x / 2, rect.y + 35, 10, UI_TEXT_SECONDARY);

    if (IsMouseInside(rect)) {
        DrawRectangleRoundedLines(rect, 0.1f, 6, accentColor);
    }
}

/**
 * @brief Draw button with press effect
 */
static void DrawButton(Rectangle rect, const char* text, bool isPressed, Color color) {
    Color bgColor = isPressed ? color : ColorAlpha(color, 0.2f);
    Color textColor = isPressed ? BLACK : color;

    DrawRectangleRounded(rect, 0.2f, 4, bgColor);
    DrawRectangleRoundedLines(rect, 0.2f, 4, color);

    Vector2 textSize = MeasureTextEx(GetFontDefault(), text, 12, 0);
    Vector2 textPos = { rect.x + rect.width / 2 - textSize.x / 2, rect.y + rect.height / 2 - textSize.y / 2 };
    DrawText(text, textPos.x, textPos.y, 12, textColor);
}

/**
 * @brief Check if mouse is inside rectangle
 */
static bool IsMouseInside(Rectangle rect) {
    Vector2 mousePos = GetMousePosition();
    return CheckCollisionPointRec(mousePos, rect);
}

/**
 * @brief Get centered rectangle
 */
static Rectangle GetCenteredRect(float x, float y, float width, float height) {
    return Rectangle{ x - width / 2, y - height / 2, width, height };
}