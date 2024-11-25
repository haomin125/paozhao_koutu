#ifndef RUNNING_STATUS_H
#define RUNNING_STATUS_H

#include <fstream>
#include <string>
#include <mutex>
#include <map>
#include <deque>
#include <array>
#include <ostream>
#include <algorithm>

#include "classifier_result.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#define  PRODUCT_SETTING_LENGTH  16
#define  MAX_BLOB_STRING_LENGTH  256
#define  MAX_TOTAL_MODEL_FILES   3
#define  USER_DEFINED_ALARM_WARNING_START 100

class ProductSetting
{
public:
	ProductSetting();
	virtual ~ProductSetting() {};

	struct PRODUCT_SETTING
	{
		PRODUCT_SETTING() : bool_settings{ false }, string_settings{ '\0' }, float_settings{ 0.0 } {}

		void loadFromJson(boost::property_tree::ptree &pt);
		void reset() { bool_settings = { false }; string_settings = { '\0' }; float_settings = { 0.0 }; }

		std::array<bool, PRODUCT_SETTING_LENGTH> bool_settings;

		// more like array[MAX_BLOB_STRING_LENGTH][MAX_TOTAL_MODEL_FILES], string could be file name
		std::array<std::array<char, MAX_BLOB_STRING_LENGTH>, MAX_TOTAL_MODEL_FILES> string_settings;

		// setting could be the follows
		// sensitivity is ranging from 0 to 1
		// alarm threashold range from 0 to 1
		// presure threashold range from 0 to 1
		// save image flag -1, 0, 1, 2, ...; 0 means all, 1 means only save defect 1, 2 means only save defect 2, ..., -1 means save all defects
		// history image enable saving or not, 0 means not save, 1 means save

		// to be compatible with existing app, the float setting is the ONLY part we can expand in product setting
		// float setting change from  64 to 256
		std::array<float, PRODUCT_SETTING_LENGTH * 16> float_settings;

		friend std::ostream& operator<<(std::ostream& os, const PRODUCT_SETTING& data);
	};

	// please note: set here only update the setting in memory, not database, you have to call UpdateCurrentProductSettingToDB to update database
	bool GetBooleanSetting(const int idx);
	void SetBooleanSetting(const int idx, const bool flag);
	float GetFloatSetting(const int idx);
	void SetFloatSetting(const int idx, const float value);
	std::string GetStringSetting(const int idx);
	void SetStringSetting(const int idx, const std::string &name);

	// copy and return the current setting in memory 
	PRODUCT_SETTING GetSettings() { return m_oProductSetting; }

	// update setting in memory
	void UpdateCurrentProductSetting(const PRODUCT_SETTING& oProductSetting);

	// reset setting in memory
	virtual void reset() { m_oProductSetting.reset(); }
private:
	PRODUCT_SETTING m_oProductSetting;
};

class RunningProductSetting : public ProductSetting
{
public:
	RunningProductSetting();
	virtual ~RunningProductSetting() {};

	// read product setting from DB, and update the memory
	bool UpdateCurrentProductSettingFromDB(const std::string& prod);
	// update the setting in database, but not in memory
	bool UpdateProductSettingToDB(const std::string& prod, const PRODUCT_SETTING& oProductSetting);
	// get product setting from DB
	bool GetProductSettingFromDB(const std::string& prod, PRODUCT_SETTING& oProductSettin);
};

class TestingProductSetting : public ProductSetting
{
public:
	TestingProductSetting();
	virtual ~TestingProductSetting() {};

	void loadFromJson(boost::property_tree::ptree &pt);

	std::string &getCameraName() { return m_sCameraName; }
	void setCameraName(const std::string &name) { m_sCameraName = name; }
	std::string &getImageName() { return m_sImageName; }
	void setImageName(const std::string &name) { m_sImageName = name; }
	int getPhase() const { return m_iPhase; }
	void setPhase(const int phase) { m_iPhase = phase; }
	int getBoardId() const { return m_iBoardId; }
	void setBoardId(const int id) { m_iBoardId = id; }

	virtual void reset();
private:
	std::string m_sCameraName;
	std::string m_sImageName;
	int m_iPhase;
	int m_iBoardId;
};

class RunningData {
public:
	RunningData();
	~RunningData() {};

	enum class alarmLevel : int
	{
		NORMAL = 0,
		ALARM = 1,
		WARNING = 2
	};

	bool useBoardIdKey() { return m_bUseBoardIdKey; }
	void useBoardIdKey(const bool flag) { m_bUseBoardIdKey = flag; }
	int boardIdKey() const { return m_iBoardIdKey; }
	void boardIdKey(const int key) { m_iBoardIdKey = key; }
	void addBoardIdInTotalNumber(const int boardId);
	void addBoardIdsInTotalNumber(const std::vector<int> &boardIds);
	std::vector<int> getBoardIdsInTotalNumber() { return m_boardIdsInTotalNumber; }

	// we change interface of this group of methods, if you need to save user defined alarm/warning into database
	// user defined id start from 100
	void setAlarm(const std::string &msg, const bool setTime = false, const int userdefinedId = -1);
	void setWarning(const std::string &msg, const bool setTime = false, const int userDefinedId = -1);
	void clearAlarm();

	bool exceedDefectRatio(const int totalBoard, const float percent);

	int GetRunningData(std::map<std::string, int>& st);
	int GetRunningMsg(std::map<std::string, std::string>& st);

	void ClearDisplayData();

	void ProcessClassifyResult(const int boardId, const ClassificationResult result, const int productCount = -1);
	void ProcessClassifyResult(const int boardId, const std::vector<ClassificationResult> &results, const int productCount = -1);

	bool saveToDatabase(const std::string &prod, const int boardId, const std::string &lotNumber);

	void setCustomerDataByName(const std::string &name, const std::string &data, const bool setTime = false);
	std::string getCustomerDataByName(const std::string &name, const bool getTime = false);
	void clearCustomerData();

	void setCameraToBoard(const std::map<int, int> &toBoard) { m_cameraToBoard = toBoard; }
	int getBoardIdFromCamera(const int camera_index) const;
	std::vector<int> getCamerasFromBoard(const int boardId);

	void setClassificationResults(const int boardId, const std::vector<std::vector<ClassificationResult>> &result);
	std::map<int, std::vector<std::vector<ClassificationResult>>> getAllClassificationResults();
	void clearClassificationResults(const int boardId);

	friend std::ostream& operator<<(std::ostream& os, const RunningData& data);
private:
	class TotalDefect
	{
	public:
		TotalDefect();

		int displayedTotalDefects() const { return m_iDisplayedTotalDefects; }
		void reset();

		bool insert(const int boardId, const int productCount, const std::vector<ClassificationResult> &results);
		void updateTotalDefects(const std::vector<ClassificationResult> &results);

		void calculate();
	private:
		std::mutex m_totalDefectMutex;
		int m_iDisplayedTotalDefects;
		std::map<int, std::deque<std::pair<int, ClassificationResult>>> m_productResult;
	};

	// total number only count by boardId = boardIdKey, but defects mapper store all defects by defect name
	// we assume all product should pass one of the key board in any applicaion, this board Id is boardIdKey, 
	// default is 0, this key can be set in the app at the very begining if the key board is not 0
	// if using board id key is false, we add option not to count all boards in total product count, app can
	// add any board id which they want to count in total product number using either addBoardIdInTotalNumber()
	// or addBoardIdsInTotalNumber()
	bool m_bUseBoardIdKey;
	int m_iBoardIdKey;
	std::vector<int> m_boardIdsInTotalNumber;

	// saved database numbers, which does not allow to be cleared from UI
	std::map<int, int> m_totalNumberInBoard;
	std::map<int, int> m_totalDefectInBoard;
	std::map<int, std::map<std::string, int>> m_defectsInBoard;

	// displayed numbers, which could be cleared from UI anytime
	int m_iDisplayTotalNumber;
	std::map<std::string, int> m_displayDefects;

	// total defects which calculate from product result with product count
	TotalDefect m_displayedTotalDefectNumber;

	// displayed numbers per board, which could be cleared from UI anytime
	std::map<int, int> m_displayTotalNumberInBoard;
	std::map<int, int> m_displayTotalDefectInBoard;
	std::map<int, std::map<std::string, int>> m_displayDefectsInBoard;

	// <name, value> pair for data exchange between UI and app
	std::map<std::string, std::string> m_customerData;

	// <camera_index, board_id> pair for UI to know how camera related to board
	std::map<int, int> m_cameraToBoard;

	// vector of classification results of each board
	std::map<int, std::vector<std::vector<ClassificationResult>>> m_classificationResults;

	// alarm
	int m_iAlarm;
	std::string m_sAlarmMsg;

	std::mutex m_mutex;

	template <typename T>
	void IncreaseNumber(const T key, std::map<T, int> &count)
	{
		if (count.find(key) == count.end())
			count.emplace(key, 1);
		else
			++count[key];
	}

	void setAlarmWarning(const std::string &msg, const bool setTime, const int userdefinedId, const alarmLevel level);
};

class ProductionInfo {
public:
	ProductionInfo();
	virtual ~ProductionInfo() {};

	struct PRODUCTION_INFO
	{
		PRODUCTION_INFO() : nAuto(0), nRun(1), nCameraDisplayMode(1), prod("N/A"), lot("N/A") {}

		void reset() { nAuto = 0;  nRun = 1;  nCameraDisplayMode = 1; prod = "N/A"; lot = "N/A"; };

		int nAuto;
		int nRun;
		int nCameraDisplayMode;

	    std::string prod;
		std::string lot;

		friend std::ostream& operator<<(std::ostream& os, const PRODUCTION_INFO& data);
	};

	int GetAutoStatus() const { return m_oProductInfo.nAuto; };
	bool SetAutoStatus(const int value);

	int GetCameraDisplayMode() const { return m_oProductInfo.nCameraDisplayMode; };
	bool SetCameraDisplayMode(const int value);

	int GetRunStatus() const { return m_oProductInfo.nRun; };
	bool SetRunStatus(const int value);

	std::string &GetCurrentProd() { return m_oProductInfo.prod; };
	bool SetCurrentProd(const std::string& prod, const bool saveToDatabase = true);

	std::string &GetCurrentLot() { return m_oProductInfo.lot; };
	bool SetCurrentLot(const std::string& lot, const bool saveToDatabase = true);

	virtual void reset() { m_oProductInfo.reset(); }
protected:
	std::mutex m_mutex;
	PRODUCTION_INFO m_oProductInfo;
};

class TestingProductionInfo : public ProductionInfo
{
public:
	TestingProductionInfo();
	virtual ~TestingProductionInfo() {};

	int GetTestStatus() const { return m_iTest; }
	void SetTestStatus(const int value);

	int GetTriggerCameraImageStatus() const { return m_iTriggerCameraImage; }
	void SetTriggerCameraImageStatus(const int value);

	virtual void reset();
private:
	int m_iTest;
	int m_iTriggerCameraImage;
};

class RunningInfo {
public: 
	static RunningInfo &instance();
	static void Exit();

	std::string &GetUser() { return m_sCurrentUser; }
	void SetUser(const std::string &user) { m_sCurrentUser = user; }
	
	RunningData& GetRunningData() { return m_runningData; };

	ProductionInfo& GetProductionInfo() { return m_productionInfo; };
	RunningProductSetting& GetProductSetting() { return m_productSetting; }

	TestingProductionInfo& GetTestProductionInfo() { return m_testProductionInfo; };
	TestingProductSetting& GetTestProductSetting() { return m_testProductSetting; }

protected:
	static RunningInfo* m_instance;

private:
	RunningInfo();
	virtual ~RunningInfo();

	static std::mutex m_mutex;

	std::string m_sCurrentUser;

	RunningData m_runningData;
	ProductionInfo m_productionInfo;
	RunningProductSetting m_productSetting;
	TestingProductionInfo m_testProductionInfo;
	TestingProductSetting m_testProductSetting;
};

#endif // RUNNING_STATUS_H