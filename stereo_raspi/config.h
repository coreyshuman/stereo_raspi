/*  Filename: config.h
    Config for stereo raspi

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

#define OCULUS_LEFT_WIDTH		640
#define OCULUS_RIGHT_WIDTH		640
#define OCULUS_LEFT_HEIGHT		800
#define OCULUS_RIGHT_HEIGHT		800

#define RASPI_LEFT_WIDTH		1920
#define RASPI_RIGHT_WIDTH		1920
#define RASPI_LEFT_HEIGHT		1080
#define RASPI_RIGHT_HEIGHT		1080


#define ROI_LEFT_TOP			((RASPI_LEFT_HEIGHT-OCULUS_LEFT_HEIGHT) / 2)
#define ROI_LEFT_LEFT			((RASPI_LEFT_WIDTH-OCULUS_LEFT_WIDTH) / 2)
#define ROI_LEFT_BOTTOM			((RASPI_LEFT_HEIGHT-OCULUS_LEFT_HEIGHT) / 2 + OCULUS_LEFT_HEIGHT)
#define ROI_LEFT_RIGHT			((RASPI_LEFT_WIDTH-OCULUS_LEFT_WIDTH) / 2 + OCULUS_LEFT_WIDTH)