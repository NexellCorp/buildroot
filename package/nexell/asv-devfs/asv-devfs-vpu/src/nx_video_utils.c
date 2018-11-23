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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <linux/videodev2.h>
#include <linux/videodev2_nxp_media.h>

#include "nx_video_utils.h"

//------------------------------------------------------------------------------
static int32_t is_regular( const char *file )
{
	struct stat statinfo;
	if( 0 > stat( file, &statinfo) )
	{
		return 0;
	}

	return S_ISREG( statinfo.st_mode );
}

//------------------------------------------------------------------------------
static int32_t is_directory( const char *file )
{
	struct stat statinfo;
	if( 0 > stat( file, &statinfo) )
	{
		return 0;
	}

	return S_ISDIR( statinfo.st_mode );
}

//------------------------------------------------------------------------------
static int32_t get_size( const char *file )
{
	struct stat statinfo;
	if( 0 > stat( file, &statinfo) )
	{
		return 0;
	}

	return statinfo.st_size;
}

//------------------------------------------------------------------------------
static int32_t get_chroma_size( NX_VID_MEMORY_INFO *memory, int32_t *width, int32_t *height )
{
	if( NULL == memory )
	{
		printf("fail, get_chroma_size().\n");
		return -1;
	}

	switch( memory->format )
	{
	case V4L2_PIX_FMT_GREY:
		*width  = 0;
		*height = 0;
		break;

	case V4L2_PIX_FMT_YUV420:
	case V4L2_PIX_FMT_YUV420M:
	case V4L2_PIX_FMT_YVU420:
	case V4L2_PIX_FMT_YVU420M:
		*width  = memory->width >> 1;
		*height = memory->height >> 1;
		break;

	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV12M:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_NV21M:
		*width  = memory->width;
		*height = memory->height >> 1;
		break;

	case V4L2_PIX_FMT_YUV422P:
	case V4L2_PIX_FMT_YUV422M:
		*width  = memory->width >> 1;
		*height = memory->height;
		break;

	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV16M:
	case V4L2_PIX_FMT_NV61:
	case V4L2_PIX_FMT_NV61M:
		*width  = memory->width;
		*height = memory->height;
		break;

	case V4L2_PIX_FMT_YUV444:
	case V4L2_PIX_FMT_YUV444M:
		*width  = memory->width << 1;
		*height = memory->height;
		break;

	case V4L2_PIX_FMT_NV24:
	case V4L2_PIX_FMT_NV24M:
	case V4L2_PIX_FMT_NV42:
	case V4L2_PIX_FMT_NV42M:
		*width  = memory->width;
		*height = memory->height;
		break;

	default:
		break;
	}

	return 0;
}

//------------------------------------------------------------------------------
int32_t nx_stream_dump( NX_STREAM_INFO* stream, const char *fmt, ... )
{
	FILE *fp;
	va_list args;
	char name[1024];
	NX_STREAM_INFO *info = NULL;

	memset( name, 0x00, sizeof(name) );

	va_start(args, fmt);
	vsnprintf(name, sizeof(name), fmt, args);
	va_end(args);

	fp = fopen( name, "wb" );
	if( fp )
	{
		if( stream && stream->buf )
		{
			fwrite( stream->buf, 1, stream->size, fp );
			return 0;
		}
		fclose(fp);
	}
	return -1;
}

//------------------------------------------------------------------------------
int32_t nx_stream_load( NX_STREAM_INFO** stream, const char *fmt, ... )
{
	FILE *fp;
	va_list args;
	char name[1024];
	NX_STREAM_INFO *info = NULL;

	memset( name, 0x00, sizeof(name) );

	va_start(args, fmt);
	vsnprintf(name, sizeof(name), fmt, args);
	va_end(args);

	fp = fopen( name, "rb" );
	if( fp )
	{
		info = (NX_STREAM_INFO*)malloc( sizeof(NX_STREAM_INFO) );
		info->size = get_size(name);
		info->buf  = (char*)malloc( info->size );

		fread( info->buf, 1, info->size, fp );
		fclose( fp );
	}

	*stream = info;
	return info ? 0 : -1;
}

//------------------------------------------------------------------------------
void nx_stream_free( NX_STREAM_INFO* stream )
{
	if( stream )
	{
		if( stream->buf ) free( stream->buf );
		free( stream );
		stream = NULL;
	}
}

//------------------------------------------------------------------------------
int32_t nx_memory_dump( NX_VID_MEMORY_INFO* memory, const char *fmt, ... )
{
	FILE *fp;
	va_list args;
	char name[1024];
	int32_t width, height;
	int32_t lwidth, lheight;
	int32_t cwidth, cheight;
	int32_t stride;
	int32_t i, h;
	uint8_t *ptr;

	memset( name, 0x00, sizeof(name) );

	va_start(args, fmt);
	vsnprintf(name, sizeof(name), fmt, args);
	va_end(args);

	if( NULL == memory )
	{
		printf("fail, input memory. ( %p )\n", memory);
		return -1;
	}

	if( NULL == memory->pBuffer[0] )
	{
		if( 0 > NX_MapVideoMemory(memory) )
		{
			printf("fail, NX_MapVideoMemory().\n");
			return -1;
		}
	}

	fp = fopen( name, "wb" );
	if( NULL == fp )
	{
		printf("fail, fopen(). ( %s )\n", name );
		return -1;
	}

	lwidth  = memory->width;
	lheight = memory->height;
	get_chroma_size( memory, &cwidth, &cheight );

	for( i = 0; i < memory->planes; i++ )
	{
		ptr    = (uint8_t*)memory->pBuffer[i];
		stride = memory->stride[i];
		width  = (i==0) ? lwidth : cwidth;
		height = (i==0) ? lheight : cheight;

		for( h = 0; h < height; h++ )
		{
			fwrite( ptr, 1, width, fp );
			ptr += stride;
		}
	}

	fclose( fp );
	return 0;
}

//------------------------------------------------------------------------------
int32_t nx_memory_load( NX_VID_MEMORY_INFO* memory, const char *fmt, ... )
{
	FILE *fp;
	va_list args;
	char name[1024];
	int32_t width, height;
	int32_t lwidth, lheight;
	int32_t cwidth, cheight;
	int32_t stride;
	int32_t i, h;
	uint8_t *buf;
	NX_STREAM_INFO* stream;
	uint8_t *in_ptr, *out_ptr;

	memset( name, 0x00, sizeof(name) );

	va_start(args, fmt);
	vsnprintf(name, sizeof(name), fmt, args);
	va_end(args);

	if( NULL == memory )
	{
		printf("fail, output memory. ( %p )\n", memory);
		return -1;
	}

	if( NULL == memory->pBuffer[0] )
	{
		if( 0 > NX_MapVideoMemory(memory) )
		{
			printf("fail, NX_MapVideoMemory().\n");
			return -1;
		}
	}

	if( 0 > nx_stream_load( &stream, name ) )
	{
		printf("fail, nx_stream_load().\n");
		return -1;
	}

	lwidth  = memory->width;
	lheight = memory->height;
	get_chroma_size( memory, &cwidth, &cheight );

	in_ptr  = stream->buf;

	for( i = 0; i < memory->planes; i++ )
	{
		out_ptr = memory->pBuffer[i];
		stride  = memory->stride[i];
		width   = (i==0) ? lwidth : cwidth;
		height  = (i==0) ? lheight : cheight;

		for( h = 0; h < height; h++ )
		{
			memcpy( out_ptr, in_ptr, width );
			out_ptr += stride;
			in_ptr  += width;
		}
	}

	nx_stream_free( stream );
	return 0;
}

//------------------------------------------------------------------------------
int32_t nx_memory_compare( NX_VID_MEMORY_INFO* src_memory, NX_VID_MEMORY_INFO* dst_memory )
{
	int32_t ret = 0;
	int32_t width, height;
	int32_t lwidth, lheight;
	int32_t cwidth, cheight;
	int32_t stride;
	int32_t i, w, h;
	uint8_t *src_ptr, *dst_ptr;

	if( NULL == src_memory )
	{
		printf("fail, input memory.\n");
		return -1;
	}

	if( NULL == dst_memory )
	{
		printf("fail, output memory.\n");
		return -1;
	}

	if( NULL == src_memory->pBuffer[0] )
	{
		if( 0 > NX_MapVideoMemory( src_memory ) )
		{
			printf("fail, NX_MapVideoMemory().\n");
			return -1;
		}
	}

	if( NULL == dst_memory->pBuffer[0] )
	{
		if( 0 > NX_MapVideoMemory( dst_memory ) )
		{
			printf("fail, NX_MapVideoMemory().\n");
			return -1;
		}
	}

	if( (src_memory->width != dst_memory->width) ||
		(src_memory->height != dst_memory->height) )
	{
		printf("fail, memory size.\n");
		return -1;
	}

	lwidth  = src_memory->width;
	lheight = src_memory->height;
	get_chroma_size( src_memory, &cwidth, &cheight );

	for( i = 0; i < src_memory->planes; i++ )
	{
		src_ptr = src_memory->pBuffer[i];
		dst_ptr = dst_memory->pBuffer[i];
		stride  = src_memory->stride[i];
		width   = (i==0) ? lwidth : cwidth;
		height  = (i==0) ? lheight : cheight;

		for( h = 0; h < height; h++ )
		{
			if( 0 != memcmp( src_ptr, dst_ptr, width ) )
			{
				printf("fail, memcmp(). ( plane: %d, line: %d, src: %p, dst: %p )\n", i, h, src_ptr, dst_ptr );
				ret = -1;
				return ret;
			}

			src_ptr += stride;
			dst_ptr += stride;
		}
	}

	return ret;
}

//------------------------------------------------------------------------------
int32_t nx_get_plane_num( uint32_t fourcc )
{
	int32_t plane_num = 0;
	switch( fourcc )
	{
		case V4L2_PIX_FMT_YUV420:
		case V4L2_PIX_FMT_YUV420M:
		case V4L2_PIX_FMT_YVU420:
		case V4L2_PIX_FMT_YVU420M:
		case V4L2_PIX_FMT_YUV422P:
		case V4L2_PIX_FMT_YUV422M:
		case V4L2_PIX_FMT_YUV444:
		case V4L2_PIX_FMT_YUV444M:
			plane_num = 3;
			break;

		case V4L2_PIX_FMT_NV12:
		case V4L2_PIX_FMT_NV12M:
		case V4L2_PIX_FMT_NV21:
		case V4L2_PIX_FMT_NV21M:
		case V4L2_PIX_FMT_NV16:
		case V4L2_PIX_FMT_NV16M:
		case V4L2_PIX_FMT_NV61:
		case V4L2_PIX_FMT_NV61M:
		case V4L2_PIX_FMT_NV24:
		case V4L2_PIX_FMT_NV24M:
		case V4L2_PIX_FMT_NV42:
		case V4L2_PIX_FMT_NV42M:
			plane_num = 2;
			break;

		case V4L2_PIX_FMT_GREY:
			plane_num = 1;
			break;

		default:
			break;
	}

	return plane_num;
}

//------------------------------------------------------------------------------
const char* nx_get_format_string( uint32_t fourcc )
{
	switch( fourcc )
	{
		/* Memory Format */
		case V4L2_PIX_FMT_YUV420:	return "YUV420(I420) - YUV420 planar";
		case V4L2_PIX_FMT_YUV420M:	return "YUV420M(I420) - YUV420 non-continuous planar";
		case V4L2_PIX_FMT_YVU420:	return "YVU420(YV12) - YUV420 planar";
		case V4L2_PIX_FMT_YVU420M:	return "YVU420M(YV12) - YUV420 non-continuous planar";
		case V4L2_PIX_FMT_YUV422P:	return "YUV422 - YUV422 planar";
		case V4L2_PIX_FMT_YUV422M:	return "YUV422M - YUV422 non-continuous planar";
		case V4L2_PIX_FMT_YUV444:	return "YUV444 - YUV444 planar";
		case V4L2_PIX_FMT_YUV444M:	return "YUV444M - YUV444 non-continuous planar";
		case V4L2_PIX_FMT_NV12:		return "NV12 - YUV420 planar interleaved";
		case V4L2_PIX_FMT_NV12M:	return "NV12M - YUV420 non-continuous planar interleaved";
		case V4L2_PIX_FMT_NV21:		return "NV21 - YVU420 planar interleaved";
		case V4L2_PIX_FMT_NV21M:	return "NV21M - YVU420 non-continuous planar interleaved";
		case V4L2_PIX_FMT_NV16:		return "NV16 - YUV422 planar interleaved";
		case V4L2_PIX_FMT_NV16M:	return "NV16M - YUV422 non-continuous planar interleaved";
		case V4L2_PIX_FMT_NV61:		return "NV61 - YVU422 planar interleaved";
		case V4L2_PIX_FMT_NV61M:	return "NV61M - YVU422 non-continuous planar interleaved";
		case V4L2_PIX_FMT_NV24:		return "NV24 - YUV444 planar interleaved";
		case V4L2_PIX_FMT_NV24M:	return "NV24M - YUV444 non-continuous planar interleaved";
		case V4L2_PIX_FMT_NV42:		return "NV42 - YVU444 planar interleaved";
		case V4L2_PIX_FMT_NV42M:	return "NV42M - YVU444 non-continuous planar interleaved";
		case V4L2_PIX_FMT_GREY:		return "GREY - Y800 planar";
		/* Compressed Format */
		case V4L2_PIX_FMT_H264:		return "H264 Compressed format";
		case V4L2_PIX_FMT_MPEG2:	return "MPEG2 Compressed format";
		case V4L2_PIX_FMT_DIV3:		return "DIV3 Compressed format";
		case V4L2_PIX_FMT_XVID:		return "XVID Compressed format";
		case V4L2_PIX_FMT_DIVX:		return "DIVX Compressed format";
		case V4L2_PIX_FMT_DIV4:		return "DIV4 Compressed format";
		case V4L2_PIX_FMT_DIV5:		return "DIV5 Compressed format";
		case V4L2_PIX_FMT_DIV6:		return "DIV6 Compressed format";
		case V4L2_PIX_FMT_MPEG4:	return "MPEG4 Compressed format";
		case V4L2_PIX_FMT_H263:		return "H263 Compressed format";
		case V4L2_PIX_FMT_WMV9:		return "WMV9 Compressed format";
		case V4L2_PIX_FMT_WVC1:		return "WVC1 Compressed format";
		case V4L2_PIX_FMT_RV8:		return "RV8 Compressed format";
		case V4L2_PIX_FMT_RV9:		return "RV9 Compressed format";
		case V4L2_PIX_FMT_VP8:		return "VP8 Compressed format";
		case V4L2_PIX_FMT_FLV1:		return "FLV1 Compressed format";
		case V4L2_PIX_FMT_THEORA:	return "THEORA Compressed format";
		case V4L2_PIX_FMT_MJPEG:	return "MJPEG Compressed format";
		default: break;
	}
	return "Unknown format";
}
