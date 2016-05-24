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

#include "DV4mini.h"
#include "Utils.h"
#include "Log.h"

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#else
#include <unistd.h>
#endif

CDV4mini::CDV4mini(const std::string& port, unsigned int frequency) :
m_serial(port, SERIAL_115200),
m_frequency(frequency)
{
}

CDV4mini::~CDV4mini()
{
}

bool CDV4mini::open()
{
	bool ret = m_serial.open();
	if (!ret)
		return false;

	getVersion();

	setFrequency();
	setMode();

	return true;
}

unsigned int CDV4mini::read(unsigned char* data, unsigned int length)
{
	unsigned char buffer[10U];
	buffer[0U] = 0x71U;
	buffer[1U] = 0xFEU;
	buffer[2U] = 0x39U;
	buffer[3U] = 0x1DU;
	buffer[4U] = 7U;
	buffer[5U] = 0U;

	m_serial.write(buffer, 6U);

	unsigned int len = getReply(data, length);
	if (len == 0U)
		return 0U;

	return len;
}

void CDV4mini::close()
{
	m_serial.close();
}

void CDV4mini::getVersion()
{
	unsigned char buffer[100U];
	buffer[0U] = 0x71U;
	buffer[1U] = 0xFEU;
	buffer[2U] = 0x39U;
	buffer[3U] = 0x1DU;
	buffer[4U] = 18U;
	buffer[5U] = 0U;

	m_serial.write(buffer, 6U);

#if defined(_WIN32) || defined(_WIN64)
	::Sleep(100UL);		// 100ms
#else
	::usleep(100000);	// 100ms
#endif

	unsigned int len = getReply(buffer, 100U);
	if (len == 0U)
		return;

	::LogMessage("DV4mini version: %s", buffer + 6U);
}

void CDV4mini::setFrequency()
{
	unsigned char buffer[20U];
	buffer[0U]  = 0x71U;
	buffer[1U]  = 0xFEU;
	buffer[2U]  = 0x39U;
	buffer[3U]  = 0x1DU;
	buffer[4U]  = 1U;
	buffer[5U]  = 8U;
	buffer[6U]  = (m_frequency >> 24) & 0xFFU;
	buffer[7U]  = (m_frequency >> 16) & 0xFFU;
	buffer[8U]  = (m_frequency >> 8) & 0xFFU;
	buffer[9U]  = (m_frequency >> 0) & 0xFFU;
	buffer[10U] = (m_frequency >> 24) & 0xFFU;
	buffer[11U] = (m_frequency >> 16) & 0xFFU;
	buffer[12U] = (m_frequency >> 8) & 0xFFU;
	buffer[13U] = (m_frequency >> 0) & 0xFFU;

	m_serial.write(buffer, 14U);
}

void CDV4mini::setMode()
{
	unsigned char buffer[10U];
	buffer[0U] = 0x71U;
	buffer[1U] = 0xFEU;
	buffer[2U] = 0x39U;
	buffer[3U] = 0x1DU;
	buffer[4U] = 2U;
	buffer[5U] = 1U;
	buffer[6U] = 'F';

	m_serial.write(buffer, 7U);
}

unsigned int CDV4mini::getReply(unsigned char* data, unsigned int length)
{
	data[0U] = 0x00U;

	while (data[0U] != 0x71U) {
		unsigned int ret = m_serial.read(data, 1U);
		if (ret == 0U) {
#if defined(_WIN32) || defined(_WIN64)
			::Sleep(5UL);		// 5ms
#else
			::usleep(5000);		// 5ms
#endif
		}
	}

	unsigned int offset = 0U;

	while (offset < 5U) {
		unsigned int ret = m_serial.read(data + offset + 1U, 5U - offset);
		if (ret == 0U) {
#if defined(_WIN32) || defined(_WIN64)
			::Sleep(5UL);		// 5ms
#else
			::usleep(5000);		// 5ms
#endif
		} else {
			offset += ret;
		}
	}

	unsigned int rest = data[5U];

	offset = 0U;

	while (offset < rest) {
		unsigned int ret = m_serial.read(data + 6U + offset, rest - offset);
		if (ret == 0U) {
#if defined(_WIN32) || defined(_WIN64)
			::Sleep(5UL);		// 5ms
#else
			::usleep(5000);		// 5ms
#endif
		} else {
			offset += ret;
		}
	}

	return rest + 5U;
}
