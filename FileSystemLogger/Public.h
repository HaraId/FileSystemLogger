#pragma once

/*
	
*/
#define IOCTL_SET_TARGET_APP CTL_CODE  (FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)


/*
	
*/
#define IOCTL_CLEAR			 CTL_CODE  (FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)


/*

*/
#define IOCTL_START_LOG		 CTL_CODE  (FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)


/*

*/
#define IOCTL_STOP_LOG		 CTL_CODE  (FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)


#define MAX_TARGET_FILE_NAME_SIZE 128
#define MIN_RECIVE_BUFFER_SIZE 1024

//
// Структура содержащая основную информацию о событии операции с файловой системой
//
struct FsOperationEvent
{
	// Размер всей структуры
	ULONG size;

	ULONG targetFileNameOffset;
	ULONG targetFileNameLength;

};