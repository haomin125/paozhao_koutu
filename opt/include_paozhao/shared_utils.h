#ifndef SHARED_UTILS_H
#define SHARED_UTILS_H

#include <string>
#include <chrono>

#include "classifier_result.h"

namespace shared_utils
{
	// get current time in format "%Y%m%d-%H:%M:%S", this function is used in log file
	std::string getTime();

	// get time in format %Y-%m-%d %H:%M:%S", Those function are used in database
	std::string getTime(const int offsetHours);
	std::string getTime(const std::string &baseTime, const int offsetHours);
	std::string getTime(const std::string &unixTime);
	
	// utilities related to classification result, and save images
	int getClassificationNumber(const ClassificationResult result);

	std::string createImageFileName(const int boardId, const int viewId, const int targetId, const std::string &prod, const int count, const ClassificationResult result);
	void extractImageFileName(const std::string &fileName, std::string &timeFromName, int &cameraId, int &defectId);

	std::string createImageTempName(const std::string &fileName, const int productIdx = -1, const std::string &ext = "png");

	// attached the good category score into file name which it ends with .png
	// ex. filename = TYPE2-TIME20190122-023554.531-BOARD0-ID0-TARGET0-PRODprodname-CNT0.png
	std::string getImageFileNameWithGS(const std::string &filename, std::vector<std::vector<float>> &goodScore, const int x, const int y);

	// others
	std::string getUniqueId();
	std::string convertTextToString(const unsigned char* text);

	// time diff in hours
	double getTimeDiff(const std::string &startTime, const std::string &endTime);

	// round to nearest integer, 3.49 to 3, and 3.50 to 4
	int getIntFromDouble(const double value);

	// Description: getSmartMeanValue
	//    if data input is empty, the return value will be 0
	//    if data input size is less than 2, the return value will be the simple mean value of all input data
	//    if data input size is more than 3, the return value will be the mean value of all input data excluding the max and min.
	double getSmartMeanValue(const std::vector<int>& values);

	namespace {
		std::string getTimeInFormat(const std::string &format);
		std::string getTimeString(const time_t &ttime, const std::string &format, std::chrono::system_clock::duration duration = std::chrono::system_clock::duration::zero());
		float getCategoryScore(const std::vector<std::vector<float>> &goodScores, const int x, const int y);
	} // namespace

} // namespace shared_utils

#endif // SHARED_UTILS_H
