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

#include <memory>
#include <mutex>
#include <string>

#define USE_DEMUX
#include <kodi/xbmc_addon_dll.h>
#include <kodi/xbmc_pvr_dll.h>
#include <version.h>

#include <kodi/libXBMC_addon.h>
#include <kodi/libKODI_guilib.h>
#include <kodi/libXBMC_pvr.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/prettywriter.h>

#include "database.h"
#include "dbtypes.h"
#include "fmstream.h"
#include "pvrstream.h"
#include "rtldevice.h"
#include "string_exception.h"
#include "sqlite_exception.h"
#include "usbdevice.h"
#include "tcpdevice.h"
#include "wxstream.h"

#pragma warning(push, 4)

//---------------------------------------------------------------------------
// MACROS
//---------------------------------------------------------------------------

// MENUHOOK_XXXXXX
//
// Menu hook identifiers
#define MENUHOOK_SETTING_IMPORTCHANNELS		10
#define MENUHOOK_SETTING_EXPORTCHANNELS		11
#define MENUHOOK_SETTING_CLEARCHANNELS		12

//---------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//---------------------------------------------------------------------------

// Device helpers
//
static std::unique_ptr<rtldevice> create_device(struct addon_settings const& settings);

// Exception helpers
//
static void handle_generalexception(char const* function);
template<typename _result> static _result handle_generalexception(char const* function, _result result);
static void handle_stdexception(char const* function, std::exception const& ex);
template<typename _result> static _result handle_stdexception(char const* function, std::exception const& ex, _result result);

// Log helpers
//
template<typename... _args> static void log_debug(_args&&... args);
template<typename... _args> static void log_error(_args&&... args);
template<typename... _args>	static void log_message(ADDON::addon_log_t level, _args&&... args);
template<typename... _args> static void log_notice(_args&&... args);

//---------------------------------------------------------------------------
// TYPE DECLARATIONS
//---------------------------------------------------------------------------

// device_connection
//
// Defines the RTL-SDR device connection type
enum device_connection {

	usb			= 0,		// Locally connected USB device
	rtltcp		= 1,		// Device connected via rtl_tcp
};

// downsample_quality
//
// Defines the FM DSP downsample quality factor
enum downsample_quality {

	fast		= 0,		// Optimize for speed
	standard	= 1,		// Standard quality
	maximum		= 2,		// Optimize for quality
};

// rds_standard
//
// Defines the Radio Data System (RDS) standard
enum rds_standard {

	automatic	= 0,		// Automatically detect RDS standard
	rds			= 1,		// Global RDS standard
	rbds		= 2,		// North American RBDS standard
};

// addon_settings
//
// Defines all of the configurable addon settings
struct addon_settings {

	// device_connection
	//
	// The type of device (USB vs network, for example)
	enum device_connection device_connection;

	// device_connection_usb_index
	//
	// The index of a USB connected device
	int device_connection_usb_index;

	// device_connection_tcp_host
	//
	// The IP address of the rtl_tcp host to connect to
	std::string device_connection_tcp_host;

	// device_connection_tcp_port
	//
	// The port number of the rtl_tcp host to connect to
	int device_connection_tcp_port;

	// device_sample_rate
	//
	// Sample rate value for the device
	int device_sample_rate;

	// device_frequency_correction
	//
	// Frequency correction calibration value for the device
	int device_frequency_correction;

	// interface_prepend_channel_numbers
	//
	// Flag to include the channel number in the channel name
	bool interface_prepend_channel_numbers;

	// fmradio_enable_rds
	//
	// Enables passing decoded RDS information to Kodi
	bool fmradio_enable_rds;

	// fmradio_rds_standard
	//
	// Specifies the Radio Data System (RDS) standard
	enum rds_standard fmradio_rds_standard;

	// fmradio_downsample_quality
	//
	// Specifies the FM DSP downsample quality factor
	enum downsample_quality fmradio_downsample_quality;

	// fmradio_output_samplerate
	//
	// Specifies the output sample rate for the FM DSP
	int fmradio_output_samplerate;

	// fmradio_output_gain
	//
	// Specified the output gain for the FM DSP
	float fmradio_output_gain;

	// wxradio_output_samplerate
	//
	// Specifies the output sample rate for the WX DSP
	int wxradio_output_samplerate;

	// wxradio_output_gain
	//
	// Specified the output gain for the WX DSP
	float wxradio_output_gain;
};

//---------------------------------------------------------------------------
// GLOBAL VARIABLES
//---------------------------------------------------------------------------

// g_addon
//
// Kodi add-on callbacks
static std::unique_ptr<ADDON::CHelper_libXBMC_addon> g_addon;

// g_capabilities (const)
//
// PVR implementation capability flags
static const PVR_ADDON_CAPABILITIES g_capabilities = {

	true,			// bSupportsEPG
	false,			// bSupportsEPGEdl
	false,			// bSupportsTV
	true,			// bSupportsRadio
	false,			// bSupportsRecordings
	false,			// bSupportsRecordingsUndelete
	false,			// bSupportsTimers
	true,			// bSupportsChannelGroups
	false,			// bSupportsChannelScan
	false,			// bSupportsChannelSettings
	true,			// bHandlesInputStream
	true,			// bHandlesDemuxing
	false,			// bSupportsRecordingPlayCount
	false,			// bSupportsLastPlayedPosition
	false,			// bSupportsRecordingEdl
	false,			// bSupportsRecordingsRename
	false,			// bSupportsRecordingsLifetimeChange
	false,			// bSupportsDescrambleInfo
	0,				// iRecordingsLifetimesSize
	{ { 0, "" } },	// recordingsLifetimeValues
	false,			// bSupportsAsyncEPGTransfer
};

// g_connpool
//
// Global SQLite database connection pool instance
static std::shared_ptr<connectionpool> g_connpool;

// g_gui
//
// Kodi GUI library callbacks
static std::unique_ptr<CHelper_libKODI_guilib> g_gui;

// g_pvr
//
// Kodi PVR add-on callbacks
static std::unique_ptr<CHelper_libXBMC_pvr> g_pvr;

// g_pvrstream
//
// DVR stream buffer instance
static std::unique_ptr<pvrstream> g_pvrstream;

// g_pvrstream_lock
//
// Synchronization object
static std::mutex g_pvrstream_lock;

// g_settings
//
// Global addon settings instance
static addon_settings g_settings = {

	device_connection::usb,				// device_connection
	0,									// device_connection_usb_index
	"",									// device_connection_tcp_host
	1234,								// device_connection_tcp_port
	(1600 KHz),							// device_sample_rate
	0,									// device_frequency_correction
	false,								// interface_prepend_channel_numbers
	false,								// fmradio_enable_rds
	rds_standard::automatic,			// fmradio_rds_standard
	downsample_quality::standard,		// fmradio_downsample_quality
	(48 KHz),							// fmradio_output_samplerate
	-3.0f,								// fmradio_output_gain
	(48 KHz),							// wxradio_output_samplerate
	-3.0f,								// wxradio_output_gain
};

// g_settings_lock
//
// Synchronization object to serialize access to addon settings
static std::mutex g_settings_lock;

// g_userpath
//
// Set to the input PVR user path string
static std::string g_userpath;

//---------------------------------------------------------------------------
// HELPER FUNCTIONS
//---------------------------------------------------------------------------

// copy_settings (inline)
//
// Atomically creates a copy of the global addon_settings structure
inline struct addon_settings copy_settings(void)
{
	std::unique_lock<std::mutex> settings_lock(g_settings_lock);
	return g_settings;
}

// create_device (local)
//
// Creates the RTL-SDR device instance
static std::unique_ptr<rtldevice> create_device(struct addon_settings const& settings)
{
	// USB device
	if(settings.device_connection == device_connection::usb)
		return usbdevice::create(settings.device_connection_usb_index);

	// Network device
	else if(settings.device_connection == device_connection::rtltcp)
		return tcpdevice::create(settings.device_connection_tcp_host.c_str(), static_cast<uint16_t>(settings.device_connection_tcp_port));

	// Unknown device type
	else throw string_exception("invalid device_connection type specified");
}


// device_connection_to_string (local)
//
// Converts a device_connection enumeration value into a string
static std::string device_connection_to_string(enum device_connection connection)
{
	switch(connection) {

		case device_connection::usb: return "USB";
		case device_connection::rtltcp: return "Network (rtl_tcp)";
	}

	return "Unknown";
}

// downsample_quality_to_string (local)
//
// Converts a downsample_quality enumeration value into a string
static std::string downsample_quality_to_string(enum downsample_quality quality)
{
	switch(quality) {

		case downsample_quality::fast: return "Fast";
		case downsample_quality::standard: return "Standard";
		case downsample_quality::maximum: return "Maximum";
	}

	return "Unknown";
}

// get_regional_rds_standard (local)
//
// Figures out whether or not RDS or RBDS should be used in this region
enum rds_standard get_regional_rds_standard(enum rds_standard standard)
{
	assert(g_addon);

	// If the standard isn't set to automatic, just regurgitate it
	if(standard != rds_standard::automatic) return standard;

	// To figure this out on Kodi Leia, I used localized strings that will
	// indicate "rbds" for North American specific locales.  This is imperfect
	// as it doesn't appear that en_ca and es_us are able to be set, but it's
	// seemingly as close as I'm going to get for now ...

	char* rdsregion = g_addon->GetLocalizedString(30600);
	return (strcmp(rdsregion, "rbds") == 0) ? rds_standard::rbds : rds_standard::rds;
}

// handle_generalexception (local)
//
// Handler for thrown generic exceptions
static void handle_generalexception(char const* function)
{
	log_error(function, " failed due to an exception");
}

// handle_generalexception (local)
//
// Handler for thrown generic exceptions
template<typename _result>
static _result handle_generalexception(char const* function, _result result)
{
	handle_generalexception(function);
	return result;
}

// handle_stdexception (local)
//
// Handler for thrown std::exceptions
static void handle_stdexception(char const* function, std::exception const& ex)
{
	log_error(function, " failed due to an exception: ", ex.what());
}

// handle_stdexception (local)
//
// Handler for thrown std::exceptions
template<typename _result>
static _result handle_stdexception(char const* function, std::exception const& ex, _result result)
{
	handle_stdexception(function, ex);
	return result;
}

// log_debug (local)
//
// Variadic method of writing a LOG_DEBUG entry into the Kodi application log
template<typename... _args>
static void log_debug(_args&&... args)
{
	log_message(ADDON::addon_log_t::LOG_DEBUG, std::forward<_args>(args)...);
}

// log_error (local)
//
// Variadic method of writing a LOG_ERROR entry into the Kodi application log
template<typename... _args>
static void log_error(_args&&... args)
{
	log_message(ADDON::addon_log_t::LOG_ERROR, std::forward<_args>(args)...);
}

// log_message (local)
//
// Variadic method of writing an entry into the Kodi application log
template<typename... _args>
static void log_message(ADDON::addon_log_t level, _args&&... args)
{
	std::ostringstream stream;
	int unpack[] = { 0, (static_cast<void>(stream << std::boolalpha << args), 0) ... };
	(void)unpack;

	if(g_addon) g_addon->Log(level, stream.str().c_str());

	// Write LOG_ERROR level messages to an appropriate secondary log mechanism
	if(level == ADDON::addon_log_t::LOG_ERROR) {

#if defined(_WINDOWS)
		std::string message = "ERROR: " + stream.str() + "\r\n";
		OutputDebugStringA(message.c_str());
#else
		fprintf(stderr, "ERROR: %s\r\n", stream.str().c_str());
#endif
	}
}

// log_notice (local)
//
// Variadic method of writing a LOG_NOTICE entry into the Kodi application log
template<typename... _args>
static void log_notice(_args&&... args)
{
	log_message(ADDON::addon_log_t::LOG_NOTICE, std::forward<_args>(args)...);
}

// menuhook_clearchannels (local)
//
// Menu hook to delete all channels from the database
static void menuhook_clearchannels(void)
{
	log_notice(__func__, ": clearing channel data");

	try {

		// Clear the channel data from the database and inform the user if successful
		clear_channels(connectionpool::handle(g_connpool));
		g_gui->Dialog_OK_ShowAndGetInput(g_addon->GetLocalizedString(30402), "Channel data successfully cleared");

		// Trigger both a channel and a channel group update in Kodi
		g_pvr->TriggerChannelUpdate();
		g_pvr->TriggerChannelGroupsUpdate();
	}

	catch(std::exception& ex) {

		// Inform the user that the operation failed, and re-throw the exception with this function name
		g_gui->Dialog_OK_ShowAndGetInput(g_addon->GetLocalizedString(30402), "An error occurred clearing the channel data:", "", ex.what());
		throw string_exception(__func__, ": ", ex.what());
	}
}

// menuhook_exportchannels (local)
//
// Menu hook to export the channel information from the database
static void menuhook_exportchannels(void)
{
	char path[1024] = {};			// Export folder path

	// The legacy Kodi GUI API doesn't provide a way to select both a folder and a file name, and doesn't support
	// the source filters (like "local|network|removable"), this will have to be done with a fixed name file
	// put into the Kodi home directory; this can probably be adjusted in Kodi Matrix with the new API
	if(g_gui->Dialog_FileBrowser_ShowAndGetFile(g_addon->TranslateSpecialProtocol("special://home"), "/w",
		g_addon->GetLocalizedString(30403), path[0], 1024)) {

		try {

			rapidjson::Document		document;				// Resultant JSON document

			// Generate the output file name based on the selected path
			std::string filepath(path);
			filepath.append("radiochannels.json");
			log_notice(__func__, ": exporting channel data to file ", filepath.c_str());

			// Export the channels from the database into a JSON string
			std::string json = export_channels(connectionpool::handle(g_connpool));

			// Parse the JSON data so that it can be pretty printed for the user
			document.Parse(json.c_str());
			if(document.HasParseError()) throw string_exception("JSON parse error during export - ",
				rapidjson::GetParseError_En(document.GetParseError()));

			// Pretty print the JSON data
			rapidjson::StringBuffer sb;
			rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
			document.Accept(writer);

			// Attempt to create the output file in the selected directory
			void* handle = g_addon->OpenFileForWrite(filepath.c_str(), true);
			if(handle == nullptr) throw string_exception("unable to open file ", filepath.c_str(), " for write access");

			ssize_t written = g_addon->WriteFile(handle, sb.GetString(), sb.GetSize());
			g_addon->CloseFile(handle);

			// If the file wasn't written properly, throw an exception
			if(written != static_cast<ssize_t>(sb.GetSize()))
				throw string_exception("short write occurred generating file ", filepath.c_str());

			// Inform the user that the operation was successful
			g_gui->Dialog_OK_ShowAndGetInput(g_addon->GetLocalizedString(30401), "Channels successfully exported to:",
				"", filepath.c_str());
		}

		catch(std::exception& ex) {

			// Inform the user that the operation failed, and re-throw the exception with this function name
			g_gui->Dialog_OK_ShowAndGetInput(g_addon->GetLocalizedString(30401), "An error occurred exporting the channel data:", "", ex.what());
			throw string_exception(__func__, ": ", ex.what());
		}
	}
}

// menuhook_importchannels (local)
//
// Menu hook to import channel information into the database
static void menuhook_importchannels(void)
{
	char path[1024] = {};			// Import file path

	// The legacy Kodi GUI API doesn't support the source filters (like "local|network|removable"), so start
	// in the Kodi home directory; this can be adjusted in Kodi Matrix with the new API
	if(g_gui->Dialog_FileBrowser_ShowAndGetFile(g_addon->TranslateSpecialProtocol("special://home"), "*.json",
		g_addon->GetLocalizedString(30404), path[0], 1024)) {

		try {

			std::string			json;			// The JSON data

			log_notice(__func__, ": importing channel data from file ", path);

			// Ensure the file exists before trying to open it
			if(!g_addon->FileExists(path, false)) throw string_exception("input file ", path, " does not exist");

			// Attempt to open the specified input file
			void* handle = g_addon->OpenFile(path, 0);
			if(handle == nullptr) throw string_exception("unable to open file ", path, " for read access");

			// Read in the input file in 1KiB chunks; it shouldn't be that big
			std::unique_ptr<char[]> buffer(new char[1 KiB]);
			ssize_t read = g_addon->ReadFile(handle, &buffer[0], 1 KiB);
			while(read > 0) {

				json.append(&buffer[0], read);
				read = g_addon->ReadFile(handle, &buffer[0], 1 KiB);
			}

			// Close the input file
			g_addon->CloseFile(handle);

			// Only try to import channels from the file if something was actually in there ...
			if(json.length()) import_channels(connectionpool::handle(g_connpool), json.c_str());

			// Inform the user that the operation was successful
			g_gui->Dialog_OK_ShowAndGetInput(g_addon->GetLocalizedString(30400), "Channels successfully imported from:", "", path);

			// Trigger both a channel and a channel group update in Kodi
			g_pvr->TriggerChannelUpdate();
			g_pvr->TriggerChannelGroupsUpdate();
		}

		catch(std::exception& ex) {

			// Inform the user that the operation failed, and re-throw the exception with this function name
			g_gui->Dialog_OK_ShowAndGetInput(g_addon->GetLocalizedString(30400), "An error occurred importing the channel data:", "", ex.what());
			throw string_exception(__func__, ": ", ex.what());
		}
	}
}

// rds_standard_to_string (local)
//
// Converts an rds_standard enumeration value into a string
static std::string rds_standard_to_string(enum rds_standard mode)
{
	switch(mode) {

		case rds_standard::automatic: return "Automatic";
		case rds_standard::rds: return "World (RDS)";
		case rds_standard::rbds: return "North America (RBDS)";
	}

	return "Unknown";
}

//---------------------------------------------------------------------------
// KODI ADDON ENTRY POINTS
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// ADDON_Create
//
// Creates and initializes the Kodi addon instance
//
// Arguments:
//
//	handle			- Kodi add-on handle
//	props			- Add-on specific properties structure (PVR_PROPERTIES)

ADDON_STATUS ADDON_Create(void* handle, void* props)
{
	PVR_MENUHOOK			menuhook;						// For registering menu hooks
	bool					bvalue = false;					// Setting value
	int						nvalue = 0;						// Setting value
	float					fvalue = 0.0f;					// Setting value
	char					strvalue[1024] = { '\0' };		// Setting value 

	if((handle == nullptr) || (props == nullptr)) return ADDON_STATUS::ADDON_STATUS_PERMANENT_FAILURE;

	// Copy anything relevant from the provided parameters
	PVR_PROPERTIES* pvrprops = reinterpret_cast<PVR_PROPERTIES*>(props);
	g_userpath.assign(pvrprops->strUserPath);

	try {

#ifdef _WINDOWS
		// On Windows, initialize winsock in case broadcast discovery is used; WSAStartup is
		// reference-counted so if it has already been called this won't hurt anything
		WSADATA wsaData;
		int wsaresult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if(wsaresult != 0) throw string_exception(__func__, ": WSAStartup failed with error code ", wsaresult);
#endif

		// Initialize SQLite
		int result = sqlite3_initialize();
		if(result != SQLITE_OK) throw sqlite_exception(result, "sqlite3_initialize() failed");

		// Create the global addon callbacks instance
		g_addon.reset(new ADDON::CHelper_libXBMC_addon());
		if(!g_addon->RegisterMe(handle)) throw string_exception(__func__, ": failed to register addon handle (CHelper_libXBMC_addon::RegisterMe)");

		// Throw a banner out to the Kodi log indicating that the add-on is being loaded
		log_notice(__func__, ": ", VERSION_PRODUCTNAME_ANSI, " v", VERSION_VERSION3_ANSI, " loading");

		try { 

			// The user data path doesn't always exist when an addon has been installed
			if(!g_addon->DirectoryExists(g_userpath.c_str())) {

				log_notice(__func__, ": user data directory ", g_userpath.c_str(), " does not exist");
				if(!g_addon->CreateDirectory(g_userpath.c_str())) throw string_exception(__func__, ": unable to create addon user data directory");
				log_notice(__func__, ": user data directory ", g_userpath.c_str(), " created");
			}

			// Load the device settings
			if(g_addon->GetSetting("device_connection", &nvalue)) g_settings.device_connection = static_cast<enum device_connection>(nvalue);
			if(g_addon->GetSetting("device_connection_usb_index", &nvalue)) g_settings.device_connection_usb_index = nvalue;
			if(g_addon->GetSetting("device_connection_tcp_host", strvalue)) g_settings.device_connection_tcp_host.assign(strvalue);
			if(g_addon->GetSetting("device_connection_tcp_port", &nvalue)) g_settings.device_connection_tcp_port = nvalue;
			if(g_addon->GetSetting("device_sample_rate", &nvalue)) g_settings.device_sample_rate = nvalue;
			if(g_addon->GetSetting("device_frequency_correction", &nvalue)) g_settings.device_frequency_correction = nvalue;

			// Load the interface settings
			if(g_addon->GetSetting("interface_prepend_channel_numbers", &bvalue)) g_settings.interface_prepend_channel_numbers = bvalue;

			// Load the FM Radio settings
			if(g_addon->GetSetting("fmradio_enable_rds", &bvalue)) g_settings.fmradio_enable_rds = bvalue;
			if(g_addon->GetSetting("fmradio_rds_standard", &nvalue)) g_settings.fmradio_rds_standard = static_cast<enum rds_standard>(nvalue);
			if(g_addon->GetSetting("fmradio_downsample_quality", &nvalue)) g_settings.fmradio_downsample_quality = static_cast<enum downsample_quality>(nvalue);
			if(g_addon->GetSetting("fmradio_output_samplerate", &nvalue)) g_settings.fmradio_output_samplerate = nvalue;
			if(g_addon->GetSetting("fmradio_output_gain", &fvalue)) g_settings.fmradio_output_gain = fvalue;

			// Load the Weather Radio settings
			if(g_addon->GetSetting("wxradio_output_samplerate", &nvalue)) g_settings.wxradio_output_samplerate = nvalue;
			if(g_addon->GetSetting("wxradio_output_gain", &fvalue)) g_settings.wxradio_output_gain = fvalue;

			// Log the setting values; these are for diagnostic purposes just use the raw values
			log_notice(__func__, ": g_settings.device_connection                 = ", static_cast<int>(g_settings.device_connection));
			log_notice(__func__, ": g_settings.device_connection_tcp_host        = ", g_settings.device_connection_tcp_host);
			log_notice(__func__, ": g_settings.device_connection_tcp_port        = ", g_settings.device_connection_tcp_port);
			log_notice(__func__, ": g_settings.device_connection_usb_index       = ", g_settings.device_connection_usb_index);
			log_notice(__func__, ": g_settings.device_frequency_correction       = ", g_settings.device_frequency_correction);
			log_notice(__func__, ": g_settings.device_sample_rate                = ", g_settings.device_sample_rate);
			log_notice(__func__, ": g_settings.fmradio_downsample_quality        = ", static_cast<int>(g_settings.fmradio_downsample_quality));
			log_notice(__func__, ": g_settings.fmradio_enable_rds                = ", g_settings.fmradio_enable_rds);
			log_notice(__func__, ": g_settings.fmradio_output_gain               = ", g_settings.fmradio_output_gain);
			log_notice(__func__, ": g_settings.fmradio_output_samplerate         = ", g_settings.fmradio_output_samplerate);
			log_notice(__func__, ": g_settings.fmradio_rds_standard              = ", static_cast<int>(g_settings.fmradio_rds_standard));
			log_notice(__func__, ": g_settings.interface_prepend_channel_numbers = ", g_settings.interface_prepend_channel_numbers);
			log_notice(__func__, ": g_settings.wxradio_output_gain               = ", g_settings.wxradio_output_gain);
			log_notice(__func__, ": g_settings.wxradio_output_samplerate         = ", g_settings.wxradio_output_samplerate);

			// Create the global gui callbacks instance
			g_gui.reset(new CHelper_libKODI_guilib());
			if(!g_gui->RegisterMe(handle)) throw string_exception(__func__, ": failed to register gui addon handle (CHelper_libKODI_guilib::RegisterMe)");

			try {

				// Create the global pvr callbacks instance
				g_pvr.reset(new CHelper_libXBMC_pvr());
				if(!g_pvr->RegisterMe(handle)) throw string_exception(__func__, ": failed to register pvr addon handle (CHelper_libXBMC_pvr::RegisterMe)");

				// MENUHOOK_SETTING_IMPORTCHANNELS
				//
				memset(&menuhook, 0, sizeof(PVR_MENUHOOK));
				menuhook.iHookId = MENUHOOK_SETTING_IMPORTCHANNELS;
				menuhook.iLocalizedStringId = 30400;
				menuhook.category = PVR_MENUHOOK_SETTING;
				g_pvr->AddMenuHook(&menuhook);

				// MENUHOOK_SETTING_EXPORTCHANNELS
				//
				memset(&menuhook, 0, sizeof(PVR_MENUHOOK));
				menuhook.iHookId = MENUHOOK_SETTING_EXPORTCHANNELS;
				menuhook.iLocalizedStringId = 30401;
				menuhook.category = PVR_MENUHOOK_SETTING;
				g_pvr->AddMenuHook(&menuhook);

				// MENUHOOK_SETTING_CLEARCHANNELS
				//
				memset(&menuhook, 0, sizeof(PVR_MENUHOOK));
				menuhook.iHookId = MENUHOOK_SETTING_CLEARCHANNELS;
				menuhook.iLocalizedStringId = 30402;
				menuhook.category = PVR_MENUHOOK_SETTING;
				g_pvr->AddMenuHook(&menuhook);

				try {

					// Generate the local file system and URL-based file names for the channels database
					std::string databasefile = std::string(g_userpath.c_str()) + "/channels.db";
					std::string databasefileuri = "file:///" + databasefile;

					// Create the global database connection pool instance
					try { g_connpool = std::make_shared<connectionpool>(databasefileuri.c_str(), DATABASE_CONNECTIONPOOL_SIZE, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI); } 
					catch(sqlite_exception const& dbex) {

						log_error(__func__, ": unable to create/open the channels database ", databasefile, " - ", dbex.what());
						throw;
					}
				}

				// Clean up the pvr callbacks instance on exception
				catch(...) { g_pvr.reset(); throw; }
			}
			
			// Clean up the gui callbacks instance on exception
			catch(...) { g_gui.reset(); throw; }
		}

		// Clean up the addon callbacks on exception; but log the error first -- once the callbacks
		// are destroyed so is the ability to write to the Kodi log file
		catch(std::exception& ex) { handle_stdexception(__func__, ex); g_addon.reset(); throw; }
		catch(...) { handle_generalexception(__func__); g_addon.reset(); throw; }
	}

	// Anything that escapes above can't be logged at this point, just return ADDON_STATUS_PERMANENT_FAILURE
	catch(...) { return ADDON_STATUS::ADDON_STATUS_PERMANENT_FAILURE; }

	// Throw a simple banner out to the Kodi log indicating that the add-on has been loaded
	log_notice(__func__, ": ", VERSION_PRODUCTNAME_ANSI, " v", VERSION_VERSION3_ANSI, " loaded");

	return ADDON_STATUS::ADDON_STATUS_OK;
}

//---------------------------------------------------------------------------
// ADDON_Destroy
//
// Destroys the Kodi addon instance
//
// Arguments:
//
//	NONE

void ADDON_Destroy(void)
{
	// Throw a message out to the Kodi log indicating that the add-on is being unloaded
	log_notice(__func__, ": ", VERSION_PRODUCTNAME_ANSI, " v", VERSION_VERSION3_ANSI, " unloading");

	g_pvrstream.reset();					// Destroy any active stream instance

	// Check for more than just the global connection pool reference during shutdown,
	// there shouldn't still be any active callbacks running during ADDON_Destroy
	long poolrefs = g_connpool.use_count();
	if(poolrefs != 1) log_notice(__func__, ": warning: g_connpool.use_count = ", g_connpool.use_count());
	g_connpool.reset();

	// Destroy the PVR and GUI callback instances
	g_pvr.reset();
	g_gui.reset();

	// Send a notice out to the Kodi log as late as possible and destroy the addon callbacks
	log_notice(__func__, ": ", VERSION_PRODUCTNAME_ANSI, " v", VERSION_VERSION3_ANSI, " unloaded");
	g_addon.reset();

	// Clean up SQLite
	sqlite3_shutdown();

#ifdef _WINDOWS
	WSACleanup();			// Release winsock reference added in ADDON_Create
#endif
}

//---------------------------------------------------------------------------
// ADDON_GetStatus
//
// Gets the current status of the Kodi addon
//
// Arguments:
//
//	NONE

ADDON_STATUS ADDON_GetStatus(void)
{
	return ADDON_STATUS::ADDON_STATUS_OK;
}

//---------------------------------------------------------------------------
// ADDON_SetSetting
//
// Changes the value of a named Kodi addon setting
//
// Arguments:
//
//	name		- Name of the setting to change
//	value		- New value of the setting to apply

ADDON_STATUS ADDON_SetSetting(char const* name, void const* value)
{
	std::unique_lock<std::mutex> settings_lock(g_settings_lock);

	// device_connection
	//
	if(strcmp(name, "device_connection") == 0) {

		int nvalue = *reinterpret_cast<int const*>(value);
		if(nvalue != static_cast<int>(g_settings.device_connection)) {

			g_settings.device_connection = static_cast<enum device_connection>(nvalue);
			log_notice(__func__, ": setting device_connection changed to ", device_connection_to_string(g_settings.device_connection).c_str());
		}
	}

	// device_connection_usb_index
	//
	else if(strcmp(name, "device_connection_usb_index") == 0) {

		int nvalue = *reinterpret_cast<int const*>(value);
		if(nvalue != static_cast<int>(g_settings.device_connection_usb_index)) {

			g_settings.device_connection_usb_index = nvalue;
			log_notice(__func__, ": setting device_connection_usb_index changed to ", g_settings.device_connection_usb_index);
		}
	}

	// device_connection_tcp_host
	//
	else if(strcmp(name, "device_connection_tcp_host") == 0) {

		if(strcmp(g_settings.device_connection_tcp_host.c_str(), reinterpret_cast<char const*>(value)) != 0) {

			g_settings.device_connection_tcp_host.assign(reinterpret_cast<char const*>(value));
			log_notice(__func__, ": setting device_connection_tcp_host changed to ", g_settings.device_connection_tcp_host.c_str());
		}
	}

	// device_connection_tcp_port
	//
	else if(strcmp(name, "device_connection_tcp_port") == 0) {

		int nvalue = *reinterpret_cast<int const*>(value);
		if(nvalue != static_cast<int>(g_settings.device_connection_tcp_port)) {

			g_settings.device_connection_tcp_port = nvalue;
			log_notice(__func__, ": setting device_connection_tcp_port changed to ", g_settings.device_connection_tcp_port);
		}
	}

	// device_sample_rate
	//
	else if(strcmp(name, "device_sample_rate") == 0) {

		int nvalue = *reinterpret_cast<int const*>(value);
		if(nvalue != static_cast<int>(g_settings.device_sample_rate)) {

			g_settings.device_sample_rate = nvalue;
			log_notice(__func__, ": setting device_sample_rate changed to ", g_settings.device_sample_rate, " Hz");
		}
	}

	// device_frequency_correction
	//
	else if(strcmp(name, "device_frequency_correction") == 0) {

		int nvalue = *reinterpret_cast<int const*>(value);
		if(nvalue != static_cast<int>(g_settings.device_frequency_correction)) {

			g_settings.device_frequency_correction = nvalue;
			log_notice(__func__, ": setting device_frequency_correction changed to ", g_settings.device_frequency_correction, " PPM");
		}
	}

	// interface_prepend_channel_numbers
	//
	else if(strcmp(name, "interface_prepend_channel_numbers") == 0) {

		bool bvalue = *reinterpret_cast<bool const*>(value);
		if(bvalue != g_settings.interface_prepend_channel_numbers) {

			g_settings.interface_prepend_channel_numbers = bvalue;
			log_notice(__func__, ": setting interface_prepend_channel_numbers changed to ", bvalue);

			// Trigger channel and channel group updates to refresh the channel information
			g_pvr->TriggerChannelUpdate();
			g_pvr->TriggerChannelGroupsUpdate();
		}
	}

	// fmradio_enable_rds
	//
	else if(strcmp(name, "fmradio_enable_rds") == 0) {

		bool bvalue = *reinterpret_cast<bool const*>(value);
		if(bvalue != g_settings.fmradio_enable_rds) {

			g_settings.fmradio_enable_rds = bvalue;
			log_notice(__func__, ": setting fmradio_enable_rds changed to ", bvalue);
		}
	}

	// fmradio_rds_standard
	//
	else if(strcmp(name, "fmradio_rds_standard") == 0) {

		int nvalue = *reinterpret_cast<int const*>(value);
		if(nvalue != static_cast<int>(g_settings.fmradio_rds_standard)) {

			g_settings.fmradio_rds_standard = static_cast<enum rds_standard>(nvalue);
			log_notice(__func__, ": setting fmradio_rds_standard changed to ", rds_standard_to_string(g_settings.fmradio_rds_standard).c_str());
		}
	}

	// fmradio_downsample_quality
	//
	else if(strcmp(name, "fmradio_downsample_quality") == 0) {

		int nvalue = *reinterpret_cast<int const*>(value);
		if(nvalue != static_cast<int>(g_settings.fmradio_downsample_quality)) {

			g_settings.fmradio_downsample_quality = static_cast<enum downsample_quality>(nvalue);
			log_notice(__func__, ": setting fmradio_downsample_quality changed to ", downsample_quality_to_string(g_settings.fmradio_downsample_quality).c_str());
		}
	}

	// fmradio_output_samplerate
	//
	else if(strcmp(name, "fmradio_output_samplerate") == 0) {

		int nvalue = *reinterpret_cast<int const*>(value);
		if(nvalue != g_settings.fmradio_output_samplerate) {

			g_settings.fmradio_output_samplerate = nvalue;
			log_notice(__func__, ": setting fmradio_output_samplerate changed to ", g_settings.fmradio_output_samplerate, "Hz");
		}
	}

	// fmradio_output_gain
	//
	else if(strcmp(name, "fmradio_output_gain") == 0) {

		float fvalue = *reinterpret_cast<float const*>(value);
		if(fvalue != g_settings.fmradio_output_gain) {

			g_settings.fmradio_output_gain = fvalue;
			log_notice(__func__, ": setting fmradio_output_gain changed to ", g_settings.fmradio_output_gain, "dB");
		}
	}

	// wxradio_output_samplerate
	//
	else if(strcmp(name, "wxradio_output_samplerate") == 0) {

		int nvalue = *reinterpret_cast<int const*>(value);
		if(nvalue != g_settings.wxradio_output_samplerate) {

			g_settings.wxradio_output_samplerate = nvalue;
			log_notice(__func__, ": setting wxradio_output_samplerate changed to ", g_settings.wxradio_output_samplerate, "Hz");
		}
	}

	// wxradio_output_gain
	//
	else if(strcmp(name, "wxradio_output_gain") == 0) {

		float fvalue = *reinterpret_cast<float const*>(value);
		if(fvalue != g_settings.wxradio_output_gain) {

			g_settings.wxradio_output_gain = fvalue;
			log_notice(__func__, ": setting wxradio_output_gain changed to ", g_settings.wxradio_output_gain, "dB");
		}
	}

	return ADDON_STATUS_OK;
}

//---------------------------------------------------------------------------
// KODI PVR ADDON ENTRY POINTS
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// GetAddonCapabilities
//
// Get the list of features that this add-on provides
//
// Arguments:
//
//	capabilities	- Capabilities structure to fill out

PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES *capabilities)
{
	if(capabilities == nullptr) return PVR_ERROR::PVR_ERROR_INVALID_PARAMETERS;

	*capabilities = g_capabilities;
	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//---------------------------------------------------------------------------
// GetBackendName
//
// Get the name reported by the backend that will be displayed in the UI
//
// Arguments:
//
//	NONE

char const* GetBackendName(void)
{
	return VERSION_PRODUCTNAME_ANSI;
}

//---------------------------------------------------------------------------
// GetBackendVersion
//
// Get the version string reported by the backend that will be displayed in the UI
//
// Arguments:
//
//	NONE

char const* GetBackendVersion(void)
{
	return VERSION_VERSION3_ANSI;
}

//---------------------------------------------------------------------------
// GetConnectionString
//
// Get the connection string reported by the backend that will be displayed in the UI
//
// Arguments:
//
//	NONE

char const* GetConnectionString(void)
{
	// Create a copy of the current addon settings structure
	struct addon_settings settings = copy_settings();

	// This property is fairly useless; just return the device connection type
	if(settings.device_connection == device_connection::usb) return "usb";
	else if(settings.device_connection == device_connection::rtltcp) return "network";
	else return "unknown";
}

//---------------------------------------------------------------------------
// GetDriveSpace
//
// Get the disk space reported by the backend (if supported)
//
// Arguments:
//
//	total		- The total disk space in bytes
//	used		- The used disk space in bytes

PVR_ERROR GetDriveSpace(long long* /*total*/, long long* /*used*/)
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// CallMenuHook
//
// Call one of the menu hooks (if supported)
//
// Arguments:
//
//	menuhook	- The hook to call
//	item		- The selected item for which the hook is called

PVR_ERROR CallMenuHook(PVR_MENUHOOK const& menuhook, PVR_MENUHOOK_DATA const& /*item*/)
{
	try {

		// Invoke the proper helper function to handle the menu hook implementation
		if(menuhook.iHookId == MENUHOOK_SETTING_IMPORTCHANNELS) menuhook_importchannels();
		else if(menuhook.iHookId == MENUHOOK_SETTING_EXPORTCHANNELS) menuhook_exportchannels();
		else if(menuhook.iHookId == MENUHOOK_SETTING_CLEARCHANNELS) menuhook_clearchannels();
	}

	catch(std::exception& ex) { return handle_stdexception(__func__, ex, PVR_ERROR::PVR_ERROR_FAILED); }
	catch(...) { return handle_generalexception(__func__, PVR_ERROR::PVR_ERROR_FAILED); }

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//---------------------------------------------------------------------------
// GetEPGForChannel
//
// Request the EPG for a channel from the backend
//
// Arguments:
//
//	handle		- Handle to pass to the callback method
//	channel		- The channel to get the EPG table for
//	start		- Get events after this time (UTC)
//	end			- Get events before this time (UTC)

PVR_ERROR GetEPGForChannel(ADDON_HANDLE /*handle*/, PVR_CHANNEL const& /*channel*/, time_t /*start*/, time_t /*end*/)
{
	// This PVR doesn't support EPG, but on the Leia baseline if it doesn't claim
	// that it does the radio and TV channels get all mixed up ...
	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//---------------------------------------------------------------------------
// IsEPGTagRecordable
//
// Check if the given EPG tag can be recorded
//
// Arguments:
//
//	tag			- EPG tag to be checked
//	recordable	- Flag indicating if the EPG tag can be recorded

PVR_ERROR IsEPGTagRecordable(EPG_TAG const* /*tag*/, bool* /*recordable*/)
{
	return PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// IsEPGTagPlayable
//
// Check if the given EPG tag can be played
//
// Arguments:
//
//	tag			- EPG tag to be checked
//	playable	- Flag indicating if the EPG tag can be played

PVR_ERROR IsEPGTagPlayable(EPG_TAG const* /*tag*/, bool* /*playable*/)
{
	return PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// GetEPGTagEdl
//
// Retrieve the edit decision list (EDL) of an EPG tag on the backend
//
// Arguments:
//
//	tag			- EPG tag
//	edl			- The function has to write the EDL list into this array
//	count		- The maximum size of the EDL, out: the actual size of the EDL

PVR_ERROR GetEPGTagEdl(EPG_TAG const* /*tag*/, PVR_EDL_ENTRY /*edl*/[], int* /*count*/)
{
	return PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// GetEPGTagStreamProperties
//
// Get the stream properties for an epg tag from the backend
//
// Arguments:
//
//	tag			- EPG tag for which to retrieve the properties
//	props		- Array in which to set the stream properties
//	numprops	- Number of properties returned by this function

PVR_ERROR GetEPGTagStreamProperties(EPG_TAG const* /*tag*/, PVR_NAMED_VALUE* /*props*/, unsigned int* /*numprops*/)
{
	return PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// GetChannelGroupsAmount
//
// Get the total amount of channel groups on the backend if it supports channel groups
//
// Arguments:
//
//	NONE

int GetChannelGroupsAmount(void)
{
	return 2;				// "FM Radio", "Weather Radio"
}

//---------------------------------------------------------------------------
// GetChannelGroups
//
// Request the list of all channel groups from the backend if it supports channel groups
//
// Arguments:
//
//	handle		- Handle to pass to the callack method
//	radio		- True to get radio groups, false to get TV channel groups

PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool radio)
{
	if(handle == nullptr) return PVR_ERROR::PVR_ERROR_INVALID_PARAMETERS;

	// The PVR only supports radio channel groups
	if(!radio) return PVR_ERROR::PVR_ERROR_NO_ERROR;

	PVR_CHANNEL_GROUP group = {};

	// FM Radio
	snprintf(group.strGroupName, std::extent<decltype(group.strGroupName)>::value, "FM Radio");
	group.bIsRadio = true;
	g_pvr->TransferChannelGroup(handle, &group);

	// Weather Radio
	snprintf(group.strGroupName, std::extent<decltype(group.strGroupName)>::value, "Weather Radio");
	group.bIsRadio = true;
	g_pvr->TransferChannelGroup(handle, &group);

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//---------------------------------------------------------------------------
// GetChannelGroupMembers
//
// Request the list of all channel groups from the backend if it supports channel groups
//
// Arguments:
//
//	handle		- Handle to pass to the callack method
//	group		- The group to get the members for

PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, PVR_CHANNEL_GROUP const& group)
{
	assert(g_pvr);

	if(handle == nullptr) return PVR_ERROR::PVR_ERROR_INVALID_PARAMETERS;

	// Only interested in radio channel groups
	if(!group.bIsRadio) return PVR_ERROR::PVR_ERROR_NO_ERROR;

	// Select the proper enumerator for the channel group
	std::function<void(sqlite3*, enumerate_channels_callback)> enumerator = nullptr;
	if(strcmp(group.strGroupName, "FM Radio") == 0) enumerator = enumerate_fmradio_channels;
	if(strcmp(group.strGroupName, "Weather Radio") == 0) enumerator = enumerate_wxradio_channels;

	// If no enumerator was selected, there isn't any work to do here
	if(enumerator == nullptr) return PVR_ERROR::PVR_ERROR_NO_ERROR;

	try {

		// Enumerate all of the channels in the specified group
		enumerator(connectionpool::handle(g_connpool), [&](struct channel const& channel) -> void {

			PVR_CHANNEL_GROUP_MEMBER member = {};					// PVR_CHANNEL_GROUP_MEMORY to send

			// strGroupName (required)
			snprintf(member.strGroupName, std::extent<decltype(member.strGroupName)>::value, "%s", group.strGroupName);

			// iChannelUniqueId (required)
			member.iChannelUniqueId = channel.id;

			// iChannelNumber
			member.iChannelNumber = channel.channel;

			// iSubChannelNumber
			member.iSubChannelNumber = channel.subchannel;

			// Transfer the generated PVR_CHANNEL_GROUP_MEMBER structure over to Kodi
			g_pvr->TransferChannelGroupMember(handle, &member);
		});
	}

	catch(std::exception& ex) { return handle_stdexception(__func__, ex, PVR_ERROR::PVR_ERROR_FAILED); }
	catch(...) { return handle_generalexception(__func__, PVR_ERROR::PVR_ERROR_FAILED); }

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//---------------------------------------------------------------------------
// OpenDialogChannelScan
//
// Show the channel scan dialog if this backend supports it
//
// Arguments:
//
//	NONE

PVR_ERROR OpenDialogChannelScan(void)
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// GetChannelsAmount
//
// The total amount of channels on the backend, or -1 on error
//
// Arguments:
//
//	NONE

int GetChannelsAmount(void)
{
	try { return get_channel_count(connectionpool::handle(g_connpool)); }
	catch(std::exception& ex) { return handle_stdexception(__func__, ex, -1); }
	catch(...) { return handle_generalexception(__func__, -1); }
}

//---------------------------------------------------------------------------
// GetChannels
//
// Request the list of all channels from the backend
//
// Arguments:
//
//	handle		- Handle to pass to the callback method
//	radio		- True to get radio channels, false to get TV channels

PVR_ERROR GetChannels(ADDON_HANDLE handle, bool radio)
{
	assert(g_pvr);	

	if(handle == nullptr) return PVR_ERROR::PVR_ERROR_INVALID_PARAMETERS;

	// The PVR only supports radio channels
	if(!radio) return PVR_ERROR::PVR_ERROR_NO_ERROR;

	// Create a copy of the current addon settings structure
	struct addon_settings settings = copy_settings();

	try {

		// Enumerate all of the channels in the database
		enumerate_channels(connectionpool::handle(g_connpool), [&](struct channel const& item) -> void {

			PVR_CHANNEL channel = {};

			// iUniqueId (required)
			channel.iUniqueId = item.id;

			// bIsRadio (required)
			channel.bIsRadio = true;

			// iChannelNumber
			channel.iChannelNumber = item.channel;

			// iSubChannelNumber
			channel.iSubChannelNumber = item.subchannel;

			// strChannelName
			if(item.name != nullptr) {

				if(settings.interface_prepend_channel_numbers)
					snprintf(channel.strChannelName, std::extent<decltype(channel.strChannelName)>::value, "%u.%u %s", item.channel, item.subchannel, item.name);

				else snprintf(channel.strChannelName, std::extent<decltype(channel.strChannelName)>::value, "%s", item.name);
			}

			// strIconPath
			if(item.logourl != nullptr) snprintf(channel.strIconPath, std::extent<decltype(channel.strIconPath)>::value, "%s", item.logourl);

			// bIsHidden
			channel.bIsHidden = item.hidden;

			// Transfer the PVR_CHANNEL structure over to Kodi
			g_pvr->TransferChannelEntry(handle, &channel);
		});
	}
	
	catch(std::exception& ex) { return handle_stdexception(__func__, ex, PVR_ERROR::PVR_ERROR_FAILED); }
	catch(...) { return handle_generalexception(__func__, PVR_ERROR::PVR_ERROR_FAILED); }

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//---------------------------------------------------------------------------
// DeleteChannel
//
// Delete a channel from the backend
//
// Arguments:
//
//	channel		- The channel to delete

PVR_ERROR DeleteChannel(PVR_CHANNEL const& channel)
{
	try { delete_channel(connectionpool::handle(g_connpool), channel.iUniqueId); }
	catch(std::exception& ex) { return handle_stdexception(__func__, ex, PVR_ERROR::PVR_ERROR_FAILED); }
	catch(...) { return handle_generalexception(__func__, PVR_ERROR::PVR_ERROR_FAILED); }

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//---------------------------------------------------------------------------
// RenameChannel
//
// Rename a channel on the backend
//
// Arguments:
//
//	channel		- The channel to rename, containing the new channel name

PVR_ERROR RenameChannel(PVR_CHANNEL const& channel)
{
	try { rename_channel(connectionpool::handle(g_connpool), channel.iUniqueId, channel.strChannelName); }
	catch(std::exception& ex) { return handle_stdexception(__func__, ex, PVR_ERROR::PVR_ERROR_FAILED); }
	catch(...) { return handle_generalexception(__func__, PVR_ERROR::PVR_ERROR_FAILED); }

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//---------------------------------------------------------------------------
// OpenDialogChannelSettings
//
// Show the channel settings dialog, if supported by the backend
//
// Arguments:
//
//	channel		- The channel to show the dialog for

PVR_ERROR OpenDialogChannelSettings(PVR_CHANNEL const& /*channel*/)
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// OpenDialogChannelAdd
//
// Show the dialog to add a channel on the backend, if supported by the backend
//
// Arguments:
//
//	channel		- The channel to add

PVR_ERROR OpenDialogChannelAdd(PVR_CHANNEL const& /*channel*/)
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// GetRecordingsAmount
//
// The total amount of recordings on the backend or -1 on error
//
// Arguments:
//
//	deleted		- if set return deleted recording

int GetRecordingsAmount(bool /*deleted*/)
{
	return -1;
}

//---------------------------------------------------------------------------
// GetRecordings
//
// Request the list of all recordings from the backend, if supported
//
// Arguments:
//
//	handle		- Handle to pass to the callback method
//	deleted		- If set return deleted recording

PVR_ERROR GetRecordings(ADDON_HANDLE /*handle*/, bool /*deleted*/)
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// DeleteRecording
//
// Delete a recording on the backend
//
// Arguments:
//
//	recording	- The recording to delete

PVR_ERROR DeleteRecording(PVR_RECORDING const& /*recording*/)
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// UndeleteRecording
//
// Undelete a recording on the backend
//
// Arguments:
//
//	recording	- The recording to undelete

PVR_ERROR UndeleteRecording(PVR_RECORDING const& /*recording*/)
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// DeleteAllRecordingsFromTrash
//
// Delete all recordings permanent which in the deleted folder on the backend
//
// Arguments:
//
//	NONE

PVR_ERROR DeleteAllRecordingsFromTrash()
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// RenameRecording
//
// Rename a recording on the backend
//
// Arguments:
//
//	recording	- The recording to rename, containing the new name

PVR_ERROR RenameRecording(PVR_RECORDING const& /*recording*/)
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// SetRecordingLifetime
//
// Set the lifetime of a recording on the backend
//
// Arguments:
//
//	recording	- The recording to change the lifetime for

PVR_ERROR SetRecordingLifetime(PVR_RECORDING const* /*recording*/)
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// SetRecordingPlayCount
//
// Set the play count of a recording on the backend
//
// Arguments:
//
//	recording	- The recording to change the play count
//	playcount	- Play count

PVR_ERROR SetRecordingPlayCount(PVR_RECORDING const& /*recording*/, int /*playcount*/)
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// SetRecordingLastPlayedPosition
//
// Set the last watched position of a recording on the backend
//
// Arguments:
//
//	recording			- The recording
//	lastposition		- The last watched position in seconds

PVR_ERROR SetRecordingLastPlayedPosition(PVR_RECORDING const& /*recording*/, int /*lastposition*/)
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// GetRecordingLastPlayedPosition
//
// Retrieve the last watched position of a recording (in seconds) on the backend
//
// Arguments:
//
//	recording	- The recording

int GetRecordingLastPlayedPosition(PVR_RECORDING const& /*recording*/)
{
	return -1;
}

//---------------------------------------------------------------------------
// GetRecordingEdl
//
// Retrieve the edit decision list (EDL) of a recording on the backend
//
// Arguments:
//
//	recording	- The recording
//	edl			- The function has to write the EDL list into this array
//	count		- in: The maximum size of the EDL, out: the actual size of the EDL

PVR_ERROR GetRecordingEdl(PVR_RECORDING const& /*recording*/, PVR_EDL_ENTRY /*edl*/[], int* /*count*/)
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// GetTimerTypes
//
// Retrieve the timer types supported by the backend
//
// Arguments:
//
//	types		- The function has to write the definition of the supported timer types into this array
//	count		- in: The maximum size of the list; out: the actual size of the list

PVR_ERROR GetTimerTypes(PVR_TIMER_TYPE /*types*/[], int* /*count*/)
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// GetTimersAmount
//
// Gets the total amount of timers on the backend or -1 on error
//
// Arguments:
//
//	NONE

int GetTimersAmount(void)
{
	return -1;
}

//---------------------------------------------------------------------------
// GetTimers
//
// Request the list of all timers from the backend if supported
//
// Arguments:
//
//	handle		- Handle to pass to the callback method

PVR_ERROR GetTimers(ADDON_HANDLE /*handle*/)
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// AddTimer
//
// Add a timer on the backend
//
// Arguments:
//
//	timer	- The timer to add

PVR_ERROR AddTimer(PVR_TIMER const& /*timer*/)
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// DeleteTimer
//
// Delete a timer on the backend
//
// Arguments:
//
//	timer	- The timer to delete
//	force	- Set to true to delete a timer that is currently recording a program

PVR_ERROR DeleteTimer(PVR_TIMER const& /*timer*/, bool /*force*/)
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// UpdateTimer
//
// Update the timer information on the backend
//
// Arguments:
//
//	timer	- The timer to update

PVR_ERROR UpdateTimer(PVR_TIMER const& /*timer*/)
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// OpenLiveStream
//
// Open a live stream on the backend
//
// Arguments:
//
//	channel		- The channel to stream

bool OpenLiveStream(PVR_CHANNEL const& channel)
{
	// Prevent race condition with SignalStatus()
	std::unique_lock<std::mutex> lock(g_pvrstream_lock);

	// Create a copy of the current addon settings structure
	struct addon_settings settings = copy_settings();

	try { 
	
		// Set up the tuner device properties
		struct tunerprops tunerprops = {};
		tunerprops.samplerate = settings.device_sample_rate;
		tunerprops.freqcorrection = settings.device_frequency_correction;

		// Retrieve the tuning properties for the channel from the database
		struct channelprops channelprops = {};
		if(!get_channel_properties(connectionpool::handle(g_connpool), channel.iUniqueId, channelprops))
			throw string_exception("channel ", channel.iUniqueId, " (", channel.strChannelName, ") was not found in the database");

		// FM Radio
		//
		if((channelprops.frequency >= 87500000) && (channelprops.frequency <= 107900000) && (channelprops.subchannel == 0)) {

			// Set up the FM digital signal processor properties
			struct fmprops fmprops = {};
			fmprops.decoderds = settings.fmradio_enable_rds;
			fmprops.isrbds = (get_regional_rds_standard(settings.fmradio_rds_standard) == rds_standard::rbds);
			fmprops.downsamplequality = static_cast<int>(settings.fmradio_downsample_quality);
			fmprops.outputrate = settings.fmradio_output_samplerate;
			fmprops.outputgain = settings.fmradio_output_gain;

			// Log information about the stream for diagnostic purposes
			log_notice(__func__, ": Creating fmstream for channel \"", channelprops.name, "\"");
			log_notice(__func__, ": tunerprops.samplerate = ", tunerprops.samplerate, " Hz");
			log_notice(__func__, ": tunerprops.freqcorrection = ", tunerprops.freqcorrection, " PPM");
			log_notice(__func__, ": fmprops.decoderds = ", (fmprops.decoderds) ? "true" : "false");
			log_notice(__func__, ": fmprops.isrbds = ", (fmprops.isrbds) ? "true" : "false");
			log_notice(__func__, ": fmprops.downsamplequality = ", downsample_quality_to_string(static_cast<enum downsample_quality>(fmprops.downsamplequality)));
			log_notice(__func__, ": fmprops.outputgain = ", fmprops.outputgain, " dB");
			log_notice(__func__, ": fmprops.outputrate = ", fmprops.outputrate, " Hz");
			log_notice(__func__, ": channelprops.frequency = ", channelprops.frequency, " Hz");
			log_notice(__func__, ": channelprops.autogain = ", (channelprops.autogain) ? "true" : "false");
			log_notice(__func__, ": channelprops.manualgain = ", channelprops.manualgain / 10, " dB");

			// Create the FM Radio stream
			g_pvrstream = fmstream::create(create_device(settings), tunerprops, channelprops, fmprops);
		}

		// Weather Radio
		//
		else if((channelprops.frequency >= 162400000) && (channelprops.frequency <= 162550000) && (channelprops.subchannel == 0)) {

			// Set up the WX digital signal processor properties
			struct wxprops wxprops = {};
			wxprops.outputrate = settings.wxradio_output_samplerate;
			wxprops.outputgain = settings.wxradio_output_gain;

			// Log information about the stream for diagnostic purposes
			log_notice(__func__, ": Creating wxstream for channel \"", channelprops.name, "\"");
			log_notice(__func__, ": tunerprops.samplerate = ", tunerprops.samplerate, " Hz");
			log_notice(__func__, ": tunerprops.freqcorrection = ", tunerprops.freqcorrection, " PPM");
			log_notice(__func__, ": wxprops.outputgain = ", wxprops.outputgain, " dB");
			log_notice(__func__, ": wxprops.outputrate = ", wxprops.outputrate, " Hz");
			log_notice(__func__, ": channelprops.frequency = ", channelprops.frequency, " Hz");
			log_notice(__func__, ": channelprops.autogain = ", (channelprops.autogain) ? "true" : "false");
			log_notice(__func__, ": channelprops.manualgain = ", channelprops.manualgain / 10, " dB");

			// Create the Weather Radio stream
			g_pvrstream = wxstream::create(create_device(settings), tunerprops, channelprops, wxprops);
		}

		else throw string_exception("channel ", channel.iUniqueId, " (", channel.strChannelName, ") has an unknown modulation type");
	}

	// Queue a notification for the user when a live stream cannot be opened, don't just silently log it
	catch(std::exception& ex) { 
		
		g_addon->QueueNotification(ADDON::queue_msg_t::QUEUE_ERROR, "Live Stream creation failed (%s).", ex.what());
		return handle_stdexception(__func__, ex, false); 
	}

	catch(...) { return handle_generalexception(__func__, false); }

	return true;
}

//---------------------------------------------------------------------------
// CloseLiveStream
//
// Closes the live stream
//
// Arguments:
//
//	NONE

void CloseLiveStream(void)
{
	// Prevent race condition with SignalStatus()
	std::unique_lock<std::mutex> lock(g_pvrstream_lock);

	try { g_pvrstream.reset(); }
	catch(std::exception& ex) { return handle_stdexception(__func__, ex); }
	catch(...) { return handle_generalexception(__func__); }
}

//---------------------------------------------------------------------------
// ReadLiveStream
//
// Read from an open live stream
//
// Arguments:
//
//	buffer		- The buffer to store the data in
//	size		- The number of bytes to read into the buffer

int ReadLiveStream(unsigned char* /*buffer*/, unsigned int /*size*/)
{
	return -1;
}

//---------------------------------------------------------------------------
// SeekLiveStream
//
// Seek in a live stream on a backend that supports timeshifting
//
// Arguments:
//
//	position	- Delta within the stream to seek, relative to whence
//	whence		- Starting position from which to apply the delta

long long SeekLiveStream(long long position, int whence)
{
	try { return (g_pvrstream) ? g_pvrstream->seek(position, whence) : -1; } 
	catch(std::exception& ex) { return handle_stdexception(__func__, ex, -1); }
	catch(...) { return handle_generalexception(__func__, -1); }
}

//---------------------------------------------------------------------------
// PositionLiveStream
//
// Gets the position in the stream that's currently being read
//
// Arguments:
//
//	NONE

long long PositionLiveStream(void)
{
	// Don't report the position for a real-time stream
	try { return (g_pvrstream && !g_pvrstream->realtime()) ? g_pvrstream->position() : -1; }
	catch(std::exception& ex) { return handle_stdexception(__func__, ex, -1); }
	catch(...) { return handle_generalexception(__func__, -1); }
}

//---------------------------------------------------------------------------
// LengthLiveStream
//
// The total length of the stream that's currently being read
//
// Arguments:
//
//	NONE

long long LengthLiveStream(void)
{
	try { return (g_pvrstream) ? g_pvrstream->length() : -1; } 
	catch(std::exception& ex) { return handle_stdexception(__func__, ex, -1); }
	catch(...) { return handle_generalexception(__func__, -1); }
}

//---------------------------------------------------------------------------
// SignalStatus
//
// Get the signal status of the stream that's currently open
//
// Arguments:
//
//	status		- The signal status

PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS& status)
{
	// Prevent race condition with functions that modify g_pvrstream
	std::unique_lock<std::mutex> lock(g_pvrstream_lock);

	// Initialize the PVR_SIGNAL_STATUS structure, it comes in uninitialized
	memset(&status, 0, sizeof(PVR_SIGNAL_STATUS));

	// Kodi may call this function before the stream is open, avoid the error log
	if(!g_pvrstream) return PVR_ERROR::PVR_ERROR_NO_ERROR;

	try {

		snprintf(status.strAdapterName, std::extent<decltype(status.strAdapterName)>::value, "%s", g_pvrstream->devicename().c_str());
		snprintf(status.strAdapterStatus, std::extent<decltype(status.strAdapterStatus)>::value, "Active");
		snprintf(status.strServiceName, std::extent<decltype(status.strServiceName)>::value, "%s", g_pvrstream->servicename().c_str());
		snprintf(status.strProviderName, std::extent<decltype(status.strProviderName)>::value, "RTL-SDR");
		snprintf(status.strMuxName, std::extent<decltype(status.strMuxName)>::value, "%s", g_pvrstream->muxname().c_str());

		status.iSNR = g_pvrstream->signaltonoise() * 655;			// Range: 0-65535
		status.iSignal = g_pvrstream->signalstrength() * 655;		// Range: 0-65535
		status.iBER = 0;
		status.iUNC = 0;
	}

	catch(std::exception& ex) { return handle_stdexception(__func__, ex, PVR_ERROR::PVR_ERROR_FAILED); }
	catch(...) { return handle_generalexception(__func__, PVR_ERROR::PVR_ERROR_FAILED); }

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//---------------------------------------------------------------------------
// GetDescrambleInfo
//
// Get the descramble information of the stream that's currently open
//
// Arguments:
//
//	descrambleinfo		- Descramble information

PVR_ERROR GetDescrambleInfo(PVR_DESCRAMBLE_INFO* /*descrambleinfo*/)
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// GetChannelStreamProperties
//
// Get the stream properties for a channel from the backend
//
// Arguments:
//
//	channel		- Channel to get the stream properties for
//	props		- Array of properties to be set for the stream
//	numprops	- Number of properties returned by this function

PVR_ERROR GetChannelStreamProperties(PVR_CHANNEL const* /*channel*/, PVR_NAMED_VALUE* /*props*/, unsigned int* /*numprops*/)
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// GetRecordingStreamProperties
//
// Get the stream properties for a recording from the backend
//
// Arguments:
//
//	recording	- Recording to get the stream properties for
//	props		- Array of properties to be set for the stream
//	numprops	- Number of properties returned by this function

PVR_ERROR GetRecordingStreamProperties(PVR_RECORDING const* /*recording*/, PVR_NAMED_VALUE* /*props*/, unsigned int* /*numprops*/)
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// GetStreamProperties
//
// Get the stream properties of the stream that's currently being read
//
// Arguments:
//
//	properties	- The properties of the currently playing stream

PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* properties)
{
	if(!g_pvrstream) return PVR_ERROR::PVR_ERROR_FAILED;

	// Enumerate the stream properties as specified by the PVR stream instance
	properties->iStreamCount = 0;
	g_pvrstream->enumproperties([&](struct streamprops const& props) -> void {

		xbmc_codec_t codecid = g_pvr->GetCodecByName(props.codec);
		if(codecid.codec_type != XBMC_CODEC_TYPE_UNKNOWN) {

			properties->stream[properties->iStreamCount].iPID = props.pid;
			properties->stream[properties->iStreamCount].iCodecType = codecid.codec_type;
			properties->stream[properties->iStreamCount].iCodecId = codecid.codec_id;
			properties->stream[properties->iStreamCount].iChannels = props.channels;
			properties->stream[properties->iStreamCount].iSampleRate = props.samplerate;
			properties->stream[properties->iStreamCount].iBitsPerSample = props.bitspersample;
			properties->stream[properties->iStreamCount].iBitRate = properties->stream[properties->iStreamCount].iSampleRate * 
				properties->stream[properties->iStreamCount].iChannels * properties->stream[properties->iStreamCount].iBitsPerSample;
			properties->stream[properties->iStreamCount].strLanguage[properties->iStreamCount] = 0;
			properties->stream[properties->iStreamCount].strLanguage[1] = 0;
			properties->stream[properties->iStreamCount].strLanguage[2] = 0;
			properties->stream[properties->iStreamCount].strLanguage[3] = 0;
			properties->iStreamCount++;
		}
	});

	return PVR_ERROR::PVR_ERROR_NO_ERROR;
}

//---------------------------------------------------------------------------
// GetStreamReadChunkSize
//
// Obtain the chunk size to use when reading streams
//
// Arguments:
//
//	chunksize	- Set to the stream chunk size

PVR_ERROR GetStreamReadChunkSize(int* /*chunksize*/)
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// OpenRecordedStream
//
// Open a stream to a recording on the backend
//
// Arguments:
//
//	recording	- The recording to open

bool OpenRecordedStream(PVR_RECORDING const& /*recording*/)
{
	return false;
}

//---------------------------------------------------------------------------
// CloseRecordedStream
//
// Close an open stream from a recording
//
// Arguments:
//
//	NONE

void CloseRecordedStream(void)
{
}

//---------------------------------------------------------------------------
// ReadRecordedStream
//
// Read from a recording
//
// Arguments:
//
//	buffer		- The buffer to store the data in
//	size		- The number of bytes to read into the buffer

int ReadRecordedStream(unsigned char* /*buffer*/, unsigned int /*size*/)
{
	return -1;
}

//---------------------------------------------------------------------------
// SeekRecordedStream
//
// Seek in a recorded stream
//
// Arguments:
//
//	position	- Delta within the stream to seek, relative to whence
//	whence		- Starting position from which to apply the delta

long long SeekRecordedStream(long long /*position*/, int /*whence*/)
{
	return -1;
}

//---------------------------------------------------------------------------
// LengthRecordedStream
//
// Gets the total length of the stream that's currently being read
//
// Arguments:
//
//	NONE

long long LengthRecordedStream(void)
{
	return -1;
}

//---------------------------------------------------------------------------
// DemuxReset
//
// Reset the demultiplexer in the add-on
//
// Arguments:
//
//	NONE

void DemuxReset(void)
{
	try { if(g_pvrstream) g_pvrstream->demuxreset(); }
	catch(std::exception& ex) { return handle_stdexception(__func__, ex); }
	catch(...) { return handle_generalexception(__func__); }
}

//---------------------------------------------------------------------------
// DemuxAbort
//
// Abort the demultiplexer thread in the add-on
//
// Arguments:
//
//	NONE

void DemuxAbort(void)
{
	try { if(g_pvrstream) g_pvrstream->demuxabort(); }
	catch(std::exception& ex) { return handle_stdexception(__func__, ex); }
	catch(...) { return handle_generalexception(__func__); }
}

//---------------------------------------------------------------------------
// DemuxFlush
//
// Flush all data that's currently in the demultiplexer buffer in the add-on
//
// Arguments:
//
//	NONE

void DemuxFlush(void)
{
	try { if(g_pvrstream) g_pvrstream->demuxflush(); }
	catch(std::exception& ex) { return handle_stdexception(__func__, ex); } 
	catch(...) { return handle_generalexception(__func__); }
}

//---------------------------------------------------------------------------
// DemuxRead
//
// Read the next packet from the demultiplexer, if there is one
//
// Arguments:
//
//	NONE

DemuxPacket* DemuxRead(void)
{
	// Prevent race condition with SignalStatus()
	std::unique_lock<std::mutex> lock(g_pvrstream_lock);

	if(!g_pvrstream) return nullptr;

	try { 
	
		// Use an inline lambda to provide the stream an std::function to use to invoke AllocateDemuxPacket()
		DemuxPacket* packet = g_pvrstream->demuxread([&](int size) -> DemuxPacket* { return g_pvr->AllocateDemuxPacket(size); });

		// Log a warning if a stream change packet was detected; this means the application isn't keeping up with the device
		if((packet != nullptr) && (packet->iStreamId == DMX_SPECIALID_STREAMCHANGE))
			log_notice(__func__, ": warning: stream buffer has been flushed; device sample rate may need to be reduced");
		
		return packet;
	}

	catch(std::exception& ex) {

		// Log the exception and alert the user of the failure with an error notification
		log_error(__func__, ": read operation failed with exception: ", ex.what());
		g_addon->QueueNotification(ADDON::queue_msg_t::QUEUE_ERROR, "Unable to read from stream: %s", ex.what());

		g_pvrstream.reset();				// Close the stream
		return nullptr;						// Return a null demultiplexer packet
	}

	catch(...) { return handle_generalexception(__func__, nullptr); }
}

//---------------------------------------------------------------------------
// CanPauseStream
//
// Check if the backend support pausing the currently playing stream
//
// Arguments:
//
//	NONE

bool CanPauseStream(void)
{
	return false;
}

//---------------------------------------------------------------------------
// CanSeekStream
//
// Check if the backend supports seeking for the currently playing stream
//
// Arguments:
//
//	NONE

bool CanSeekStream(void)
{
	try { return (g_pvrstream) ? g_pvrstream->canseek() : false; }
	catch(std::exception& ex) { return handle_stdexception(__func__, ex, false); }
	catch(...) { return handle_generalexception(__func__, false); }
}

//---------------------------------------------------------------------------
// PauseStream
//
// Notify the pvr addon that XBMC (un)paused the currently playing stream
//
// Arguments:
//
//	paused		- Paused/unpaused flag

void PauseStream(bool /*paused*/)
{
}

//---------------------------------------------------------------------------
// SeekTime
//
// Notify the pvr addon/demuxer that XBMC wishes to seek the stream by time
//
// Arguments:
//
//	time		- The absolute time since stream start
//	backwards	- True to seek to keyframe BEFORE time, else AFTER
//	startpts	- Can be updated to point to where display should start

bool SeekTime(double /*time*/, bool /*backwards*/, double* /*startpts*/)
{
	return false;
}

//---------------------------------------------------------------------------
// SetSpeed
//
// Notify the pvr addon/demuxer that XBMC wishes to change playback speed
//
// Arguments:
//
//	speed		- The requested playback speed

void SetSpeed(int /*speed*/)
{
}

//---------------------------------------------------------------------------
// GetBackendHostname
//
// Get the hostname of the pvr backend server
//
// Arguments:
//
//	NONE

char const* GetBackendHostname(void)
{
	return "";
}

//---------------------------------------------------------------------------
// IsTimeshifting
//
// Check if timeshift is active
//
// Arguments:
//
//	NONE

bool IsTimeshifting(void)
{
	return false;
}

//---------------------------------------------------------------------------
// IsRealTimeStream
//
// Check for real-time streaming
//
// Arguments:
//
//	NONE

bool IsRealTimeStream(void)
{
	try { return (g_pvrstream) ? g_pvrstream->realtime() : false; }
	catch(std::exception& ex) { return handle_stdexception(__func__, ex, false); }
	catch(...) { return handle_generalexception(__func__, false); }
}

//---------------------------------------------------------------------------
// SetEPGTimeFrame
//
// Tell the client the time frame to use when notifying epg events back to Kodi
//
// Arguments:
//
//	days	- number of days from "now". EPG_TIMEFRAME_UNLIMITED means that Kodi is interested in all epg events

PVR_ERROR SetEPGTimeFrame(int /*days*/)
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------
// OnSystemSleep
//
// Notification of system sleep power event
//
// Arguments:
//
//	NONE

void OnSystemSleep()
{
}

//---------------------------------------------------------------------------
// OnSystemWake
//
// Notification of system wake power event
//
// Arguments:
//
//	NONE

void OnSystemWake()
{
}

//---------------------------------------------------------------------------
// OnPowerSavingActivated
//
// Notification of system power saving activation event
//
// Arguments:
//
//	NONE

void OnPowerSavingActivated()
{
}

//---------------------------------------------------------------------------
// OnPowerSavingDeactivated
//
// Notification of system power saving deactivation event
//
// Arguments:
//
//	NONE

void OnPowerSavingDeactivated()
{
}

//---------------------------------------------------------------------------
// GetStreamTimes
//
// Temporary function to be removed in later PVR API version

PVR_ERROR GetStreamTimes(PVR_STREAM_TIMES* /*times*/)
{
	return PVR_ERROR::PVR_ERROR_NOT_IMPLEMENTED;
}

//---------------------------------------------------------------------------

#pragma warning(pop)
