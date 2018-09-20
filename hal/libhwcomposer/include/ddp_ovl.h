/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _DDP_OVL_H_
#define _DDP_OVL_H_

#define OVL_MAX_WIDTH  (4095)
#define OVL_MAX_HEIGHT (4095)

#define OVL_LAYER_NUM  (4)
#define OVL_LAYER_NUM_PER_OVL 4

typedef enum {
	DDP_OVL1_STATUS_IDLE = 0,
	DDP_OVL1_STATUS_PRIMARY = 1,	/* used for primary 8 layer blending */
	DDP_OVL1_STATUS_SUB = 2,	/* used for sub 4 layer bleding */
	DDP_OVL1_STATUS_SUB_REQUESTING = 3,	/* sub request to use OVL1 */
	DDP_OVL1_STATUS_PRIMARY_RELEASED = 4,
	DDP_OVL1_STATUS_PRIMARY_RELEASING = 5,
	DDP_OVL1_STATUS_PRIMARY_DISABLE = 6
} DISP_OVL1_STATUS;

typedef enum {
	DISP_MODULE_OVL0 = 0,
	DISP_MODULE_OVL1,
	DISP_MODULE_RDMA0,
	DISP_MODULE_RDMA1,
	DISP_MODULE_WDMA0,
	DISP_MODULE_COLOR0,
	DISP_MODULE_CCORR,
	DISP_MODULE_AAL,
	DISP_MODULE_GAMMA,
	DISP_MODULE_DITHER,
	DISP_MODULE_UFOE,	/* 10 */
	DISP_MODULE_PWM0,
	DISP_MODULE_WDMA1,
	DISP_MODULE_DSI0,
	DISP_MODULE_DPI,
	DISP_MODULE_SMI,
	DISP_MODULE_CONFIG,
	DISP_MODULE_CMDQ,
	DISP_MODULE_MUTEX,

	DISP_MODULE_COLOR1,
	DISP_MODULE_RDMA2,
	DISP_MODULE_PWM1,
	DISP_MODULE_OD,
	DISP_MODULE_MERGE,
	DISP_MODULE_SPLIT0,
	DISP_MODULE_SPLIT1,
	DISP_MODULE_DSI1,
	DISP_MODULE_DSIDUAL,

	DISP_MODULE_SMI_LARB0,
	DISP_MODULE_SMI_COMMON,
	DISP_MODULE_UNKNOWN,	/* 20 */
	DISP_MODULE_NUM
} DISP_MODULE_ENUM;

/* start overlay module */
int ovl_start(DISP_MODULE_ENUM module, void *handle);

/* stop overlay module */
int ovl_stop(DISP_MODULE_ENUM module, void *handle);

/* reset overlay module */
int ovl_reset(DISP_MODULE_ENUM module, void *handle);

/* set region of interest */
int ovl_roi(DISP_MODULE_ENUM module, unsigned int bgW, unsigned int bgH,	/* region size */
	    unsigned int bgColor,	/* border color */

	    void *handle);

/* switch layer on/off */
int ovl_layer_switch(DISP_MODULE_ENUM module, unsigned layer, unsigned int en, void *handle);
/* get ovl input address */
void ovl_get_address(DISP_MODULE_ENUM module, unsigned long *add);

int ovl_3d_config(DISP_MODULE_ENUM module,
		  unsigned int layer_id,
		  unsigned int en_3d, unsigned int landscape, unsigned int r_first, void *handle);

void ovl_dump_analysis(DISP_MODULE_ENUM module);
void ovl_dump_reg(DISP_MODULE_ENUM module);

void ovl_get_info(int idx, void *data);
void ovl_reset_by_cmdq(void *handle, DISP_MODULE_ENUM module);

DISP_OVL1_STATUS ovl_get_status(void);
void ovl_set_status(DISP_OVL1_STATUS status);
unsigned int ddp_ovl_get_cur_addr(bool rdma_mode, int layerid);

#endif
