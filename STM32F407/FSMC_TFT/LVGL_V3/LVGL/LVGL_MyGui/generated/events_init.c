/*
* Copyright 2025 NXP
* NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "events_init.h"
#include <stdio.h>
#include "lvgl.h"

#if LV_USE_GUIDER_SIMULATOR && LV_USE_FREEMASTER
#include "freemaster_client.h"
#endif


static void screen_btn_2_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
	case LV_EVENT_CLICKED:
	{
		static uint8_t img_index = 0;
				lv_ui *ui = lv_event_get_user_data(e);
				
				const void *images[] = {
						&_clear_alpha_100x100,
						&_cloudy_alpha_100x100,
						&_Cloud_alpha_100x100
				};
				
				img_index = (img_index + 1) % 3;
				lv_img_set_src(ui->screen_img_1, images[img_index]);
		break;
	}
    default:
        break;
    }
}

void events_init_screen (lv_ui *ui)
{
	lv_obj_add_event_cb(ui->screen_btn_2, screen_btn_2_event_handler, LV_EVENT_ALL, ui);
}


void events_init(lv_ui *ui)
{

}
