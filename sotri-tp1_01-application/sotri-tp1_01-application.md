# Análisis de Funcionamiento: Proyecto STM32 con FreeRTOS

Este documento detalla el análisis del código fuente para un microcontrolador de la familia STM32F4 (basado en los archivos suministrados), explicando el flujo de ejecución, la evolución de variables críticas y la interacción entre temporizadores y el sistema operativo.

## 1. Análisis de los Archivos del Proyecto

### 📂 startup_stm32f446retx.s
Es el archivo de ensamblador que contiene el **Vector de Interrupciones** y el código de inicio del procesador.
* **Puntero de Pila (SP):** Inicializa el stack en la dirección más alta de la RAM.
* **Reset_Handler:** Es la primera función ejecutada tras un reset. Realiza la copia de la sección `.data` de Flash a RAM y limpia la sección `.bss` (pone a cero variables no inicializadas).
* **Saltos Iniciales:** Llama a `SystemInit` para la configuración básica del reloj y luego salta a `__libc_init_array` antes de entrar finalmente a la función `main()`.

### 📂 main.c
Es el punto de entrada de la aplicación en C. Sus funciones principales son:
* **Inicialización de HAL:** Llama a `HAL_Init()` para resetear periféricos y configurar el Tick de la HAL.
* **Configuración de Reloj:** `SystemClock_Config()` ajusta el PLL y los buses para que el MCU funcione a su máxima frecuencia.
* **Inicialización de Periféricos:** Configura GPIO, UART2 para depuración y el **TIM2** para estadísticas de FreeRTOS.
* **Lanzamiento del Kernel:** Define las tareas (como `defaultTask`) y arranca el planificador con `osKernelStart()`.

### 📂 stm32f4xx_it.c
Contiene los manejadores de interrupciones (ISR).
* Maneja excepciones del sistema como `HardFault_Handler`.
* Contiene los manejadores para **TIM1** (usado por la HAL) y **TIM2** (usado por FreeRTOS), derivando la ejecución a la biblioteca HAL mediante `HAL_TIM_IRQHandler`.

### 📂 FreeRTOSConfig.h
Archivo de configuración del núcleo de FreeRTOS.
* Define `configCPU_CLOCK_HZ` (frecuencia del CPU) y `configTICK_RATE_HZ` (frecuencia del tick del sistema, usualmente 1000Hz o 1ms).
* Mapea los manejadores de interrupciones nativos de ARM (`SVC_Handler`, `PendSV_Handler`, `SysTick_Handler`) a las funciones internas del kernel de FreeRTOS.

### 📂 freertos.c
Implementa la lógica específica del sistema operativo.
* Define las funciones de "Hook" (retrollamadas), como `vApplicationIdleHook` (ejecutada cuando no hay tareas activas) y la gestión de memoria estática para la tarea *Idle*.

---

## 2. Evolución de Variables Críticas

A continuación se describe cómo cambian las variables desde el `Reset_Handler` hasta llegar al punto previo al loop principal.

| Momento de Ejecución | `SystemCoreClock` | `SysTick` (Configuración/Registro) |
| :--- | :--- | :--- |
| **Reset_Handler** | Valor indeterminado (Basura en RAM). | Deshabilitado (0). |
| **SystemInit** | Se establece al valor por defecto (ej. 16 MHz HSI). | Inicializado según la frecuencia base. |
| **HAL_Init (en main)** | Se mantiene el valor inicial. | Se configura para generar una interrupción cada 1ms para la HAL. |
| **SystemClock_Config** | **Cambia** al valor final configurado (ej. 180 MHz). | Se re-ajusta para mantener el tick de 1ms con la nueva frecuencia. |
| **osKernelStart** | Se mantiene constante. | **FreeRTOS toma el control**: El registro de SysTick se configura para disparar la conmutación de tareas. |

---

## 3. Comportamiento del Programa (Startup hasta Loop Principal)

El flujo cronológico es el siguiente:

1.  **Hardware Reset:** El PC (Program Counter) carga la dirección de `Reset_Handler`.
2.  **Preparación de RAM:** Se copian las variables globales y estáticas de la Flash a la memoria volátil.
3.  **Configuración de Sistema:** `SystemInit` prepara el bus de memoria.
4.  **Entrada a Main:**
    * Se inicializa la capa HAL.
    * Se configura el reloj del sistema mediante PLL.
    * Se configuran los periféricos (UART2, GPIO).
    * Se inicializa el Timer 2 (`MX_TIM2_Init`) y se activa su interrupción (`HAL_TIM_Base_Start_IT`).
    * Se llama a `app_init()` (lógica de aplicación del usuario).
5.  **Creación de Tareas:** Se reserva memoria (TCB y Stack) para la tarea `defaultTask`.
6.  **Arranque del Scheduler:** Se ejecuta `osKernelStart()`.

**Nota Crítica:** El programa **nunca llega al `while(1)` de `main.c`**. Al llamar a `osKernelStart()`, el control del CPU pasa íntegramente al planificador de FreeRTOS. El `while(1)` al final de `main.c` es solo una medida de seguridad en caso de que el kernel falle al iniciar.

---

## 4. Interacción de SysTick y Timers con FreeRTOS

### SysTick
* **Cómo:** Está mapeado mediante la macro `xPortSysTickHandler` en `FreeRTOSConfig.h`.
* **Para qué:** Es el latido del sistema operativo. Genera la interrupción de "Tick" (cada 1ms). En cada tick, FreeRTOS decide si debe realizar un cambio de contexto (cambiar de una tarea a otra) y actualiza los contadores de tiempo de bloqueos (`osDelay`).

### Timer 1 (TIM1)
* **Interacción:** En este proyecto, el **TIM1** es utilizado por la HAL de STM32 en lugar del SysTick.
* **Para qué:** Debido a que FreeRTOS "secuestra" el SysTick para su funcionamiento interno, la HAL necesita otro temporizador para funciones como `HAL_Delay()`. El TIM1 incrementa la variable `uwTick` de la HAL independientemente de las tareas del RTOS.

---

## 5. Interacción del Timer 2 (TIM2) con la HAL y FreeRTOS

El Timer 2 tiene un rol especial relacionado con el análisis de rendimiento:

1.  **Interacción con la HAL:**
    * Se configura en `main.c` para generar interrupciones a una frecuencia muy alta.
    * Cada vez que desborda, se ejecuta `HAL_TIM_PeriodElapsedCallback`, el cual incrementa la variable global `ulHighFrequencyTimerTicks`.
2.  **Interacción con FreeRTOS:**
    * En `FreeRTOSConfig.h`, se habilitan las estadísticas de tiempo de ejecución (`configGENERATE_RUN_TIME_STATS`).
    * FreeRTOS utiliza las macros `portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()` (para iniciar el TIM2) y `portGET_RUN_TIME_COUNTER_VALUE()` (para leer `ulHighFrequencyTimerTicks`).
    * **Para qué:** Permite al desarrollador saber exactamente qué porcentaje de CPU está consumiendo cada tarea, proporcionando una base de tiempo mucho más precisa que el simple tick de 1ms.



    
# Análisis de la Aplicación (Event-Triggered System con FreeRTOS) - PASO 8

El código fuente adjunto conforma una aplicación basada en un sistema operativo en tiempo real (FreeRTOS) estructurada mediante **máquinas de estados (Statecharts)**. El paradigma del programa es un sistema disparado por eventos (Event-Triggered System), donde la pulsación de un botón altera el estado de un LED.

A continuación, se detalla el funcionamiento de cada archivo:

## 1. `app.c`: Inicialización de la Aplicación
Este archivo es el punto de arranque de la lógica de usuario del sistema operativo.
* **Inicialización (`app_init`)**: Se inicializan a cero los contadores globales del sistema (`g_app_tick_cnt`, `g_task_idle_cnt`, `g_app_stack_overflow_cnt`).
* **Creación de Tareas (Threads)**: Se instancian dos hilos de ejecución concurrentes utilizando la API `xTaskCreate` de FreeRTOS:
    1.  `task_btn`: Tarea encargada de leer el botón.
    2.  `task_led`: Tarea encargada de controlar el LED.
* **Prioridades**: Ambas tareas son creadas con la misma prioridad (`tskIDLE_PRIORITY + 1ul`, que equivale a prioridad 1) y el mismo tamaño de pila (stack) reservado.
* Al finalizar, se verifica la cantidad de memoria Heap restante y se inicializa el contador de ciclos del procesador.

## 2. `task_btn.c`: Lógica y Anti-rebote del Botón
Contiene un bucle infinito que implementa la tarea de escaneo del botón de usuario de la placa. Su núcleo es la máquina de estados `task_btn_statechart()`.
* **Lectura Cíclica**: En cada iteración, se lee el estado físico del pin del botón mediante la HAL (`HAL_GPIO_ReadPin`).
* **Máquina de Estados y Debounce (Anti-rebote)**: Posee cuatro estados principales para implementar de forma robusta el filtro anti-rebotes:
    * `ST_BTN_UP`: Estado de reposo. Si detecta presión, pasa a evaluar la caída.
    * `ST_BTN_FALLING`: Espera un tiempo `DEL_BTN_MAX` (50 ms, usando `xTaskGetTickCount()`). Si luego de ese tiempo el botón sigue presionado, se confirma el evento y se envía el comando `EV_LED_BLINK` a la tarea del LED.
    * `ST_BTN_DOWN`: El botón está presionado y validado. Espera a que se suelte para evaluar la subida.
    * `ST_BTN_RISING`: Realiza la misma validación de tiempo (50 ms) para evitar falsos rebotes al soltar el botón. Si es validado, envía el comando `EV_LED_OFF`.

## 3. `task_led_interface.c`: Interfaz de Comunicación
Actúa como una API o "puente" para desacoplar el módulo del LED del módulo del botón.
* **Función `put_event_task_led(event)`**: Recibe el evento disparado por el botón (parpadear o apagar) y escribe directamente sobre la estructura de datos privada del LED (`task_led_dta`). Levanta una bandera (`flag = true`) indicando que un nuevo evento está pendiente de ser procesado. *(Nota técnica: en un entorno estrictamente RTOS, esta comunicación por variables globales suele reemplazarse por Queues o Task Notifications para evitar problemas de concurrencia, aunque aquí se asume exclusión mutua por diseño).*

## 4. `task_led.c`: Control y Secuencia del LED
Implementa la tarea encargada de reaccionar a los eventos del botón y manejar el encendido, apagado o parpadeo del LED. Consta de la máquina de estados `task_led_statechart()`.
* **Recepción de Eventos**: Evalúa si la interfaz ha levantado la bandera de evento (`task_led_dta.flag == true`).
* **Estados**:
    * `ST_LED_OFF`: Si recibe el evento `EV_LED_BLINK`, baja la bandera de lectura, guarda la marca de tiempo (Tick actual), enciende el LED mediante la HAL de STM32 y pasa al estado de parpadeo.
    * `ST_LED_BLINK`: Si recibe el evento `EV_LED_OFF`, apaga el LED y vuelve al estado `OFF`. Si no recibe ninguna orden de apagado, utiliza retardos no bloqueantes analizando la resta entre el tiempo actual y el guardado; si superan `DEL_LED_MAX` (500 ms), invierte el estado del pin del LED (`HAL_GPIO_TogglePin`), generando así un parpadeo periódico.

## 5. `freertos.c`: Hooks (Callbacks) del Sistema Operativo
Contiene funciones de retrollamada (hooks) que el kernel de FreeRTOS invoca automáticamente ante ciertos eventos internos.
* **`vApplicationIdleHook`**: Se ejecuta exclusivamente cuando no hay tareas de usuario listas para procesar (es decir, el CPU está libre). El código incrementa la variable `g_task_idle_cnt`. Esto es muy valioso para poder medir, a posteriori, el porcentaje de carga y uso de CPU del microcontrolador.
* **`vApplicationTickHook`**: Se dispara con cada interrupción del "Tick" del RTOS (usualmente cada 1 milisegundo). Incrementa `g_app_tick_cnt`. Esta función debe ser extremadamente rápida ya que interrumpe el flujo normal del procesador.
* **`vApplicationStackOverflowHook`**: Una función crítica de depuración. Si FreeRTOS detecta que alguna de las tareas creadas (como `Task BTN` o `Task LED`) se ha quedado sin memoria en su pila asignada, salta aquí. El código utiliza `configASSERT( 0 )` y `taskENTER_CRITICAL()` para "congelar" y atrapar al procesador, permitiendo al programador inspeccionar el fallo en un entorno de depuración (debugger) y se suma al contador `g_app_stack_overflow_cnt`.


#TP1 – Actividad 02 – 5to Proyecto p/placa NUCLEO-F103RB con FreeRTOS

##Paso 02

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
Cuando una tarea ya no es útil, se debe llamar a vTaskDelete() para que FreeRTOS libere los recursos de memoria que ocupaba. Puedes eliminar otra tarea pasándole su Handle específico, o una tarea puede "suicidarse" llamando a vTaskDelete(NULL) (lo cual es obligatorio hacer en lugar de usar un return si se desea salir del bucle).