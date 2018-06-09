/*
 * image_format_convert.h
 *
 *  Created on: Nov 20, 2017
 *      Author: tony
 */

#ifndef IMAGE_FORMAT_CONVERT_H_
#define IMAGE_FORMAT_CONVERT_H_

#define RGB565_MASK_RED        0xF800
#define RGB565_MASK_GREEN      0x07E0
#define RGB565_MASK_BLUE       0x001F


static inline void RGB565_2_gray(unsigned char *RGB565_buf, int img_width, \
		int img_height, unsigned char *gray_buf)
{
	int i=0, j=0;
	unsigned short R_comp, G_comp, B_comp;
	unsigned short *RGB565_bufp = (unsigned short*)RGB565_buf;
	int index = 0;

	for(i=0; i < img_height; i++)
	{
		for(j=0; j<img_width; j++)
		{
			index = i*img_width + j;
			R_comp = ((*(RGB565_bufp + index) & RGB565_MASK_RED) >> 11);
			G_comp = ((*(RGB565_bufp + index) & RGB565_MASK_GREEN) >> 5);
			B_comp = ((*(RGB565_bufp + index) & RGB565_MASK_BLUE));
			*(gray_buf + index) = (unsigned char) (R_comp*0.3 + G_comp*0.59 + B_comp*0.11);
		}
	}
}

#endif /* IMAGE_FORMAT_CONVERT_H_ */
