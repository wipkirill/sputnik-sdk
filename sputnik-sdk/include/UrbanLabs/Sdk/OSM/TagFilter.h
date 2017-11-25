#pragma once

#include <map>
#include <set>
#include <utility>
#include <string>
#include <vector>
#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/Utils/StringUtils.h>
#include <UrbanLabs/Sdk/Utils/Logger.h>
#include <UrbanLabs/Sdk/Config/XmlConfig.h>

using std::set;
using std::map;
using std::unordered_map;
using std::pair;
using std::string;
using std::vector;

/**
 * the default tag filter
 * @brief The TagFilter class
 */
class TagFilter {
public:
    typedef std::unordered_map<std::string, std::set<std::string> > AssocArray;
protected:
    set<string> splittableTags_;
    set<string> userEnteredProps_;
    set<string> nonSplittableTags_;
    static AssocArray fixedKeyValue_;
    map<string, set<string> > notWritableKeyValue_;
    map<string, set<string> > aloneDeprecatedKeyValue_;

    set<string> addressTags_;
    set<string> addressArea_;
    set<string> addressDecodable_;
    map<string, int> addressAreaLevel_;
    vector<vector<string> > addressAreasHierarchy_;

    set<string> aggregateTags_;
    set<string> uniqueKeyValues_;
    set<string> ignoredMetaValues_;
    set<string> nonFullTextKeys_;
    set<string> fullTextKeys_;

    set<string> turnRestrictions_;
    set<string> countryCodes_, countryCodesLower_;
    map<pair<string,string>,pair<string,string> > canonicalTags_;
public:
    TagFilter() {

        notWritableKeyValue_ = {{"building",{"yes"}},
                                {"shop", {"yes"}},
                                {"office", {"yes"}},
                                {"amenity", {"public_building"}},
                                {"ref", {"*"}}};

        ignoredMetaValues_ = {"yes","no","limited","true","false","maybe","1","0","german"};

        aloneDeprecatedKeyValue_ = {{"amenity", {"parking"}},
                                    {"highway", {"*"}},
                                    {"ref", {"*"}}};

        userEnteredProps_ = {"email",
                             "fax",
                             "phone",
                             "website",
                             "wikipedia",
                             "opening_hours",
                             "operator",

                             "name",
                             "ref",
                             "postal_code",
                             "alt_name",
                             "int_name",
                             "loc_name",
                             "nat_name",
                             "official_name",
                             "old_name",
                             "reg_name",
                             "short_name"};

        addressDecodable_ = {"addr:country",
                             "addr:state",
                             "addr:city",
                             "addr:suburb",
                             "addr:district",
                             "addr:postcode",
                             "addr:zip",
                             "addr:street",
                             "addr:housenumber"};

        addressTags_ = {"addr:housenumber",
                        "addr:housename",
                        "addr:street",
                        "addr:place",
                        "addr:postcode",
                        "addr:city",
                        "addr:country",
                        "addr:suburb",
                        "addr:subdistrict",
                        "addr:district",
                        "addr:hamlet"
                        "addr:province",
                        "addr:state",
                        "addr:full"};

        splittableTags_ = {"name",
                           "alt_name",
                           "int_name",
                           "loc_name",
                           "nat_name",
                           "official_name",
                           "old_name",
                           "reg_name",
                           "short_name",
                           "operator"};
        splittableTags_.insert(addressTags_.begin(), addressTags_.end());

        // http://www.iso.org/iso/country_codes/iso_3166_code_lists/country_names_and_code_elements.htm
        countryCodes_ ={"AF",//AFGHANISTAN
                        "AX",//ÅLAND ISLANDS
                        "AL",//ALBANIA
                        "DZ",//ALGERIA
                        "AS",//AMERICAN SAMOA
                        "AD",//ANDORRA
                        "AO",//ANGOLA
                        "AI",//ANGUILLA
                        "AQ",//ANTARCTICA
                        "AG",//ANTIGUA AND BARBUDA
                        "AR",//ARGENTINA
                        "AM",//ARMENIA
                        "AW",//ARUBA
                        "AU",//AUSTRALIA
                        "AT",//AUSTRIA
                        "AZ",//AZERBAIJAN
                        "BS",//BAHAMAS
                        "BH",//BAHRAIN
                        "BD",//BANGLADESH
                        "BB",//BARBADOS
                        "BY",//BELARUS
                        "BE",//BELGIUM
                        "BZ",//BELIZE
                        "BJ",//BENIN
                        "BM",//BERMUDA
                        "BT",//BHUTAN
                        "BO",//BOLIVIA, PLURINATIONAL STATE OF
                        "BQ",//BONAIRE, SINT EUSTATIUS AND SABA
                        "BA",//BOSNIA AND HERZEGOVINA
                        "BW",//BOTSWANA
                        "BV",//BOUVET ISLAND
                        "BR",//BRAZIL
                        "IO",//BRITISH INDIAN OCEAN TERRITORY
                        "BN",//BRUNEI DARUSSALAM
                        "BG",//BULGARIA
                        "BF",//BURKINA FASO
                        "BI",//BURUNDI
                        "KH",//CAMBODIA
                        "CM",//CAMEROON
                        "CA",//CANADA
                        "CV",//CAPE VERDE
                        "KY",//CAYMAN ISLANDS
                        "CF",//CENTRAL AFRICAN REPUBLIC
                        "TD",//CHAD
                        "CL",//CHILE
                        "CN",//CHINA
                        "CX",//CHRISTMAS ISLAND
                        "CC",//COCOS (KEELING) ISLANDS
                        "CO",//COLOMBIA
                        "KM",//COMOROS
                        "CG",//CONGO
                        "CD",//CONGO, THE DEMOCRATIC REPUBLIC OF THE
                        "CK",//COOK ISLANDS
                        "CR",//COSTA RICA
                        "CI",//CÔTE D'IVOIRE
                        "HR",//CROATIA
                        "CU",//CUBA
                        "CW",//CURAÇAO
                        "CY",//CYPRUS
                        "CZ",//CZECH REPUBLIC
                        "DK",//DENMARK
                        "DJ",//DJIBOUTI
                        "DM",//DOMINICA
                        "DO",//DOMINICAN REPUBLIC
                        "EC",//ECUADOR
                        "EG",//EGYPT
                        "SV",//EL SALVADOR
                        "GQ",//EQUATORIAL GUINEA
                        "ER",//ERITREA
                        "EE",//ESTONIA
                        "ET",//ETHIOPIA
                        "FK",//FALKLAND ISLANDS (MALVINAS)
                        "FO",//FAROE ISLANDS
                        "FJ",//FIJI
                        "FI",//FINLAND
                        "FR",//FRANCE
                        "GF",//FRENCH GUIANA
                        "PF",//FRENCH POLYNESIA
                        "TF",//FRENCH SOUTHERN TERRITORIES
                        "GA",//GABON
                        "GM",//GAMBIA
                        "GE",//GEORGIA
                        "DE",//GERMANY
                        "GH",//GHANA
                        "GI",//GIBRALTAR
                        "GR",//GREECE
                        "GL",//GREENLAND
                        "GD",//GRENADA
                        "GP",//GUADELOUPE
                        "GU",//GUAM
                        "GT",//GUATEMALA
                        "GG",//GUERNSEY
                        "GN",//GUINEA
                        "GW",//GUINEA-BISSAU
                        "GY",//GUYANA
                        "HT",//HAITI
                        "HM",//HEARD ISLAND AND MCDONALD ISLANDS
                        "VA",//HOLY SEE (VATICAN CITY STATE)
                        "HN",//HONDURAS
                        "HK",//HONG KONG
                        "HU",//HUNGARY
                        "IS",//ICELAND
                        "IN",//INDIA
                        "ID",//INDONESIA
                        "IR",//IRAN, ISLAMIC REPUBLIC OF
                        "IQ",//IRAQ
                        "IE",//IRELAND
                        "IM",//ISLE OF MAN
                        "IL",//ISRAEL
                        "IT",//ITALY
                        "JM",//JAMAICA
                        "JP",//JAPAN
                        "JE",//JERSEY
                        "JO",//JORDAN
                        "KZ",//KAZAKHSTAN
                        "KE",//KENYA
                        "KI",//KIRIBATI
                        "KP",//KOREA, DEMOCRATIC PEOPLE'S REPUBLIC OF
                        "KR",//KOREA, REPUBLIC OF
                        "KW",//KUWAIT
                        "KG",//KYRGYZSTAN
                        "LA",//LAO PEOPLE'S DEMOCRATIC REPUBLIC
                        "LV",//LATVIA
                        "LB",//LEBANON
                        "LS",//LESOTHO
                        "LR",//LIBERIA
                        "LY",//LIBYA
                        "LI",//LIECHTENSTEIN
                        "LT",//LITHUANIA
                        "LU",//LUXEMBOURG
                        "MO",//MACAO
                        "MK",//MACEDONIA, THE FORMER YUGOSLAV REPUBLIC OF
                        "MG",//MADAGASCAR
                        "MW",//MALAWI
                        "MY",//MALAYSIA
                        "MV",//MALDIVES
                        "ML",//MALI
                        "MT",//MALTA
                        "MH",//MARSHALL ISLANDS
                        "MQ",//MARTINIQUE
                        "MR",//MAURITANIA
                        "MU",//MAURITIUS
                        "YT",//MAYOTTE
                        "MX",//MEXICO
                        "FM",//MICRONESIA, FEDERATED STATES OF
                        "MD",//MOLDOVA, REPUBLIC OF
                        "MC",//MONACO
                        "MN",//MONGOLIA
                        "ME",//MONTENEGRO
                        "MS",//MONTSERRAT
                        "MA",//MOROCCO
                        "MZ",//MOZAMBIQUE
                        "MM",//MYANMAR
                        "NA",//NAMIBIA
                        "NR",//NAURU
                        "NP",//NEPAL
                        "NL",//NETHERLANDS
                        "NC",//NEW CALEDONIA
                        "NZ",//NEW ZEALAND
                        "NI",//NICARAGUA
                        "NE",//NIGER
                        "NG",//NIGERIA
                        "NU",//NIUE
                        "NF",//NORFOLK ISLAND
                        "MP",//NORTHERN MARIANA ISLANDS
                        "NO",//NORWAY
                        "OM",//OMAN
                        "PK",//PAKISTAN
                        "PW",//PALAU
                        "PS",//PALESTINE, STATE OF
                        "PA",//PANAMA
                        "PG",//PAPUA NEW GUINEA
                        "PY",//PARAGUAY
                        "PE",//PERU
                        "PH",//PHILIPPINES
                        "PN",//PITCAIRN
                        "PL",//POLAND
                        "PT",//PORTUGAL
                        "PR",//PUERTO RICO
                        "QA",//QATAR
                        "RE",//RÉUNION
                        "RO",//ROMANIA
                        "RU",//RUSSIAN FEDERATION
                        "RW",//RWANDA
                        "BL",//SAINT BARTHÉLEMY
                        "SH",//SAINT HELENA, ASCENSION AND TRISTAN DA CUNHA
                        "KN",//SAINT KITTS AND NEVIS
                        "LC",//SAINT LUCIA
                        "MF",//SAINT MARTIN (FRENCH PART)
                        "PM",//SAINT PIERRE AND MIQUELON
                        "VC",//SAINT VINCENT AND THE GRENADINES
                        "WS",//SAMOA
                        "SM",//SAN MARINO
                        "ST",//SAO TOME AND PRINCIPE
                        "SA",//SAUDI ARABIA
                        "SN",//SENEGAL
                        "RS",//SERBIA
                        "SC",//SEYCHELLES
                        "SL",//SIERRA LEONE
                        "SG",//SINGAPORE
                        "SX",//SINT MAARTEN (DUTCH PART)
                        "SK",//SLOVAKIA
                        "SI",//SLOVENIA
                        "SB",//SOLOMON ISLANDS
                        "SO",//SOMALIA
                        "ZA",//SOUTH AFRICA
                        "GS",//SOUTH GEORGIA AND THE SOUTH SANDWICH ISLANDS
                        "SS",//SOUTH SUDAN
                        "ES",//SPAIN
                        "LK",//SRI LANKA
                        "SD",//SUDAN
                        "SR",//SURINAME
                        "SJ",//SVALBARD AND JAN MAYEN
                        "SZ",//SWAZILAND
                        "SE",//SWEDEN
                        "CH",//SWITZERLAND
                        "SY",//SYRIAN ARAB REPUBLIC
                        "TW",//TAIWAN, PROVINCE OF CHINA
                        "TJ",//TAJIKISTAN
                        "TZ",//TANZANIA, UNITED REPUBLIC OF
                        "TH",//THAILAND
                        "TL",//TIMOR-LESTE
                        "TG",//TOGO
                        "TK",//TOKELAU
                        "TO",//TONGA
                        "TT",//TRINIDAD AND TOBAGO
                        "TN",//TUNISIA
                        "TR",//TURKEY
                        "TM",//TURKMENISTAN
                        "TC",//TURKS AND CAICOS ISLANDS
                        "TV",//TUVALU
                        "UG",//UGANDA
                        "UA",//UKRAINE
                        "AE",//UNITED ARAB EMIRATES
                        "GB",//UNITED KINGDOM
                        "US",//UNITED STATES
                        "UM",//UNITED STATES MINOR OUTLYING ISLANDS
                        "UY",//URUGUAY
                        "UZ",//UZBEKISTAN
                        "VU",//VANUATU
                        "VE",//VENEZUELA, BOLIVARIAN REPUBLIC OF
                        "VN",//VIET NAM
                        "VG",//VIRGIN ISLANDS, BRITISH
                        "VI",//VIRGIN ISLANDS, U.S.
                        "WF",//WALLIS AND FUTUNA
                        "EH",//WESTERN SAHARA
                        "YE",//YEMEN
                        "ZM",//ZAMBIA
                        "ZW"//ZIMBABWE
                       };

        countryCodesLower_ = {
                        "ad",//andorra
                        "ae",//united arab emirates
                        "af",//afghanistan
                        "ag",//antigua and barbuda
                        "ai",//anguilla
                        "al",//albania
                        "am",//armenia
                        "ao",//angola
                        "aq",//antarctica
                        "ar",//argentina
                        "as",//american samoa
                        "at",//austria
                        "au",//australia
                        "aw",//aruba
                        "ax",//åland islands
                        "az",//azerbaijan
                        "ba",//bosnia and herzegovina
                        "bb",//barbados
                        "bd",//bangladesh
                        "be",//belgium
                        "bf",//burkina faso
                        "bg",//bulgaria
                        "bh",//bahrain
                        "bi",//burundi
                        "bj",//benin
                        "bl",//saint barthélemy
                        "bm",//bermuda
                        "bn",//brunei darussalam
                        "bo",//bolivia, plurinational state of
                        "bq",//bonaire, sint eustatius and saba
                        "br",//brazil
                        "bs",//bahamas
                        "bt",//bhutan
                        "bv",//bouvet island
                        "bw",//botswana
                        "by",//belarus
                        "bz",//belize
                        "ca",//canada
                        "cc",//cocos (keeling) islands
                        "cd",//congo, the democratic republic of the
                        "cf",//central african republic
                        "cg",//congo
                        "ch",//switzerland
                        "ci",//côte d'ivoire
                        "ck",//cook islands
                        "cl",//chile
                        "cm",//cameroon
                        "cn",//china
                        "co",//colombia
                        "cr",//costa rica
                        "cu",//cuba
                        "cv",//cape verde
                        "cw",//curaçao
                        "cx",//christmas island
                        "cy",//cyprus
                        "cz",//czech republic
                        "de",//germany
                        "dj",//djibouti
                        "dk",//denmark
                        "dm",//dominica
                        "do",//dominican republic
                        "dz",//algeria
                        "ec",//ecuador
                        "ee",//estonia
                        "eg",//egypt
                        "eh",//western sahara
                        "er",//eritrea
                        "es",//spain
                        "et",//ethiopia
                        "fi",//finland
                        "fj",//fiji
                        "fk",//falkland islands (malvinas)
                        "fm",//micronesia, federated states of
                        "fo",//faroe islands
                        "fr",//france
                        "ga",//gabon
                        "gb",//united kingdom
                        "gd",//grenada
                        "ge",//georgia
                        "gf",//french guiana
                        "gg",//guernsey
                        "gh",//ghana
                        "gi",//gibraltar
                        "gl",//greenland
                        "gm",//gambia
                        "gn",//guinea
                        "gp",//guadeloupe
                        "gq",//equatorial guinea
                        "gr",//greece
                        "gs",//south georgia and the south sandwich islands
                        "gt",//guatemala
                        "gu",//guam
                        "gw",//guinea-bissau
                        "gy",//guyana
                        "hk",//hong kong
                        "hm",//heard island and mcdonald islands
                        "hn",//honduras
                        "hr",//croatia
                        "ht",//haiti
                        "hu",//hungary
                        "id",//indonesia
                        "ie",//ireland
                        "il",//israel
                        "im",//isle of man
                        "in",//india
                        "io",//british indian ocean territory
                        "iq",//iraq
                        "ir",//iran, islamic republic of
                        "is",//iceland
                        "it",//italy
                        "je",//jersey
                        "jm",//jamaica
                        "jo",//jordan
                        "jp",//japan
                        "ke",//kenya
                        "kg",//kyrgyzstan
                        "kh",//cambodia
                        "ki",//kiribati
                        "km",//comoros
                        "kn",//saint kitts and nevis
                        "kp",//korea, democratic people's republic of
                        "kr",//korea, republic of
                        "kw",//kuwait
                        "ky",//cayman islands
                        "kz",//kazakhstan
                        "la",//lao people's democratic republic
                        "lb",//lebanon
                        "lc",//saint lucia
                        "li",//liechtenstein
                        "lk",//sri lanka
                        "lr",//liberia
                        "ls",//lesotho
                        "lt",//lithuania
                        "lu",//luxembourg
                        "lv",//latvia
                        "ly",//libya
                        "ma",//morocco
                        "mc",//monaco
                        "md",//moldova, republic of
                        "me",//montenegro
                        "mf",//saint martin (french part)
                        "mg",//madagascar
                        "mh",//marshall islands
                        "mk",//macedonia, the former yugoslav republic of
                        "ml",//mali
                        "mm",//myanmar
                        "mn",//mongolia
                        "mo",//macao
                        "mp",//northern mariana islands
                        "mq",//martinique
                        "mr",//mauritania
                        "ms",//montserrat
                        "mt",//malta
                        "mu",//mauritius
                        "mv",//maldives
                        "mw",//malawi
                        "mx",//mexico
                        "my",//malaysia
                        "mz",//mozambique
                        "na",//namibia
                        "nc",//new caledonia
                        "ne",//niger
                        "nf",//norfolk island
                        "ng",//nigeria
                        "ni",//nicaragua
                        "nl",//netherlands
                        "no",//norway
                        "np",//nepal
                        "nr",//nauru
                        "nu",//niue
                        "nz",//new zealand
                        "om",//oman
                        "pa",//panama
                        "pe",//peru
                        "pf",//french polynesia
                        "pg",//papua new guinea
                        "ph",//philippines
                        "pk",//pakistan
                        "pl",//poland
                        "pm",//saint pierre and miquelon
                        "pn",//pitcairn
                        "pr",//puerto rico
                        "ps",//palestine, state of
                        "pt",//portugal
                        "pw",//palau
                        "py",//paraguay
                        "qa",//qatar
                        "re",//réunion
                        "ro",//romania
                        "rs",//serbia
                        "ru",//russian federation
                        "rw",//rwanda
                        "sa",//saudi arabia
                        "sb",//solomon islands
                        "sc",//seychelles
                        "sd",//sudan
                        "se",//sweden
                        "sg",//singapore
                        "sh",//saint helena, ascension and tristan da cunha
                        "si",//slovenia
                        "sj",//svalbard and jan mayen
                        "sk",//slovakia
                        "sl",//sierra leone
                        "sm",//san marino
                        "sn",//senegal
                        "so",//somalia
                        "sr",//suriname
                        "ss",//south sudan
                        "st",//sao tome and principe
                        "sv",//el salvador
                        "sx",//sint maarten (dutch part)
                        "sy",//syrian arab republic
                        "sz",//swaziland
                        "tc",//turks and caicos islands
                        "td",//chad
                        "tf",//french southern territories
                        "tg",//togo
                        "th",//thailand
                        "tj",//tajikistan
                        "tk",//tokelau
                        "tl",//timor-leste
                        "tm",//turkmenistan
                        "tn",//tunisia
                        "to",//tonga
                        "tr",//turkey
                        "tt",//trinidad and tobago
                        "tv",//tuvalu
                        "tw",//taiwan, province of china
                        "tz",//tanzania, united republic of
                        "ua",//ukraine
                        "ug",//uganda
                        "um",//united states minor outlying islands
                        "us",//united states
                        "uy",//uruguay
                        "uz",//uzbekistan
                        "va",//holy see (vatican city state)
                        "vc",//saint vincent and the grenadines
                        "ve",//venezuela, bolivarian republic of
                        "vg",//virgin islands, british
                        "vi",//virgin islands, u.s.
                        "vn",//viet nam
                        "vu",//vanuatu
                        "wf",//wallis and futuna
                        "ws",//samoa
                        "ye",//yemen
                        "yt",//mayotte
                        "za",//south africa
                        "zm",//zambia
                        "zw"//zimbabwe
                        };

        //  Hierarchy of tags is like
        //  country > state, province > city, town, village >
        //          district, suburb, hamlet, neighborhood > postcode
        //                                                 > street
        //  down: decreasing importance
        //  right: increasing importance
        addressAreasHierarchy_ = {{"country"},
                                  {"province", "region", "county", "state"},
                                  {"island", "archipelago", "village", "town", "city"},
                                  {"neighbourhood", "locality", "hamlet", "district", "suburb"},
                                  {"street", "postcode"}};

        for(int i = 0; i < addressAreasHierarchy_.size(); i++) {
            for(int j = 0; j < addressAreasHierarchy_[i].size(); j++) {
                addressAreaLevel_[addressAreasHierarchy_[i][j]] = i;
            }
        }

        for(int i = 0; i < addressAreasHierarchy_.size(); i++) {
            for(int j = 0; j < addressAreasHierarchy_[i].size(); j++) {
                addressArea_.insert("addr:"+addressAreasHierarchy_[i][j]);
            }
        }

        turnRestrictions_ = {"no_right_turn", "no_left_turn", "no_u_turn",
                             "no_straight_on", "only_right_turn", "only_left_turn",
                             "only_straight_on", "no_entry", "no_exit"};

        aggregateTags_ = {"amenity", "building", "leisure", "place", "public_transport", "shop",
                          "sport", "tourism", "office", "highway", "emergency"};

        canonicalTags_ = {{{"highway", "bus_stop"},{"public_transport", "bus_stop"}},
                          {{"amenity", "bus_station"},{"public_transport", "bus_stop"}},
                          {{"public_transport", "halt"}, {"public_transport", "stop"}},
                          {{"public_transport","subway_entrance"}, {"public_transport", "subway"}},
                          {{"public_transport", "railway"}, {"public_transport", "train"}},
                          {{"public_transport", "platform"}, {"public_transport", "station"}},
                          {{"public_transport", "stop_position"}, {"public_transport", "stop"}},
                          {{"railway","tram"},{"public_transport","tram"}},
                          {{"railway","tram_stop"},{"public_transport","tram"}},
                          {{"railway","platform"},{"railway","station"}},
                          {{"railway","halt"},{"railway","stop"}},
                          {{"amenity","nightclub"}, {"amenity","club"}},
                          {{"postal_code","*"},{"addr:postcode","*"}}};

        nonSplittableTags_ = {"opening_hours", "phone", "fax"};

        nonFullTextKeys_ = {"email",
                            "fax",
                            "phone",
                            "website",
                            "wikipedia",
                            "opening_hours",
                            "addr:housenumber"};

        fullTextKeys_ = {"leisure", "railway", "tourism", "public_transport", "shop"};

        uniqueKeyValues_ = {};
        uniqueKeyValues_.insert(addressTags_.begin(), addressTags_.end());
    }
public:
    /**
     * @brief isFullText
     * @param key
     * @return
     */
    bool isFullText(const std::string &key) {
        return inProps(key, fullTextKeys_);
    }

    /**
     * @brief isFullTextKeyValue
     * @param key
     * @param value
     * @return
     */
    bool isFullTextKeyValue(const std::string &key, const std::string &/*value*/) {
        return !inProps(key, nonFullTextKeys_);
    }

    /**
     * @brief validValue
     * @param s
     * @return
     */
    bool validValue(const std::string &s) {
        return s.find(StringConsts::SEPARATOR) == string::npos;
    }

    /**
     * @brief isValidKeyValue
     * @return
     */
    bool isUniqueKeyValue(const string &key, const string &/*value*/) const {
        return uniqueKeyValues_.find(key) != uniqueKeyValues_.end();
    }

    /**
     * @brief isValidKeyValue
     * @param key
     * @param value
     * @return
     */
    bool isValidUserKeyValue(const string &key, const string &value) {
        if(key == "addr:postcode") {
            set<char> valid = {' ', '-'};
            for(int i = 0; i < value.size(); i++) {
                if(!isalnum(value[i]) && !valid.count(value[i]))
                    return false;
            }

            // TODO: need to handle UK zipcodes
            int alpha = 0;
            for(int i = 0; i < value.size(); i++) {
                alpha += isalpha(value[i]);
            }

            if(alpha > 2)
                return false;
        } else if(key == "phone" || key == "fax") {
            set<char> valid = {'(', ')', '+', '-', ' '};
            for(int i = 0; i < value.size(); i++) {
                if(!isdigit(value[i]) && !valid.count(value[i]))
                    return false;
            }
        }
        return true;
    }

    /**
     * @brief matchNameTag
     * @param key
     * @return
     */
    bool userEnteredTag(const string &key) const {
        return isNameTag(key) || inProps(key, userEnteredProps_) || inProps(key, addressTags_);
    }
    /**
     * @brief isNameTag
     * @param key
     * @return
     */
    bool isNameTag(const string &key) const {
        return key.find("name") == 0;
    }
    /**
     * @brief allowFixedKeyValue
     * @param key
     * @param val
     * @return
     */
    bool allowFixedKeyValue(const string &key, const string &val) {
        if(fixedKeyValue_.find(key) != fixedKeyValue_.end()) {
            // find exclusion in the list
            if(fixedKeyValue_[key].find("-"+val) != fixedKeyValue_[key].end())
                return false;

            // check if key->value are allowed
            if(fixedKeyValue_[key].find(val) != fixedKeyValue_[key].end())
                return true;

            // wild card accepting
            if(fixedKeyValue_[key].find("*") != fixedKeyValue_[key].end()) {
                // only accept tags that contain ; _ and alphanumerics
                bool ok = isValidFixedValue(val);
                if(!ok) {
                    LOGG(Logger::DEBUG) << "Ignoring tag: " << key << " " << val << Logger::FLUSH;
                }
                return ok;
            }
        }
        return false;
    }

    /**
     * @brief isMetaKeyValue
     * @param key
     * @param value
     * @return
     */
    bool isMetaKeyValue(const string &/*key*/, const string &value) {
        return !inProps(value, ignoredMetaValues_);
    }

    /**
     * @brief isSplittableFixedMultiValue
     * @param value
     * @return
     */
    bool isValidFixedValue(const std::string &val) const {
        bool ok = true;
        for(int i = 0; i < val.size(); i++) {
            bool lowAlpha = isalpha(val[i]) && islower(val[i]);
            if(!lowAlpha && val[i] != ';' && val[i] != '_')
                ok = false;
        }
        return ok;
    }

    /**
     * @brief splitMultipleFixedKeyValue
     * @param key
     * @return
     */
    std::vector<std::string> splitMultipleFixedKeyValue(const string &val) {
        SimpleTokenator tokenator(val, ';', '\"', false);
        std::vector<std::string> tokens = tokenator.getTokens();
        for(int i = 0; i < tokens.size(); i++)
            tokens[i] = StringUtils::trim(tokens[i]);

        return tokens;
    }

    /**
     * @brief acceptObject
     * @param node
     * @return
     */
    template <typename T>
    bool acceptObject(T* object) {
        // other tags with useful info
        int otherTags = 0;
        // tags can be stored
        int writableTags = 0;
        for(int i = 0; i < object->nTags; ++i) {
            string key = object->tags[i].key;
            string value = object->tags[i].value;
            if(writeKeyValue(key, value) && allowFixedKeyValue(key, value) && !aloneDeprecated(key, value)) {
                writableTags++;
            }
            if(inProps(key, userEnteredProps_)) {
                otherTags++;
            }
        }

        // TODO: check is this makes sense when there are only non writable tags
        return (writableTags + otherTags) > 0;
    }
    /**
     * @brief storeKeyWalue
     * @param key
     * @param val
     * @return
     */
    bool writeKeyValue(const string &key, const string &val) {
        if(notWritableKeyValue_.find(key) != notWritableKeyValue_.end()) {
            if(notWritableKeyValue_[key].find("*") != notWritableKeyValue_[key].end())
                return false;
            if(notWritableKeyValue_[key].find(val) != notWritableKeyValue_[key].end())
                return false;
        }
        return true;
    }
    /**
     * @brief aloneDeprecated key->value pairs
     * @param key
     * @param val
     * @return
     */
    bool aloneDeprecated(const string &key, const string &val) {
        if(aloneDeprecatedKeyValue_.find(key) != aloneDeprecatedKeyValue_.end()) {
            if(aloneDeprecatedKeyValue_[key].find(val) != aloneDeprecatedKeyValue_[key].end())
                return true;

            if(aloneDeprecatedKeyValue_[key].find("*") != aloneDeprecatedKeyValue_[key].end())
                return true;
        }
        return false;
    }

    /**
     * @brief inProps
     * @param key
     * @param props
     * @return
     */
    bool inProps(const string &key, const set<string> &props) const {
        return props.find(key) != props.end();
    }
    /**
     * @brief acceptTag if it is (allowed & writable)
     * or in goodProps_ or is user entered
     * @param key
     * @param val
     * @return
     */
    bool acceptTag(const string &key, const string &val) {
        return (allowFixedKeyValue(key,val) && writeKeyValue(key,val)) || userEnteredTag(key);
    }
    /**
     * @brief isSplittableTag
     * @param key
     * @return
     */
    bool isSplittableTag(const string &key) {
        return !inProps(key, nonSplittableTags_) || inProps(key, splittableTags_) || userEnteredTag(key);
    }
    /**
     * @brief isAddressDecodable
     * @param key
     * @return
     */
    bool isAddressDecodable(const string &key) {
        return inProps(key, addressDecodable_);
    }
    /**
     * @brief getAddressDecodable
     */
    set<string> getAddressDecodable() {
        set<string> areas;
        for(const string &tag : addressDecodable_) {
            string area = tag.substr(string("addr:").size());
            areas.insert(area);
        }
        return areas;
    }
    /**
     * @brief extractAdressAreaPrefix
     */
    string extractAdressAreaPrefix(const string &s) {
        string addr = "addr:";
        if(!isAddressDecodable(s) || s.find(addr) == string::npos)
            return "";
        return s.substr(s.find(addr)+addr.size());
    }
    /**
     * @brief isAddressTag
     * @param key
     * @return
     */
    bool isAddressTag(const string &key) {
        return inProps(key, addressTags_);
    }
    /**
     * @brief isNotAddressTag
     * @param key
     * @return
     */
    bool isNotAddressTag(const string &key) {
        return !isAddressTag(key);
    }
    /**
     * @brief isAddressArea
     * @param key
     * @return
     */
    bool isAddressArea(const string &key) {
        return inProps(key, addressArea_);
    }
    /**
     * this returns the candidate parent areas in the increasing order of their importance
     * like country comes after the city or town.
     * @brief getParentAreas
     * @return
     */
    vector<string> getParentAreas(const string &key) {
        // assumes (key, value) is an area
        int last = -1;
        for(int i = 0; i < addressAreasHierarchy_.size() && last == -1; i++) {
            for(int j = 0; j < addressAreasHierarchy_[i].size(); j++) {
                if(key == addressAreasHierarchy_[i][j]) {
                    last = i;
                    break;
                }
            }
        }
        vector<string> parents;
        for(int i = 0; i < last; i++)
            for(int j = 0; j < addressAreasHierarchy_[i].size(); j++) {
                parents.push_back(addressAreasHierarchy_[i][j]);
            }
        reverse(parents.begin(), parents.end());
        return parents;
    }
    /**
     * @brief getAddressAreaLevel
     * @param token
     * @return
     */
    int getAddressAreaLevel(const string &token) {
        if(addressAreaLevel_.count(token) == 0)
            return -1;
        return addressAreaLevel_[token];
    }
    /**
     * @brief isCountryCode
     * @param code
     * @return
     */
    bool isCountryCode(const string &code) {
        return countryCodesLower_.count(code) || countryCodes_.count(code);
    }

    /**
     * @brief isAggregateTag
     * @param tag
     * @return
     */
    bool isAggregateTag(const string &tag) {
        return inProps(tag, aggregateTags_);
    }

    /**
     * @brief getCanonicalTag
     * @param key
     * @param value
     * @return
     */
    pair<string,string> getCanonicalTag(const string &key, const string &value) {
        pair<string, string> tag = {key, value};
        if(canonicalTags_.count(tag)) {
            if(canonicalTags_[tag].second == "*") {
                return {canonicalTags_[tag].first,value};
            } else
                return canonicalTags_[tag];
        }
        return tag;
    }
    /**
     * @brief readConfig
     * @param configFile
     * @return
     */
    static bool readConfig(const string &configFile);
};
