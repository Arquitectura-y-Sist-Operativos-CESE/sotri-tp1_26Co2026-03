# TP1 – Actividad 03 – 6to Proyecto p/placa NUCLEO-F103RB con FreeRTOS

## 1. ¿Cómo usar el parámetro de Tarea (`pvParameters`)?

Cuándo creas una tarea con `xTaskCreate()`, el cuarto argumento es un puntero genérico de tipo `void *`. Esto te permite envíarle cualquier tipo de dato a la tarea al momento de su inicialización (un número, una cadena de texto o un puntero a una estructura compleja).

**Paso a paso:**
1. Defines el dato o la estructura que quieres pasar (debe ser global o estática para que no se destruya de la memoria antes de que la tarea la lea).
2. Lo pasas como parámetro en `xTaskCreate` haciendo un *cast* a `(void *)`.
3. Dentro de la tarea, haces el *cast* inverso para recuperar tus datos.

**Ejemplo de implementación:**

```c
/* 1. Definimos una estructura de datos para la configuración */
typedef struct {
    uint8_t task_id;
    uint32_t delay_ms;
} TaskConfig_t;

/* Instanciamos la configuración (DEBE ser global o estática) */
TaskConfig_t configTarea1 = { 1, 500 };
TaskConfig_t configTarea2 = { 2, 1000 };

void app_init(void) {
    /* 2. Pasamos la dirección de memoria de la configuración como parámetro */
    xTaskCreate(vMiTarea, "Tarea 1", 128, (void *)&configTarea1, 1, NULL);
    xTaskCreate(vMiTarea, "Tarea 2", 128, (void *)&configTarea2, 1, NULL);
}

/* 3. Dentro de la tarea, recuperamos el parámetro */
void vMiTarea(void *pvParameters) {
    /* Hacemos un cast del void* al tipo de dato original */
    TaskConfig_t *mi_config = (TaskConfig_t *)pvParameters;
    
    /* Extraemos los valores */
    uint8_t id = mi_config->task_id;
    uint32_t delay = pdMS_TO_TICKS(mi_config->delay_ms);

    for(;;) {
        LOGGER_INFO("Ejecutando tarea ID: %d", id);
        vTaskDelay(delay);
    }
}

```

## 2. ¿Cómo cambiar la prioridad de una Tarea ya creada?

Para modificar la prioridad de una tarea en tiempo de ejecución, FreeRTOS proporciona la API vTaskPrioritySet(). Esta función es muy útil cuando necesitas que una tarea adquiera mayor importancia dinámicamente frente a un evento crítico.

Sintaxis:
void vTaskPrioritySet( TaskHandle_t xTask, UBaseType_t uxNewPriority );

xTask: Es el Handle (manejador) de la tarea a la que le quieres cambiar la prioridad. Si le pasas NULL, la tarea cambiará su propia prioridad.

uxNewPriority: El número de la nueva prioridad (recuerda que los números más altos indican mayor prioridad).

Ejemplo de implementación:

```c
TaskHandle_t h_tarea_sensor;

void app_init(void) {
    // Creamos la tarea con prioridad baja (1) y guardamos su Handle
    xTaskCreate(vTareaSensor, "Sensor", 128, NULL, 1, &h_tarea_sensor);
}

void vOtraTarea(void *pvParameters) {
    for(;;) {
        if (detecta_emergencia()) {
            /* Subimos la prioridad de la tarea sensor a 3 (Alta) */
            vTaskPrioritySet(h_tarea_sensor, 3);
        }
        vTaskDelay(100);
    }
}

void vTareaSensor(void *pvParameters) {
    for(;;) {
        /* Si esta tarea necesita bajarse su PROPIA prioridad temporalmente */
        vTaskPrioritySet(NULL, 0); // Prioridad mínima (0)
        
        // ... lógica de la tarea ...
        vTaskDelay(50);
    }
}

```

Nota importante: Si al usar vTaskPrioritySet() le asignas a una tarea una prioridad mayor que la de la tarea que se está ejecutando actualmente, se producirá un cambio de contexto inmediato (preempción) y la CPU saltará a ejecutar esa tarea sin esperar al siguiente Tick.

## 3. Dos instancias de `task_btn` para gestionar botones diferentes

Se observa que la tarea instancienadose dos veces, puede manejar dos GPIO Input diferentes y envíar a la tarea del led los eventos configurados.

Se adjunta la salida de la consola:

```
xPSR: 0x01000000 pc: 0x08000c94 msp: 0x20020000, semihosting
[info]  
[info] app_init is running - Tick [mS] =   0
[info]  RTOS - Event-Triggered Systems (ETS)
[info]  soe-tp0_03-application: Demo Code
[info]  
[info] Task LED is running - Tick [mS] =   0
[info]  
[info] Task BTN 1 is running - Tick [mS] =   1
[info] Me llamaron Puerto 0x40020800 , Pin 8192
[info]  
[info] Task BTN 2 is running - Tick [mS] =   1
[info] Me llamaron Puerto 0x40020000 , Pin 256
[info]  Task BTN 1 - BTN PRESSED
[info]  Task LED - LED BLINK
[info]  Task BTN 1 - BTN HOVER
[info]  Task LED - LED OFF
[info]  Task BTN 2 - BTN PRESSED
[info]  Task LED - LED BLINK
[info]  Task BTN 2 - BTN HOVER
[info]  Task LED - LED OFF
[info]  Task BTN 2 - BTN PRESSED
[info]  Task LED - LED BLINK
[info]  Task BTN 2 - BTN HOVER
[info]  Task LED - LED OFF

```

## 4. Prioridad inicial de `task_led` y dos instancias

Para asegurar que las tareas de LED sean las primeras en ejecutar al arrancar el scheduler, se crearon con una prioridad temporal mayor:

```c
#define TASK_PRIORITY_NORMAL   (tskIDLE_PRIORITY + 1ul)
#define TASK_PRIORITY_STARTUP  (tskIDLE_PRIORITY + 2ul)

```

Las dos instancias de `task_led` se crean con `TASK_PRIORITY_STARTUP`. Al entrar en la tarea, cada instancia inicializa su GPIO, apaga el LED correspondiente y luego recupera la prioridad relativa original con:

```c
vTaskPrioritySet(NULL, led->normal_priority);

```

De esta forma, al inicio ejecutan primero `Task LED LD2` y `Task LED LD3`. Luego ambas vuelven a prioridad normal, igual que las tareas de botones.

También se instanciaron dos tareas `task_led`, una para `LD2` y otra para `led3`, usando parámetros persistentes:

```c
#define TASK_LED_LD2_ID  0u
#define TASK_LED_LD3_ID  1u

task_led_parameters_t ld2 = {LD2_GPIO_Port, LD2_Pin, TASK_LED_LD2_ID, TASK_PRIORITY_NORMAL};
task_led_parameters_t ld3 = {led3_GPIO_Port, led3_Pin, TASK_LED_LD3_ID, TASK_PRIORITY_NORMAL};

```

Cada botón tiene asociado un `led_id`, por lo que `Task BTN 1` envía eventos al LED 0 y `Task BTN 2` envía eventos al LED 1. Para evitar que las dos instancias de `task_led` se pisen entre sí, se reemplazó el evento global único por una tabla de eventos indexada por `led_id`.

Comportamiento observado/esperado en depuración:

```text
[info] Task LED LD2 is running - Tick [mS] = 0
[info] LED: Me llamaron Puerto 0x40020000 , Pin 32
[info] Task LED LD3 is running - Tick [mS] = 0
[info] LED: Me llamaron Puerto 0x40020400 , Pin 256
[info] Task BTN 1 is running - Tick [mS] = 1
[info] Task BTN 2 is running - Tick [mS] = 1
[info] Task BTN 1 - BTN PRESSED
[info] Task LED LD2 - LED BLINK
[info] Task BTN 1 - BTN HOVER
[info] Task LED LD2 - LED OFF
[info] Task BTN 2 - BTN PRESSED
[info] Task LED LD3 - LED BLINK
[info] Task BTN 2 - BTN HOVER
[info] Task LED LD3 - LED OFF

```

Conclusión: la prioridad temporal mayor fuerza que las tareas de LED ejecuten primero su inicialización. Una vez restaurada la prioridad normal, las cuatro tareas quedan al mismo nivel relativo y los botones controlan LEDs distintos sin compartir el mismo estado interno.
