#ifndef VIEW_H
#define VIEW_H

#include "camera.h"
#include "factory.hpp"
#include "target_object.h"

#include <string>
#include <vector>
#include <memory>

//
//  View contains multiple target objects, each target object contains
//  the basic image and bounding box
//

class View
{
public:
	View();
	View(const int viewId, const int targetNumbers);
	virtual ~View() {};

	// virtual function for camera which should be override by its subclasses
	// be careful here, getCamera will return a copy of shared_ptr, even both points to
	// the same camera data structure
	virtual void setCamera(const std::shared_ptr<BaseCamera> pCamera) { m_pCamera = pCamera; }
	virtual std::shared_ptr<BaseCamera> getCamera() { return m_pCamera; }

	// common methods used by view, board, and lines
	bool merge();
	bool distribute(const std::vector<ClassificationResult> &results);

	std::vector<cv::Mat> &imageBatch() { return m_batchedImage; }
	std::vector<ClassificationResult> &resultBatch() { return m_batchedResult; }

	// other view related methods
	int viewId() { return m_iViewId; }
	bool isValid() { return m_iViewId >= 0 && m_targets.size() > 0; }

	bool isClassified() { return m_bIsClassified; }
	void setClassificationLevel(const bool level) { m_bIsClassified = level; }

	std::shared_ptr<TargetObject> getTarget(const int targetId);
	size_t targetsSize() { return m_targets.size(); }

	ClassificationResult getResult() { return m_result; }
	void resetResult();

private:
	int m_iViewId;
	bool m_bIsClassified;
	
	ClassificationResult m_result;
	std::vector<std::shared_ptr<TargetObject>> m_targets;
	std::vector<cv::Mat> m_batchedImage;
	std::vector<ClassificationResult> m_batchedResult;

	std::shared_ptr<BaseCamera> m_pCamera;

	void initTargetObjects(const int targetNumbers);
	void setResult(const ClassificationResult result) { m_result = result; }
};

#endif // VIEW_H