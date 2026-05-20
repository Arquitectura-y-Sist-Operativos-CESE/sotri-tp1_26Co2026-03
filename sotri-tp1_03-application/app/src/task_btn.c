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
#include "task_btn_attribute.h"
#include "task_led_attribute.h"
#include "task_led_interface.h"
#include "task_btn.h"

/********************** macros and definitions *******************************/
#define DEL_BTN_MIN			0ul
#define DEL_BTN_MED			25ul
#define DEL_BTN_MAX			50ul

#define EV_SYS_IDLE			0ul
#define EV_SYS_LOOP_DET		1ul

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/
static void task_btn_statechart(task_btn_dta_t *data);

/********************** internal data definition *****************************/

/********************** external data declaration ****************************/

/********************** external functions definition ************************/
/* Task BTN thread */
void task_btn(void *parameters)
{
	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("%s is running - Tick [mS] = %3d", pcTaskGetName(NULL), (int)xTaskGetTickCount());



	/* Hacemos un cast del void* al tipo de dato original */
	task_btn_parameters_t *button = (task_btn_parameters_t *)parameters;
	LOGGER_INFO("Me llamaron Puerto %p , Pin %u", (void *)button->gpio_port, button->pin);

	task_btn_dta_t dta = {
	        .event = EV_BTN_UP,
	        .state = ST_BTN_UP,
	        .tick = DEL_BTN_MIN,
	        .gpio_port = button->gpio_port,
	        .pin = button->pin,
	    };



	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
	{
		/* Print out: Task execution */
		//LOGGER_INFO(" %s - Tick [mS] = %3d", pcTaskGetName(NULL), (int)xTaskGetTickCount());

		/* Run Task Statechart */
    	task_btn_statechart(&dta);

    	vTaskDelay(100);
	}
}

static void task_btn_statechart(task_btn_dta_t *data)
{

	/* Get Events to excite Task */
	if (BTN_PRESSED == HAL_GPIO_ReadPin(data->gpio_port, data->pin))
	{
		data->event = EV_BTN_DOWN;
	}
	else
	{
		data->event = EV_BTN_UP;
	}

	/* Run to Completion Statechart */
	switch (data->state)
	{
		case ST_BTN_UP:

			if (EV_BTN_DOWN == data->event)
			{
				data->tick = xTaskGetTickCount();
				data->state = ST_BTN_FALLING;
			}

			break;

		case ST_BTN_FALLING:

			if (DEL_BTN_MAX <= (xTaskGetTickCount() - data->tick))
			{
				if (EV_BTN_DOWN == data->event)
				{
					/* Print out: Task execution */
					LOGGER_INFO(" %s - BTN PRESSED", pcTaskGetName(NULL));

					put_event_task_led(EV_LED_BLINK);
					data->state = ST_BTN_DOWN;
				}
				else
				{
					data->state = ST_BTN_UP;
				}
			}

			break;

		case ST_BTN_DOWN:

			if (EV_BTN_UP == data->event)
			{
				data->tick = xTaskGetTickCount();
				data->state = ST_BTN_RISING;
			}

			break;

		case ST_BTN_RISING:

			if (DEL_BTN_MAX <= (xTaskGetTickCount() - data->tick))
			{
				if (EV_BTN_UP == data->event)
				{
					/* Print out: Task execution */
					LOGGER_INFO(" %s - BTN HOVER", pcTaskGetName(NULL));

					put_event_task_led(EV_LED_OFF);
					data->state = ST_BTN_UP;
				}
				else
				{
					data->state = ST_BTN_DOWN;
				}
			}

			break;

		default:

			data->tick  = xTaskGetTickCount();
			data->state = ST_BTN_UP;
			data->event = EV_BTN_UP;

			break;
	}

}

/********************** end of file ******************************************/
