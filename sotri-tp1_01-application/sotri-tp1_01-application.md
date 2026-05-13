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