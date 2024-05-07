/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include <map>
#include <memory>
#include <string_view>
#include <vector>

#define KiB *(1 << 10)
#define MiB *(1 << 20)
#define GiB *(1 << 30)

#define KHz *(1000)
#define MHz *(1000000)

namespace RTLRADIO
{

constexpr const char* PVR_STREAM_PROPERTY_UNIQUEID = "pvr.rtlradio.uniqueid";
constexpr const char* PVR_STREAM_PROPERTY_FREQUENCY = "pvr.rtlradio.frequency";
constexpr const char* PVR_STREAM_PROPERTY_SUBCHANNEL = "pvr.rtlradio.subchannel";
constexpr const char* PVR_STREAM_PROPERTY_MODULATION = "pvr.rtlradio.modulation";

enum class Modulation : uint8_t
{
  UNDEFINED = 0x1F,
  ALL = 0xFF,

  //! Medium wave (MW) for AM radio broadcasting
  MW = 0,

  //! Frequency modulation (FM)
  FM = 1,

  //! Digital Audio Broadcast radio (DAB/DAB+)
  DAB = 2,

  //! Hybrid Digital radio
  HD = 3,

  //! VHF Weather radio
  WX = 4
};

enum class TransportMode : uint8_t
{
  STREAM_MODE_AUDIO = 0,
  STREAM_MODE_DATA = 1,
  PACKET_MODE_DATA = 2,
  UNDEFINED = 0xFF,
};

enum class ProgrammType : uint8_t
{
  NONE = 0,
  NEWS = 1,
  CURRENT_AFFAIRS = 2,
  INFORMATION = 3,
  SPORT = 4,
  EDUCATION = 5,
  DRAMA = 6,
  ARTS = 7,
  SCIENCE = 8,
  TALK = 9,
  POP_MUSIC = 10,
  ROCK_MUSIC = 11,
  EASY_LISTENING = 12,
  LIGHT_CLASSICAL = 13,
  CLASSICAL_MUSIC = 14,
  MUSIC = 15,
  WEATHER = 16,
  FINANCE = 17,
  CHILDREN = 18,
  FACTUAL = 19,
  RELIGION = 20,
  PHONE_IN = 21,
  TRAVEL = 22,
  LEISURE = 23,
  JAZZ_AND_BLUES = 24,
  COUNTRY_MUSIC = 25,
  NATIONAL_MUSIC = 26,
  OLDIES_MUSIC = 27,
  FOLK_MUSIC = 28,
  DOCUMENTARY = 29,
  UNDEFINED = 0xFF,
};

enum class CountryCode
{
  GERMANY,
  ALGERIA,
  ANDORRA,
  ISRAEL,
  ITALY,
  BELGIUM,
  RUSSIAN_FEDERATION,
  PALESTINE,
  ALBANIA,
  AUSTRIA,
  HUNGARY,
  MALTA,
  EGYPT,
  GREECE,
  CYPRUS,
  SAN_MARINO,
  SWITZERLAND,
  JORDAN,
  FINLAND,
  LUXEMBOURG,
  BULGARIA,
  DENMARK,
  FAROE,
  GIBRALTAR,
  IRAQ,
  UNITED_KINGDOM,
  LIBYA,
  ROMANIA,
  FRANCE,
  MOROCCO,
  CZECH_REPUBLIC,
  POLAND,
  VATICAN,
  SLOVAKIA,
  SYRIA,
  TUNISIA,
  LIECHTENSTEIN,
  ICELAND,
  MONACO,
  LITHUANIA,
  SERBIA,
  CANARY_ISLANDS,
  SPAIN,
  NORWAY,
  MONTENEGRO,
  IRELAND,
  TURKEY,
  TAJIKISTAN,
  NETHERLANDS,
  LATVIA,
  LEBANON,
  AZERBAIJAN,
  CROATIA,
  KAZAKHSTAN,
  SWEDEN,
  BELARUS,
  MOLDOVA,
  ESTONIA,
  MACEDONIA,
  UKRAINE,
  KOSOVO,
  AZORES,
  MADEIRA,
  PORTUGAL,
  SLOVENIA,
  ARMENIA,
  UZBEKISTAN,
  GEORGIA,
  TURKMENISTAN,
  BOSNIA_HERZEGOVINA,
  KYRGYZSTAN,
  CAMEROON,
  CENTRAL_AFRICAN_REPUBLIC,
  DJIBOUTI,
  MADAGASCAR,
  MALI,
  ANGOLA,
  EQUATORIAL_GUINEA,
  GABON,
  REPUBLIC_OF_GUINEA,
  SOUTH_AFRICA,
  BURKINA_FASO,
  CONGO,
  TOGO,
  BENIN,
  MALAWI,
  NAMIBIA,
  LIBERIA,
  GHANA,
  MAURITANIA,
  SAO_TOME_AND_PRINCIPE,
  CAPE_VERDE,
  SENEGAL,
  GAMBIA,
  BURUNDI,
  ASCENSION_ISLAND,
  BOTSWANA,
  COMOROS,
  TANZANIA,
  ETHIOPIA,
  NIGERIA,
  SIERRA_LEONE,
  ZIMBABWE,
  MOZAMBIQUE,
  UGANDA,
  SWAZILAND,
  KENYA,
  SOMALIA,
  NIGER,
  CHAD,
  GUINEA_BISSAU,
  ZAIRE,
  COTE_D_IVOIRE,
  ZANZIBAR,
  ZAMBIA,
  WESTERN_SAHARA,
  RWANDA,
  LESOTHO,
  SEYCHELLES,
  MAURITIUS,
  SUDAN,
  UNITED_STATES_OF_AMERICA,
  PUERTO_RICO,
  VIRGIN_ISLANDS_USA,
  CANADA,
  GREENLAND,
  ANGUILLA,
  ANTIGUA_AND_BARBUDA,
  ECUADOR,
  FALKLAND_ISLANDS,
  BARBADOS,
  BELIZE,
  CAYMAN_ISLANDS,
  COSTA_RICA,
  CUBA,
  ARGENTINA,
  BRAZIL,
  BERMUDA,
  NETHERLANDS_ANTILLES,
  GUADELOUPE,
  BAHAMAS,
  BOLIVIA,
  COLOMBIA,
  JAMAICA,
  MARTINIQUE,
  PARAGUAY,
  NICARAGUA,
  PANAMA,
  DOMINICA,
  DOMINICAN_REPUBLIC,
  CHILE,
  GRENADA,
  TURKS_AND_CAICOS_ISLANDS,
  GUYANA,
  GUATEMALA,
  HONDURAS,
  ARUBA,
  MONTSERRAT,
  TRINIDAD_AND_TOBAGO,
  PERU,
  SURINAM,
  URUGUAY,
  ST_KITTS,
  ST_LUCIA,
  EL_SALVADOR,
  HAITI,
  VENEZUELA,
  MEXICO,
  ST_VINCENT,
  VIRGIN_ISLANDS_BRITISH,
  ST_PIERRE_AND_MIQUELON,
  LAOS,
  AUSTRALIA,
  VANUATU,
  YEMEN,
  SRI_LANKA,
  BRUNEI_DARUSSALAM,
  JAPAN,
  FIJI,
  IRAN,
  KOREA_SOUTH,
  CAMBODIA,
  HONG_KONG,
  SOLOMON_ISLANDS,
  BAHRAIN,
  WESTERN_SAMOA,
  TAIWAN,
  MALAYSIA,
  SINGAPORE,
  PAKISTAN,
  CHINA,
  MYANMAR_BURMA,
  NAURU,
  KIRIBATI,
  BANGLADESH,
  VIETNAM,
  PHILIPPINES,
  BHUTAN,
  OMAN,
  NEPAL,
  UNITED_ARAB_EMIRATES,
  KUWAIT,
  QATAR,
  KOREA_NORTH,
  NEW_ZEALAND,
  TONGA,
  MICRONESIA,
  MACAU,
  INDIA,
  SAUDI_ARABIA,
  MONGOLIA,
  MALDIVES,
  PAPUA_NEW_GUINEA,
  AFGHANISTAN,
  INDONESIA,
  THAILAND,
  UNDEFINED = 0xFF,
};

enum class LanguageCode
{
  UNKNOWN,
  ALBANIAN,
  BRETON,
  CATALAN,
  CROATIAN,
  WELSH,
  CZECH,
  DANISH,
  GERMAN,
  ENGLISH,
  SPANISH,
  ESPERANTO,
  ESTONIAN,
  BASQUE,
  FAROESE,
  FRENCH,
  FRISIAN,
  IRISH,
  GAELIC,
  GALICIAN,
  ICELANDIC,
  ITALIAN,
  SAMI,
  LATIN,
  LATVIAN,
  LUXEMBOURGIAN,
  LITHUANIAN,
  HUNGARIAN,
  MALTESE,
  DUTCH,
  NORWEGIAN,
  OCCITAN,
  POLISH,
  PORTUGUESE,
  ROMANIAN,
  ROMANSH,
  SERBIAN,
  SLOVAK,
  SLOVENE,
  FINNISH,
  SWEDISH,
  TURKISH,
  FLEMISH,
  ZULU,
  VIETNAMESE,
  UZBEK,
  URDU,
  UKRANIAN,
  THAI,
  TELUGU,
  TATAR,
  TAMIL,
  TADZHIK,
  SWAHILI,
  SRANAN_TONGO,
  SOMALI,
  SINHALESE,
  SHONA,
  SERBO_CROAT,
  RUSYN,
  RUSSIAN,
  QUECHUA,
  PUSHTU,
  PUNJABI,
  PERSIAN,
  PAPIAMENTO,
  ORIYA,
  NEPALI,
  NDEBELE,
  MARATHI,
  MOLDAVIAN,
  MALAYSIAN,
  MALAGASAY,
  MACEDONIAN,
  LAOTIAN,
  KOREAN,
  KHMER,
  KAZAKH,
  KANNADA,
  JAPANESE,
  INDONESIAN,
  HINDI,
  HEBREW,
  HAUSA,
  GURANI,
  GUJURATI,
  GREEK,
  GEORGIAN,
  FULANI,
  DARI,
  CHUVASH,
  CHINESE,
  BURMESE,
  BULGARIAN,
  BENGALI,
  BELORUSSIAN,
  BAMBORA,
  AZERBAIJANI,
  ASSAMESE,
  ARMENIAN,
  ARABIC,
  AMHARIC,
  UNDEFINED = 0xFF,
};

union channelid_t
{
  struct
  {
    // FFFFFFFFFFFFFFFFFFFF SSSSSSSS MMMM (little endian)
    //
    unsigned int modulation : 4; // Modulation (0-15)
    unsigned int subchannel : 8; // Subchannel (0-255)
    unsigned int frequency : 20; // Frequency in KHz (0-1048575)
  } parts;

  unsigned int value; // Complete channel id
};

static_assert(sizeof(channelid_t) == sizeof(uint32_t),
              "channelid_t union must be same size as a uint32_t");

class CChannelId
{
public:
  CChannelId(unsigned int channelid) { m_channelid.value = channelid; }

  CChannelId(int frequency, enum Modulation modulation)
  {
    m_channelid.parts.frequency = static_cast<unsigned int>(frequency / 1000);
    m_channelid.parts.subchannel = 0;
    m_channelid.parts.modulation = static_cast<unsigned int>(modulation);
  }

  CChannelId(int frequency, int subchannel, enum Modulation modulation)
  {
    m_channelid.parts.frequency = static_cast<unsigned int>(frequency / 1000);
    m_channelid.parts.subchannel = static_cast<unsigned int>(subchannel);
    m_channelid.parts.modulation = static_cast<unsigned int>(modulation);
  }

  CChannelId(const CChannelId& other) { m_channelid.value = other.m_channelid.value; }

  bool operator==(const CChannelId& other) const
  {
    return (m_channelid.value == other.m_channelid.value);
  }

  unsigned int Frequency() const
  {
    return static_cast<unsigned int>(m_channelid.parts.frequency) * 1000;
  }

  unsigned int Id() const { return m_channelid.value; }

  enum Modulation Modulation() const
  {
    return static_cast<enum Modulation>(m_channelid.parts.modulation);
  }

  unsigned int SubChannel() const
  {
    return static_cast<unsigned int>(m_channelid.parts.subchannel);
  }

private:
  CChannelId() = delete;

  channelid_t m_channelid; // Underlying channel identifier
};

struct ChannelProps
{
  ChannelProps() = delete;

  ChannelProps(unsigned int channelid)
    : id(channelid),
      frequency(id.Frequency()),
      subchannelnumber(id.SubChannel()),
      modulation(id.Modulation())
  {
  }

  ChannelProps(int frequency, enum Modulation modulation)
    : id(frequency, modulation), frequency(frequency), modulation(modulation)
  {
  }

  ChannelProps(int frequency, int subchannel, enum Modulation modulation)
    : id(frequency, subchannel, modulation),
      frequency(frequency),
      subchannelnumber(subchannel),
      modulation(modulation)
  {
  }

  bool ChannelProps::operator==(const ChannelProps& other) const
  {
    return (channelnumber == other.channelnumber) && (subchannelnumber == other.subchannelnumber) &&
           (frequency == other.frequency) && (modulation == other.modulation) &&
           (name == other.name) && (provider_id == other.provider_id) &&
           (logourl == other.logourl) && (programmtype == other.programmtype) &&
           (country == other.country) && (language == other.language) &&
           (transportmode == other.transportmode) && (mimetype == other.mimetype) &&
           (visible == other.visible);
  }

  struct Fallback
  {
    Fallback(uint32_t freq, enum Modulation mod, uint32_t id)
      : frequency(freq), modulation(mod), service_id(id)
    {
    }
    uint32_t frequency;
    enum Modulation modulation;
    uint32_t service_id;
  };

  CChannelId id;
  uint32_t channelnumber;
  uint32_t subchannelnumber;
  uint32_t frequency;
  enum Modulation modulation;
  std::string name;
  std::string usereditname;
  std::string provider;
  uint32_t provider_id;
  std::string logourl;
  std::string userlogourl;
  enum ProgrammType programmtype;
  std::string country;
  std::string language;
  enum TransportMode transportmode;
  std::string mimetype;
  std::vector<Fallback> fallbacks;
  bool notpublic;
  bool visible;

  bool autogain{false}; // Flag indicating if automatic gain should be used
  int manualgain{0}; // Manual gain value as 10*dB (i.e. 32.8dB = 328)
  int freqcorrection{0}; // Frequency correction for this channel
};

struct ProviderProps
{
  ProviderProps() = default;

  ProviderProps(const ProviderProps& other) { *this = other; }

  ProviderProps& operator=(const ProviderProps& other)
  {
    // Guard self assignment
    if (this == &other)
      return *this;

    id = other.id;
    name = other.name;
    logourl = other.logourl;
    country = other.country;
    language = other.language;
    return *this;
  }

  bool operator==(const ProviderProps& other)
  {
    // Guard self assignment
    if (this == &other)
      return true;

    if (id == other.id && name == other.name && logourl == other.logourl &&
        country == other.country && language == other.language)
      return true;
    return false;
  }

  unsigned int id;
  std::string name;
  std::string logourl;

  //! @brief ISO 3166 country codes, separated by PROVIDER_STRING_TOKEN_SEPARATOR
  /// (e.g 'GB,IE,FR,CA'), an empty string means this value is undefined
  std::string country;

  //! @brief RFC 5646 language codes, separated by PROVIDER_STRING_TOKEN_SEPARATOR
  /// (e.g. 'en_GB,fr_CA'), an empty string means this value is undefined
  std::string language;
};

} // namespace RTLRADIO
