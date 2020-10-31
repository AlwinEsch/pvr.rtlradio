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

#ifndef __FMSTREAM_H_
#define __FMSTREAM_H_
#pragma once

#pragma warning(push, 4)

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "fmdsp/demodulator.h"
#include "fmdsp/fractresampler.h"

#include "props.h"
#include "pvrstream.h"
#include "rdsdecoder.h"
#include "rtldevice.h"
#include "scalar_condition.h"

//---------------------------------------------------------------------------
// Class fmstream
//
// Implements a FM radio stream

class fmstream : public pvrstream
{
public:

	// Destructor
	//
	virtual ~fmstream();

	//-----------------------------------------------------------------------
	// Member Functions

	// canseek
	//
	// Flag indicating if the stream allows seek operations
	bool canseek(void) const override;

	// close
	//
	// Closes the stream
	void close(void) override;

	// create (static)
	//
	// Factory method, creates a new fmstream instance
	static std::unique_ptr<fmstream> create(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops, 
		struct channelprops const& channelprops, struct fmprops const& fmprops);

	// demuxabort
	//
	// Aborts the demultiplexer
	void demuxabort(void) override;

	// demuxflush
	//
	// Flushes the demultiplexer
	void demuxflush(void) override;

	// demuxread
	//
	// Reads the next packet from the demultiplexer
	DemuxPacket* demuxread(std::function<DemuxPacket*(int)> const& allocator) override;

	// demuxreset
	//
	// Resets the demultiplexer
	void demuxreset(void) override;

	// devicename
	//
	// Gets the device name associated with the stream
	std::string devicename(void) const override;

	// enumproperties
	//
	// Enumerates the stream properties
	void enumproperties(std::function<void(struct streamprops const& props)> const& callback) override;

	// length
	//
	// Gets the length of the stream
	long long length(void) const override;

	// muxname
	//
	// Gets the mux name associated with the stream
	std::string muxname(void) const override;

	// position
	//
	// Gets the current position of the stream
	long long position(void) const override;

	// read
	//
	// Reads available data from the stream
	size_t read(uint8_t* buffer, size_t count) override;

	// realtime
	//
	// Gets a flag indicating if the stream is real-time
	bool realtime(void) const override;

	// seek
	//
	// Sets the stream pointer to a specific position
	long long seek(long long position, int whence) override;

	// servicename
	//
	// Gets the service name associated with the stream
	std::string servicename(void) const override;

	// signalstrength
	//
	// Gets the signal strength as a percentage
	int signalstrength(void) const override;

	// signaltonoise
	//
	// Gets the signal to noise ratio as a percentage
	int signaltonoise(void) const override;

private:

	fmstream(fmstream const&) = delete;
	fmstream& operator=(fmstream const&) = delete;

	// DEFAULT_DEVICE_BLOCK_SIZE
	//
	// Default device block size
	static size_t const DEFAULT_DEVICE_BLOCK_SIZE;

	// DEFAULT_DEVICE_SAMPLE_RATE
	//
	// Default device sample rate
	static uint32_t const DEFAULT_DEVICE_SAMPLE_RATE;

	// DEFAULT_RINGBUFFER_SIZE
	//
	// Default ring buffer size
	static size_t const DEFAULT_RINGBUFFER_SIZE;

	// STREAM_ID_AUDIO
	//
	// Stream identifier for the audio output stream
	static int const STREAM_ID_AUDIO;

	// STREAM_ID_UECP
	//
	// Stream identifier for the UECP output stream
	static int const STREAM_ID_UECP;

	// Instance Constructor
	//
	fmstream(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops, 
		struct channelprops const& channelprops, struct fmprops const& fmprops);

	//-----------------------------------------------------------------------
	// Private Member Functions

	// transfer
	//
	// Worker thread procedure used to transfer data into the ring buffer
	void transfer(scalar_condition<bool>& started);

	//-----------------------------------------------------------------------
	// Member Variables

	std::unique_ptr<rtldevice>			m_device;				// RTL-SDR device instance
	std::unique_ptr<CDemodulator>		m_demodulator;			// CuteSDR demodulator instance
	std::unique_ptr<CFractResampler>	m_resampler;			// CuteSDR resampler instance
	bool const							m_decoderds;			// Flag to send decoded RDS data
	rdsdecoder							m_rdsdecoder;			// RDS decoder instance

	uint32_t const						m_samplerate;			// Device sample rate
	uint32_t const						m_pcmsamplerate;		// Output sample rate
	TYPEREAL const						m_pcmgain;				// Output gain

	// STREAM CONTROL
	//
	mutable std::mutex					m_lock;					// Synchronization object
	std::condition_variable				m_cv;					// Transfer event condvar
	std::thread							m_worker;				// Data transfer thread
	scalar_condition<bool>				m_stop{ false };		// Condition to stop data transfer
	std::atomic<bool>					m_stopped{ false };		// Data transfer stopped flag

	// RING BUFFER
	//
	size_t const						m_buffersize;			// Size of the ring buffer
	std::unique_ptr<uint8_t[]>			m_buffer;				// Ring buffer stroage
	std::atomic<size_t>					m_bufferhead{ 0 };		// Head (write) buffer position
	std::atomic<size_t>					m_buffertail{ 0 };		// Tail (read) buffer position
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __FMSTREAM_H_
