/*
 *   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
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

#if !defined(DMRDefines_H)
#define	DMRDefines_H

const unsigned int DMR_FRAME_LENGTH_BITS  = 264U;
const unsigned int DMR_FRAME_LENGTH_BYTES = 33U;

const unsigned int DMR_SYNC_LENGTH_BITS  = 48U;
const unsigned int DMR_SYNC_LENGTH_BYTES = 6U;

const unsigned int DMR_EMB_LENGTH_BITS  = 8U;
const unsigned int DMR_EMB_LENGTH_BYTES = 1U;

const unsigned int DMR_SLOT_TYPE_LENGTH_BITS  = 8U;
const unsigned int DMR_SLOT_TYPE_LENGTH_BYTES = 1U;

const unsigned int DMR_EMBEDDED_SIGNALLING_LENGTH_BITS  = 32U;
const unsigned int DMR_EMBEDDED_SIGNALLING_LENGTH_BYTES = 4U;

const unsigned int DMR_AMBE_LENGTH_BITS  = 108U * 2U;
const unsigned int DMR_AMBE_LENGTH_BYTES = 27U;

const unsigned int DMR_CACH_LENGTH_BITS  = 24U;
const unsigned int DMR_CACH_LENGTH_BYTES = 3U;

const unsigned char PAYLOAD_LEFT_MASK[]       = {0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xF0U};
const unsigned char PAYLOAD_RIGHT_MASK[]      = {0x0FU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};

const unsigned char VOICE_LC_HEADER_CRC_MASK[]    = {0x96U, 0x96U, 0x96U};
const unsigned char TERMINATOR_WITH_LC_CRC_MASK[] = {0x99U, 0x99U, 0x99U};
const unsigned char PI_HEADER_CRC_MASK[]          = {0x69U, 0x69U};
const unsigned char DATA_HEADER_CRC_MASK[]        = {0xCCU, 0xCCU};
const unsigned char CSBK_CRC_MASK[]               = {0xA5U, 0xA5U};

const unsigned int DMR_SLOT_TIME = 60U;
const unsigned int AMBE_PER_SLOT = 3U;

const unsigned char DT_MASK               = 0x0FU;
const unsigned char DT_VOICE_PI_HEADER    = 0x00U;
const unsigned char DT_VOICE_LC_HEADER    = 0x01U;
const unsigned char DT_TERMINATOR_WITH_LC = 0x02U;
const unsigned char DT_CSBK               = 0x03U;
const unsigned char DT_DATA_HEADER        = 0x06U;
const unsigned char DT_RATE_12_DATA       = 0x07U;
const unsigned char DT_RATE_34_DATA       = 0x08U;
const unsigned char DT_IDLE               = 0x09U;
const unsigned char DT_RATE_1_DATA        = 0x0AU;

// Dummy values
const unsigned char DT_VOICE_SYNC  = 0xF0U;
const unsigned char DT_VOICE       = 0xF1U;

const unsigned char DMR_IDLE_RX    = 0x80U;
const unsigned char DMR_SYNC_DATA  = 0x40U;
const unsigned char DMR_SYNC_AUDIO = 0x20U;

const unsigned char DMR_SLOT1      = 0x00U;
const unsigned char DMR_SLOT2      = 0x80U;

const unsigned char DPF_UDT              = 0x00U;
const unsigned char DPF_RESPONSE         = 0x01U;
const unsigned char DPF_UNCONFIRMED_DATA = 0x02U;
const unsigned char DPF_CONFIRMED_DATA   = 0x03U;
const unsigned char DPF_DEFINED_SHORT    = 0x0DU;
const unsigned char DPF_DEFINED_RAW      = 0x0EU;
const unsigned char DPF_PROPRIETARY      = 0x0FU;

const unsigned char FID_ETSI = 0U;
const unsigned char FID_DMRA = 16U;

enum FLCO {
	FLCO_GROUP = 0,
	FLCO_USER_USER = 3
};

#endif
