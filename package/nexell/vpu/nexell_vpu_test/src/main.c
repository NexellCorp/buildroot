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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <linux/videodev2.h>
#include <linux/videodev2_nxp_media.h>

#include "nx_video_api.h"
#include "nx_video_utils.h"
#include "nx_list.h"

//------------------------------------------------------------------------------
#define NX_IMAGE_FORMAT		V4L2_PIX_FMT_YUV420
#define NX_TIME_EXPECTED	10000
#define NX_TIME_COMPARE		2000

enum {
	NX_RET_SUCCESS					=  0,
	NX_RET_FAIL_DECODE				= -1,
	NX_RET_FAIL_STREAM_DATA			= -2,
	NX_RET_FAIL_STREAM_LOAD			= -3,
	NX_RET_FAIL_GOLDEN_DATA			= -4,
	NX_RET_FAIL_GOLDEN_LOAD			= -5,
	NX_RET_FAIL_DATA_COMPARE		= -6,
};

//------------------------------------------------------------------------------
typedef struct NX_APP_INFO {
	int32_t		width;
	int32_t		height;
	uint32_t	codec;

	int32_t		key_interval;

	NX_LIST_HANDLE	input_list;
	NX_LIST_HANDLE	golden_list;

	uint64_t	start_time;
	uint64_t	expected_time;
	uint64_t	compare_time;
} NX_APP_INFO;

//------------------------------------------------------------------------------
static void signal_handler( int32_t sig )
{
	printf("Aborted by signal %s (%d)..\n", (char*)strsignal(sig), sig);

	switch( sig )
	{
		case SIGINT :
			printf("SIGINT..\n"); 	break;
		case SIGTERM :
			printf("SIGTERM..\n");	break;
		case SIGABRT :
			printf("SIGABRT..\n");	break;
		default :
			break;
	}

	exit(EXIT_FAILURE);
}

//------------------------------------------------------------------------------
static void register_signal( void )
{
	signal( SIGINT,  signal_handler );
	signal( SIGTERM, signal_handler );
	signal( SIGABRT, signal_handler );
}

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
static void make_directory( const char *fmt, ... )
{
	va_list args;
	char dir[1024];
	char *ptr = dir;
	memset( dir, 0x00, sizeof(dir) );

	va_start(args, fmt);
	vsnprintf(dir, sizeof(dir), fmt, args);
	va_end(args);

	while( *ptr )
	{
		if( '/' == *ptr )
		{
			*ptr = 0x00;
			if( 0 != access( dir, F_OK ) && (ptr != dir) )
			{
				printf("Make Directory. ( %s )\n", dir);
				mkdir( dir, 0777 );
			}
			*ptr = '/';
		}
		ptr++;
	}

	if( 0 != access( dir, F_OK) )
	{
		printf("Make Directory. ( %s )\n", dir);
		mkdir( dir, 0777 );
	}
}

//------------------------------------------------------------------------------
static int32_t sort_func( void* data1, void* data2, void* obj )
{
	return strcmp( (char*)data2, (char*)data1 );
}

//------------------------------------------------------------------------------
static int32_t make_file_list( NX_LIST_HANDLE handle, char *directory )
{
	DIR *dir = NULL;
	struct dirent *stdirent;
	char file[1024];

	if( NULL == (dir = opendir( directory )) )
	{
		return -1;
	}

	while( NULL != (stdirent = readdir(dir)) )
	{
		if( !strcmp( stdirent->d_name, "." ) ||
			!strcmp( stdirent->d_name, ".." ) )
			continue;

		snprintf(file, sizeof(file), "%s/%s", directory, stdirent->d_name);
		if( is_directory(file) )
		{
			// make_file_list(file);
		}
		else if( is_regular(file) )
		{
			nx_list_add( handle, (void*)strdup(file), 0 );
		}
	}
	closedir( dir );

	nx_list_sort( handle, sort_func, NULL );
	return 0;
}

//------------------------------------------------------------------------------
static int32_t free_file_list( NX_LIST_HANDLE handle )
{
	char *data;
	while( 0 < nx_list_get_count(handle) )
	{
		nx_list_search( handle, (void*)&data, 0 );
		nx_list_remove( handle, 0 );

		if( data )
		{
			free( data );
		}
	}
}

//------------------------------------------------------------------------------
static int32_t get_codec_type( int32_t codec )
{
	uint32_t fmt;
	switch( codec )
	{
		case 0:		fmt = V4L2_PIX_FMT_H264;	break;
		case 1: 	fmt = V4L2_PIX_FMT_H263;	break;
		case 2: 	fmt = V4L2_PIX_FMT_MPEG4;	break;
		case 3: 	fmt = V4L2_PIX_FMT_MPEG2;	break;
		default:	fmt = V4L2_PIX_FMT_H264;	break;
	}

	return fmt;
}

//------------------------------------------------------------------------------
static uint64_t get_time( void )
{
	struct timeval	tv;
	struct timezone	tz;
	gettimeofday( &tv, &tz );
	return ((uint64_t)tv.tv_sec)*1000 + tv.tv_usec/1000;
}

//------------------------------------------------------------------------------
static int32_t vpu_dec_test( NX_APP_INFO* info )
{
	uint8_t strm_buf[4*1024*1024];
	NX_V4L2DEC_HANDLE dec = NULL;

	int32_t ret;
	int32_t init = false;
	int32_t size, additional;
	int32_t frm_cnt = 0, strm_cnt = 0, img_cnt = 0;
	int32_t prv_index = -1;
	uint64_t elapsed_time = 0;

	char* file, file_output[1024];
	NX_VID_MEMORY_INFO* mem_golden = NULL;

	NX_V4L2DEC_SEQ_IN seq_in;;
	NX_V4L2DEC_SEQ_OUT seq_out;

	NX_V4L2DEC_IN dec_in;
	NX_V4L2DEC_OUT dec_out;

	NX_STREAM_INFO* stream;

	dec = NX_V4l2DecOpen( info->codec );
	if( NULL == dec )
	{
		ret = NX_RET_FAIL_DECODE;
		goto TERMINATE;
	}

	mem_golden = NX_AllocateVideoMemory( info->width, info->height, nx_get_plane_num(NX_IMAGE_FORMAT), NX_IMAGE_FORMAT, 4096 );

	do {
		if( !init )
		{
			ret = nx_list_search( info->input_list, (void*)&file, strm_cnt++ );
			if( 0 > ret )
			{
				ret = NX_RET_FAIL_STREAM_DATA;
				goto TERMINATE;
			}

			ret = nx_stream_load(&stream, file);
			if( 0 > ret )
			{
				ret = NX_RET_FAIL_STREAM_LOAD;
				goto TERMINATE;
			}

			memcpy( strm_buf, stream->buf, stream->size );
			additional = stream->size;

			nx_stream_free(stream);
		}

		{
			ret = nx_list_search( info->input_list, (void*)&file, strm_cnt );
			if( 0 > ret )
			{
				ret = NX_RET_FAIL_STREAM_DATA;
				goto TERMINATE;
			}

			ret = nx_stream_load(&stream, file);
			if( 0 > ret )
			{
				ret = NX_RET_FAIL_STREAM_LOAD;
				goto TERMINATE;
			}

			memcpy( strm_buf+additional, stream->buf, stream->size );
			size = stream->size;

			nx_stream_free(stream);

			strm_cnt = (strm_cnt % info->key_interval) + 1;
		}

		if( !init )
		{
			memset( &seq_in, 0x00, sizeof(seq_in) );
			seq_in.width    = info->width;
			seq_in.height   = info->height;
			seq_in.strmBuf  = strm_buf + additional;
			seq_in.strmSize = size;
			seq_in.seqBuf   = strm_buf;
			seq_in.seqSize  = additional;
			seq_in.timeStamp = 0;

			ret = NX_V4l2DecParseVideoCfg( dec, &seq_in, &seq_out );
			if( 0 > ret )
			{
				printf("fail, NX_V4l2DecParseVideoCfg().\n");
				ret = NX_RET_FAIL_DECODE;
				goto TERMINATE;
			}

			seq_in.width       = seq_out.width;
			seq_in.height      = seq_out.height;
			seq_in.imgPlaneNum = nx_get_plane_num(NX_IMAGE_FORMAT);
			seq_in.imgFormat   = NX_IMAGE_FORMAT;
			seq_in.numBuffers  = seq_out.minBuffers + 3;

			ret = NX_V4l2DecInit(dec, &seq_in);
			if( 0 > ret )
			{
				printf("fail, NX_V4l2DecInit().\n");
				ret = NX_RET_FAIL_DECODE;
				goto TERMINATE;
			}

			init = true;
			additional = 0;
			continue;
		}

		do {
			memset(&dec_in, 0, sizeof(NX_V4L2DEC_IN));
			dec_in.strmBuf   = (size > 0) ? strm_buf : NULL;
			dec_in.strmSize  = (size > 0) ? size + additional : 0;
			dec_in.timeStamp = 0;
			dec_in.eos       = (size > 0) ? 0 : 1;

			ret = NX_V4l2DecDecodeFrame(dec, &dec_in, &dec_out);
			if( 0 < ret )
			{
				continue;
			}

			if( 0 <= dec_out.dispIdx )
			{
				if( ((info->expected_time - info->compare_time) <= (get_time() - info->start_time)) &&
					(img_cnt != 0 ) )
				{
					ret = nx_list_search( info->golden_list, (void*)&file, img_cnt );
					if( 0 > ret )
					{
						printf("fail, nx_list_search().\n");
						ret = NX_RET_FAIL_GOLDEN_DATA;
						goto TERMINATE;
					}

					elapsed_time = get_time();
					printf("Memory Load. ( golden data: %s )\n", file);
					ret = nx_memory_load( mem_golden, file );
					if( 0 > ret )
					{
						printf("fail, nx_memory_load().\n");
						ret = NX_RET_FAIL_GOLDEN_LOAD;
						goto TERMINATE;
					}
					printf("Memory Load Done. ( elapsed time: %lld mSec )\n", get_time() - elapsed_time);

					elapsed_time = get_time();
					printf("Data Comapre. ( golden data: %s )\n", file);
					ret = nx_memory_compare( &dec_out.hImg, mem_golden );
					if( 0 > ret )
					{
						printf("fail, nx_memory_compare().\n");
						ret = NX_RET_FAIL_DATA_COMPARE;
						goto TERMINATE;
					}

					printf("Data Compare Done. ( elapsed time: %lld mSec )\n", get_time() - elapsed_time);
					ret = NX_RET_SUCCESS;
					goto TERMINATE;
				}

				if( 0 <= prv_index )
				{
					ret = NX_V4l2DecClrDspFlag(dec, NULL, prv_index);
					if( 0 > ret )
					{
						printf("fail, NX_V4l2DecClrDspFlag().\n");
						ret = NX_RET_FAIL_DECODE;
						goto TERMINATE;
					}
				}
				prv_index = dec_out.dispIdx;

				img_cnt = ((img_cnt+1) % info->key_interval);
				frm_cnt++;
			}
		} while( size == 0 && dec_out.remainByte != 0 );
	} while(1);

TERMINATE:
	if( mem_golden )
	{
		NX_FreeVideoMemory( mem_golden );
	}

	if( dec )
	{
		if( 0 <= prv_index )
		{
			NX_V4l2DecClrDspFlag(dec, NULL, prv_index);
			prv_index = -1;
		}

		NX_V4l2DecClose(dec);
	}

	return ret;
}

//------------------------------------------------------------------------------
static void print_usage(const char *name)
{
	/*
	# nx_vpu_dump -i /mnt/Default/h264_1080p_60fps_bframe.mkv -o /mnt/test -j 10 -l 250
	*/
	printf(
		"Usage : %s -i [input path] -g [golden path] -s [width],[height] -k [key interval]\n"
		"   -s [width],[height]   [M] : input image size\n"
		"   -k [key interval]     [M] : key frame interval\n"
		"   -i [input path]       [M] : input file path\n"
		"   -g [golden path]      [M] : golden file path\n"
		"   -c [codec]            [O] : 0:H264, 1:H263, 2:MPEG4, 3:MPEG2 (def: H264)\n"
		"   -t [expected time]    [O] : expected working time (def: 10000 mSec)\n"
		"   -p [compare time]     [o] : prediction compare time (def: 2000 mSec)\n"
		"   -h : help\n"
		,name
	);
}

//------------------------------------------------------------------------------
int32_t main( int32_t argc, char *argv[] )
{
	int32_t ret;
	int32_t opt;

	char *input_path = NULL;
	char *golden_path = NULL;

	NX_APP_INFO info;
	memset( &info, 0x00, sizeof(info) );

	register_signal();

	info.start_time = get_time();

	while (-1 != (opt = getopt(argc, argv, "s:k:i:g:t:p:h")))
	{
		switch(opt)
		{
		case 's':	sscanf(optarg, "%d,%d", &info.width, &info.height);	break;
		case 'k':   info.key_interval = atoi(optarg);	break;
		case 'c':	info.codec = atoi(optarg);			break;
		case 'i':	input_path = strdup(optarg);		break;
		case 'g':	golden_path = strdup(optarg);		break;
		case 't':	info.expected_time = (uint64_t)atoi(optarg);	break;
		case 'p':	info.compare_time = (uint64_t)atoi(optarg);		break;
		case 'h':	print_usage(argv[0]);				return 0;
		default:	break;
		}
	}

	if( input_path == NULL || golden_path == NULL ||
		info.key_interval == 0 ||
		info.width == 0 || info.height == 0 )
	{
		print_usage(argv[0]);
		goto TERMINATE;
	}

	if( info.expected_time == 0 )
		info.expected_time = NX_TIME_EXPECTED;

	if( info.compare_time == 0 )
		info.compare_time = NX_TIME_COMPARE;

	info.codec = get_codec_type(info.codec);

	info.input_list  = nx_list_create();
	info.golden_list = nx_list_create();

	make_file_list( info.input_list, input_path );
	make_file_list( info.golden_list, golden_path );

	ret = vpu_dec_test( &info );
	printf("Done. ( ret = %d )\n", ret);

	free_file_list( info.input_list );
	free_file_list( info.golden_list );

TERMINATE:
	if( info.input_list )	nx_list_destroy( info.input_list );
	if( info.golden_list )	nx_list_destroy( info.golden_list );

	if( input_path )	free( input_path );
	if( golden_path )	free( golden_path );

	return ret;
}
