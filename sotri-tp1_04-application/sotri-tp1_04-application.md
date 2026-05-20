# TP1 - Actividad 04 - 7mo Proyecto p/placa NUCLEO-F103RB con FreeRTOS

## Paso 02 - Procesamiento periódico e IDLE

### Cómo implementar el procesamiento periódico mediante una Tarea

En FreeRTOS una tarea periódica se implementa con una función de tarea que contiene un bucle infinito. Dentro del bucle se ejecuta el procesamiento de la tarea y luego se bloquea la tarea hasta el próximo instante de ejecución.

Para implementar un período estable se utiliza `vTaskDelayUntil()`. Esta función recibe una referencia temporal previa y un incremento de tiempo. A diferencia de `vTaskDelay()`, que espera una cantidad de ticks desde el momento en que se la llama, `vTaskDelayUntil()` permite mantener una cadencia periódica más precisa.

Ejemplo general:

```c
#define TASK_PERIOD_MS  10ul

void task_example(void *parameters)
{
    TickType_t last_wake_time = xTaskGetTickCount();

    for (;;)
    {
        task_example_statechart();

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(TASK_PERIOD_MS));
    }
}

```

Para que `vTaskDelayUntil()` esté disponible, en `FreeRTOSConfig.h` debe estar habilitada la opción:

```c
#define INCLUDE_vTaskDelayUntil  1

```

### Cuándo se ejecutará la Tarea IDLE y cómo se puede utilizar

La Tarea IDLE se ejecuta cuando no hay ninguna otra tarea lista para ejecutar. Esto sucede cuando las tareas de la aplicación están bloqueadas, por ejemplo esperando un delay, un evento, una cola, un semáforo o alguna otra condición.

En este proyecto, al convertir `task_led` y `task_btn` en tareas periódicas, ambas tareas pasan parte del tiempo bloqueadas por `vTaskDelayUntil()`. Durante esos intervalos el scheduler puede ejecutar la Tarea IDLE.

El proyecto tiene habilitado el hook de IDLE:

```c
#define configUSE_IDLE_HOOK  1

```

Y en `vApplicationIdleHook()` se incrementa el contador global:

```c
g_task_idle_cnt++;

```

Por lo tanto, durante la depuración se puede observar `g_task_idle_cnt` para verificar que la Tarea IDLE está ejecutándose. Si este contador aumenta, significa que el procesador tiene tiempo libre entre las ejecuciones periódicas de las tareas de aplicación.

La Tarea IDLE puede utilizarse para medir tiempo ocioso del sistema, estimar carga de CPU o ejecutar acciones de muy baja prioridad. No debe bloquearse ni ejecutar procesamiento largo, porque es una tarea interna del sistema operativo.

## Paso 03 - Procesamiento periódico de `task_led`

Se modificó `task_led` para que su statechart no se ejecute continuamente, sino de manera periódica.

Se agregó el período de ejecución:

```c
#define TASK_LED_PERIOD_MS  10ul

```

Dentro de `task_led()` se inicializó una referencia temporal:

```c
TickType_t last_wake_time = xTaskGetTickCount();

```

Luego, al final de cada iteración del bucle infinito, se agregó:

```c
vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(TASK_LED_PERIOD_MS));

```

Con esta modificación, `task_led_statechart()` se ejecuta cada 10 ms aproximadamente. La tarea LED sigue respondiendo a los eventos `EV_LED_BLINK` y `EV_LED_OFF`, y cuando está en estado `ST_LED_BLINK` mantiene el parpadeo usando la comparación con `xTaskGetTickCount()`.

Observación durante la depuración:

- Antes de agregar procesamiento periódico, `task_led` ejecutaba su bucle continuamente.
- Luego de agregar `vTaskDelayUntil()`, la tarea LED queda bloqueada entre activaciones periódicas.
- El LED mantiene el comportamiento esperado.
- El sistema dispone de más tiempo libre para ejecutar la Tarea IDLE.

## Paso 04 - Procesamiento periódico de `task_btn`

Se modificó `task_btn` para que el muestreo del pulsador también sea periódico.

Se agregó el período de ejecución:

```c
#define TASK_BTN_PERIOD_MS  10ul

```

Dentro de `task_btn()` se inicializó una referencia temporal:

```c
TickType_t last_wake_time = xTaskGetTickCount();

```

Luego, al final de cada iteración del bucle infinito, se agregó:

```c
vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(TASK_BTN_PERIOD_MS));

```

Con esta modificación, `task_btn_statechart()` se ejecuta cada 10 ms aproximadamente. La tarea realiza el muestreo periódico del GPIO del botón y mantiene el antirrebote mediante los estados `ST_BTN_FALLING` y `ST_BTN_RISING`, usando el tiempo transcurrido con `xTaskGetTickCount()`.

Observación durante la depuración:

- La tarea botón detecta la presión del pulsador y genera el evento `EV_LED_BLINK`.
- Al soltar el pulsador genera el evento `EV_LED_OFF`.
- El LED responde a los eventos generados por la tarea botón.
- Al quedar bloqueada periódicamente, `task_btn` deja tiempo libre para otras tareas y para la Tarea IDLE.

## Conclusiones

El procesamiento periódico permite que las tareas ejecuten su lógica con una frecuencia conocida y que no ocupen la CPU permanentemente. Al utilizar `vTaskDelayUntil()`, el período de ejecución se mantiene más estable que con un retardo relativo simple.

Luego de modificar `task_led` y `task_btn`, ambas tareas se ejecutan periódicamente y se bloquean entre activaciones. Esto permite observar la ejecución de la Tarea IDLE mediante el contador `g_task_idle_cnt`, confirmando que el sistema tiene tiempo ocioso cuando las tareas de aplicación no están listas para ejecutar.
