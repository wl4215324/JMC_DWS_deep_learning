/*
 * video_encode_attribute.h
 *
 *  Created on: Mar 3, 2020
 *      Author: tony
 */

#ifndef VIDEO_ENCODE_ATTRIBUTE_H_
#define VIDEO_ENCODE_ATTRIBUTE_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "video_encode.h"


int set_params_into_encoder(VideoEncoder *VideoEnc, encode_param_t *encode_param);

#endif /* VIDEO_ENCODE_ATTRIBUTE_H_ */
