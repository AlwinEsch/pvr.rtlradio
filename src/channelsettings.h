//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------

#ifndef __CHANNELSETTINGS_H_
#define __CHANNELSETTINGS_H_
#pragma once

#include <kodi/gui/controls/Button.h>
#include <kodi/gui/controls/Edit.h>
#include <kodi/gui/controls/Image.h>
#include <kodi/gui/controls/RadioButton.h>
#include <kodi/gui/controls/Rendering.h>
#include <kodi/gui/controls/SettingsSlider.h>
#include <kodi/gui/Window.h>
#include <memory>
#include <vector>
#include <string>
#include <utility>

#include "fmmeter.h"
#include "props.h"
#include "rtldevice.h"

#pragma warning(push, 4)

using namespace kodi::gui::controls;

//---------------------------------------------------------------------------
// Class channelsettings
//
// Implements the "Channel Settings" dialog

class ATTRIBUTE_HIDDEN channelsettings : public kodi::gui::CWindow
{
public:

	// Destructor
	//
	virtual ~channelsettings();
	
	//-----------------------------------------------------------------------
	// Member Functions

	// create (static)
	//
	// Factory method, creates a new channelsettings instance
	static std::unique_ptr<channelsettings> create(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops, 
		struct channelprops const& channelprops);

	// get_channel_properties
	//
	// Gets the updated channel properties from the dialog box
	void get_channel_properties(struct channelprops& channelprops) const;

	// get_dialog_result
	//
	// Gets the result code from the dialog box
	bool get_dialog_result(void) const;

private:

	channelsettings(channelsettings const&) = delete;
	channelsettings& operator=(channelsettings const&) = delete;

	// Instance Constructor
	//
	channelsettings(std::unique_ptr<rtldevice> device, struct tunerprops const& tunerprops, struct channelprops const& channelprops);

	// FMRADIO_BANDWIDTH
	//
	// Bandwidth of an analog FM radio channel
	static uint32_t const FMRADIO_BANDWIDTH;

	// HDRADIO_BANDWIDTH
	//
	// Bandwidth of a Hybrid Digital (HD) FM radio channel
	static uint32_t const HDRADIO_BANDWIDTH;

	// WXRADIO_BANDWIDTH
	//
	// Bandwidth of a VHF weather radio channel
	static uint32_t const WXRADIO_BANDWIDTH;

	//-------------------------------------------------------------------------
	// CWindow Implementation

	// OnAction
	//
	// Receives action codes that are sent to this window
	bool OnAction(ADDON_ACTION actionId) override;

	// OnClick
	//
	// Receives click event notifications for a control
	bool OnClick(int controlId) override;

	// OnInit
	//
	// Called to initialize the window object
	bool OnInit(void) override;

	//-------------------------------------------------------------------------
	// Private Member Functions

	// fm_meter_exception
	//
	// Callback to handle an exception raised by the signal meter
	void fm_meter_exception(std::exception const& ex);

	// fm_meter_status
	//
	// Updates the state of the signal meter
	void fm_meter_status(struct fmmeter::signal_status const& status);

	// gain_to_percent
	//
	// Converts a manual gain value into a percentage
	int gain_to_percent(int gain) const;

	// nearest_valid_gain
	//
	// Gets the closest valid value for a manual gain setting
	int nearest_valid_gain(int gain) const;

	// percent_to_gain
	//
	// Converts a percentage into a manual gain value
	int percent_to_gain(int percent) const;

	// update_gain
	//
	// Updates the state of the gain control
	void update_gain(void);

	//-------------------------------------------------------------------------
	// Member Variables

	std::unique_ptr<rtldevice> const	m_device;				// Device instance
	struct channelprops					m_channelprops;			// Channel properties
	std::unique_ptr<fmmeter>			m_signalmeter;			// Signal meter instance
	std::vector<int>					m_manualgains;			// Manual gain values
	bool								m_result = false;		// Dialog result

	// CONTROLS
	//
	std::unique_ptr<CEdit>				m_edit_frequency;		// Frequency
	std::unique_ptr<CEdit>				m_edit_channelname;		// Channel name
	std::unique_ptr<CButton>			m_button_channelicon;	// Channel icon
	std::unique_ptr<CImage>				m_image_channelicon;	// Channel icon
	std::unique_ptr<CRadioButton>		m_radio_autogain;		// Automatic gain
	std::unique_ptr<CSettingsSlider>	m_slider_manualgain;	// Manual gain
	std::unique_ptr<CRendering>			m_render_signalmeter;	// Signal meter
	std::unique_ptr<CEdit>				m_edit_signalgain;		// Active gain
	std::unique_ptr<CEdit>				m_edit_signalpower;		// Active power
	std::unique_ptr<CEdit>				m_edit_signalsnr;		// Active SNR
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __CHANNELSETTINGS_H_
