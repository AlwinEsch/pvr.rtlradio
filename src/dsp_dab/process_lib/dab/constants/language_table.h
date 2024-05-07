#pragma once

#include <string>
#include <unordered_map>
#include "../database/dab_database_types.h"

// Referring to database/dab_database_types.h
// language_id_t: 8bits

struct DAB_LANGUAGE_NAME
{
  std::string fullname;
  std::string rfc5646;
};

// DOC: ETSI TS 101 756
const auto DAB_LANGUAGE_TABLE = std::unordered_map<language_id_t, const DAB_LANGUAGE_NAME>{
    // Table 9: European Latin-based languages
    { 0x00, { "Unknown", "" }},
    { 0x01, { "Albanian", "sq" }},
    { 0x02, { "Breton", "br" }},
    { 0x03, { "Catalan", "ca" }},
    { 0x04, { "Croatian", "hr" }},
    { 0x05, { "Welsh", "cy" }},
    { 0x06, { "Czech", "cs" }},
    { 0x07, { "Danish", "da" }},
    { 0x08, { "German", "de" }},
    { 0x09, { "English", "en" }},
    { 0x0A, { "Spanish", "es" }},
    { 0x0B, { "Esperanto", "eo" }},
    { 0x0C, { "Estonian", "et" }},
    { 0x0D, { "Basque", "eu" }},
    { 0x0E, { "Faroese", "fo" }},
    { 0x0F, { "French", "fr" }},
    { 0x10, { "Frisian", "fy" }},
    { 0x11, { "Irish", "ga" }},
    { 0x12, { "Gaelic", "gd" }},
    { 0x13, { "Galician", "gl" }},
    { 0x14, { "Icelandic", "is" }},
    { 0x15, { "Italian", "it" }},
    { 0x16, { "Sami", "se" }},
    { 0x17, { "Latin", "la" }},
    { 0x18, { "Latvian", "lv" }},
    { 0x19, { "Luxembourgian", "lb" }},
    { 0x1A, { "Lithuanian", "lt" }},
    { 0x1B, { "Hungarian", "hu" }},
    { 0x1C, { "Maltese", "mt" }},
    { 0x1D, { "Dutch", "nl" }},
    { 0x1E, { "Norwegian", "no" }},
    { 0x1F, { "Occitan", "oc" }},
    { 0x20, { "Polish", "pl" }},
    { 0x21, { "Portuguese", "pt" }},
    { 0x22, { "Romanian", "ro" }},
    { 0x23, { "Romansh", "rm" }},
    { 0x24, { "Serbian", "sr" }},
    { 0x25, { "Slovak", "sk" }},
    { 0x26, { "Slovene", "sl" }},
    { 0x27, { "Finnish", "fi" }},
    { 0x28, { "Swedish", "sv" }},
    { 0x29, { "Turkish", "tr" }},
    { 0x2A, { "Flemish", "vgt" }},
    { 0x2B, { "Walloon", "wa" }},
    { 0x2C, { "RFU", "" }},
    { 0x2D, { "RFU", "" }},
    { 0x2E, { "RFU", "" }},
    { 0x2F, { "RFU", "" }},
    { 0x30, { "Reserved national", "" }},
    { 0x31, { "Reserved national", "" }},
    { 0x32, { "Reserved national", "" }},
    { 0x33, { "Reserved national", "" }},
    { 0x34, { "Reserved national", "" }},
    { 0x35, { "Reserved national", "" }},
    { 0x36, { "Reserved national", "" }},
    { 0x37, { "Reserved national", "" }},
    { 0x38, { "Reserved national", "" }},
    { 0x39, { "Reserved national", "" }},
    { 0x3A, { "Reserved national", "" }},
    { 0x3B, { "Reserved national", "" }},
    { 0x3C, { "Reserved national", "" }},
    { 0x3D, { "Reserved national", "" }},
    { 0x3E, { "Reserved national", "" }},
    { 0x3F, { "Reserved national", "" }},

    // Table 10: Other languages
    { 0x40, { "Background sound/clean feed", "" }},
    { 0x41, { "rfu", "" }},
    { 0x42, { "rfu", "" }},
    { 0x43, { "rfu", "" }},
    { 0x44, { "rfu", "" }},
    { 0x45, { "Zulu", "zu" }},
    { 0x46, { "Vietnamese", "vi" }},
    { 0x47, { "Uzbek", "uz" }},
    { 0x48, { "Urdu", "ur" }},
    { 0x49, { "Ukranian", "uk" }},
    { 0x4A, { "Thai", "th" }},
    { 0x4B, { "Telugu", "te" }},
    { 0x4C, { "Tatar", "tt" }},
    { 0x4D, { "Tamil", "ta" }},
    { 0x4E, { "Tadzhik", "" }},
    { 0x4F, { "Swahili", "sw" }},
    { 0x50, { "Sranan Tongo", "srn" }},
    { 0x51, { "Somali", "so" }},
    { 0x52, { "Sinhalese", "si" }},
    { 0x53, { "Shona", "sn" }},
    { 0x54, { "Serbo-Croat", "hrv" }},
    { 0x55, { "Rusyn", "rue" }},
    { 0x56, { "Russian", "ru" }},
    { 0x57, { "Quechua", "qu" }},
    { 0x58, { "Pushto", "ps" }},
    { 0x59, { "Punjabi", "pa" }},
    { 0x5A, { "Persian", "fa" }},
    { 0x5B, { "Papiamento", "pap" }},
    { 0x5C, { "Oriya", "or" }},
    { 0x5D, { "Nepali", "ne" }},
    { 0x5E, { "Ndebele", "" }},
    { 0x5F, { "Marathi", "mr" }},
    { 0x60, { "Moldavian", "mo" }},
    { 0x61, { "Malaysian", "ms" }},
    { 0x62, { "Malagasy", "mg" }},
    { 0x63, { "Macedonian", "mk" }},
    { 0x64, { "Laotian", "lo" }},
    { 0x65, { "Korean", "ko" }},
    { 0x66, { "Khmer", "km" }},
    { 0x67, { "Kazakh", "kk" }},
    { 0x68, { "Kannada", "kn" }},
    { 0x69, { "Japanese", "ja" }},
    { 0x6A, { "Indonesian", "id" }},
    { 0x6B, { "Hindi", "hi" }},
    { 0x6C, { "Hebrew", "he" }},
    { 0x6D, { "Hausa", "ha" }},
    { 0x6E, { "Gurani", "hac" }},
    { 0x6F, { "Gujurati", "gu" }},
    { 0x70, { "Greek", "el" }},
    { 0x71, { "Georgian", "ka" }},
    { 0x72, { "Fulani", "" }},
    { 0x73, { "Dari", "prs" }},
    { 0x74, { "Chuvash", "cv" }},
    { 0x75, { "Chinese", "zh" }},
    { 0x76, { "Burmese", "my" }},
    { 0x77, { "Bulgarian", "bg" }},
    { 0x78, { "Bengali", "bn" }},
    { 0x79, { "Belorussian", "be" }},
    { 0x7A, { "Bambora", "bm" }},
    { 0x7B, { "Azerbaijani", "az" }},
    { 0x7C, { "Assamese", "as" }},
    { 0x7D, { "Armenian", "hy" }},
    { 0x7E, { "Arabic", "ar" }},
    { 0x7F, { "Amharic", "am" }},
};

enum class LanguageNameType : uint8_t
{
  FULLNAME,
  RFC5646
};

static const std::string& GetLanguageName(language_id_t language,
                                          LanguageNameType type = LanguageNameType::FULLNAME) {
    static const auto DEFAULT_RETURN = std::string("Undefined");
    auto res = DAB_LANGUAGE_TABLE.find(language);
    if (res == DAB_LANGUAGE_TABLE.end()) {
        return DEFAULT_RETURN;
    }
    if (type == LanguageNameType::RFC5646)
      return res->second.rfc5646;
    return res->second.fullname;
}