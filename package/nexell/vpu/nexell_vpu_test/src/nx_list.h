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

#ifndef __NX_LIST_H__
#define __NX_LIST_H__

#include <stdint.h>

typedef struct NX_LIST_INFO* NX_LIST_HANDLE;

NX_LIST_HANDLE nx_list_create( void );
void nx_list_destroy( NX_LIST_HANDLE handle );

int32_t nx_list_add( NX_LIST_HANDLE handle, void* data, int32_t index );
int32_t nx_list_remove( NX_LIST_HANDLE handle, int32_t index );
int32_t nx_list_remove_all( NX_LIST_HANDLE handle );

int32_t nx_list_get_count( NX_LIST_HANDLE handle );
int32_t nx_list_search( NX_LIST_HANDLE handle, void **data, int32_t index );

int32_t nx_list_swap( NX_LIST_HANDLE handle, int32_t index1, int32_t index2 );
int32_t nx_list_sort( NX_LIST_HANDLE handle, int32_t (*cbFunc)(void*, void*, void*), void* obj );

void nx_list_dump( NX_LIST_HANDLE handle, int32_t index );
void nx_list_dump_all( NX_LIST_HANDLE handle );
void nx_list_dump_data( NX_LIST_HANDLE handle, void (*cbFunc)(int32_t, void*, void*), void* obj );

#endif	// __NX_LIST_H__
