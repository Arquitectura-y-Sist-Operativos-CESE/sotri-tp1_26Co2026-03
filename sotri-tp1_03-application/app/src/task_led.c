/*
 * Copyright (c) 2026 Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @author : Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>
 */

/********************** inclusions *******************************************/
/* Project includes */
#include "main.h"
#include "cmsis_os.h"

/* Demo includes */
#include "logger.h"
#include "dwt.h"

/* Application & Tasks includes */
#include "board.h"
#include "app.h"
#include "task_led_attribute.h"
#include "task_led_interface.h"
#include "task_led.h"

/********************** macros and definitions *******************************/
#define DEL_LED_MIN			0ul
#define DEL_LED_MED			250ul
#define DEL_LED_MAX			500ul

/********************** internal functions declaration ***********************/
static void task_led_statechart(task_led_dta_t *data);
/********************** internal data definition *****************************/

/********************** external data declaration ****************************/

/********************** external functions definition ************************/
/* Task LED thread */
void task_led(void *parameters)
{
	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("%s is running - Tick [mS] = %3d", pcTaskGetName(NULL), (int)xTaskGetTickCount());

	task_led_parameters_t *led = (task_led_parameters_t *)parameters;
	LOGGER_INFO("LED: Me llamaron Puerto %p , Pin %u", (void *)led->gpio_port, led->pin);

	task_led_dta_t dta ={
			//false, EV_LED_OFF, ST_LED_OFF, DEL_LED_MIN,
				.flag= false,
				.event= EV_LED_OFF,
				.state= ST_LED_OFF,
				.tick= DEL_LED_MIN,
				.gpio_port= led->gpio_port,
				.pin= led->pin,
				.led_id= led->led_id,
	};

	HAL_GPIO_WritePin(dta.gpio_port, dta.pin, LED_OFF);
	vTaskPrioritySet(NULL, led->normal_priority);

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
	{
		/* Print out: Task execution */
		//LOGGER_INFO(" %s - Tick [mS] = %3d", pcTaskGetName(NULL), (int)xTaskGetTickCount());

		/* Run Task Statechart */
    	task_led_statechart(&dta);
    	vTaskDelay(10);
	}
}

static void task_led_statechart(task_led_dta_t *dta)
{
	task_led_ev_t event = EV_LED_OFF;
	bool has_event = get_event_task_led(dta->led_id, &event);

	switch (dta->state)
	{
		case ST_LED_OFF:

			if ((true == has_event) && (EV_LED_BLINK == event))
			{
				/* Print out: Task execution */
				LOGGER_INFO(" %s - LED BLINK", pcTaskGetName(NULL));

				dta->tick = xTaskGetTickCount();
				dta->state = ST_LED_BLINK;
				HAL_GPIO_WritePin(dta->gpio_port, dta->pin, LED_ON);
			}

			break;

		case ST_LED_BLINK:

			if ((true == has_event) && (EV_LED_OFF == event))
			{
				/* Print out: Task execution */
				LOGGER_INFO(" %s - LED OFF", pcTaskGetName(NULL));

				dta->state = ST_LED_OFF;
				HAL_GPIO_WritePin(dta->gpio_port, dta->pin, LED_OFF);
			}
			else
			{
				if (DEL_LED_MAX <= (xTaskGetTickCount() - dta->tick))
				{
					dta->tick = xTaskGetTickCount();
					HAL_GPIO_TogglePin(dta->gpio_port, dta->pin);
				}
			}

			break;

		default:

			dta->state = ST_LED_OFF;
			dta->tick  = xTaskGetTickCount();
			HAL_GPIO_WritePin(dta->gpio_port, dta->pin, LED_OFF);

			break;
	}
}

/********************** end of file ******************************************/
