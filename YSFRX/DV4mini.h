/*
*   Copyright (C) 2016 by Thomas Sailor HB9JNX
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef DV4mini_H
#define DV4mini_H

#include "SerialController.h"

#include <string>

class CDV4mini {
public:
	CDV4mini(const std::string& port, unsigned int frequency);
	~CDV4mini();

	bool open();

	unsigned int read(unsigned char* data, unsigned int length);

	void close();

private:
	CSerialController m_serial;
	unsigned int      m_frequency;

	void getVersion();

	void setFrequency();
	void setMode();

	void readWatchdog();

	unsigned int getReply(unsigned char* data, unsigned int length);
};

#endif
