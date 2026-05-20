# TP1 – Actividad 02 – 5to Proyecto p/placa NUCLEO-F103RB con FreeRTOS

## Paso 02 - Preguntas sobre planificación, prioridades, estados e instancias

1. ¿Cómo FreeRTOS asigna tiempo de procesamiento a cada Tarea en una aplicación?
FreeRTOS utiliza un planificador (scheduler) preemptivo que se basa en interrupciones de hardware periódicas llamadas "Ticks" (usualmente cada 1 ms). Mediante un algoritmo de "Round-Robin" (Time Slicing), el planificador aprovecha cada Tick para pausar la tarea actual y asignarle una "rebanada de tiempo" equitativa a la siguiente tarea que tenga su misma prioridad.

2. ¿Cómo FreeRTOS elige qué Tarea debe ejecutarse en un momento dado?
El planificador sigue una regla estricta e inquebrantable: el procesador siempre ejecutará la tarea que tenga la prioridad más alta de entre todas las que se encuentren listas para correr en ese instante preciso.

3. ¿Cómo la prioridad relativa de cada Tarea afecta el comportamiento del sistema?
La prioridad define la jerarquía absoluta de ejecución (siendo 0 la más baja). Si una tarea de alta prioridad despierta, interrumpe inmediatamente (preempción) a cualquier tarea de menor prioridad; esto implica que si las tareas de alta prioridad no ceden el control bloqueándose intencionalmente, las de menor prioridad sufrirán "inanición" y nunca se ejecutarán.

4. ¿Cuáles son los estados en los que puede encontrarse una Tarea?
Una tarea solo puede estar en uno de cuatro estados: Running (usando activamente el CPU), Ready (lista para ejecutarse pero esperando que tareas de mayor prioridad liberen el CPU), Blocked (suspendida temporalmente esperando un evento, dato o retardo) o Suspended (ignorada totalmente por el planificador hasta que se la reanude manualmente).

5. ¿Cómo implementar Tareas?
Una tarea se implementa como una función estándar de C (void vMiTarea(void *pvParameters)) que nunca debe retornar. Su estructura típica incluye una pequeña fase de inicialización seguida de un bucle infinito (for(;;)) que contiene la lógica operativa y funciones bloqueantes (como vTaskDelay()) para ceder el procesador.

6. ¿Cómo crear una o más instancias de una Tarea?
Para que el sistema operativo registre una tarea, se debe invocar a la API xTaskCreate(), proporcionándole la función a ejecutar, la memoria para la pila, la prioridad y un puntero de control (Handle). Puedes crear múltiples instancias concurrentes utilizando la misma función base, simplemente llamando a xTaskCreate() varias veces con diferentes argumentos en su parámetro pvParameters.

7. ¿Cómo eliminar una Tarea?
Cuándo una tarea ya no es útil, se debe llamar a vTaskDelete() para que FreeRTOS libere los recursos de memoria que ocupaba. Puedes eliminar otra tarea pasándole su Handle específico, o una tarea puede "suicidarse" llamando a vTaskDelete(NULL) (lo cual es obligatorio hacer en lugar de usar un return si se desea salir del bucle).

## Paso 03 - Modificación de prioridades relativas de `task_btn` y `task_led`

De app.c modificamos las prioridades

    /* Task BTN thread at priority 1 */
    ret = xTaskCreate(task_btn,							/* Pointer to the function thats implement the task. */
					  "Task BTN",						/* Text name for the task. This is to facilitate debugging only. */
					  (2 * configMINIMAL_STACK_SIZE),	/* Stack depth in words. */
					  NULL,								/* We are not using the task parameter. */
					  (tskIDLE_PRIORITY + 1ul),			/* This task will run at priority 1. */
					  &h_task_btn);						/* We are using a variable as task handle. */

    /* Check the thread was created successfully. */
    configASSERT(pdPASS == ret);

    /* Task LED thread at priority 1 */
    ret = xTaskCreate(task_led,							/* Pointer to the function thats implement the task. */
					  "Task LED",						/* Text name for the task. This is to facilitate debugging only. */
					  (2 * configMINIMAL_STACK_SIZE),	/* Stack depth in words. */
					  NULL,								/* We are not using the task parameter. */
					  (tskIDLE_PRIORITY + 2ul),			/* This task will run at priority 1. */
					  &h_task_led);						/* We are using a variable as task handle. */

    /* Check the thread was created successfully. */

    Si cambiamos las prioridades, el que tenga la prioridad más alta se adueña del microcontrolador y no deja ejecutar la otra tarea.

## Paso 04 - Tres instancias de `task_btn` y eliminación de una instancia

 /* Task BTN thread at priority 1 */
    ret = xTaskCreate(task_btn,							/* Pointer to the function thats implement the task. */
					  "Task BTN 1",						/* Text name for the task. This is to facilitate debugging only. */
					  (2 * configMINIMAL_STACK_SIZE),	/* Stack depth in words. */
					  NULL,								/* We are not using the task parameter. */
					  (tskIDLE_PRIORITY + 1ul),			/* This task will run at priority 1. */
					  &h_task_btn_1);						/* We are using a variable as task handle. */

    /* Check the thread was created successfully. */
    configASSERT(pdPASS == ret);

    /* Task BTN thread at priority 1 */
    ret = xTaskCreate(task_btn,							/* Pointer to the function thats implement the task. */
					  "Task BTN 2",						/* Text name for the task. This is to facilitate debugging only. */
					  (2 * configMINIMAL_STACK_SIZE),	/* Stack depth in words. */
					  NULL,								/* We are not using the task parameter. */
					  (tskIDLE_PRIORITY + 2ul),			/* This task will run at priority 1. */
					  &h_task_btn_2);						/* We are using a variable as task handle. */

    /* Check the thread was created successfully. */
    configASSERT(pdPASS == ret);

    /* Task BTN thread at priority 1 */
    ret = xTaskCreate(task_btn,							/* Pointer to the function thats implement the task. */
					  "Task BTN 3",						/* Text name for the task. This is to facilitate debugging only. */
					  (2 * configMINIMAL_STACK_SIZE),	/* Stack depth in words. */
					  NULL,								/* We are not using the task parameter. */
					  (tskIDLE_PRIORITY + 2ul),			/* This task will run at priority 1. */
					  &h_task_btn_3);						/* We are using a variable as task handle. */

    /* Check the thread was created successfully. */
    configASSERT(pdPASS == ret);

    /* Task LED thread at priority 1 */
    ret = xTaskCreate(task_led,							/* Pointer to the function thats implement the task. */
					  "Task LED",						/* Text name for the task. This is to facilitate debugging only. */
					  (2 * configMINIMAL_STACK_SIZE),	/* Stack depth in words. */
					  NULL,								/* We are not using the task parameter. */
					  (tskIDLE_PRIORITY + 2ul),			/* This task will run at priority 1. */
					  &h_task_led);						/* We are using a variable as task handle. */

    /* Check the thread was created successfully. */
    configASSERT(pdPASS == ret);

    Consola

    [info]
[info] app_init is running - Tick [mS] =   0
[info]  RTOS - Event-Triggered Systems (ETS)
[info]  soe-tp0_03-application: Demo Code
[info]
[info] Task LED is running - Tick [mS] =   0
[info]
[info] Task BTN 2 is running - Tick [mS] =   1
[info]  Task BTN 2 - BTN PRESSED
[info]  Task LED - LED BLINK
[info]  Task BTN 2 - BTN HOVER
[info]  Task LED - LED OFF
[info]  Task BTN 2 - BTN PRESSED
[info]  Task LED - LED BLINK
[info]  Task BTN 2 - BTN HOVER
[info]  Task LED - LED OFF
[info]  Task BTN 2 - BTN PRESSED
[info]  Task LED - LED BLINK
[info]  Task BTN 2 - BTN HOVER
[info]  Task LED - LED OFF

### Observaciones de la Implementación (Paso 04)

Durante la prueba de instanciación múltiple de tareas, se verificaron los siguientes comportamientos del RTOS:

* **Gestión de Prioridades (Preempción):** Al ejecutar múltiples instancias de la tarea de botón (`task_btn`) con diferentes prioridades, el planificador otorgó el control absoluto de la CPU a la tarea con la prioridad más alta, desplazando a las demás.
* **Igualdad de Prioridades:** Al asignar la misma jerarquía a todas las instancias de `task_btn`, se observó que la primera tarea en ser creada (instanciada en el código) fue la que retuvo el control inicial de la ejecución.
* **Eliminación Dinámica de Tareas:** Se validó el correcto funcionamiento de la API de FreeRTOS para la destrucción de hilos. La tarea controladora del LED (`task_led`) logró recibir exitosamente el *Handle* y ejecutar `vTaskDelete()` para eliminar de forma definitiva la instancia número 3 del botón ("BTN 3").
