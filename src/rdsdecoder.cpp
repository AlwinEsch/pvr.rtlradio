//---------------------------------------------------------------------------
// Copyright (c) 2020-2021 Michael G. Brehm
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "rdsdecoder.h"

#include <cstring>

#include "fmdsp/rbdsconstants.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// rdsdecoder Constructor
//
// Arguments:
//
//	isrbds		- Flag if input will be RBDS (North America) or RDS

rdsdecoder::rdsdecoder(bool isrbds) : m_isrbds(isrbds)
{
	m_ps_data.fill(0x00);				// Initialize PS buffer
	m_rt_data.fill(0x00);				// Initialize RT buffer
}

//---------------------------------------------------------------------------
// rdsdecoder Destructor

rdsdecoder::~rdsdecoder()
{
}

//---------------------------------------------------------------------------
// rdsdecoder::decode_basictuning
//
// Decodes Group Type 0A and 0B - Basic Tuning and swithing information
//
// Arguments:
//
//	rdsgroup	- RDS group to be processed

void rdsdecoder::decode_basictuning(tRDS_GROUPS const& rdsgroup)
{
	uint8_t const ps_codebits = rdsgroup.BlockB & 0x03;

	m_ps_data[ps_codebits * 2] = (rdsgroup.BlockD >> 8) & 0xFF;
	m_ps_data[ps_codebits * 2 + 1]= rdsgroup.BlockD & 0xFF;

	// Accumulate segments until all 4 (0xF) have been received
	m_ps_ready |= (0x01 << ps_codebits);
	if(m_ps_ready == 0x0F) {

		// UECP_MEC_PS
		//
		struct uecp_data_frame frame = {};
		struct uecp_message* message = &frame.msg;

		message->mec = UECP_MEC_PS;
		message->dsn = UECP_MSG_DSN_CURRENT_SET;
		message->psn = UECP_MSG_PSN_MAIN;

		// Kodi expects the 8 characters to start at the address of mel_len
		// when processing UECP_MEC_PS
		uint8_t* mel_data = &message->mel_len;
		memcpy(mel_data, &m_ps_data[0], m_ps_data.size());

		frame.seq = UECP_DF_SEQ_DISABLED;
		frame.msg_len = 3 + 8;				// mec, dsn, psn + mel_data[8]

		// Convert the UECP data frame into a packet and queue it up
		m_uecp_packets.emplace(uecp_create_data_packet(frame));

		// Reset the segment accumulator back to zero
		m_ps_ready = 0x00;
	}
}

//---------------------------------------------------------------------------
// rdsdecoder::decode_programidentification
//
// Decodes Program Identification (PI)
//
// Arguments:
//
//	rdsgroup	- RDS group to be processed

void rdsdecoder::decode_programidentification(tRDS_GROUPS const& rdsgroup)
{
	uint16_t const pi = rdsgroup.BlockA;

	// Indicate a change to the Program Identification flags
	if(pi != m_pi) {

		// UECP_MEC_PI
		//
		struct uecp_data_frame frame = {};
		struct uecp_message* message = &frame.msg;

		message->mec = UECP_MEC_PI;
		message->dsn = UECP_MSG_DSN_CURRENT_SET;
		message->psn = UECP_MSG_PSN_MAIN;

		// Kodi expects a single word for PI at the address of mel_len
		*reinterpret_cast<uint8_t*>(&message->mel_len) = pi & 0xFF;
		*reinterpret_cast<uint8_t*>(&message->mel_data[0]) = (pi >> 8) & 0xFF;

		frame.seq = UECP_DF_SEQ_DISABLED;
		frame.msg_len = 3 + 2;				// mec, dsn, psn + mel_data[2]

		// Convert the UECP data frame into a packet and queue it up
		m_uecp_packets.emplace(uecp_create_data_packet(frame));

		// Save the current PI flags
		m_pi = pi;
	}
}

//---------------------------------------------------------------------------
// rdsdecoder::decode_programtype
//
// Decodes Program Type (PTY)
//
// Arguments:
//
//	rdsgroup	- RDS group to be processed

void rdsdecoder::decode_programtype(tRDS_GROUPS const& rdsgroup)
{
	uint8_t const pty = (rdsgroup.BlockB >> 5) & 0x1F;

	// Indicate a change to the Program Type flags
	if(pty != m_pty) {

		// UECP_MEC_PTY
		//
		struct uecp_data_frame frame = {};
		struct uecp_message* message = &frame.msg;

		message->mec = UECP_MEC_PTY;
		message->dsn = UECP_MSG_DSN_CURRENT_SET;
		message->psn = UECP_MSG_PSN_MAIN;

		// Kodi expects a single byte for PTY at the address of mel_len
		*reinterpret_cast<uint8_t*>(&message->mel_len) = pty;

		frame.seq = UECP_DF_SEQ_DISABLED;
		frame.msg_len = 3 + 1;				// mec, dsn, psn + mel_data[1]

		// Convert the UECP data frame into a packet and queue it up
		m_uecp_packets.emplace(uecp_create_data_packet(frame));

		// Save the current PTY flags
		m_pty = pty;
	}
}

//---------------------------------------------------------------------------
// rdsdecoder::decode_radiotext
//
// Decodes Group Type 2A and 2B - RadioText
//
// Arguments:
//
//	rdsgroup	- RDS group to be processed

void rdsdecoder::decode_radiotext(tRDS_GROUPS const& rdsgroup)
{
	bool hascr = false;				// Flag if carriage return detected

	// Get the text segment address and A/B indicators from the data
	uint8_t textsegmentaddress = rdsgroup.BlockB & 0x000F;
	uint8_t const ab = (rdsgroup.BlockB >> 4) & 0x01;

	// Set the initial A/B flag the first time it's been seen
	if(!m_rt_init) { m_rt_ab = ab; m_rt_init = true; }

	// Clear any existing radio text when the A/B flag changes
	if(ab != m_rt_ab) {

		m_rt_ab = ab;				// Toggle A/B
		m_rt_data.fill(0x00);		// Clear existing RT data
		m_rt_ready = 0x0000;		// Clear existing segment flags
	}

	// Determine if this is Group A or Group B data
	bool const groupa = ((rdsgroup.BlockB & 0x0800) == 0x0000);

	// Group A contains two RadioText segments in block C and D
	if(groupa) {

		size_t offset = (textsegmentaddress << 2);
		m_rt_data[offset + 0] = (rdsgroup.BlockC >> 8) & 0xFF;
		m_rt_data[offset + 1] = rdsgroup.BlockC & 0xFF;
		m_rt_data[offset + 2] = (rdsgroup.BlockD >> 8) & 0xFF;
		m_rt_data[offset + 3] = rdsgroup.BlockD & 0xFF;

		// Check if a carriage return has been sent in this text segment
		hascr = ((m_rt_data[offset + 0] == 0x0D) || (m_rt_data[offset + 1] == 0x0D) ||
			(m_rt_data[offset + 2] == 0x0D) || (m_rt_data[offset + 3] == 0x0D));
	}

	// Group B contains one RadioText segment in block D
	else {

		size_t offset = (textsegmentaddress << 1);
		m_rt_data[offset + 0] = (rdsgroup.BlockD >> 8) & 0xFF;
		m_rt_data[offset + 1] = rdsgroup.BlockD & 0xFF;

		// Check if a carriage return has been sent in this text segment
		hascr = ((m_rt_data[offset + 0] == 0x0D) || (m_rt_data[offset + 1] == 0x0D));
	}

	// Indicate that this segment has been received, and if a CR was detected flag
	// all remaining text segments as received (they're not going to come anyway)
	m_rt_ready |= (0x01 << textsegmentaddress);
	while((hascr) && (++textsegmentaddress < 16)) {

		// Clear any RT information that may have been previously set
		size_t offset = (textsegmentaddress << 2);
		m_rt_data[offset + 0] = 0x00;
		m_rt_data[offset + 1] = 0x00;
		if(groupa) m_rt_data[offset + 2] = 0x00;
		if(groupa) m_rt_data[offset + 3] = 0x00;

		// Flag this segment as ready in the bitmask
		m_rt_ready |= (0x01 << textsegmentaddress);
	}

	// The RT information is ready to be sent if all 16 segments have been retrieved
	// for a Group A signal, or the first 8 segments of a Group B signal
	if((groupa) ? m_rt_ready == 0xFFFF : ((m_rt_ready & 0x00FF) == 0x00FF)) {

		// UECP_MEC_RT
		//
		struct uecp_data_frame frame = {};
		struct uecp_message* message = &frame.msg;

		message->mec = UECP_MEC_RT;
		message->dsn = UECP_MSG_DSN_CURRENT_SET;
		message->psn = UECP_MSG_PSN_MAIN;

		message->mel_len = 0x01;
		message->mel_data[0] = m_rt_ab;

		for(size_t index = 0; index < m_rt_data.size(); index++) {

			if(m_rt_data[index] == 0x00) break;
			message->mel_data[message->mel_len++] = m_rt_data[index];
		}

		frame.seq = UECP_DF_SEQ_DISABLED;
		frame.msg_len = 4 + message->mel_len;

		// Convert the UECP data frame into a packet and queue it up
		m_uecp_packets.emplace(uecp_create_data_packet(frame));

		// Reset the segment accumulator back to zero
		m_rt_ready = 0x0000;
	}
}

//---------------------------------------------------------------------------
// rdsdecoder::decode_rbds_programidentification
//
// Decodes RBDS Program Identification (PI)
//
// Arguments:
//
//	rdsgroup	- RDS group to be processed

void rdsdecoder::decode_rbds_programidentification(tRDS_GROUPS const& rdsgroup)
{
	uint16_t pi = rdsgroup.BlockA;

	// TODO: This is a rudimentary implementation that does not take into account
	// Canada, Mexico, and a whole host of special cases .. US only for now

	// Indicate a change to the Program Identification flags
	if(pi != m_rbds_pi) {

		m_rbds_callsign.fill(0x00);

		// SPECIAL CASE: AFxx -> xx00
		//
		if((pi & 0xAF00) == 0xAF00) pi <<= 8;

		// SPECIAL CASE: Axxx -> x0xx
		//
		else if((pi & 0xA000) == 0xA000) pi = (((pi & 0xF00) << 4) | (pi & 0xFF));

		// USA 3-LETTER-ONLY (ref: NRSC-4-B 04.2011 Table D.7)
		//
		if((pi >= 0x9950) && (pi <= 0x9EFF)) {

			// The 3-letter only callsigns are static and represented in a lookup table
			for(auto const iterator : CALL3TABLE) {

				if(iterator.pi == pi) {

					m_rbds_callsign[0] = iterator.csign[0];
					m_rbds_callsign[1] = iterator.csign[1];
					m_rbds_callsign[2] = iterator.csign[2];
					break;
				}
			}
		}

		// USA EAST (Wxxx)
		//
		else if((pi >= 21672) && (pi <= 39247)) {

			uint16_t char1 = (pi - 21672) / 676;
			uint16_t char2 = ((pi - 21672) - (char1 * 676)) / 26;
			uint16_t char3 = ((pi - 21672) - (char1 * 676) - (char2 * 26));

			m_rbds_callsign[0] = 'W';
			m_rbds_callsign[1] = static_cast<char>(static_cast<uint16_t>('A') + char1);
			m_rbds_callsign[2] = static_cast<char>(static_cast<uint16_t>('A') + char2);
			m_rbds_callsign[3] = static_cast<char>(static_cast<uint16_t>('A') + char3);
		}

		// USA WEST (Kxxx)
		//
		else if((pi >= 4096) && (pi <= 21671)) {

			uint16_t char1 = (pi - 4096) / 676;
			uint16_t char2 = ((pi - 4096) - (char1 * 676)) / 26;
			uint16_t char3 = ((pi - 4096) - (char1 * 676) - (char2 * 26));

			m_rbds_callsign[0] = 'K';
			m_rbds_callsign[1] = static_cast<char>(static_cast<uint16_t>('A') + char1);
			m_rbds_callsign[2] = static_cast<char>(static_cast<uint16_t>('A') + char2);
			m_rbds_callsign[3] = static_cast<char>(static_cast<uint16_t>('A') + char3);
		}

		// Generate a couple fake UECP packets anytime the PI changes to allow Kodi
		// to get the internals right for North American broadcasts with RBDS
		struct uecp_data_frame frame = {};
		struct uecp_message* message = &frame.msg;

		// UECP_MEC_PI
		//
		message->mec = UECP_MEC_PI;
		message->dsn = UECP_MSG_DSN_CURRENT_SET;
		message->psn = UECP_MSG_PSN_MAIN;

		// Kodi expects a single word for PI at the address of mel_len
		*reinterpret_cast<uint8_t*>(&message->mel_len) = 0x10;
		*reinterpret_cast<uint8_t*>(&message->mel_data[0]) = 0x00;

		frame.seq = UECP_DF_SEQ_DISABLED;
		frame.msg_len = 3 + 2;				// mec, dsn, psn + mel_data[2]

		// Convert the UECP data frame into a packet and queue it up
		m_uecp_packets.emplace(uecp_create_data_packet(frame));

		// UECP_EPP_TM_INFO
		//
		message->mec = UECP_EPP_TM_INFO;
		message->dsn = UECP_MSG_DSN_CURRENT_SET;
		message->psn = 0xA0;				// "US"

		frame.seq = UECP_DF_SEQ_DISABLED;
		frame.msg_len = 3;					// mec, dsn, psn

		// Convert the UECP data frame into a packet and queue it up
		m_uecp_packets.emplace(uecp_create_data_packet(frame));

		// Save the current PI flags
		m_rbds_pi = pi;
	}
}

//---------------------------------------------------------------------------
// rdsdecoder::decode_rdsgroup
//
// Decodes the next RDS group
//
// Arguments:
//
//	rdsgroup		- Next RDS group to be processed

void rdsdecoder::decode_rdsgroup(tRDS_GROUPS const& rdsgroup)
{
	// Ignore spurious RDS packets that contain no data
	// todo: figure out how this happens; could be a bug in the FM DSP
	if((rdsgroup.BlockA == 0x0000) && (rdsgroup.BlockB == 0x0000) && (rdsgroup.BlockC == 0x0000) && (rdsgroup.BlockD == 0x0000)) return;

	// Determine the group type code
	uint8_t grouptypecode = (rdsgroup.BlockB >> 12) & 0x0F;

	// Program Identification
	//
	if(m_isrbds) decode_rbds_programidentification(rdsgroup);
	else decode_programidentification(rdsgroup);

	// Program Type
	//
	decode_programtype(rdsgroup);

	// Traffic Program / Traffic Announcement
	//
	decode_trafficprogram(rdsgroup);

	// Invoke the proper handler for the specified group type code
	switch(grouptypecode) {

		// Group Type 0: Basic Tuning and switching information
		//
		case 0:
			decode_basictuning(rdsgroup);
			break;

		// Group Type 1: Slow Labelling Codes
		//
		case 1:
			decode_slowlabellingcodes(rdsgroup);
			break;

		// Group Type 2: RadioText
		//
		case 2:
			decode_radiotext(rdsgroup);
			break;
	}
}

//---------------------------------------------------------------------------
// rdsdecoder::decode_slowlabellingcodes
//
// Decodes Group 1A - Slow Labelling Codes
//
// Arguments:
//
//	rdsgroup	- RDS group to be processed

void rdsdecoder::decode_slowlabellingcodes(tRDS_GROUPS const& rdsgroup)
{
	// Determine if this is Group A or Group B data
	bool const groupa = ((rdsgroup.BlockB & 0x0800) == 0x0000);

	if(groupa) {

		// UECP_MEC_SLOW_LABEL_CODES
		//
		struct uecp_data_frame frame = {};
		struct uecp_message* message = &frame.msg;

		message->mec = UECP_MEC_SLOW_LABEL_CODES;
		message->dsn = UECP_MSG_DSN_CURRENT_SET;
		//message->psn = UECP_MSG_PSN_MAIN;

		// For whatever reason, Kodi expects the high byte of the data to 
		// be in the message PSN field -- could be a bug in Kodi, but whatever
		message->psn = ((rdsgroup.BlockC >> 8) & 0xFF);
		*reinterpret_cast<uint8_t*>(&message->mel_len) = (rdsgroup.BlockC & 0xFF);

		frame.seq = UECP_DF_SEQ_DISABLED;
		frame.msg_len = 3 + 1;				// mec, dsn, psn + mel_data[1]

		// Convert the UECP data frame into a packet and queue it up
		m_uecp_packets.emplace(uecp_create_data_packet(frame));
	}
}

//---------------------------------------------------------------------------
// rdsdecoder::decode_trafficprogram
//
// Decodes Traffic Program / Traffic Announcement (TP/TA)
//
// Arguments:
//
//	rdsgroup	- RDS group to be processed

void rdsdecoder::decode_trafficprogram(tRDS_GROUPS const& rdsgroup)
{
	uint8_t const ta_tp = (((rdsgroup.BlockB & 0x0010) >> 4) || ((rdsgroup.BlockB & 0x0400) >> 9));

	// Indicate a change to the Traffic Announcement / Traffic Program flags
	if(ta_tp != m_ta_tp) {

		// UECP_MEC_TA_TP
		//
		struct uecp_data_frame frame = {};
		struct uecp_message* message = &frame.msg;

		message->mec = UECP_MEC_TA_TP;
		message->dsn = UECP_MSG_DSN_CURRENT_SET;
		message->psn = UECP_MSG_PSN_MAIN;

		// Kodi expects a single byte for TA/TP  at the address of mel_len
		*reinterpret_cast<uint8_t*>(&message->mel_len) = ta_tp;

		frame.seq = UECP_DF_SEQ_DISABLED;
		frame.msg_len = 3 + 1;				// mec, dsn, psn + mel_data[1]

		// Convert the UECP data frame into a packet and queue it up
		m_uecp_packets.emplace(uecp_create_data_packet(frame));

		// Save the current TA/TP flags
		m_ta_tp = ta_tp;
	}
}

//---------------------------------------------------------------------------
// rdsdecoder::get_rdbs_callsign
//
// Retrieves the RBDS call sign (if present)
//
// Arguments:
//
//	NONE

std::string rdsdecoder::get_rbds_callsign(void) const
{
	return std::string(m_rbds_callsign.begin(), m_rbds_callsign.end());
}

//---------------------------------------------------------------------------
// rdsdecoder::has_rbds_callsign
//
// Flag indicating that the RDBS call sign has been decoded
//
// Arguments:
//
//	NONE

bool rdsdecoder::has_rbds_callsign(void) const
{
	return m_rbds_callsign[0] != '\0';
}

//---------------------------------------------------------------------------
// rdsdecoder::pop_uecp_data_packet
//
// Pops the topmost UECP data packet out of the packet queue
//
// Arguments:
//
//	packet		- On success, contains the UECP packet data

bool rdsdecoder::pop_uecp_data_packet(uecp_data_packet& packet)
{
	if(m_uecp_packets.empty()) return false;

	// Swap the front packet from the top of the queue and pop it off
	m_uecp_packets.front().swap(packet);
	m_uecp_packets.pop();

	return true;
}

//---------------------------------------------------------------------------

#pragma warning(pop)
