#ifndef WORKFLOW_H
#define WORKFLOW_H

#include "classifier_result.h"
#include "config.h"
#include "view.h"

#include <opencv2/opencv.hpp>

class BaseWorkflow
{
public:
	BaseWorkflow(const int workflowId, const std::shared_ptr<WorkflowConfig> pConfig);
	virtual ~BaseWorkflow();

	int workflowId() const { return m_iWorkflowId; }
	bool isValidWorkflowImage() { return m_bValidWorkflowImage; }

	cv::Mat getWorkflowImage() { return m_workflowImage; }
	cv::Mat getWorkflowProcessedImage() { return m_workflowProcessedImage; }

	int boardId() const { return m_iBoardId; }
	void setBoardId(const int boardId) { m_iBoardId = boardId; }

	void setView(const std::shared_ptr<View> pView) { m_pView = pView; }
	std::shared_ptr<View> getView() { return m_pView; }

	bool containsExceptionKeyword(const std::string &keyword);

	virtual bool getNextFrame(const bool retry = false);
	virtual bool drawView(const double scale = 1.0, const int thickness = 3);

	virtual bool readFrameFromFile(const std::string &fileName);
	virtual bool saveFrameToFile(const std::string &filePath, const std::string &fileName);

	// This one will pre-process image, and extract data into targets of the view, leave it to app for details
	virtual bool imagePreProcess() = 0;

	// Since we have more and more traditional computer vision process need to be handled to locate defects
	// we seperate it from pre-process, and handle it with deep learning parallelly
	virtual bool computerVisionProcess() = 0;
protected:
	std::shared_ptr<View> m_pView;
	std::shared_ptr<WorkflowConfig> m_pConfig;
	cv::Mat m_workflowImage;
	cv::Mat m_workflowProcessedImage;
	std::string m_sWorkflowImageException;

	virtual bool updateViewResults(const std::vector<ClassificationResult> &results);
	virtual void drawDesignedTargets(const double scale, const int thickness = 3);
	virtual void drawTarget(const std::shared_ptr<TargetObject> pTarget, const double scale, const int thickness = 3);
private:
	int m_iWorkflowId;
	int m_iBoardId;
	bool m_bValidWorkflowImage;

	cv::Scalar getColor(const ClassificationResult result);
};

#endif //WORKFLOW_H