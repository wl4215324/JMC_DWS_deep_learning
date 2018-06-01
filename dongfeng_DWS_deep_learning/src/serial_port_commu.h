/*
 * serial_port_commu.h
 *
 *  Created on: Nov 10, 2017
 *      Author: tony
 */

#ifndef SERIAL_PORT_COMMU_H_
#define SERIAL_PORT_COMMU_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <fcntl.h>

#define  RS232_DEV_NAME "/dev/ttymxc1"

int open_set_serial_port( );

#endif /* SERIAL_PORT_COMMU_H_ */
