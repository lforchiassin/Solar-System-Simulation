# EDA #level1: Orbital simulation

## Integrantes del grupo y contribución al trabajo de cada integrante

Dylan Frigerio: Implementación del sistema solar, agujero negro, asteroides, interfaz y menú. Optimización del código y mejora de la 
complejidad computacional de las funciones.

Luca Forchiassin: Implementación de Alphacentauri y de la nave espacial. Optimización del código y mejora de la complejidad 
computacional de las funciones.

## Verificación del timestep

Se tiene que cada vez que actúa el update, se actualiza 5 días la simulación. Pero eso es acelerado ya que se llaman i updates por frame, por 
lo que ...

## Verificación del tipo de datos float

Se utilizó double para calcular la magnitud de la fuerza de la gravedad ya que este programa no itera entre y=0 y j=0. Itera entre y=0, j=y+1.
Y cada vez que se entra en el for, se le calcula la aceleración al objeto y y al objeto j. Con esto se saltean pasos, pero a costa de esto es
necesario usar doubles.

## Complejidad computacional con asteroides

La función computeGravitationalAccelerations que configuraba el cálculo de las fuerzas relacionadas a ellos tenía una complejidad
computacional de O(n*2), lo que hacía que al poner una cantidad algo grande (más de 300) asteroides, bajaba considerablemente la eficiencia 
del programa que se reflejaba en los FPS (frames por segundo) de la simulación. Se tuvo que cambiar la lógica de la función a una con el
mismo O pero que no calcule la aceleración de todos los asteroides, sino que se calcule la que deberían tener respecto al sol, y para los 
que estén dentro de una determinada distancia de un planeta, se calcule la aceleración respecto a ese planeta. De esta manera, la complejidad 
computacional se reduce no por el orden, sino por la cantidad de cálculos que se hacen en cada iteración, lo que posibilita que se agrege 
una cantidad más grande de asteroides sin dificultar el rendimiento. 

## Mejora de la complejidad computacional

[completar]

## Bonus points

Para los bonus points, se implementaron las siguientes características:
Una nave espacial que puede ser controlada con las flechas del teclado. Su modelaje fue obtenido en Turbosquid. Tiene la capacidad de lanzar
agujeros negros que atrae y hace desaparecer a todos los objetos cercanos.
Se implementó una interfaz gráfica la cual permite cambiar las diferentes funcionalidades de la simulación en tiempo real, como establecer 
cuántos asteroides se implementan. La cantidad ideal es 500 ya que va a 60 FPS. Con 1000 asteroides va a 40 FPS, y con más de 5000 asteroides
es inviable. También se puede habilitar el eater egg de phi=0 y el de Júpiter desde ahí.
La Estrella Alpha Centauri y su sistema planetario fueron implementados y se puede acceder desde la interfaz.
Distintos tipos de render. Dependiendo la distancia que se posiciona la cámara a cierto cuerpo, se renderiza más detallado o menos. Mejora la 
eficiencia.
Se pueden elegir diferentes formas de generar los asteroides. Se puede seleccionar que estén muy cerca, normal, lejos o extremadamente 
lejos. Mientras más lejos estén, mas eficiente va a ser ya que los lejanos no se van a renderizar por la implementación ya nombrada.

## Controles 

Increase LOD           1
Decrease LOD           2
Reset LOD              R
Create Black Hole      K
Open Menu              M/ESC
Quick Reset            F5
Free Camera            WASD
Camera Look            Mouse
Show/Hide Interface    F3

## Bibliografía

Claude: Herramienta de desarrollo utilizada principalmente en el diseño gráfico.  
https://claude.ai/new
Turbosquid: Página web de donde se obtuvo el diseño de la nave espacial.
https://www.turbosquid.com/3d-models/3d-ufo-1333537

