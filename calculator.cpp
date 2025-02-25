#include "calculator.h"
#include <cmath>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cctype>

#define M_PI 3.14

using namespace std;

double RunCalculator::calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371000;
    double phi1 = lat1 * M_PI / 180;
    double phi2 = lat2 * M_PI / 180;
    double deltaPhi = (lat2 - lat1) * M_PI / 180;
    double deltaLambda = (lon2 - lon1) * M_PI / 180;

    double a = sin(deltaPhi / 2) * sin(deltaPhi / 2) +
               cos(phi1) * cos(phi2) *
               sin(deltaLambda / 2) * sin(deltaLambda / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return R * c;
}

int RunCalculator::calculateTimeDifference(const string& time1, const string& time2) {
    auto parseTime = [](const string& timeStr) -> chrono::system_clock::time_point {
        tm tm = {};
        int milliseconds;
        istringstream ss(timeStr);
        ss >> get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        ss.ignore();
        ss >> milliseconds;
        if (ss.fail()) {
            throw runtime_error("Failed to parse time: " + timeStr);
        }
        auto time = chrono::system_clock::from_time_t(mktime(&tm));
        time += chrono::milliseconds(milliseconds);
        return time;
    };

    try {
        auto t1 = parseTime(time1);
        auto t2 = parseTime(time2);
        return chrono::duration_cast<chrono::seconds>(t2 - t1).count();
    } catch (const exception& e) {
        cerr << "Error in time calculation: " << e.what() << endl;
        return 0;
    }
}

RunInfo RunCalculator::calculateRunInfo(const vector<GPSPoint>& gpsData) {
    RunInfo runInfo;
    double totalDistance = 0;
    int totalTime = 0;
    int totalKcal = 0;

    if (gpsData.size() < 2) {
        cerr << "Not enough GPS points to calculate run info" << endl;
        return runInfo;
    }

    for (size_t i = 1; i < gpsData.size(); ++i) {
        double segmentDistance = calculateDistance(gpsData[i-1].lat, gpsData[i-1].lon, 
                                                   gpsData[i].lat, gpsData[i].lon);
        totalDistance += segmentDistance;

        int segmentTime = calculateTimeDifference(gpsData[i-1].time, gpsData[i].time);
        totalTime += segmentTime;

        int segmentKcal = static_cast<int>(segmentDistance * 0.0001 * 70);
        totalKcal += segmentKcal;
    }

    runInfo.distance = totalDistance / 1000;
    runInfo.time = totalTime;
    runInfo.kcal = totalKcal;
    runInfo.meanPace = (totalDistance > 0) ? static_cast<int>(totalTime / (runInfo.distance)) : 0;

    if (runInfo.distance <= 5) runInfo.difficulty = "EASY";
    else if (runInfo.distance <= 10) runInfo.difficulty = "EASY_NORMAL";
    else if (runInfo.distance <= 15) runInfo.difficulty = "NORMAL";
    else if (runInfo.distance <= 20) runInfo.difficulty = "NORMAL_HARD";
    else runInfo.difficulty = "HARD";

    runInfo.runStartDate = gpsData.front().time;

    return runInfo;
}

vector<int> RunCalculator::calculatePaceSections(const vector<GPSPoint>& gpsData) {
    vector<int> paceSections;
    double sectionDistance = 0;
    int sectionTime = 0;
    const double sectionLength = 1000; // 1km in meters

    for (size_t i = 1; i < gpsData.size(); ++i) {
        double segmentDistance = calculateDistance(gpsData[i-1].lat, gpsData[i-1].lon, 
                                                   gpsData[i].lat, gpsData[i].lon);
        int segmentTime = calculateTimeDifference(gpsData[i-1].time, gpsData[i].time);

        sectionDistance += segmentDistance;
        sectionTime += segmentTime;

        if (sectionDistance >= sectionLength || i == gpsData.size() - 1) {
            int sectionPace = (sectionDistance > 0) ? static_cast<int>(sectionTime / (sectionDistance / 1000)) : 0;
            paceSections.push_back(sectionPace);

            sectionDistance = 0;
            sectionTime = 0;
        }
    }

    return paceSections;
}

vector<int> RunCalculator::calculateKcalSections(const vector<GPSPoint>& gpsData) {
    vector<int> kcalSections;
    double sectionDistance = 0;
    int sectionKcal = 0;
    const double sectionLength = 1000; // 1km in meters

    for (size_t i = 1; i < gpsData.size(); ++i) {
        double segmentDistance = calculateDistance(gpsData[i-1].lat, gpsData[i-1].lon, 
                                                   gpsData[i].lat, gpsData[i].lon);
        int segmentKcal = static_cast<int>(segmentDistance * 0.0001 * 70);

        sectionDistance += segmentDistance;
        sectionKcal += segmentKcal;

        if (sectionDistance >= sectionLength || i == gpsData.size() - 1) {
            kcalSections.push_back(sectionKcal);

            sectionDistance = 0;
            sectionKcal = 0;
        }
    }

    return kcalSections;
}