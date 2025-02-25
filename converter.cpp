#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include "calculator.h"

using namespace std;
namespace fs = filesystem;

class XMLParser {
public:
    static string getElementContent(const string& xml, const string& elementName) {
        size_t startPos = xml.find("<" + elementName + ">");
        size_t endPos = xml.find("</" + elementName + ">");
        if (startPos != string::npos && endPos != string::npos) {
            startPos += elementName.length() + 2;
            return xml.substr(startPos, endPos - startPos);
        }
        return "";
    }

    static vector<string> getElements(const string& xml, const string& elementName) {
        vector<string> elements;
        size_t pos = 0;
        while ((pos = xml.find("<" + elementName, pos)) != string::npos) {
            size_t endPos = xml.find("</" + elementName + ">", pos);
            if (endPos != string::npos) {
                elements.push_back(xml.substr(pos, endPos - pos + elementName.length() + 3));
                pos = endPos + elementName.length() + 3;
            } else {
                break;
            }
        }
        return elements;
    }

    static string getAttribute(const string& element, const string& attributeName) {
        size_t pos = element.find(attributeName + "=\"");
        if (pos != string::npos) {
            pos += attributeName.length() + 2;
            size_t endPos = element.find("\"", pos);
            if (endPos != string::npos) {
                return element.substr(pos, endPos - pos);
            }
        }
        return "";
    }
};

class JSONBuilder {
private:
    ostringstream json;
    bool isFirstElement;

public:
    JSONBuilder() : isFirstElement(true) {
        json << "{";
    }

    void addKeyValue(const string& key, const string& value) {
        addCommaIfNeeded();
        json << "\"" << key << "\":\"" << value << "\"";
    }

    void addKeyValue(const string& key, double value) {
        addCommaIfNeeded();
        json << "\"" << key << "\":" << value;
    }

    void addKeyValue(const string& key, int value) {
        addCommaIfNeeded();
        json << "\"" << key << "\":" << value;
    }

    void startObject(const string& key) {
        addCommaIfNeeded();
        if (!key.empty()) {
            json << "\"" << key << "\":";
        }
        json << "{";
        isFirstElement = true;
    }

    void endObject() {
        json << "}";
        isFirstElement = false;
    }

    void startArray(const string& key) {
        addCommaIfNeeded();
        if (!key.empty()) {
            json << "\"" << key << "\":";
        }
        json << "[";
        isFirstElement = true;
    }

    void endArray() {
        json << "]";
        isFirstElement = false;
    }

    void addArrayValue(int value) {
        addCommaIfNeeded();
        json << value;
    }

    string build() {
        json << "}";
        return json.str();
    }

private:
    void addCommaIfNeeded() {
        if (!isFirstElement) {
            json << ",";
        }
        isFirstElement = false;
    }
};

class GPXToJSONConverter {
private:
    vector<GPSPoint> gpsData;
    RunInfo runInfo;
    vector<int> paceSections;
    vector<int> kcalSections;

    void parseGPXFile(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            throw runtime_error("Failed to open GPX file");
        }

        stringstream buffer;
        buffer << file.rdbuf();
        string content = buffer.str();

        runInfo.location = XMLParser::getElementContent(content, "name");
        
        auto trkptElements = XMLParser::getElements(content, "trkpt");
        for (const auto& trkpt : trkptElements) {
            GPSPoint point;
            point.lat = stod(XMLParser::getAttribute(trkpt, "lat"));
            point.lon = stod(XMLParser::getAttribute(trkpt, "lon"));
            point.time = XMLParser::getElementContent(trkpt, "time");
            gpsData.push_back(point);
        }

        runInfo = RunCalculator::calculateRunInfo(gpsData);
        paceSections = RunCalculator::calculatePaceSections(gpsData);
        kcalSections = RunCalculator::calculateKcalSections(gpsData);
    }

    string createJSON() {
        JSONBuilder json;

        json.startObject("runInfo");
        json.addKeyValue("runStartDate", runInfo.runStartDate);
        json.addKeyValue("location", runInfo.location);
        json.addKeyValue("distance", runInfo.distance);
        json.addKeyValue("time", runInfo.time);
        json.addKeyValue("kcal", runInfo.kcal);
        json.addKeyValue("meanPace", runInfo.meanPace);
        json.addKeyValue("difficulty", runInfo.difficulty);
        json.endObject();

        json.startObject("sectionData");
        json.startArray("pace");
        for (int pace : paceSections) {
            json.addArrayValue(pace);
        }
        json.endArray();
        json.startArray("kcal");
        for (int kcal : kcalSections) {
            json.addArrayValue(kcal);
        }
        json.endArray();
        json.endObject();

        json.startArray("gpsData");
        for (const auto& point : gpsData) {
            json.startObject("");
            json.addKeyValue("lon", point.lon);
            json.addKeyValue("lat", point.lat);
            json.addKeyValue("time", point.time);
            json.endObject();
        }
        json.endArray();

        return json.build();
    }

public:
    void convertSingle(const string& inputFile, const string& outputFolder) {
        parseGPXFile(inputFile);
        string jsonContent = createJSON();

        fs::path outputPath = fs::path(outputFolder) / (fs::path(inputFile).stem().string() + ".txt");
        ofstream out(outputPath);
        if (!out.is_open()) {
            throw runtime_error("Failed to open output file: " + outputPath.string());
        }
        out << jsonContent << endl;
        out.close();
    }


    void convertBatch(const string& inputFolder, const string& outputFolder) {
        if (!fs::is_directory(inputFolder)) {
            throw runtime_error("cannot find input_file");
        }

        fs::create_directories(outputFolder);

        for (const auto& entry : fs::directory_iterator(inputFolder)) {
            if (entry.path().extension() == ".gpx") {
                try {
                    convertSingle(entry.path().string(), outputFolder);
                    cout << "CONVERTED: " << entry.path().string() << " -> " 
                              << (fs::path(outputFolder) / (entry.path().stem().string() + ".txt")).string() << endl;
                } catch (const exception& e) {
                    cerr << "CONVERT ERROR: " << entry.path().string() << ": " << e.what() << endl;
                }
            }
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <input_folder> <output_folder>" << endl;
        return 1;
    }

    try {
        GPXToJSONConverter converter;
        converter.convertBatch(argv[1], argv[2]);
        cout << "SUCCESS" << endl;
    } catch (const exception& e) {
        cerr << "ERROR: " << e.what() << endl;
        return 1;
    }

    return 0;
}