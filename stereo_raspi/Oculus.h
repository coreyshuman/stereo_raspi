/*  Filename: Oculus.h
    Oculus Rift common interface.

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

#ifndef _OCULUS_H
#define _OCULUS_H

#include "OVR.h"
#include <stdio.h>
#include <cstdio>
#include <stdlib.h>
#include <Windows.h>
#include <string>
#include <conio.h>


#define OR_ERROR(message) fprintf(stderr, "ERROR Oculus: %s (%s:%d)\n", message, __FILE__, __LINE__)


class oculus_common
{
    OVR::Ptr<OVR::DeviceManager> pManager;
    OVR::Ptr<OVR::HMDDevice>     pHMD;
    OVR::Ptr<OVR::SensorDevice>  pSensor;
    OVR::SensorFusion			FusionResult;
	OVR::Quatf					OrientQuat;
	OVR::HMDInfo hmdInfo;

	int CenterOffsetPixels;

public:
	oculus_common(void);
	~oculus_common(void);
	BOOL init(void);
	void getEulerAngles(float *yaw, float *pitch, float *roll);
	int getFrameCenterPixelOffset(void);
};

#endif