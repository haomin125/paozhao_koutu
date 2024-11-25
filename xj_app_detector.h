#ifndef XJ_APP_DETECTOR_H
#define XJ_APP_DETECTOR_H

#include "detector.h"
#include "xj_app_config.h"
#include "xj_app_workflow.h"

class AppDetector : public BaseDetector
{
public:
	AppDetector(const std::shared_ptr<AppDetectorConfig> &pConfig, const std::shared_ptr<BaseTracker> &pTracker,
		const std::shared_ptr<Board> &pBoard) : BaseDetector(pConfig, pTracker, pBoard)
	{
		pConfig->exposeTargetResults(true);
	}
	virtual ~AppDetector() {};

	virtual bool createBoardTrackingHistory(const int plcDistance);

    bool reconfigWorkflow();

	bool signal_test(std::array<bool, 16> &signals,std::vector<int> mergeResults,const int signal_type,const int channel);

protected:
	virtual bool addToTrackingHistory();
	virtual bool decideToSave(const cv::Mat &img, const ClassificationResult result, const std::string &imgName);
	virtual bool customizedSavedImageName(const std::string &name, const int viewIdx, const int targetIdx, std::string &returned);
	virtual bool purgeBoardResult(const std::vector<std::vector<ClassificationResult>> &result, const bool needCalculationResult = true);
private:
	bool m_usemodel;
};

class BeforeSealingDetector : public AppDetector
{
public:
	BeforeSealingDetector(const std::shared_ptr<AppDetectorConfig> &pConfig, const std::shared_ptr<BaseTracker> &pTracker,
		const std::shared_ptr<Board> &pBoard) : AppDetector(pConfig, pTracker, pBoard)
	{
		initWorkflows<BeforeSealingWorkflow>();
	}
};

class AfterSealingDetector : public AppDetector
{
public:
	AfterSealingDetector(const std::shared_ptr<AppDetectorConfig> &pConfig, const std::shared_ptr<BaseTracker> &pTracker,
		const std::shared_ptr<Board> &pBoard) : AppDetector(pConfig, pTracker, pBoard)
	{
		initWorkflows<AfterSealingWorkflow>();
	}
};

#endif // XJ_APP_DETECTOR_H
