
# TP1 – Actividad 03 – 6to Proyecto p/placa NUCLEO-F103RB con FreeRTOS

## 1. ¿Cómo usar el parámetro de Tarea (`pvParameters`)?

Cuando creas una tarea con `xTaskCreate()`, el cuarto argumento es un puntero genérico de tipo `void *`. Esto te permite enviarle cualquier tipo de dato a la tarea al momento de su inicialización (un número, una cadena de texto o un puntero a una estructura compleja).

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