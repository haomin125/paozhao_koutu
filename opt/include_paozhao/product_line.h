#ifndef PRODUCT_LINE_H
#define PRODUCT_LINE_H

#include "config.h"
#include "board.h"

class ProductLine
{
public:
	ProductLine();
	ProductLine(const std::shared_ptr<ProductLineConfig> pConfig);
	virtual ~ProductLine() {};

	void setConfig(const std::shared_ptr<ProductLineConfig> pConfig) { m_pConfig = pConfig; }
	std::shared_ptr<ProductLineConfig> getConfig() { return m_pConfig; }

	bool setBoardCamera(const int boardId, const std::vector<std::shared_ptr<BaseCamera>> &cameras);
	std::shared_ptr<BaseCamera> getBoardCamera(const int boardId, const int viewId);

	// common methods used by view, board, and lines
	bool merge();
	bool distribute(const std::vector<std::vector<std::vector<ClassificationResult>>> &results);

	std::vector<std::vector<std::vector<cv::Mat>>> &imageBatch() { return m_batchedImage; }
	std::vector<std::vector<std::vector<ClassificationResult>>> &resultBatch() { return m_batchedResult; }

	// other view related methods
	bool isValid() { return m_boards.size() > 0; }

	bool isClassified() { return m_bIsClassified; }
	void setClassificationLevel(const bool level) { m_bIsClassified = level; }

	std::shared_ptr<Board> getBoard(const int boardId);
	size_t boardsSize() { return m_boards.size(); }

	ClassificationResult getResult() { return m_result; }
	void resetResult();

	int productCount() { return m_iProductCount; }
private:
	bool m_bIsClassified;
	int m_iProductCount;
	std::shared_ptr<ProductLineConfig> m_pConfig;

	ClassificationResult m_result;
	std::vector<std::shared_ptr<Board>> m_boards;
	std::vector<std::vector<std::vector<cv::Mat>>> m_batchedImage;
	std::vector<std::vector<std::vector<ClassificationResult>>> m_batchedResult;

	void initBoards();
	void setResult(const ClassificationResult result) { m_result = result; }
};

#endif // PRODUCT_LINE_H