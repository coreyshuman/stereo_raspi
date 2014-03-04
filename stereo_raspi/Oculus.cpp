/*  Filename: Oculus.cpp
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

#include "StdAfx.h"
#include "Oculus.h"

oculus_common::oculus_common(void)
{
	OVR::System::Init(OVR::Log::ConfigureDefaultLog(OVR::LogMask_All));
}

oculus_common::~oculus_common(void)
{
    pSensor.Clear();
    pHMD.Clear();
    pManager.Clear();

    OVR::System::Destroy();
}

BOOL oculus_common::init(void)
{
    pManager = *OVR::DeviceManager::Create();
    pHMD     = *pManager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();
    if (!pHMD)
	{
		OR_ERROR("Enumeration Failed");
        return 0;
	}

    pSensor  = *pHMD->GetSensor();

    if(!pSensor)
	{
		OR_ERROR("No Sensor Detected");
		return 0;
	}
	
	if(!pHMD->GetDeviceInfo(&hmdInfo))
	{
		OR_ERROR("Couldn't retrieve device info");
		return 0;
	}

	// calculate hmd physical data and offset information
	CenterOffsetPixels =  (int)((float)hmdInfo.HResolution / hmdInfo.HScreenSize) * (hmdInfo.InterpupillaryDistance / 2.);

    FusionResult.AttachToSensor(pSensor);
}

void oculus_common::getEulerAngles(float *yaw, float *pitch, float *roll)
{
	OrientQuat = FusionResult.GetOrientation();
	OrientQuat.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(yaw, pitch, roll);
}

int oculus_common::getFrameCenterPixelOffset(void)
{
	return CenterOffsetPixels;
}