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

#ifndef __SIGNALMETER_H_
#define __SIGNALMETER_H_
#pragma once

#include <atomic>
#include <memory>
#include <thread>

#include "props.h"
#include "rtldevice.h"
#include "scalar_condition.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// Class signalmeter
//
// Implements the signal meter
//
// Portions based on:
//
// rtl-sdr (rtl_power.c)
// https://git.osmocom.org/rtl-sdr/
// Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de>
// Copyright (C) 2012 by Hoernchen <la@tfc - server.de>
// Copyright (C) 2012 by Kyle Keen <keenerd@gmail.com>
// GPLv2

class signalmeter
{
public:

	// Destructor
	//
	~signalmeter();

	//-----------------------------------------------------------------------
	// Member Functions

	// create (static)
	//
	// Factory method, creates a new signalmeter instance
	static std::unique_ptr<signalmeter> create(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops);

	// get_automatic_gain
	//
	// Gets the currently set automatic gain value
	bool get_automatic_gain(void) const;

	// get_frequency
	//
	// Gets the currently set frequency
	uint32_t get_frequency(void) const;

	// get_manual_gain
	//
	// Gets the currently set manual gain value
	int get_manual_gain(void) const;

	// get_valid_manual_gains
	//
	// Gets the valid tuner manual gain values for the device
	void get_valid_manual_gains(std::vector<int>& dbs) const;

	// set_automatic_gain
	//
	// Sets the automatic gain flag
	void set_automatic_gain(bool autogain);

	// set_frequency
	//
	// Sets the frequency to be tuned
	void set_frequency(uint32_t frequency);

	// set_manual_gain
	//
	// Sets the manual gain value
	void set_manual_gain(int manualgain);

	// start
	//
	// Starts the signal meter
	void start(void);

	// stop
	//
	// Stops the signal meter
	void stop(void);

private:

	signalmeter(signalmeter const&) = delete;
	signalmeter& operator=(signalmeter const&) = delete;

	// Instance Constructor
	//
	signalmeter(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops);

	// DEFAULT_DEVICE_BLOCK_SIZE
	//
	// Default device block size
	static size_t const DEFAULT_DEVICE_BLOCK_SIZE;

	// DEFAULT_DEVICE_FREQUENCY
	//
	// Default device frequency
	static uint32_t const DEFAULT_DEVICE_FREQUENCY;

	// DEFAULT_DEVICE_SAMPLE_RATE
	//
	// Default device sample rate
	static uint32_t const DEFAULT_DEVICE_SAMPLE_RATE;

	//-----------------------------------------------------------------------
	// Private Member Functions

	//-----------------------------------------------------------------------
	// Member Variables

	std::unique_ptr<rtldevice>	m_device;				// RTL-SDR device instance
	bool						m_autogain = false;		// Automatic gain enabled/disabled
	int							m_manualgain = 0;		// Current manual gain value
	uint32_t					m_frequency = 0;		// Current frequency value

	// STREAM CONTROL
	//
	std::thread					m_worker;				// Data transfer thread
	scalar_condition<bool>		m_stop{ false };		// Condition to stop data transfer
	std::atomic<bool>			m_stopped{ false };		// Data transfer stopped flag
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __SIGNALMETER_H_