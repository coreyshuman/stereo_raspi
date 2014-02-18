### stereo_raspi Documentation

This document describes the purpose and usage of the stereo_raspi project as of February 2nd 2014.

There are three main components to this project:

  cap_ffmpeg_stream.cpp -

This file provides an interface to OpenCV which allows for the capture of raw h264 video streams, specifically from Raspberry Pis using the raspivid application. This interface is modeled after the built in video capture interfaces in OpenCV.

  barrel_dist.c -

This provides a very simple barrel distortion that can be applied to the output frame from cap_ffmpeg_stream.

  stereo_raspi.cpp -
  
This is the main entry point to the project. This function initilizes two instances of cap_ffmpeg_stream and retrieves the frames in parallel using simple threading. New threads are then distorted using the barrel distortion algorithm, and then combined into a single frame for output. This output feed can be used with an Oculus Rift.


#### Requirements:

FFmpeg Win32 shared build version: 2014-01-15 git-785dc14

OpenCV version 2.4.7.2


#### Using the Firmware:

The application is run like a console app, and expects two arguments to define the two TCP ports it will listen on for video streams. An example:

    > stereo_raspi.exe 2003 2004
    
To initiate the video feed on each Raspberry Pi, you can use the following command, making sure that the IP address points to the device running stereo_raspi, and the port number matches what was used above:

    > raspivid -t 9999999 -n -hf -vf -w 640 -h 800 -g 10 -pf baseline -o - | nc <IP> 2003
    
Once the application is running, there are a few commands that can be used from the output video window:

    C - this will exit the application
    
    F - this will flush the video buffer, a work-around to fix sync between the streams
    
    D - This will toggle between different interpupillary distance options
    
    S - This will save a snapshot to the local directory
