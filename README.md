# EDA #level1: Orbital simulation

## Integrantes del grupo y contribución al trabajo de cada integrante

**Dylan Frigerio**: Implementación del sistema solar, agujero negro, asteroides, interfaz y menú. Optimización del código y mejora de la complejidad computacional de las funciones.

**Luca Forchiassin**: Implementación de Alpha Centauri y de la nave espacial. Optimización del código y mejora de la complejidad computacional de las funciones.

## Verificación del timestep

El sistema utiliza una configuración temporal optimizada para balancear precisión y velocidad de simulación:

- **Timestep base**: 5 días por paso de simulación
- **Aceleración por frame**: 20 iteraciones por frame renderizado
- **Velocidad de simulación**: 100 días simulados por segundo real
- **FPS objetivo**: 60 frames por segundo

Esta configuración permite mantener alta precisión en los cálculos físicos mientras acelera significativamente la visualización de fenómenos orbitales de largo plazo.

## Verificación del tipo de datos float vs double

Se utilizó double para cálculos críticos por dos razones principales:

- **Límites de rango**: Las escalas astronómicas (>10¹¹ metros) elevadas al cuadrado o cubo exceden el rango máximo de float (3.4 × 10³⁸), causando overflow
- **Calculo de distancias**: Se calcula la distancia entre dos objetos una sola vez por par, reutilizando el valor para ambos cálculos de fuerza, minimizando errores acumulativos. Se utiliza el calculo de distancia al cuadrado para evitar operaciones de raíz cuadrada innecesarias.
- **Optimización de algoritmo**: El sistema calcula fuerzas entre pares de objetos una sola vez, aplicando la tercera ley de Newton (F_ij = -F_ji), reduciendo el número de cálculos y manteniendo consistencia numérica.


## Complejidad computacional con asteroides

### Problema inicial
La implementación original tenía complejidad O(n²) donde se calculaban las interacciones gravitatorias entre todos los pares de objetos. Con más de 300 asteroides, esto resultaba en:
- Caída significativa de FPS (de 60 a <20)
- Más de 150,000 cálculos de fuerza por frame
- Recursos computacionales excesivos para interacciones mínimas

### Impacto en rendimiento
- **500 asteroides**: ~125,000 interacciones por frame
- **1000 asteroides**: ~500,000 interacciones por frame
- **5000 asteroides**: Más de 12 millones de interacciones por frame

## Mejora de la complejidad computacional

### Optimizaciones implementadas

1. **Jerarquía de influencias gravitatorias**:
#### Sistema Solar
   - **Sol/estrella principal**: Influencia calculada para todos los asteroides
   - **Planetas**: Fuerzas calculadas entre todos los planetas, y solo con asteroides dentro del radio de influencia (1E15 m²)
   - **Asteroides entre sí**: Interacciones eliminadas (despreciables comparado con cuerpos masivos)

#### Sistema Alpha Centauri
   - **Estrellas binarias**: Influencia calculada para todos los asteroides
   - **Asteroides entre sí**: Interacciones eliminadas

2. **Optimizaciones de cálculo**:
   - Uso de `x * x` en lugar de `pow(x, 2)` para exponentes
   - Reutilización de cálculos de distancia
   - Eliminación de divisiones y multiplicaciones redundantes
   - Límites de distancia mínima para evitar singularidades

3. **Reducción de complejidad efectiva**:
   - De O(n²) a O(n) para asteroides
   - Mantenimiento de O(p²) solo para planetas (p << n)
   - Resultado: O(n + p²) donde p es constante y pequeño

### Resultados de optimización
- **500 asteroides**: 60 FPS estables
- **1000 asteroides**: ~50 FPS
- **5000 asteroides**: ~15-20 FPS (aún funcional)

## Sistema de renderizado LOD (Level of Detail)

Implementación de un sistema dinámico de nivel de detalle para optimizar el rendering:

### Niveles de LOD
- **Distancia cercana (< 30% del límite)**: Esferas completas con alta resolución
- **Distancia media (30-70%)**: Esferas con resolución reducida
- **Distancia lejana (70-100%)**: Esferas de baja resolución
- **Fuera del límite**: Puntos 3D simples

### Controles LOD
- **Tecla 1**: Aumentar multiplicador LOD (mostrar más objetos)
- **Tecla 2**: Disminuir multiplicador LOD (mostrar menos objetos)
- **Tecla R**: Resetear LOD al valor por defecto

### Culling inteligente para asteroides
- Algoritmo pseudo-aleatorio determinístico para asteroides: (i * 73 + 17) % 1000 < lodFactor * 1000
- Factor de LOD dinámico según distancia
- Reducción progresiva de asteroides renderizados en distancias lejanas

## Bonus points

### 1. Interfaz Gráfica Interactiva
Desarrollo de un sistema de menús completo que permite modificar la simulación en tiempo real:

#### Configuración de Sistema
- Selección entre Sistema Solar y Alpha Centauri
- Cambio instantáneo entre sistemas planetarios

#### Control de Asteroides
- Campo de texto editable para cantidad (0-5000)
- Cuatro tipos de dispersión:
  - **Tight**: 2E11 - 6E11 metros
  - **Normal**: 2E11 - 12E11 metros  
  - **Wide**: 2E11 - 18E11 metros
  - **Extreme**: 2E11 - 20E12 metros

#### Easter Eggs
- **Phi Effect**: Todos los asteroides generados en phi = 0
- **Jupiter 1000x**: Jupiter con masa multiplicada por 1000. Cuando esta activa se incluye para los calculos gravitatorios de los asteroides.

### 2. Nave Espacial Interactiva
- Modelo 3D de nave UFO que orbita la cámara
- Animación de rotación continua
- Capacidad de lanzar agujeros negros con rayo láser violeta
- Posicionamiento relativo a la cámara

### 3. Sistema de Agujeros Negros
Implementación física completa de agujeros negros:

#### Características físicas
- **Masa**: 10 masas solares
- **Radio de Schwarzschild**: Calculado dinámicamente
- **Disco de acreción**: Renderizado con partículas animadas
- **Crecimiento**: Aumenta masa y tamaño al consumir objetos

#### Interacciones
- Atracción gravitatoria hacia todos los objetos
- Detección de colisiones con radio de acreción
- Eliminación de objetos que cruzan el horizonte de eventos
- Efecto visual de partículas orbitando

### 4. Sistema de Alpha Centauri
Implementación del sistema estelar binario más cercano.

### 5. Interfaz de Usuario Avanzada
#### Panel de estado en tiempo real
- Contador de planetas renderizados
- Contador de asteroides visibles
- Estado de agujeros negros activos
- Información de configuración actual

#### Controles visuales
- Botones con efectos hover y animaciones
- Campos de texto editables con cursor parpadeante
- Confirmación de reset con temporizador de seguridad
- Indicadores de estado animados

## Arquitectura de variables globales
El sistema de interfaz utiliza variables globales estratégicamente:

- uiAnim: Mantiene estados de animación entre frames para transiciones suaves sin recálculos
- menuState: Persiste selecciones, entrada de texto y timers a través de múltiples llamadas a renderView()
- shipRenderer: Evita recargar modelos OpenGL, optimizando recursos gráficos y manteniendo transformaciones

## Controles del juego

### Navegación
- **WASD**: Movimiento libre de cámara
- **Mouse**: Control de vista/rotación
- **Espacio**: Subir cámara
- **Ctrl**: Bajar cámara
- **Q/E**: Rotar cámara izquierda/derecha

### Controles de simulación
- **M**: Abrir/cerrar menú principal
- **F5**: Reset rápido (con confirmación)
- **K**: Crear agujero negro desde la nave
- **F3**: Mostrar/ocultar interfaz de usuario

### Controles LOD
- **1**: Aumentar nivel de detalle
- **2**: Disminuir nivel de detalle  
- **R**: Resetear nivel de detalle

### Controles de menú
- **Click izquierdo**: Seleccionar opciones
- **Teclas de flecha**: Mover cursor en campos de texto
- **Enter**: Confirmar entrada de texto
- **Backspace/Delete**: Editar texto

## Rendimiento

### Configuraciones recomendadas
- **Óptimo**: 500 asteroides (60 FPS constantes)
- **Bueno**: 1000 asteroides (45-50 FPS)
- **Límite**: 5000 asteroides (15-25 FPS)

### Factores de rendimiento
- LOD multiplier bajo mejora FPS significativamente
- Dispersión "Extreme" reduce carga de rendering
- Sistemas con pocos planetas son más eficientes
- Agujeros negros añaden carga computacional mínima

## Bibliografía

**Claude AI**: Herramienta de desarrollo utilizada principalmente en el diseño gráfico y optimización de algoritmos.  
https://claude.ai/

**TurboSquid**: Plataforma de modelos 3D de donde se obtuvo el diseño de la nave espacial UFO.  
https://www.turbosquid.com/3d-models/3d-ufo-1333537

**NASA JPL Horizons**: Sistema de efemérides utilizado para datos astronómicos precisos.  
https://ssd.jpl.nasa.gov/horizons/

**Raylib**: Biblioteca gráfica utilizada para rendering 3D y manejo de entrada.  
https://www.raylib.com/