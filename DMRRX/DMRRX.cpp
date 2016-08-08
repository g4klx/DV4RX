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

#include "DMRRX.h"
#include "Version.h"
#include "DMRDataHeader.h"
#include "DMRShortLC.h"
#include "DMRSlotType.h"
#include "DMRFullLC.h"
#include "Hamming.h"
#include "DMRCSBK.h"
#include "AMBEFEC.h"
#include "DV4mini.h"
#include "DMREMB.h"
#include "Utils.h"
#include "Log.h"

#include "DMRDefines.h"

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>

const unsigned char IDLE_DATA[] =
	{0x53U, 0xC2U, 0x5EU, 0xABU, 0xA8U, 0x67U, 0x1DU, 0xC7U, 0x38U, 0x3BU, 0xD9U,
	 0x36U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x03U, 0xF6U,
	 0xE4U, 0x65U, 0x17U, 0x1BU, 0x48U, 0xCAU, 0x6DU, 0x4FU, 0xC6U, 0x10U, 0xB4U};

const unsigned char PAYLOAD_MASK[] =
	{0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
	 0xFFU, 0xC0U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x03U, 0xFFU,
	 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};

const unsigned int CACH_INTERLEAVE[] =
	{1U, 2U, 3U, 5U, 6U, 7U, 9U, 10U, 11U, 13U, 15U, 16U, 17U, 19U, 20U, 21U, 23U};

const unsigned char BIT_MASK_TABLE[] = {0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U};

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

const uint64_t AUDIO_SYNC = 0x755FD7DF75F7U;
const uint64_t DATA_SYNC  = 0xDFF57D75DF5DU;

const uint64_t SYNC_MASK  = 0xFFFFFFFFFFFFU;

int main(int argc, char** argv)
{
	if (argc == 2 && (::strcmp(argv[1], "-v") == 0 || ::strcmp(argv[1], "--version") == 0)) {
		::fprintf(stdout, "DMRRX version %s\n", VERSION);
		return 0;
	}

	if (argc != 3 && argc != 6) {
		::fprintf(stderr, "Usage: DMRRX [-v|--version] <port> <frequency> [<address> <port> <slot>]\n");
		return 1;
	}

	std::string port = std::string(argv[1U]);
	unsigned int frequency = ::atoi(argv[2U]);

	CDMRRX rx(port, frequency);

	if (argc == 6) {
		std::string address = std::string(argv[3U]);
		unsigned int port = ::atoi(argv[4U]);
		unsigned int slot = ::atoi(argv[5]);

		bool ret = rx.output(address, port, slot);
		if (!ret) {
			::fprintf(stderr, "DMRRX: cannot open the UDP output port\n");
			return 1;
		}
	}

	::LogInitialise(".", "DMRRX", 1U, 1U);

	rx.run();

	::LogFinalise();

	return 0;
}

CDMRRX::CDMRRX(const std::string& port, unsigned int frequency) :
m_port(port),
m_frequency(frequency),
m_udpAddress(),
m_udpPort(0U),
m_udpSlot(0U),
m_socket(NULL),
m_bits(0U),
m_count(0U),
m_pos(0U),
m_slotNo(1U),
m_type(SYNC_NONE),
m_receiving(false),
m_buffer(NULL),
m_shortN(0U),
m_shortLC(NULL),
m_n(0U),
m_embeddedLC(),
m_idleBits(0U),
m_idleErrs(0U)
{
	m_buffer  = new unsigned char[DMR_FRAME_LENGTH_BYTES];
	m_shortLC = new unsigned char[9U];
}

CDMRRX::~CDMRRX()
{
	delete[] m_buffer;
	delete[] m_shortLC;
}

bool CDMRRX::output(const std::string& address, unsigned int port, unsigned int slot)
{
	m_udpAddress = CUDPSocket::lookup(address);
	if (m_udpAddress.s_addr == INADDR_NONE)
		return false;

	m_udpPort = port;
	m_udpSlot = slot;

	m_socket = new CUDPSocket;

	bool ret = m_socket->open();
	if (!ret) {
		delete m_socket;
		return false;
	}

	return true;
}

void CDMRRX::run()
{
	CDV4mini dv4mini(m_port, m_frequency);

	bool ret = dv4mini.open();
	if (!ret)
		return;

	LogMessage("DMRRX-%s starting", VERSION);

	for (;;) {
		unsigned char buffer[300U];
		unsigned int len = dv4mini.read(buffer, 300U);

		if (len > 0U) {
			unsigned int length = buffer[5U];
			decode(buffer + 6U, length);
		}

#if defined(_WIN32) || defined(_WIN64)
		::Sleep(5UL);		// 5ms
#else
		::usleep(5000);		// 5ms
#endif
	}

	dv4mini.close();

	if (m_socket != NULL) {
		m_socket->close();
		delete m_socket;
	}
}

void CDMRRX::decode(const unsigned char* data, unsigned int length)
{
	for (unsigned int i = 0U; i < (length * 8U); i++) {
		bool b = READ_BIT(data, i) != 0x00U;

		m_bits <<= 1;
		if (b)
			m_bits |= 0x01U;

		uint64_t v = (m_bits & SYNC_MASK) ^ DATA_SYNC;

		unsigned int errs = 0U;
		while (v != 0U) {
			v &= v - 1U;
			errs++;
		}

		if (errs < 3U) {
			m_count = 0U;
			m_pos = 155U;
			m_type = SYNC_DATA;
			m_receiving = true;
		}

		v = (m_bits & SYNC_MASK) ^ AUDIO_SYNC;

		errs = 0U;
		while (v != 0U) {
			v &= v - 1U;
			errs++;
		}

		if (errs < 3U) {
			m_count = 0U;
			m_pos = 155U;
			m_type = SYNC_AUDIO;
			m_receiving = true;
		}

		if (m_receiving)
			processBit(b);
	}
}

void CDMRRX::processBit(bool b)
{
	WRITE_BIT(m_buffer, m_pos, b);
	m_pos++;

	if (m_pos == DMR_FRAME_LENGTH_BITS) {
		// CUtils::dump("DMR Frame bytes", m_buffer, DMR_FRAME_LENGTH_BYTES);

		switch (m_type) {
		case SYNC_AUDIO: {
				CAMBEFEC fec;
				unsigned int ber = fec.regenerateDMR(m_buffer);
				LogMessage("%u [Audio Sync] BER=%.1f%%", m_slotNo, float(ber) / 1.41F);
				m_n = 0U;

				if (m_socket != NULL && m_slotNo == m_udpSlot)
					writeAMBE(m_buffer);
		}
			break;
		case SYNC_DATA:
			processDataSync(m_buffer);
			break;
		case SYNC_NONE:
			processAudio(m_buffer);
			break;
		default:
			break;
		}

		m_type = SYNC_NONE;

		m_count++;
		if (m_count >= 10U) {
			if (m_idleBits == 0U) m_idleBits = 1U;
			LogMessage("Signal lost, BER=%.1f%%", float(m_idleErrs * 100U) / float(m_idleBits));
			m_receiving = false;
			m_idleBits = 0U;
			m_idleErrs = 0U;
		}
	}

	if (m_pos == (DMR_FRAME_LENGTH_BITS + DMR_CACH_LENGTH_BITS)) {
		// CUtils::dump("DMR CACH bytes", m_buffer + DMR_FRAME_LENGTH_BYTES, DMR_CACH_LENGTH_BYTES);
		processCACH(m_buffer + DMR_FRAME_LENGTH_BYTES);
		m_pos = 0U;
	}
}

void CDMRRX::processDataSync(const unsigned char* buffer)
{
	CDMRSlotType slotType;
	slotType.putData(buffer);

	unsigned char type = slotType.getDataType();
	unsigned char cc   = slotType.getColorCode();

	if (type == DT_IDLE) {
		unsigned int ber = idleBER(m_buffer);
		LogMessage("%u [Data Sync] [IDLE] CC=%u BER=%.1f%%", m_slotNo, cc, float(ber) / 1.96F);
		m_idleBits += 196U;
		m_idleErrs += ber;
	} else if (type == DT_VOICE_LC_HEADER) {
		CDMRFullLC fullLC;
		CDMRLC* lc = fullLC.decode(buffer, type);
		if (lc != NULL) {
			LogMessage("%u [Data Sync] [VOICE_LC_HEADER] CC=%u src=%u dest=%s%u", m_slotNo, cc, lc->getSrcId(), lc->getFLCO() == FLCO_GROUP ? "TG" : "", lc->getDstId());
			delete lc;
		} else {
			LogMessage("%u [Data Sync] [VOICE_LC_HEADER] CC=%u <Invalid LC>", m_slotNo, cc);
		}
	} else if (type == DT_TERMINATOR_WITH_LC) {
		CDMRFullLC fullLC;
		CDMRLC* lc = fullLC.decode(buffer, type);
		if (lc != NULL) {
			LogMessage("%u [Data Sync] [DT_TERMINATOR_WITH_LC] CC=%u src=%u dest=%s%u", m_slotNo, cc, lc->getSrcId(), lc->getFLCO() == FLCO_GROUP ? "TG" : "", lc->getDstId());
			delete lc;
		} else {
			LogMessage("%u [Data Sync] [DT_TERMINATOR_WITH_LC] CC=%u <Invalid LC>", m_slotNo, cc);
		}
	} else if (type == DT_VOICE_PI_HEADER) {
		LogMessage("%u [Data Sync] [VOICE_PI_HEADER] CC=%u", m_slotNo, cc);
	} else if (type == DT_DATA_HEADER) {
		CDMRDataHeader header;
		bool valid = header.put(buffer);
		if (valid)
			LogMessage("%u [Data Sync] [DATA_HEADER] CC=%u src=%u dest=%s%u", m_slotNo, cc, header.getSrcId(), header.getGI() ? "TG" : "", header.getDstId());
		else
			LogMessage("%u [Data Sync] [DATA_HEADER] CC=%u invalid", m_slotNo, cc);
	} else if (type == DT_RATE_12_DATA) {
		LogMessage("%u [Data Sync] [RATE_1/2_DATA] CC=%u", m_slotNo, cc);
	} else if (type == DT_RATE_34_DATA) {
		LogMessage("%u [Data Sync] [RATE_3/4_DATA] CC=%u", m_slotNo, cc);
	} else if (type == DT_RATE_1_DATA) {
		LogMessage("%u [Data Sync] [RATE_1_DATA] CC=%u", m_slotNo, cc);
	} else if (type == DT_CSBK) {
		CDMRCSBK csbk;
		bool valid = csbk.put(buffer);
		if (valid)
			LogMessage("%u [Data Sync] [CSBK] CC=%u src=%u dest=%s%u", m_slotNo, cc, csbk.getSrcId(), csbk.getGI() ? "TG" : "", csbk.getDstId());
		else
			LogMessage("%u [Data Sync] [CSBK] CC=%u invalid", m_slotNo, cc);
	} else {
		LogMessage("%u [Data Sync] [UNKNOWN] CC=%u type=%u", m_slotNo, cc, type);
	}
}

void CDMRRX::processCACH(const unsigned char* buffer)
{
	bool word[7U];

	word[0U] = (buffer[0U] & 0x80U) == 0x80U;
	word[1U] = (buffer[0U] & 0x08U) == 0x08U;

	word[2U] = (buffer[1U] & 0x80U) == 0x80U;
	word[3U] = (buffer[1U] & 0x08U) == 0x08U;

	word[4U] = (buffer[1U] & 0x02U) == 0x02U;
	word[5U] = (buffer[2U] & 0x20U) == 0x20U;
	word[6U] = (buffer[2U] & 0x02U) == 0x02U;

	CHamming::decode743(word);

	if (!word[2U] && word[3U])
		m_shortN = 0U;
	else if (word[2U] && !word[3U])
		m_shortN = 3U;

	for (unsigned int i = 0U; i < 17U; i++) {
		unsigned int m = CACH_INTERLEAVE[i];
		bool b = READ_BIT(buffer, m) != 0x00U;

		unsigned int n = i + (m_shortN * 17U);
		WRITE_BIT(m_shortLC, n, b);
	}

	if (word[2U] && !word[3U]) {
		unsigned char lc[5U];

		CDMRShortLC shortLC;
		bool valid = shortLC.decode(m_shortLC, lc);

		if (valid)
			LogMessage("  [CACH] AT=%d TC=%d LCSS=%d%d %02X %02X %02X %02X %02X %02X %02X %02X %02X LC=%02X %02X %02X %02X %02X", word[0U] ? 1 : 0, word[1U] ? 1 : 0, word[2U] ? 1 : 0, word[3U] ? 1 : 0, m_shortLC[0U], m_shortLC[1U], m_shortLC[2U], m_shortLC[3U], m_shortLC[4U], m_shortLC[5U], m_shortLC[6U], m_shortLC[7U], m_shortLC[8U], lc[0U], lc[1U], lc[2U], lc[3U], lc[4U]);
		else
			LogMessage("  [CACH] AT=%d TC=%d LCSS=%d%d %02X %02X %02X %02X %02X %02X %02X %02X %02X <Invalid LC>", word[0U] ? 1 : 0, word[1U] ? 1 : 0, word[2U] ? 1 : 0, word[3U] ? 1 : 0, m_shortLC[0U], m_shortLC[1U], m_shortLC[2U], m_shortLC[3U], m_shortLC[4U], m_shortLC[5U], m_shortLC[6U], m_shortLC[7U], m_shortLC[8U]);
	} else {
		LogMessage("  [CACH] AT=%d TC=%d LCSS=%d%d", word[0U] ? 1 : 0, word[1U] ? 1 : 0, word[2U] ? 1 : 0, word[3U] ? 1 : 0);
	}

	m_shortN++;
	if (m_shortN >= 4U)
		m_shortN = 0U;

	m_slotNo = word[1U] ? 2U : 1U;
}

void CDMRRX::processAudio(const unsigned char* buffer)
{
	CAMBEFEC fec;
	unsigned int ber = fec.regenerateDMR(m_buffer);

	if (m_socket != NULL && m_slotNo == m_udpSlot)
		writeAMBE(m_buffer);

	CDMREMB emb;
	emb.putData(buffer);

	unsigned char colorCode = emb.getColorCode();
	unsigned char lcss      = emb.getLCSS();

	bool l1 = (lcss & 0x02U) == 0x02U;
	bool l0 = (lcss & 0x01U) == 0x01U;

	CDMRLC* lc = m_embeddedLC.addData(m_buffer, lcss);

	m_n++;

	if (lcss == 0x02U) {
		if (lc == NULL) {
			LogMessage("%u [Audio] BER=%.1f%% CC=%u LCSS=%d%d n=%u <Invalid LC>", m_slotNo, float(ber) / 1.41F, colorCode, l0 ? 1 : 0, l0 ? 1 : 0, m_n);
		} else {
			LogMessage("%u [Audio] BER=%.1f%% CC=%u LCSS=%d%d n=%u src=%u dest=%s%u", m_slotNo, float(ber) / 1.41F, colorCode, l0 ? 1 : 0, l0 ? 1 : 0, m_n, lc->getSrcId(), lc->getFLCO() == FLCO_GROUP ? "TG" : "", lc->getDstId());
			delete lc;
		}
	} else {
		LogMessage("%u [Audio] BER=%.1f%% CC=%u LCSS=%d%d n=%u", m_slotNo, float(ber) / 1.41F, colorCode, l0 ? 1 : 0, l0 ? 1 : 0, m_n);
	}
}

unsigned int CDMRRX::idleBER(const unsigned char* buffer)
{
	unsigned int errs = 0U;

	for (unsigned int i = 0U; i < DMR_FRAME_LENGTH_BYTES; i++) {
		unsigned char v1 = buffer[i] & PAYLOAD_MASK[i];
		unsigned char v2 = IDLE_DATA[i] & PAYLOAD_MASK[i];
		unsigned char v = v1 ^ v2;

		while (v != 0U) {
			v &= v - 1U;
			errs++;
		}
	}

	return errs;
}

void CDMRRX::writeAMBE(const unsigned char* buffer)
{
	unsigned char ambe[DMR_AMBE_LENGTH_BYTES];

	for (unsigned int i = 0U; i < 13U; i++) {
		ambe[i + 0U]  = buffer[i + 0U];
		ambe[i + 14U] = buffer[i + 20U];
	}

	ambe[13U] = (buffer[13U] & 0xF0U) | (buffer[19U] & 0x0FU);

	m_socket->write(ambe, DMR_AMBE_LENGTH_BYTES, m_udpAddress, m_udpPort);
}
