/**
 *  @file
 *  @brief Implementation of GpzdaConverter converter
 *
 * \verbatim
 *  ___    ___
 *  \  \  /  /
 *   \  \/  /   Fixposition AG
 *   /  /\  \   All right reserved.
 *  /__/  \__\
 * \endverbatim
 *
 */

/* SYSTEM / STL */
#include <ctime>
#include <iomanip>
#include <iostream>

/* PACKAGE */
#include <fixposition_driver_lib/messages/nmea_type.hpp>
#include <fixposition_driver_lib/messages/base_converter.hpp>
#include <fixposition_driver_lib/time_conversions.hpp>

namespace fixposition {

/// msg field indices
static constexpr const int time_idx = 1;
static constexpr const int day_idx = 2;
static constexpr const int month_idx = 3;
static constexpr const int year_idx = 4;
static constexpr const int local_hr_idx = 5;
static constexpr const int local_min_idx = 6;

// Function to convert UTC time with milliseconds to GPS time
void convertToGPSTime(const std::string& utcTimeString, std::string& gpsWeek, std::string& gpsTimeOfWeek) {
    // Define constants
    const double secondsInWeek = 604800.0; // 7 days in seconds

    // Parse the input string
    std::tm tmTime = {};
    std::istringstream iss(utcTimeString);
    iss >> std::get_time(&tmTime, "%d/%m/%Y %H:%M:%S");

    // Read milliseconds from input
    char dot;
    std::string milliseconds;
    iss >> dot >> milliseconds;
    double ms = std::stod("0." + milliseconds);
    
    if (iss.fail()) {
        std::cerr << "Error parsing input string.\n";
        return;
    }
    
    // Convert UTC time to time since epoch
    std::time_t utcTime = std::mktime(&tmTime);

    // GPS epoch time (January 6, 1980)
    std::tm gpsEpoch = {};
    gpsEpoch.tm_year = 80; // years since 1900
    gpsEpoch.tm_mon = 0;   // months since January
    gpsEpoch.tm_mday = 6;  // day of the month
    std::time_t gpsEpochTime = std::mktime(&gpsEpoch);

    // Calculate GPS time of week and GPS week number
    double timeDifference = std::difftime(utcTime, gpsEpochTime);
    int gpsWeekNumber = static_cast<int>(std::floor(timeDifference / secondsInWeek));
    double gpsTime = std::fmod(timeDifference, secondsInWeek);

    // Add milliseconds to GPS time
    gpsTime += (fixposition::times::Constants::gps_leap_time_s + ms);

    // Convert results to strings
    std::ostringstream ossWeek, ossTime;
    ossWeek << gpsWeekNumber;
    ossTime << std::fixed << std::setprecision(6) << gpsTime;

    gpsWeek = ossWeek.str();
    gpsTimeOfWeek = ossTime.str();
}

void GP_ZDA::ConvertFromTokens(const std::vector<std::string>& tokens) {
    // Check if message size is wrong
    bool ok = tokens.size() == kSize_;
    if (!ok) {
        std::cout << "Error in parsing GPZDA string with " << tokens.size() << " fields! GPZDA message will be empty.\n";
        ResetData();
        return;
    }

    // Check that critical message fields are populated
    for (int i = 1; i < 6; i++) {
        if (tokens.at(i).empty()) {
            ResetData();
            return;
        }
    }

    // Populate time fields
    time_str = tokens.at(time_idx);
    date_str = tokens.at(day_idx) + '/' + tokens.at(month_idx) + '/' + tokens.at(year_idx);

    // Generate GPS timestamp
    std::string utcTimeString = date_str + " " + time_str.substr(0,2) + ":" + time_str.substr(2,2) + ":" + time_str.substr(4);
    std::string gps_tow, gps_week;
    convertToGPSTime(utcTimeString, gps_week, gps_tow);
    stamp = ConvertGpsTime(gps_week, gps_tow);

    // Get local time
    local_hr = StringToDouble(tokens.at(local_hr_idx));
    local_min = StringToDouble(tokens.at(local_min_idx));
}

}  // namespace fixposition
