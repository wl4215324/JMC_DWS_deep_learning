/*
 * timers_driver.h
 *
 *  Created on: Nov 10, 2017
 *      Author: tony
 */

#ifndef TIMERS_DRIVER_H_
#define TIMERS_DRIVER_H_

#include "timerscfg.h"
#include "user_timer.h"


/**
 * @ingroup timer
 * @brief Acquire mutex
 */
void EnterMutex(void);

/**
 * @ingroup timer
 * @brief Release mutex
 */
void LeaveMutex(void);


/**
 * @ingroup timer
 * @brief Initialize Timer
 */
extern void TimerInit(void);

/**
 * @ingroup timer
 * @brief Cleanup Timer
 */
void TimerCleanup(void);

/**
 * @ingroup timer
 * @brief Start the timer task
 * @param Callback A callback function
 */
void StartTimerLoop(TimerCallback_t Callback);

/**
 * @ingroup timer
 * @brief Stop the timer task
 * @param Callback A callback function
 */
void StopTimerLoop(TimerCallback_t Callback);


#endif /* TIMERS_DRIVER_H_ */
