/*  Filename: barrel_dist.hpp
    This applies a simple barrel distortion

	Original code found here: http://stackoverflow.com/questions/6199636/formulas-for-barrel-pincushion-distortion

    Copyright (C) 2014  Corey Shuman <ctshumancode@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "barrel_dist.hpp"

using namespace cv;
IplImage* mapx;
IplImage* mapy;
int barrel_ready = 0;


void prepare_barrel_distortion(IplImage* img, double Cx,double Cy,double kx,double ky)
{
	if(barrel_ready)
		return;

    mapx = cvCreateImage( cvGetSize(img), IPL_DEPTH_32F, 1 );
    mapy = cvCreateImage( cvGetSize(img), IPL_DEPTH_32F, 1 );

    int w= img->width;
    int h= img->height;

    float* pbuf = (float*)mapx->imageData;
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {         
            float u= Cx+(x-Cx)*(1+kx*((x-Cx)*(x-Cx)+(y-Cy)*(y-Cy)));
            *pbuf = u;
            ++pbuf;
        }
    }

    pbuf = (float*)mapy->imageData;
    for (int y = 0;y < h; y++)
    {
        for (int x = 0; x < w; x++) 
        {
            *pbuf = Cy+(y-Cy)*(1+ky*((x-Cx)*(x-Cx)+(y-Cy)*(y-Cy)));
            ++pbuf;
        }
    }

	barrel_ready = 1;
}

IplImage* barrel_distortion(IplImage* img)
{
	IplImage* temp = cvCloneImage(img);
    cvRemap( temp, img, mapx, mapy ); 
    cvReleaseImage(&temp);

    return img;
}

void delete_barrel_distortion(void)
{
	if(!barrel_ready)
		return;

	if(mapx)
		cvReleaseImage(&mapx);
	if(mapy)
		cvReleaseImage(&mapy);

	barrel_ready = 0;
}