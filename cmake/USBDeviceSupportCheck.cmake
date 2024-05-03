#
# Set value about USB support
#
# This code below checks the in KODI's build system defined target about
# allowance for USB RTL-SDR support.
# Further are here the configure values for settings.xml.in set.
#
# The USB support set can also be overritten with cmake command value
# -DUSB_DEVICE_SUPPORT=[ON|OFF].
#

set(OS_LIST_USB_NOT_ALLOWED
  windowsstore
  darwin_embedded
  android
)

if(${CORE_SYSTEM_NAME} IN_LIST OS_LIST_USB_NOT_ALLOWED)
  set(USB_ALLOWED OFF)
else()
  set(USB_ALLOWED ON)
endif()

option(USB_DEVICE_SUPPORT "Build add-on with USB device support, if set to \"OFF\" RTL-SDR can only be accessed via network" ${USB_ALLOWED})
message(STATUS "RTL-SDR USB support is set to '${USB_DEVICE_SUPPORT}'")

# Set values used about configure of settings.xml.in on add-on
if(NOT USB_DEVICE_SUPPORT)
  set(SETTINGS_XML_IN_CONNECTION_TYPE_NOTE "USB device not supported on this add-on, due to this the following setting is not usable and marked as not visible.")
  set(SETTINGS_XML_IN_CONNECTION_TYPE_DEFAULT "1")
  set(SETTINGS_XML_IN_CONNECTION_TYPE_VISIBLE "false")
  set(SETTINGS_XML_IN_NO_USB_BEGIN "<!--")
  set(SETTINGS_XML_IN_NO_USB_END "-->")
else()
  set(SETTINGS_XML_IN_CONNECTION_TYPE_NOTE "USB device supported on this add-on.")
  set(SETTINGS_XML_IN_CONNECTION_TYPE_DEFAULT "0")
  set(SETTINGS_XML_IN_CONNECTION_TYPE_VISIBLE "true")
  set(SETTINGS_XML_IN_NO_USB_BEGIN "")
  set(SETTINGS_XML_IN_NO_USB_END "")
endif()

unset(OS_LIST_USB_NOT_ALLOWED)
unset(USB_ALLOWED)
