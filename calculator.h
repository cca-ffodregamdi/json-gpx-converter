#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <string>
#include <vector>

using namespace std;

struct GPSPoint {
    double lat;
    double lon;
    string time;
};

struct RunInfo {
    string runStartDate;
    string location;
    double distance;
    int time;
    int kcal;
    int meanPace;
    string difficulty;
};

class RunCalculator {
public:
    static double calculateDistance(double lat1, double lon1, double lat2, double lon2);
    static int calculateTimeDifference(const string& time1, const string& time2);
    static RunInfo calculateRunInfo(const vector<GPSPoint>& gpsData);
    static vector<int> calculatePaceSections(const vector<GPSPoint>& gpsData);
    static vector<int> calculateKcalSections(const vector<GPSPoint>& gpsData);
};

#endif