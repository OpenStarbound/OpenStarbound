/**********************************************************************
 *
 * Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
 * Copyright(c) 2008 Imagination Technologies Ltd. All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope it will be useful but, except 
 * as otherwise stated in writing, without any warranty; without even the 
 * implied warranty of merchantability or fitness for a particular purpose. 
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * 
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information:
 * Imagination Technologies Ltd. <gpl-support@imgtec.com>
 * Home Park Estate, Kings Langley, Herts, WD4 8LZ, UK 
 *
 ******************************************************************************/

#ifndef __BC_CAT_H__
#define __BC_CAT_H__

#include <linux/ioctl.h>

#define BC_FOURCC(a,b,c,d) \
    ((unsigned long) ((a) | (b)<<8 | (c)<<16 | (d)<<24))

#define BC_PIX_FMT_NV12     BC_FOURCC('N', 'V', '1', '2') /*YUV 4:2:0*/
#define BC_PIX_FMT_UYVY     BC_FOURCC('U', 'Y', 'V', 'Y') /*YUV 4:2:2*/
#define BC_PIX_FMT_YUYV     BC_FOURCC('Y', 'U', 'Y', 'V') /*YUV 4:2:2*/
#define BC_PIX_FMT_RGB565   BC_FOURCC('R', 'G', 'B', 'P') /*RGB 5:6:5*/

enum BC_memory {
    BC_MEMORY_MMAP          = 1,
    BC_MEMORY_USERPTR       = 2,
};

typedef struct BCIO_package_TAG {
    int input;
    int output;
}BCIO_package;

/* 
 * the following types are tested for fourcc in struct bc_buf_params_t
 *   NV12
 *   UYVY
 *   RGB565 - not tested yet
 *   YUYV
 */
typedef struct bc_buf_params {
    int count;              /*number of buffers, [in/out]*/
    int width;              /*buffer width in pixel, multiple of 8 or 32*/
    int height;             /*buffer height in pixel*/
    unsigned int fourcc;    /*buffer pixel format*/
    enum BC_memory type;
} bc_buf_params_t;

typedef struct bc_buf_ptr {
    unsigned int index;
    int size;
    unsigned long pa;
} bc_buf_ptr_t;

#define BCIO_GID                    'g'
#define BC_IOWR(INDEX)            _IOWR(BCIO_GID, INDEX, BCIO_package)

#define BCIOGET_BUFFERCOUNT       BC_IOWR(0)  /*obsolete, since BCIOREQ_BUFFERS
                                                return the number of buffers*/
#define BCIOGET_BUFFERPHYADDR     BC_IOWR(1)  /*get physical address by index*/
#define BCIOGET_BUFFERIDX         BC_IOWR(2)  /*get index by physical address*/

#define BCIOREQ_BUFFERS           BC_IOWR(3)
#define BCIOSET_BUFFERPHYADDR     BC_IOWR(4)

#endif 

