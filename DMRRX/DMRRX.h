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

#if !defined(DMRRX_H)
#define	DMRRX_H

#include <string>

#include <cstdint>

enum SYNC_TYPE {
	SYNC_NONE,
	SYNC_DATA,
	SYNC_AUDIO
};

class CDMRRX {
public:
	CDMRRX(const std::string& port, unsigned int frequency);
	~CDMRRX();

	void run();

private:
	std::string  m_port;
	unsigned int m_frequency;
	uint64_t     m_bits;
	unsigned int m_count;
	unsigned int m_pos;
	SYNC_TYPE    m_type;
	bool         m_receiving;
	unsigned char* m_buffer;

	void decode(const unsigned char* data, unsigned int length);
	void processBit(bool b);

	void processDataSync(const unsigned char* buffer);
	void processCACH(const unsigned char* buffer);
};

#endif
