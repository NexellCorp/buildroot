//------------------------------------------------------------------------------
//
//  Copyright (C) 2018 Nexell Co. All Rights Reserved
//  Nexell Co. Proprietary & Confidential
//
//  NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND	WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
//
//  Module	:
//  File	:
//  Description	:
//  Author	:
//  Export	:
//  History	:
//
//------------------------------------------------------------------------------

#ifndef __NX_VIDEO_UTILS_H__
#define __NX_VIDEO_UTILS_H__

#include <stdint.h>
#include "nx_video_alloc.h"

typedef struct NX_STREAM_INFO {
	char*	buf;
	int32_t size;
} NX_STREAM_INFO;

int32_t nx_stream_dump( NX_STREAM_INFO* stream, const char *fmt, ... );
int32_t nx_stream_load( NX_STREAM_INFO** stream, const char *fmt, ... );
void	nx_stream_free( NX_STREAM_INFO* stream );

int32_t nx_memory_dump( NX_VID_MEMORY_INFO* memory, const char *fmt, ... );
int32_t nx_memory_load( NX_VID_MEMORY_INFO* memory, const char *fmt, ... );

int32_t nx_memory_compare( NX_VID_MEMORY_INFO* src_memory, NX_VID_MEMORY_INFO* dst_memory );

int32_t nx_get_plane_num( uint32_t fourcc );
const char* nx_get_format_string( uint32_t fourcc );

#endif	// __NX_VIDEO_UTILS_H__
