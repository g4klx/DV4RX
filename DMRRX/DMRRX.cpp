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

#include "DMRRX.h"
#include "Version.h"
#include "DMRDataHeader.h"
#include "DMRSlotType.h"
#include "DMRFullLC.h"
#include "DMRCSBK.h"
#include "DV4mini.h"
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

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

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

	if (argc < 3) {
		::fprintf(stderr, "Usage: DMRRX [-v|--version] <port> <frequency\n");
		return 1;
	}

	std::string port = std::string(argv[1U]);
	unsigned int frequency = ::atoi(argv[2U]);

	::LogInitialise(".", "DMRRX", 1U, 1U);

	CDMRRX rx(port, frequency);
	rx.run();

	::LogFinalise();

	return 0;
}

CDMRRX::CDMRRX(const std::string& port, unsigned int frequency) :
m_port(port),
m_frequency(frequency),
m_bits(0U),
m_count(0U),
m_pos(0U),
m_type(SYNC_NONE),
m_receiving(false),
m_buffer(NULL)
{
	m_buffer = new unsigned char[DMR_FRAME_LENGTH_BYTES];
}

CDMRRX::~CDMRRX()
{
	delete[] m_buffer;
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
			m_pos = 156U;
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
			m_pos = 156U;
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
		case SYNC_AUDIO:
			LogMessage("[Audio Sync]");
			break;
		case SYNC_DATA:
			processDataSync(m_buffer);
			break;
		default:
			break;
		}

		m_type = SYNC_NONE;

		m_count++;
		if (m_count >= 10U) {
			LogMessage("Signal lost");
			m_receiving = false;
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
	if (type == DT_IDLE) {
		LogMessage("[Data Sync] [IDLE]");
	} else if (type == DT_VOICE_LC_HEADER) {
		CDMRFullLC fullLC;
		CDMRLC* lc = fullLC.decode(buffer, type);
		if (lc != NULL) {
			LogMessage("[Data Sync] [VOICE_LC_HEADER] src=%u dest=%s%u", lc->getSrcId(), lc->getFLCO() == FLCO_GROUP ? "TG" : "", lc->getDstId());
			delete lc;
		} else {
			LogMessage("[Data Sync] [VOICE_LC_HEADER] invalid");
		}
	} else if (type == DT_TERMINATOR_WITH_LC) {
		CDMRFullLC fullLC;
		CDMRLC* lc = fullLC.decode(buffer, type);
		if (lc != NULL) {
			LogMessage("[Data Sync] [DT_TERMINATOR_WITH_LC] src=%u dest=%s%u", lc->getSrcId(), lc->getFLCO() == FLCO_GROUP ? "TG" : "", lc->getDstId());
			delete lc;
		} else {
			LogMessage("[Data Sync] [DT_TERMINATOR_WITH_LC] invalid");
		}
	} else if (type == DT_VOICE_PI_HEADER) {
		LogMessage("[Data Sync] [VOICE_PI_HEADER]");
	} else if (type == DT_DATA_HEADER) {
		CDMRDataHeader header;
		bool valid = header.put(buffer);
		if (valid) {
			LogMessage("[Data Sync] [DATA_HEADER] src=%u dest=%s%u", header.getSrcId(), header.getGI() ? "TG" : "", header.getDstId());
		} else {
			LogMessage("[Data Sync] [DATA_HEADER] invalid");
		}
	} else if (type == DT_RATE_12_DATA || type == DT_RATE_34_DATA || type == DT_RATE_1_DATA) {
		LogMessage("[Data Sync] [DATA]");
	} else if (type == DT_CSBK) {
		CDMRCSBK csbk;
		bool valid = csbk.put(buffer);
		if (valid) {
			LogMessage("[Data Sync] [CSBK] src=%u dest=%s%u", csbk.getSrcId(), csbk.getGI() ? "TG" : "", csbk.getDstId());
		} else {
			LogMessage("[Data Sync] [CSBK] invalid");
		}
	} else {
		LogMessage("[Data Sync] [UNKNOWN] type=%u", type);
	}
}

void CDMRRX::processCACH(const unsigned char* buffer)
{

}
