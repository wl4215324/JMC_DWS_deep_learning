/*
 * algorithm.hpp
 *
 *  Created on: Mar 19, 2018
 *      Author: tony
 */

#ifndef ALGORITHM_HPP_
#define ALGORITHM_HPP_

#include "AlgoLib.h"

extern "C" {
#include <pthread.h>
#include <string.h>
#include "v4l2_tvin.h"
#include "serial_pack_parse.h"
#include "applicfg.h"
#include "user_timer.h"
}


extern void* algorithm_process(void *);

#endif /* ALGORITHM_HPP_ */
