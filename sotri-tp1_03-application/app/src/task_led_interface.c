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
/* Project includes. */
#include "main.h"
#include "cmsis_os.h"

/* Demo includes. */
#include "logger.h"
#include "dwt.h"

/* Application & Tasks includes. */
#include "board.h"
#include "app.h"
#include "task_led_attribute.h"
#include "task_led_interface.h"

/********************** macros and definitions *******************************/

/********************** internal data declaration ****************************/
typedef struct
{
	bool flag;
	task_led_ev_t event;
} task_led_event_t;

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/
static task_led_event_t task_led_events[TASK_LED_COUNT];

/********************** external data declaration ****************************/

/********************** external functions definition ************************/
void put_event_task_led(uint32_t led_id, task_led_ev_t event)
{
	if (TASK_LED_COUNT <= led_id)
	{
		return;
	}

	taskENTER_CRITICAL();
	task_led_events[led_id].event = event;
	task_led_events[led_id].flag = true;
	taskEXIT_CRITICAL();
}

bool get_event_task_led(uint32_t led_id, task_led_ev_t *event)
{
	bool ret = false;

	if ((TASK_LED_COUNT <= led_id) || (NULL == event))
	{
		return false;
	}

	taskENTER_CRITICAL();
	if (true == task_led_events[led_id].flag)
	{
		*event = task_led_events[led_id].event;
		task_led_events[led_id].flag = false;
		ret = true;
	}
	taskEXIT_CRITICAL();

	return ret;
}

/********************** end of file ******************************************/
