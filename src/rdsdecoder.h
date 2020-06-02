//-----------------------------------------------------------------------------
// Copyright (c) 2020 Michael G. Brehm
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
//-----------------------------------------------------------------------------

#ifndef __RDSDECODER_H_
#define __RDSDECODER_H_
#pragma once

#include <array>
#include <queue>

#include "fmdsp/demodulator.h"
#include "uecp.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// Class rdsdecoder
//
// Implements an RDS decoder to convert the data from the demodulator into
// UECP packets that will be understood by Kodi

class rdsdecoder
{
public:

	// Instance Constructor
	//
	rdsdecoder();

	// Destructor
	//
	~rdsdecoder();

	//-----------------------------------------------------------------------
	// Member Functions

	// decode_rdsgroup
	//
	// Decodes the next RDS group
	void decode_rdsgroup(tRDS_GROUPS const& rdsgroup);

	// pop_uecp_data_packet
	//
	// Pops the topmost UECP data packet from the queue
	bool pop_uecp_data_packet(uecp_data_packet& frame);

private:

	rdsdecoder(rdsdecoder const&) = delete;
	rdsdecoder& operator=(rdsdecoder const&) = delete;

	//-----------------------------------------------------------------------
	// Private Type Declarations

	// uecp_packet_queue
	//
	// queue<> of formed UECP packets to be sent via the demultiplexer
	using uecp_packet_queue = std::queue<uecp_data_packet>;

	//-----------------------------------------------------------------------
	// Private Member Functions

	// decode_basictuning
	//
	// Decodes Group Type 0A and 0B - Basic Tuning and switching information
	void decode_basictuning(tRDS_GROUPS const& rdsgroup);

	// decode_radiotext
	//
	// Decodes Group Type 2A and 2B - RadioText
	void decode_radiotext(tRDS_GROUPS const& rdsgroup);

	//-----------------------------------------------------------------------
	// Member Variables

	// UECP
	//
	uecp_packet_queue			m_uecp_packets;		// Queued UECP packets

	// GROUP 0 - BASIC TUNING AND SWITCHING INFORMATION
	//
	uint8_t						m_ps_ready = 0x00;		// PS name ready indicator
	std::array<char, 8>			m_ps_data;				// Program Service name

	// GROUP 2 - RADIOTEXT
	//
	bool						m_rt_init = false;		// RadioText init flag
	uint16_t					m_rt_ready = 0x0000;	// RadioText ready indicator
	uint8_t						m_rt_ab = 0x00;			// RadioText A/B flag
	std::array<uint8_t, 64>		m_rt_data;				// RadioText data
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __RDSDECODER_H_
