/*  Filename: gimbal_position.cpp
    Send oculus orientation values to gimbal processor.

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
#include <stdio.h>
#include "Oculus.h"
#include "gimbal_position.h"
#include <process.h>
#include "common_utils.h"


const int LOCAL_PORT=25001; /*the client port number*/

unsigned int __stdcall gimbalProcThread(void* arguments);

gimbal_position::gimbal_position()
{
	WSAStartup(0x0202,&wsaData); /*windows socket startup */

	memset((char*)&clientAddr,0,sizeof(clientAddr));
	clientAddr.sin_family=AF_INET; /*set client address protocol family*/
	clientAddr.sin_addr.s_addr=INADDR_ANY;
	clientAddr.sin_port=htons((u_short)LOCAL_PORT); /*set client port*/
	serverAddr.sin_family=AF_INET;

	oculus.init();

}

gimbal_position::~gimbal_position()
{
	stop();
	while(flags.running); // pause current thread until gimbal thread exits, up to 30000 us
}

BOOL gimbal_position::start(char* IPAddr, unsigned int port)
{
	serverAddr.sin_addr.s_addr=inet_addr(IPAddr);/*get the ip address*/
	if (serverAddr.sin_addr.s_addr==INADDR_NONE){
		GP_ERROR("Bad IP Address");
		return 0;
	}

	if (port == 0){
		GP_ERROR("Bad port number");
		return 0;
	}

	serverAddr.sin_port=htons((u_short)port);/*get the port*/

	clientSock=socket(PF_INET,SOCK_DGRAM,0);/*create a socket*/
	if (clientSock<0){
		GP_ERROR("Create Socket Failed");
		return 0;
	}

	if (bind(clientSock,(LPSOCKADDR)&clientAddr,sizeof(struct sockaddr)) <0 ){/*bind a client address and port*/
		GP_ERROR("Socket bind failed");
		return 0;
	}

	thread_data.clientSock = &clientSock;
	thread_data.oculus = &oculus;
	thread_data.serverAddr = &serverAddr;
	thread_data.flags = &flags;

	flags.running = true;
	gThreadHandle = (HANDLE)_beginthreadex(0, 0, &gimbalProcThread, (void*)&thread_data, 0, 0);

}

BOOL gimbal_position::stop(void)
{
	flags.stopThread = true;
	return true;
}

void gimbal_position::test(void)
{
	_thread_data thread_data;

	thread_data.clientSock = &clientSock;
	thread_data.oculus = &oculus;
	thread_data.serverAddr = &serverAddr;
	thread_data.flags = &flags;
	thread_data.flags->stopThread = true;

	gimbalProcThread((void*)&thread_data);
}

unsigned int __stdcall gimbalProcThread(void* arguments)
{
	float yaw=0, pitch=0, roll=0;
	double tmp;
	byte buf[3];
	struct _thread_data *thread_data = (_thread_data*)arguments;
	printf("\r\nenter gimbal thread"); //cts debug
	do
	{
		thread_data->oculus->getEulerAngles(&yaw, &pitch, &roll);
		printf("\r\nyaw=%f pitch=%f roll=%f", yaw, pitch, roll);
		tmp = ((3.2 - yaw*2.) * 255. / 6.4);
		if(tmp < 0 ) tmp = 0;
		if(tmp > 255) tmp = 255;
		buf[0] = (byte)tmp;
		buf[1] = (char)((3.2 - pitch) * 255. / 6.4);
		tmp = ((3.2 - roll*4.) * 255. / 6.4);
		if(tmp < 0 ) tmp = 0;
		if(tmp > 255) tmp = 255;
		buf[2] = (byte)tmp;
		// send value (single byte) over UDP
		sendto(*thread_data->clientSock,(char*)buf,3,0,(LPSOCKADDR)thread_data->serverAddr,sizeof(struct sockaddr));

		usleep(30000);
	} while(!thread_data->flags->stopThread);
	thread_data->flags->running = false;
	printf("Exit gimbal thread");//cts debug
	return 0;
}
