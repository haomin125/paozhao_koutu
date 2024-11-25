#ifndef TARGET_OBJECT_H
#define TARGET_OBJECT_H

#include "classifier_result.h"
#include "bound_box.h"

#include <opencv2/opencv.hpp>
#include <vector>
#include <mutex>

//
// each target has its own image and result
//

class TargetObject
{
public:
	TargetObject();
	TargetObject(const int targetId);
	virtual ~TargetObject() {}

	int targetId() { return m_iTargetId; }
	bool isValid() { return m_iTargetId >= 0; }

	bool isClassified() { return m_bIsClassified; }
	void setClassificationLevel(const bool level) { m_bIsClassified = level; }

	void setBoundMargin(const unsigned int bottom, const unsigned int top,
						const unsigned int left, const unsigned int right);
	BoundingBox &getBoundMargin() { return m_box; }

	void setImage(const cv::Mat &image);
	cv::Mat getImage() { return m_image; }

	void setResult(const ClassificationResult result);
	ClassificationResult getResult() { return m_result; }
	void resetResult();

private:
	int m_iTargetId;
	bool m_bIsClassified;

	BoundingBox m_box;
	cv::Mat m_image;
	ClassificationResult m_result;

	// result is possible to be set by different thread, which may need to be locked
	static std::mutex m_resultMutex;
};

#endif  // TARGET_OBJECT_H