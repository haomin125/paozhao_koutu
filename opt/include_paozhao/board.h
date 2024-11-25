#ifndef BOARD_H
#define BOARD_H

#include "view.h"

class Board
{
public:
	Board();
	Board(const int boardId, const int viewNumbers, const int targetNumbers);
	virtual ~Board() {};

	bool setViewCamera(const int viewId, const std::shared_ptr<BaseCamera> pCamera);
	std::shared_ptr<BaseCamera> getViewCamera(const int viewId);

	// common methods used by view, board, and lines
	bool merge();
	bool distribute(const std::vector<std::vector<ClassificationResult>> &results);

	std::vector<std::vector<cv::Mat>> &imageBatch() { return m_batchedImage; }
	std::vector<std::vector<ClassificationResult>> &resultBatch() { return m_batchedResult; }

	// other view related methods
	int boardId() { return m_iBoardId; }
	bool isValid() { return m_iBoardId >= 0 && m_views.size() > 0; }

	bool isClassified() { return m_bIsClassified; }
	void setClassificationLevel(const bool level) { m_bIsClassified = level; }

	std::shared_ptr<View> getView(const int viewId);
	size_t viewsSize() { return m_views.size(); }

	ClassificationResult getResult() { return m_result; }
	void resetResult();

	int productCount() { return m_iProductCount; }
private:
	int m_iBoardId;
	bool m_bIsClassified;
	int m_iProductCount;

	ClassificationResult m_result;
	std::vector<std::shared_ptr<View>> m_views;
	std::vector<std::vector<cv::Mat>> m_batchedImage;
	std::vector<std::vector<ClassificationResult>> m_batchedResult;

	void initViews(const int viewNumbers, const int targetNumbers);
	void setResult(const ClassificationResult result) { m_result = result; }
};

#endif // BOARD_H