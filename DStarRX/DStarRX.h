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

#if !defined(DSTARRX_H)
#define	DSTARRX_H

#include "UDPSocket.h"

#include <string>

#include <cstdint>

enum RX_STATE {
	RX_NONE,
	RX_HEADER,
	RX_DATA
};

class CDStarRX {
public:
	CDStarRX(const std::string& port, unsigned int frequency);
	~CDStarRX();

	bool output(const std::string& address, unsigned int port);

	void run();

private:
	std::string  m_port;
	unsigned int m_frequency;
	in_addr      m_udpAddress;
	unsigned int m_udpPort;
	CUDPSocket*  m_socket;
	uint32_t     m_bits;
	unsigned int m_count;
	unsigned int m_pos;
	RX_STATE     m_state;
	unsigned char* m_buffer;
	unsigned int m_mar;
	int          m_pathMetric[4U];
	unsigned int m_pathMemory0[42U];
	unsigned int m_pathMemory1[42U];
	unsigned int m_pathMemory2[42U];
	unsigned int m_pathMemory3[42U];
	uint8_t      m_fecOutput[42U];

	void decode(const unsigned char* data, unsigned int length);
	void processHeader(bool b);
	void processData(bool b);

	bool rxHeader(unsigned char* in, unsigned char* out);
	void acs(int* metric);
	void viterbiDecode(int* data);
	void traceBack();
};

#endif
