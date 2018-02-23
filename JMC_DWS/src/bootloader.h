/*
 * bootloader.h
 *
 *  Created on: Jan 23, 2018
 *      Author: tony
 */

#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define MINORBITS   16
#define MINORMASK   ((1U << MINORBITS) - 1)
#define MAJOR(dev)  ((unsigned int) ((dev) >> MINORBITS))
#define MINOR(dev)  ((unsigned int) ((dev) & MINORMASK))
#define MKDEV(ma,mi) (((ma) << MINORBITS) | (mi))


#define  DIAG_SESS_CTRL    0x10
#define  ECU_RESET         0x11
#define  CLEAR_DIAG_INFO   0x14
#define  READ_DTC_INFO     0x19
#define  READ_DAT_BY_ID    0x22
#define  READ_MEM_BY_ADDR  0x23
#define  SECURE_ACCESS     0x27
#define  COMMUN_CTRL       0x28
#define  READ_DAT_BY_PID   0x2A
#define  DYNA_DEFI_DAT_ID  0x2C
#define  WRITE_DAT_BY_ID   0x2E
#define  INPUT_OUTPUT_CTRL_BY_ID  0x2F
#define  ROUTINE_CTRL      0x31
#define  REQUEST_DOWNLOAD  0x34
#define  TRANSFER_DATA     0x36
#define  REQ_TRANSFER_DAT_EXIT  0x37
#define  WRITE_MEM_BY_ID   0x3D

struct BootloaderData{
	unsigned int state;
	unsigned char *precv_frame;

};

typedef enum {
	pre_programming = 0,
	being_programming,
	after_programming
} BootloaderState;

typedef enum {
	DiagnosticSessionControl = 0,
	RoutineControl,
	ControlDTCSetting,
	CommunicationControl,
	ReadDataByIdentifier
} PreProgramSubstate;


typedef enum {
	DiagnosticSessionControl = 0,
	SafeAcess,
	DriverLoading,
	FingerPrint,
    EraseMemory,
    Downloading,
    IntegrityCheck,
    DependencyCheck,
    ResetECU
} BeingProgramSubstate;

typedef enum {
	DiagnosticSessionControl = 0,

} AfterProgramSubstate;



#endif /* BOOTLOADER_H_ */
