/*  Filename: gimbal_position.h
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

#ifndef _GIMBAL_POSITION_H
#define _GIMBAL_POSITION_H


#define GP_ERROR(message) fprintf(stderr, "ERROR Gimbal Position: %s (%s:%d)\n", message, __FILE__, __LINE__)


typedef struct _gpos_flags {
	union {
		BYTE allF;
		struct {
			BYTE running	: 1;
			BYTE stopThread : 1;
			BYTE unused		: 6;
		};
	};
} gpos_flags;

struct _thread_data {
	int *clientSock;
	struct sockaddr_in *serverAddr; 
	oculus_common *oculus;
	gpos_flags *flags;
};

class gimbal_position
{
	oculus_common oculus;
	_thread_data thread_data;
	struct sockaddr_in serverAddr; /*server address*/
	struct sockaddr_in clientAddr; /*client address*/
	int clientSock; /*client sock*/
	char buf[100]; /*buffer the message send and receive*/
	int serverPort; /*protocol port*/
	WSADATA wsaData;
	HANDLE gThreadHandle;
	gpos_flags flags;

public:
	gimbal_position();
	~gimbal_position();
	BOOL start(char* IPAddr, unsigned int port);
	BOOL stop(void);
	void test(void);

	
};

#endif
