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
#include <stdlib.h>
#include <string.h>

#include "nx_list.h"

//------------------------------------------------------------------------------
typedef struct _NX_NODE	NX_NODE;

struct _NX_NODE {
	void*				data;
	struct _NX_NODE*	next;
};

typedef struct NX_LIST_INFO {
	NX_NODE*	head;
	int32_t		count;
} NX_LIST_INFO;

//------------------------------------------------------------------------------
NX_LIST_HANDLE nx_list_create( void )
{
	NX_LIST_HANDLE handle = NULL;
	handle = (NX_LIST_HANDLE)malloc(sizeof(NX_LIST_INFO));
	memset( handle, 0x00, sizeof(NX_LIST_INFO) );
	handle->head = (NX_NODE*)malloc( sizeof(NX_NODE) );
	handle->head->data = NULL;
	handle->head->next = NULL;

	return handle;
}

//------------------------------------------------------------------------------
void nx_list_destroy( NX_LIST_HANDLE handle )
{
	if( handle )
	{
		nx_list_remove_all( handle );

		if( handle->head ) free( handle->head );
		free( handle );
	}
}

//------------------------------------------------------------------------------
int32_t nx_list_add( NX_LIST_HANDLE handle, void* data, int32_t index )
{
	NX_NODE *cur_node, *new_node;
	int32_t i = 0;

	if( !handle || !handle->head )
		return -1;

	if( 0 > index || handle->count < index )
		return -1;

	cur_node = handle->head;
	while( (cur_node != NULL) && (++i <= index) )
		cur_node = cur_node->next;

	new_node = (NX_NODE*)malloc( sizeof(NX_NODE) );
	if( NULL == new_node )
		return -1;

	new_node->data = data;
	new_node->next = (cur_node->next != NULL) ? cur_node->next : NULL;
	cur_node->next = new_node;

	handle->count++;
	return 0;
}

//------------------------------------------------------------------------------
int32_t nx_list_remove( NX_LIST_HANDLE handle, int32_t index )
{
	NX_NODE *cur_node, *tmp_node;
	int32_t i = 0;

	if( !handle || !handle->head )
		return -1;

	if( 0 > index || handle->count <= index )
		return -1;

	cur_node = handle->head;
	while( (cur_node != NULL) && (++i <= index) )
		cur_node = cur_node->next;

	tmp_node = (cur_node->next->next != NULL) ? cur_node->next->next : NULL;
	free( cur_node->next );
	cur_node->next = tmp_node;

	handle->count--;
	return 0;
}

//------------------------------------------------------------------------------
int32_t nx_list_remove_all( NX_LIST_HANDLE handle )
{
	NX_NODE *cur_node, *tmp_node;

	if( !handle || !handle->head )
		return -1;

	while( handle->count > 0 )
	{
		int32_t i = 0;
		int32_t index = handle->count - 1;

		cur_node = handle->head;
		while( (cur_node != NULL) && (++i <= index) )
			cur_node = cur_node->next;

		tmp_node = (cur_node->next->next != NULL ) ? cur_node->next->next : NULL;
		free( cur_node->next );
		cur_node->next = tmp_node;

		handle->count--;
	}

	return 0;
}

//------------------------------------------------------------------------------
int32_t nx_list_get_count( NX_LIST_HANDLE handle )
{
	if( !handle || !handle->head )
		return -1;

	return handle->count;
}

//------------------------------------------------------------------------------
int32_t nx_list_search( NX_LIST_HANDLE handle, void **data, int32_t index )
{
	NX_NODE *cur_node;
	int32_t i = 0;

	if( !handle || !handle->head )
		return -1;

	if( 0 > index || handle->count <= index )
		return -1;

	cur_node = handle->head;
	while( (cur_node != NULL) && (++i <= index) )
		cur_node = cur_node->next;

	*data = cur_node->next->data;
	return 0;
}

//------------------------------------------------------------------------------
int32_t nx_list_swap( NX_LIST_HANDLE handle, int32_t index1, int32_t index2 )
{
	void *tmp_data;
	NX_NODE *cur_node1, *cur_node2;
	int32_t i = 0, j = 0;
	int32_t idx1 = (index1 < index2) ? index1 : index2;
	int32_t idx2 = (index1 < index2) ? index2 : index1;

	if( !handle || !handle->head )
		return -1;

	if( (index1 == index2) ||
		(idx1 < 0 || idx1 >= handle->count) ||
		(idx2 < 0 || idx2 >= handle->count) )
		return -1;

	cur_node1 = handle->head;
	while( (cur_node1 != NULL) && (++i <= idx1) )
		cur_node1 = cur_node1->next;

	cur_node2 = handle->head;
	while( (cur_node2 != NULL) && (++j <= idx2) )
		cur_node2 = cur_node2->next;

	cur_node1 = cur_node1->next;
	cur_node2 = cur_node2->next;

	tmp_data = cur_node2->data;
	cur_node2->data = cur_node1->data;
	cur_node1->data = tmp_data;

	return 0;
}

//------------------------------------------------------------------------------
int32_t nx_list_sort( NX_LIST_HANDLE handle, int32_t (*cbFunc)(void*, void*, void*), void* obj )
{
	int32_t i, j;
	void *data1, *data2;

	if( !handle || !handle->head || !cbFunc )
		return -1;

	for( i = 0; i < handle->count-1; i++ )
	{
		for( j = 0; j < handle->count-1; j++ )
		{
			nx_list_search( handle, &data1, j   );
			nx_list_search( handle, &data2, j+1 );

			if( 0 > cbFunc( data1, data2, obj ) )
				nx_list_swap( handle, j, j+1 );
		}
	}

	return 0;
}

//------------------------------------------------------------------------------
void nx_list_dump( NX_LIST_HANDLE handle, int32_t index )
{
	NX_NODE* cur_node;
	int32_t i;

	if( !handle || !handle->head )
		return;

	if( 0 == index || handle->count <= index )
		return;

	cur_node = handle->head;
	for( i = 0; i < index+1; i++ )
		cur_node = cur_node->next;

	printf( "[ %d ] node( %p ) / next( %p ) / data( %p ) \n",
		index, cur_node, cur_node->next, cur_node->data );
}

//------------------------------------------------------------------------------
void nx_list_dump_all( NX_LIST_HANDLE handle )
{
	NX_NODE* cur_node;
	int32_t i;

	if( !handle || !handle->head )
		return;

	cur_node = handle->head;
	printf( "------------------------------------------------------------------------------\n" );
	printf( "[ head ] node( %p ) / next( %p )\n", cur_node, cur_node->next );
	printf( "------------------------------------------------------------------------------\n" );

	for( i = 0; i < handle->count; i++ )
	{
		nx_list_dump( handle, i );
	}
}

//------------------------------------------------------------------------------
void nx_list_dump_data( NX_LIST_HANDLE handle, void (*cbFunc)(int32_t, void*, void*), void* obj )
{
	NX_NODE* cur_node;
	int32_t i;

	if( !handle || !handle->head )
		return;

	cur_node = handle->head;
	for( i = 0; i < handle->count; i++ )
	{
		cur_node = cur_node->next;
		cbFunc( i, cur_node->data, obj );
	}
}
