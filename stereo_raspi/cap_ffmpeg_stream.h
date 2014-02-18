/*  Filename: cap_ffmpeg_stream.h
    Provides an interface for accepting a raw video stream over TCP,
	such as a Rasperry Pi stream using raspicam.

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

#ifndef _CAP_FFMPEG_STREAM_H
#define _CAP_FFMPEG_STREAM_H

#include <winsock2.h>
#include <assert.h>
#include <algorithm>
#include <limits>
#include <windows.h>

#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;

extern "C" {
	#include <libavformat/avio.h>
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
}

#define CV_WARN(message) fprintf(stderr, "warning: %s (%s:%d)\n", message, __FILE__, __LINE__)
#define CALC_FFMPEG_VERSION(a,b,c) ( a<<16 | b<<8 | c )
struct Image_FFMPEG
{
    unsigned char* data;
    int step;
    int width;
    int height;
    int cn;

};

class cap_ffmpeg_stream
{
public:
	cap_ffmpeg_stream(void);
	~cap_ffmpeg_stream(void);
	void init(void);
	bool open(int p);
	void close(void);
	bool openSocket(int p);
	bool waitForConnect(void);
	bool startStreamCap(void);
	bool grabFrame(void);
	bool retrieveFrame(int, unsigned char** data, int* step, int* width, int* height, int* cn);
	IplImage *retrieve(void);
	bool retrieveMat(Mat& image);
	bool read(Mat& image);
	int readSocketRaw(char* buff, int bufLen);

	double r2d(AVRational r);
	double get_fps();
	double dts_to_sec(int64_t dts);
	int64_t dts_to_frame_number(int64_t dts);

	bool			ffmpeg_open;
	AVFormatContext * ic;
	AVCodec         * avcodec;
	int               video_stream;
	AVStream        * video_st;
	AVFrame         * picture;
	AVFrame           rgb_picture;
	int64_t           picture_pts;

	AVPacket          packet;
	Image_FFMPEG      frame;
	struct SwsContext *img_convert_ctx;

	int64_t frame_number, first_frame_number;
	double eps_zero;

	// opencv vars
	IplImage cvFrame;	

	// port
	int avioBufferSize;
	unsigned char *avioBuffer;
	AVIOContext *avioContext;

	// tcp socket variables
	WSADATA wsaData;
	int port;
	struct sockaddr_in serverAddr, cliAddr; /*server address*/
	int serverSock; /*client sock*/
	int sockLen;
	int cliLen;
	int cliSock;

};

#endif