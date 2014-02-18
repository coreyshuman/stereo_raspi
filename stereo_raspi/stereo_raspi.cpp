/*  Filename: stereo_raspi.cpp
    This file defines the entry point for the application.
	Accepts two raw h264 video feeds from Raspberry Pis and
	applies a simple barrel distortion to be used with Oculus Rift.

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
#include "cap_ffmpeg_stream.h"
#include <stdio.h>
#include <process.h>
#include "config.h"
#include "barrel_dist.hpp"

struct t {
	cap_ffmpeg_stream *cap;
	Mat *image;
};

unsigned int __stdcall frameProcThread(void* arguments);


int main(int argc, char* argv[])
{
	int ret;
	HANDLE thHandleA, thHandleB;
	cap_ffmpeg_stream capture1, capture2;
	Mat image1, image2;
	IplImage *_img1;
	t arg1, arg2;
	int hozOff = 40;

	if(argc < 3)
	{
		CV_WARN("Usage: stereo_raspi <port1> <port2>");
		return -1;
	}

	ret = capture1.open(atoi(argv[1]));
	if(!ret)
	{
		CV_WARN("OPEN FAILED");
		capture1.close();
		return -1;
	}

	ret = capture1.waitForConnect();
	if(!ret)
	{
		CV_WARN("CONNECT FAILED");
		capture1.close();
		return -1;
	}

	ret = capture1.startStreamCap();
	if(!ret)
	{
		CV_WARN("STREAM FAILED");
		capture1.close();
		return -1;
	}

	ret = capture2.open(atoi(argv[2]));
	if(!ret)
	{
		CV_WARN("OPEN FAILED");
		capture2.close();
		return -1;
	}

	ret = capture2.waitForConnect();
	if(!ret)
	{
		CV_WARN("CONNECT FAILED");
		capture2.close();
		return -1;
	}

	ret = capture2.startStreamCap();
	if(!ret)
	{
		CV_WARN("STREAM FAILED");
		capture2.close();
		return -1;
	}

	printf("\r\nENTER READ LOOP...");
	// If we get here, stream is ready to be processed.

	// do a dummy read to set up distortion
	capture1.grabFrame();
	_img1 = capture1.retrieve();
	prepare_barrel_distortion(_img1, _img1->width/2,_img1->height/2,.0000008,.0000016);

	// setup arguments for thread passthrough
	arg1.cap = &capture1;
	arg1.image = &image1;
//	arg1->img = _img1;
	arg2.cap = &capture2;
	arg2.image = &image2;
//	arg2->img = _img2;

	// Enter processing loop...
	while(1)
	{
		thHandleA = (HANDLE)_beginthreadex(0, 0, &frameProcThread, (void*)&arg1, 0, 0);
		thHandleB = (HANDLE)_beginthreadex(0, 0, &frameProcThread, (void*)&arg2, 0, 0);

/* Working Distortion Code
		capture1.grabFrame();
		_img = capture1.retrieve();

		if( _img )
		{
			//IplImage* barrel_distortion(IplImage* img, double Cx,double Cy,double kx,double ky);
			prepare_barrel_distortion(_img, _img->width/2,_img->height/2,.0000008,.0000016);
			_img = barrel_distortion(_img);
			Mat(_img).copyTo(image1);
//			imshow( "Barrel Distortion 1", image1 );
		}
		else
		{
			CV_WARN("--! No Captured frame -> Break !--");
			break;
		}
		capture2.grabFrame();
		_img = 0;
		_img = capture2.retrieve();

		if( _img )
		{
			//IplImage* barrel_distortion(IplImage* img, double Cx,double Cy,double kx,double ky);
			prepare_barrel_distortion(_img, _img->width/2,_img->height/2,.0000008,.0000016);
			_img = barrel_distortion(_img);
			Mat(_img).copyTo(image2);
//			imshow( "Barrel Distortion 2", image2 );
		}
		else
		{
			CV_WARN("--! No Captured frame -> Break !--");
			break;
		}
		// end barrel test code
*/
/* original image read code
		ret = capture1.read(image1);
//		image1.adjustROI(ROI_LEFT_TOP, ROI_LEFT_BOTTOM, ROI_LEFT_LEFT, ROI_LEFT_RIGHT);

		if(ret)
			imshow( "Capture1", image1 );
		else
		{
			CV_WARN("--! No Captured frame -> Break !--");
			break;
		}

		ret = capture2.read(image2);
//		image2.adjustROI(ROI_LEFT_TOP, ROI_LEFT_BOTTOM, ROI_LEFT_LEFT, ROI_LEFT_RIGHT);

		if(ret)
			imshow( "Capture2", image2 );
		else
		{
			CV_WARN("--! No Captured frame -> Break !--");
			break;
		}
*/

		WaitForSingleObject(thHandleA, INFINITE);
		WaitForSingleObject(thHandleB, INFINITE);

		Size sz1 = image1.size();
		Size sz2 = image2.size();
		Mat im3(sz1.height, sz1.width+sz2.width, CV_8UC3);
		Mat left(im3, Rect(hozOff, 0, sz1.width-hozOff, sz1.height));
		//Mat leftOff(image1, Rect(0, 0, sz1.width, sz1.height));
		Mat(image1, Rect(0, 0, sz1.width-hozOff, sz1.height)).copyTo(left);
		Mat right(im3, Rect(sz1.width, 0, sz2.width-hozOff, sz2.height));
		Mat(image2, Rect(hozOff, 0, sz1.width-hozOff, sz1.height) ).copyTo(right);

	
		imshow( "Stereo Raspi", im3 );

		

		int c = waitKey(10);
        if( (char)c == 'c' ) { break; }
		else if( (char)c == 'f' )
		{
			while(1)
			{
				char tmp[8192*8];
				int r1, r2;

				r1 = capture1.readSocketRaw(tmp, 8192*8);
				r2 = capture2.readSocketRaw(tmp, 8192*8);

				if(r1 < 8192 && r2 < 8192)
				{
					CV_WARN("FLUSH");
					break;
				}
			}
		}
		else if( (char)c == 'd' )
		{
			printf("\r\nToggle Displacement");
			if(hozOff >= 50)
				hozOff = 0;
			else
				hozOff += 10;
		}
		else if( (char)c == 's' )
		{
			time_t t = time(0);
			char buff[256];
			sprintf(buff, "D:/Devel/Visual Studio/C++/stereo_raspi/Debug/images/img_%u.png", t);
			imwrite( buff, im3 );
			printf("\r\nSave Image: %s", buff);
		}

	}

	delete_barrel_distortion();
	capture1.close();
	capture2.close();
	CloseHandle(thHandleA);
	CloseHandle(thHandleB);
	return 0;
}

unsigned int __stdcall frameProcThread(void* arguments)
{
	IplImage *_img;	
	t *args = (t*)arguments;

	args->cap->grabFrame();
	_img = args->cap->retrieve();

	if( _img )
	{
		_img = barrel_distortion(_img);
		Mat(_img).copyTo(*args->image);
	}
	return 0;
}