/*  Filename: cap_ffmpeg_stream.cpp
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

#include "StdAfx.h"

#include <winsock2.h>
#include <assert.h>
#include <algorithm>
#include <limits>
#include <windows.h>

#pragma comment(lib,"ws2_32.lib")
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

#include "cap_ffmpeg_stream.h"

cap_ffmpeg_stream::cap_ffmpeg_stream(void)
{
	ffmpeg_open = false;
	av_log_set_level(AV_LOG_DEBUG);
	av_register_all();
	avcodec_register_all();
}


cap_ffmpeg_stream::~cap_ffmpeg_stream(void)
{
	if(ffmpeg_open)
		close();
}

static int get_number_of_cpus(void)
{

#if defined WIN32 || defined _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo( &sysinfo );

    return (int)sysinfo.dwNumberOfProcessors;
#elif defined __linux__
    return (int)sysconf( _SC_NPROCESSORS_ONLN );
#elif defined __APPLE__
    int numCPU=0;
    int mib[4];
    size_t len = sizeof(numCPU);

    // set the mib for hw.ncpu
    mib[0] = CTL_HW;
    mib[1] = HW_AVAILCPU;  // alternatively, try HW_NCPU;

    // get the number of CPUs from the system
    sysctl(mib, 2, &numCPU, &len, NULL, 0);

    if( numCPU < 1 )
    {
        mib[1] = HW_NCPU;
        sysctl( mib, 2, &numCPU, &len, NULL, 0 );

        if( numCPU < 1 )
            numCPU = 1;
    }

    return (int)numCPU;
#else
    return 1;
#endif
}

int readPackets(void *opaque, uint8_t *buf, int buf_size)
{
	int len;
	int *cSock = (int *)opaque;

	// read from socket
	//len = recv(cliSock, (char*)buf, buf_size, 0);
	// cts - use *opaque to pass through socket number
	len = recv(*cSock, (char*)buf, buf_size, 0);

	return len;
}

int64_t seekPackets(void *opaque, int64_t offset, int whence)
{
	return -1;
}

double cap_ffmpeg_stream::r2d(AVRational r)
{
    return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
}

double cap_ffmpeg_stream::get_fps()
{
    double fps = r2d(ic->streams[video_stream]->r_frame_rate);

#if LIBAVFORMAT_BUILD >= CALC_FFMPEG_VERSION(52, 111, 0)
    if (fps < eps_zero)
    {
        fps = r2d(ic->streams[video_stream]->avg_frame_rate);
    }
#endif

    if (fps < eps_zero)
    {
        fps = 1.0 / r2d(ic->streams[video_stream]->codec->time_base);
    }

    return fps;
}

double cap_ffmpeg_stream::dts_to_sec(int64_t dts)
{
    return (double)(dts - ic->streams[video_stream]->start_time) *
        r2d(ic->streams[video_stream]->time_base);
}

int64_t cap_ffmpeg_stream::dts_to_frame_number(int64_t dts)
{
    double sec = dts_to_sec(dts);
    return (int64_t)(get_fps() * sec + 0.5);
}

void cap_ffmpeg_stream::init(void)
{
	ic = 0;
    video_stream = -1;
    video_st = 0;
    picture = 0;
    picture_pts = AV_NOPTS_VALUE;
    first_frame_number = -1;
    memset( &rgb_picture, 0, sizeof(rgb_picture) );
    memset( &frame, 0, sizeof(frame) );
    memset(&packet, 0, sizeof(packet));
	memset(&cvFrame, 0, sizeof(IplImage));
    av_init_packet(&packet);
    img_convert_ctx = 0;
    avcodec = 0;
    frame_number = 0;
    eps_zero = 0.000025;
	avioBufferSize = 8192*8;
	avioBuffer = (unsigned char *)av_malloc(avioBufferSize + FF_INPUT_BUFFER_PADDING_SIZE);
	avioContext = avio_alloc_context(avioBuffer, avioBufferSize, 0, (void *)&cliSock, readPackets, NULL, seekPackets);
	sockLen=sizeof(struct sockaddr);
	cliLen = sizeof(cliAddr);
}

bool cap_ffmpeg_stream::open(int p)
{
	int ret;
	if(ffmpeg_open)
		close();

	init();

	ret = openSocket(p);
	if( !ret ) {
		CV_WARN("Open Socket Failed");
		return 0;
	}
	ffmpeg_open = true;
	return 1;
}

bool cap_ffmpeg_stream::waitForConnect(void)
{
	// socket opened, wait for connection
	printf("\r\nSocket open, waiting for connection on port %d...", port);
	if(!ffmpeg_open)
		return 0;

	cliSock = accept(serverSock, (LPSOCKADDR)&cliAddr, &cliLen);

	if(cliSock < 0)
	{
		CV_WARN( "socket accept Failed");
		return 0;
	}

	printf("\r\nConnection received from %d.%d.%d.%d \r\n", cliAddr.sin_addr.S_un.S_un_b.s_b1,
		cliAddr.sin_addr.S_un.S_un_b.s_b2, cliAddr.sin_addr.S_un.S_un_b.s_b3, cliAddr.sin_addr.S_un.S_un_b.s_b4);

	return 1;
}

bool cap_ffmpeg_stream::startStreamCap(void)
{
	int err, i;
	bool valid = false;

	if(!ffmpeg_open)
		return 0;
	/************************************************************************/
	ic = avformat_alloc_context();
	ic->pb = avioContext;
	// CTS - test format options
	ic->max_analyze_duration = 40000;

	err = avformat_open_input(&ic, "dummyfilename", NULL, NULL);
	//err = avformat_open_input_stream
	if (err < 0)
    {
        CV_WARN("Error opening file");
        goto exit_func;
    }

	err = avformat_find_stream_info(ic, NULL);

    if (err < 0)
    {
        CV_WARN("Could not find codec parameters");
        goto exit_func;
    }

    for(i = 0; i < ic->nb_streams; i++)
    {
#if LIBAVFORMAT_BUILD > 4628
        AVCodecContext *enc = ic->streams[i]->codec;
#else
        AVCodecContext *enc = &ic->streams[i]->codec;
#endif
        enc->thread_count = get_number_of_cpus();

        if( AVMEDIA_TYPE_VIDEO == enc->codec_type && video_stream < 0)
        {
            // backup encoder' width/height
            int enc_width = enc->width;
            int enc_height = enc->height;

            AVCodec *codec = avcodec_find_decoder(enc->codec_id);
            if (!codec ||
#if LIBAVCODEC_VERSION_INT >= ((53<<16)+(8<<8)+0)
                avcodec_open2(enc, codec, NULL)
#else
                avcodec_open(enc, codec)
#endif
                < 0)
                goto exit_func;

            // checking width/height (since decoder can sometimes alter it, eg. vp6f)
            if (enc_width && (enc->width != enc_width)) { enc->width = enc_width; }
            if (enc_height && (enc->height != enc_height)) { enc->height = enc_height; }

            video_stream = i;
            video_st = ic->streams[i];
            picture = avcodec_alloc_frame();

            rgb_picture.data[0] = (uint8_t*)malloc(
                    avpicture_get_size( PIX_FMT_BGR24,
                                        enc->width, enc->height ));
            avpicture_fill( (AVPicture*)&rgb_picture, rgb_picture.data[0],
                            PIX_FMT_BGR24, enc->width, enc->height );

            frame.width = enc->width;
            frame.height = enc->height;
            frame.cn = 3;
            frame.step = rgb_picture.linesize[0];
            frame.data = rgb_picture.data[0];
            break;
        }
    }

    if(video_stream >= 0) valid = true;

exit_func:
	return valid;
}

void cap_ffmpeg_stream::close(void)
{
    if( img_convert_ctx )
    {
        sws_freeContext(img_convert_ctx);
        img_convert_ctx = 0;
    }

    if( picture )
        av_free(picture);

    if( video_st && video_st->codec )
    {
#if LIBAVFORMAT_BUILD > 4628
        avcodec_close( video_st->codec );

#else
        avcodec_close( &(video_st->codec) );

#endif
        video_st = NULL;
    }

    if( ic )
    {
        avformat_close_input(&ic);

        ic = NULL;
    }

    if( rgb_picture.data[0] )
    {
        free( rgb_picture.data[0] );
        rgb_picture.data[0] = 0;
    }

    // free last packet if exist
    if (packet.data) {
        av_free_packet (&packet);
        packet.data = NULL;
    }

//	av_free(avioContext);
//	av_free(avioBuffer);
	closesocket(cliSock);
	closesocket(serverSock);
    WSACleanup();

	ffmpeg_open = false;
}

bool cap_ffmpeg_stream::openSocket(int p)
{
	printf("<opensock>");	//cts debug
	WSAStartup(0x0202,&wsaData); /*windows socket startup */

	serverAddr.sin_family=AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	port = p;
	if (port>0)
		serverAddr.sin_port=htons((u_short)port);/*get the port*/
	else{
		fprintf(stderr, "bad port number %d",port);
		return 0;
	}


	serverSock=socket(AF_INET,SOCK_STREAM,0);/*create a socket*/
	if (serverSock<0){
		CV_WARN("socket creation failed");
		return 0;
	}

	if (bind(serverSock,(LPSOCKADDR)&serverAddr,sockLen)<0){/*bind a client address and port*/
		CV_WARN("bind failed");
		return 0;
	}

	if(listen(serverSock, 5) != 0)
	{
		CV_WARN("listen failed");
		return 0;
	}

	return 1;
}

bool cap_ffmpeg_stream::grabFrame(void)
{
    bool valid = false;
    int got_picture;

    int count_errs = 0;
    const int max_number_of_attempts = 1 << 16;

	if(!ffmpeg_open)
		return 0;

    if( !ic || !video_st )  return false;

    if( ic->streams[video_stream]->nb_frames > 0 &&
        frame_number > ic->streams[video_stream]->nb_frames )
        return false;

    av_free_packet (&packet);

    picture_pts = AV_NOPTS_VALUE;

    // get the next frame
    while (!valid)
    {
        int ret = av_read_frame(ic, &packet);
        if (ret == AVERROR(EAGAIN)) continue;

        /* else if (ret < 0) break; */

        if( packet.stream_index != video_stream )
        {
            av_free_packet (&packet);
            count_errs++;
            if (count_errs > max_number_of_attempts)
                break;
            continue;
        }

        // Decode video frame
        #if LIBAVFORMAT_BUILD >= CALC_FFMPEG_VERSION(53, 2, 0)
            avcodec_decode_video2(video_st->codec, picture, &got_picture, &packet);
        #elif LIBAVFORMAT_BUILD > 4628
                avcodec_decode_video(video_st->codec,
                                     picture, &got_picture,
                                     packet.data, packet.size);
        #else
                avcodec_decode_video(&video_st->codec,
                                     picture, &got_picture,
                                     packet.data, packet.size);
        #endif

        // Did we get a video frame?
        if(got_picture)
        {
            //picture_pts = picture->best_effort_timestamp;
            if( picture_pts == AV_NOPTS_VALUE )
                picture_pts = packet.pts != AV_NOPTS_VALUE && packet.pts != 0 ? packet.pts : packet.dts;
            frame_number++;
            valid = true;
        }
        else
        {
            count_errs++;
            if (count_errs > max_number_of_attempts)
                break;
        }

        av_free_packet (&packet);
    }

    if( valid && first_frame_number < 0 )
        first_frame_number = dts_to_frame_number(picture_pts);

    // return if we have a new picture or not
    return valid;
}

bool cap_ffmpeg_stream::retrieveFrame(int, unsigned char** data, int* step, int* width, int* height, int* cn)
{
	if(!ffmpeg_open)
		return 0;
	if( !video_st || !picture->data[0] )
        return false;

    avpicture_fill((AVPicture*)&rgb_picture, rgb_picture.data[0], PIX_FMT_RGB24,
                   video_st->codec->width, video_st->codec->height);

    if( img_convert_ctx == NULL ||
        frame.width != video_st->codec->width ||
        frame.height != video_st->codec->height )
    {
        if( img_convert_ctx )
            sws_freeContext(img_convert_ctx);

        frame.width = video_st->codec->width;
        frame.height = video_st->codec->height;

        img_convert_ctx = sws_getCachedContext(
                NULL,
                video_st->codec->width, video_st->codec->height,
                video_st->codec->pix_fmt,
                video_st->codec->width, video_st->codec->height,
                PIX_FMT_BGR24,
                SWS_BICUBIC,
                NULL, NULL, NULL
                );

        if (img_convert_ctx == NULL)
            return false;//CV_Error(0, "Cannot initialize the conversion context!");
    }

    sws_scale(
            img_convert_ctx,
            picture->data,
            picture->linesize,
            0, video_st->codec->height,
            rgb_picture.data,
            rgb_picture.linesize
            );

    *data = frame.data;
    *step = frame.step;
    *width = frame.width;
    *height = frame.height;
    *cn = frame.cn;
    return true;
}

IplImage *cap_ffmpeg_stream::retrieve(void)
{
	unsigned char* data = 0;
	int step=0, width=0, height=0, cn=0;
	
	if(!ffmpeg_open)
		return 0;
	if(!retrieveFrame(0, &data, &step, &width, &height, &cn))
		return 0;

    cvInitImageHeader(&cvFrame, cvSize(width, height), 8, cn);
    cvSetData(&cvFrame, data, step);

    return &cvFrame;
}

bool cap_ffmpeg_stream::retrieveMat(Mat& image)
{
    IplImage* _img;
	
	if(!ffmpeg_open)
		return 0;

	_img = retrieve();

    if( !_img )
    {
		image.release();
        return false;
    }
    if(_img->origin == IPL_ORIGIN_TL)
	{
		Mat(_img).copyTo(image);
	}
    else
    {
		Mat temp(_img);
        flip(temp, image, 0);
    }
	return true;
}

bool cap_ffmpeg_stream::read(Mat& image)
{
	if(!ffmpeg_open)
		return 0;

	if(grabFrame())
		retrieveMat(image);
	else
		image.release();
	return !image.empty();
}

int cap_ffmpeg_stream::readSocketRaw(char* buff, int bufLen)
{
	int len;

	len = recv(cliSock, buff, bufLen, 0);

	return len;
}