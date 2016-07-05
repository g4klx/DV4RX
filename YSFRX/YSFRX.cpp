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

#include "YSFRX.h"
#include "Version.h"
#include "DV4mini.h"
#include "Utils.h"
#include "Log.h"

#include "YSFDefines.h"
#include "YSFPayload.h"
#include "YSFFICH.h"

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

const uint64_t SYNC_DATA = 0xD471C9634DU;
const uint64_t SYNC_MASK = 0xFFFFFFFFFFU;

int main(int argc, char** argv)
{
	if (argc == 2 && (::strcmp(argv[1], "-v") == 0 || ::strcmp(argv[1], "--version") == 0)) {
		::fprintf(stdout, "YSFRX version %s\n", VERSION);
		return 0;
	}

	if (argc < 3) {
		::fprintf(stderr, "Usage: YSFRX [-v|--version] <port> <frequency\n");
		return 1;
	}

	std::string port = std::string(argv[1U]);
	unsigned int frequency = ::atoi(argv[2U]);

	::LogInitialise(".", "YSFRX", 1U, 1U);

	CYSFRX rx(port, frequency);
	rx.run();

	::LogFinalise();

	return 0;
}

CYSFRX::CYSFRX(const std::string& port, unsigned int frequency) :
m_port(port),
m_frequency(frequency),
m_bits(0U),
m_count(0U),
m_pos(0U),
m_receiving(false),
m_buffer(NULL)
{
	m_buffer = new unsigned char[YSF_FRAME_LENGTH_BYTES];
}

CYSFRX::~CYSFRX()
{
	delete[] m_buffer;
}

void CYSFRX::run()
{
	CDV4mini dv4mini(m_port, m_frequency);

	bool ret = dv4mini.open();
	if (!ret)
		return;

	LogMessage("YSFRX-%s starting", VERSION);

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

void CYSFRX::decode(const unsigned char* data, unsigned int length)
{
	for (unsigned int i = 0U; i < (length * 8U); i++) {
		bool b = READ_BIT(data, i) != 0x00U;

		m_bits <<= 1;
		if (b)
			m_bits |= 0x01U;

		uint64_t v = (m_bits & SYNC_MASK) ^ SYNC_DATA;

		unsigned int errs = 0U;
		while (v != 0U) {
			v &= v - 1U;
			errs++;
		}

		if (errs < 3U) {
			m_count = 0U;
			m_pos = 39U;
			if (!m_receiving)
				::memcpy(m_buffer, YSF_SYNC_BYTES, YSF_SYNC_LENGTH_BYTES);
			m_receiving = true;
		}

		if (m_receiving)
			processBit(b);
	}
}

void CYSFRX::processBit(bool b)
{
	WRITE_BIT(m_buffer, m_pos, b);
	m_pos++;

	if (m_pos < YSF_FRAME_LENGTH_BITS)
		return;

	// CUtils::dump("YSF Frame bytes", m_buffer, YSF_FRAME_LENGTH_BYTES);

	CYSFFICH fich;
	bool valid = fich.decode(m_buffer);
	if (valid) {
		unsigned char dt = fich.getDT();
		unsigned char fi = fich.getFI();
		unsigned char bn = fich.getBN();
		unsigned char bt = fich.getBT();
		unsigned char fn = fich.getFN();
		unsigned char ft = fich.getFT();

		LogMessage("FICH: FI: %u, DT: %u, BN: %u, BT: %u, FN: %u, FT: %u", fi, dt, bn, bt, fn, ft);

		CYSFPayload payload;

		switch (fi) {
		case YSF_FI_HEADER:
			payload.processHeaderData(m_buffer);
			break;

		case YSF_FI_TERMINATOR:
			payload.processTerminatorData(m_buffer);
			m_receiving = false;
			m_count = 0U;
			break;

		case YSF_FI_COMMUNICATIONS:
			switch (dt) {
			case YSF_DT_VD_MODE1: {
					payload.processVDMode1Data(m_buffer, fn);
					unsigned int errors = payload.processVDMode1Audio(m_buffer);
					LogMessage("YSF, V/D Mode 1, BER=%.1f%%", float(errors) / 2.35F);
				}
				break;

			case YSF_DT_VD_MODE2: {
					payload.processVDMode2Data(m_buffer, fn);
					unsigned int errors = payload.processVDMode2Audio(m_buffer);
					LogMessage("YSF, V/D Mode 2, BER=%.1f%%", float(errors) / 1.35F);
				}
				break;

			case YSF_DT_DATA_FR_MODE:
				payload.processDataFRModeData(m_buffer, fn);
				break;

			case YSF_DT_VOICE_FR_MODE:
				if (fn == 0U && ft == 1U) {
					payload.processVoiceFRModeData(m_buffer);
				} else {
					unsigned int errors = payload.processVoiceFRModeAudio(m_buffer);
					LogMessage("YSF, V Mode 3, BER=%.1f%%", float(errors) / 7.2F);
				}
				break;

			default:
				break;
			}
			break;

		default:
			break;
		}
	}

	m_pos = 0U;

	m_count++;
	if (m_count >= 4U) {
		LogMessage("Signal lost");
		m_receiving = false;
	}
}
