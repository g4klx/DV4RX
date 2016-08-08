/*
*   Copyright (C) 2016 by Jonathan Naylor G4KLX
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

#include "DMREmbeddedLC.h"
#include "UDPSocket.h"

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

	bool output(const std::string& address, unsigned int port, unsigned int slot);

	void run();

private:
	std::string    m_port;
	unsigned int   m_frequency;
	in_addr        m_udpAddress;
	unsigned int   m_udpPort;
	unsigned int   m_udpSlot;
	CUDPSocket*    m_socket;
	uint64_t       m_bits;
	unsigned int   m_count;
	unsigned int   m_pos;
	unsigned int   m_slotNo;
	SYNC_TYPE      m_type;
	bool           m_receiving;
	unsigned char* m_buffer;
	unsigned int   m_shortN;
	unsigned char* m_shortLC;
	unsigned int   m_n;
	CDMREmbeddedLC m_embeddedLC;
	unsigned int   m_idleBits;
	unsigned int   m_idleErrs;

	void decode(const unsigned char* data, unsigned int length);
	void processBit(bool b);

	void processDataSync(const unsigned char* buffer);
	void processCACH(const unsigned char* buffer);
	void processAudio(const unsigned char* buffer);
	unsigned int idleBER(const unsigned char* buffer);

	void writeAMBE(const unsigned char* buffer);
};

#endif
