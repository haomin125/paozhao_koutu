#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>

#include <boost/property_tree/ptree.hpp>

class BaseConfig {
public:
	BaseConfig() {}
	virtual ~BaseConfig() {}

	virtual bool setConfig() = 0;
};

class CameraConfig : public BaseConfig {
public:
	CameraConfig();
	CameraConfig(const std::string &model, const bool blockRead = true);
	CameraConfig(const std::string &model, const int timeout, const bool blockRead = true);
	virtual ~CameraConfig() {}

	virtual bool setConfig();

	virtual std::stringstream getConfigParameters() = 0;
	virtual void setConfigParameters(const std::stringstream &parameters) = 0;

	std::string& modelId() { return m_sModelId; }
	void modelId(const std::string &model) { m_sModelId = model; }

	// set camera read timeout in milliseconds
	int timeout() const { return m_iTimeout; }
	void timeout(const int time) { m_iTimeout = time; }

	bool isBlockRead() { return m_bBlockRead; }

	struct ImageDimension
	{
		int64_t m_width;
		int64_t m_height;
		int64_t m_fpsOrLineRate;

		ImageDimension(const int w, const int h, const int flr) : m_width(w), m_height(h), m_fpsOrLineRate(flr) {}
		ImageDimension(const ImageDimension &x) : m_width(x.m_width), m_height(x.m_height), m_fpsOrLineRate(x.m_fpsOrLineRate) {}
		friend std::ostream &operator<<(std::ostream &os, const ImageDimension &t)
		{
			os << std::endl << "Width: " << t.m_width << ", Height: " << t.m_height << ", FPS: " << t.m_fpsOrLineRate << std::endl;
			return os;
		}
	};

	struct ImageOffset
	{
		int m_x;
		int m_y;

		ImageOffset() : m_x(0), m_y(0) {}
		ImageOffset(const int x, const int y) : m_x(x), m_y(y) {}
		ImageOffset(const ImageOffset &offset) : m_x(offset.m_x), m_y(offset.m_y) {}
		friend std::ostream &operator<<(std::ostream &os, const ImageOffset &t)
		{
			os << std::endl << "Offset: " << t.m_x << ", " << t.m_y << std::endl;
			return os;
		}
	};
protected:
	void readPtree(const std::stringstream &content);
	std::stringstream writePtree();

	boost::property_tree::ptree m_root;
private:
	std::string m_sModelId;
	bool m_bBlockRead;
	int m_iTimeout;
};

/*
** We add 2 new parameters in product line configuration, one is the map from board to each views, another is the map from view to the board
** precondition: each view has its own unique id, for example, we do not allow 2 views have the same id in our system, even it is in 
**               different board
*/
class ProductLineConfig : public BaseConfig {
public:
	ProductLineConfig() : BaseConfig() {}
	ProductLineConfig(const std::map<int, std::vector<int>> &boardToView, const std::map<int, int> &viewToBoard, const int targetsPerView);
	ProductLineConfig(const std::map<int, std::vector<int>> &boardToView, const std::map<int, int> &viewToBoard, const std::vector<int> &targetsPerView);
	virtual ~ProductLineConfig() {}

	virtual bool setConfig() = 0;

	bool isValid();

	int totalBoards() const { return (int)m_boardToView.size(); }
	int totalViews() const { return (int)m_viewToBoard.size(); }

	int targetsPerView() const { return theSame(m_targetsPerView); }
	int targetsPerView(const int viewId) const { return m_targetsPerView[viewId]; }

	// we assume the targets in all view per board should be the same.
	// this method will return 0 if we broke this restriction
	int targetsInViewPerBoard(const int boardId);

	std::vector<int> &getViews(const int boardId) { return m_boardToView[boardId]; }
	int viewsPerBoard(const int boardId) { return (int)m_boardToView[boardId].size(); }

	int getBoard(const int viewId) { return m_viewToBoard[viewId]; }
private:
	std::map<int, std::vector<int>> m_boardToView;
	std::map<int, int> m_viewToBoard;
	std::vector<int> m_targetsPerView;

	int theSame(const std::vector<int> &t) const
	{
		return std::adjacent_find(t.begin(), t.end(), std::not_equal_to<int>()) == t.end() ? t[0] : 0;
	}
};

class WorkflowConfig : public BaseConfig {
public:
	WorkflowConfig(const int retries = 0) : BaseConfig(), m_iRetries(retries) {}
	virtual ~WorkflowConfig() {}

	virtual bool setConfig() = 0;

	int retries() { return m_iRetries; }
	void setRetries(const int retries) { m_iRetries = retries; }
private:
	int m_iRetries;
};

class DetectorConfig : public BaseConfig {
public:
	DetectorConfig();
	DetectorConfig(const int workflowNumbers);
	virtual ~DetectorConfig() {}

	virtual bool setConfig() = 0;

	int totalWorkflows() const { return m_iTotalWorkflows; }

	bool exposeTargetResults() const { return m_bExposeTargetResults; }
	void exposeTargetResults(const bool expose) { m_bExposeTargetResults = expose; }

	// the real workflow config should be defined in its subclass by each app
	virtual std::shared_ptr<WorkflowConfig> getWorkflowConfig(const int workflowId) = 0;
protected:
	std::vector<std::shared_ptr<WorkflowConfig> > m_workflowConfigs;

	template<typename T> void initWorkflowConfigs() {
		m_workflowConfigs.clear();
		for (int idx = 0; idx < m_iTotalWorkflows; idx++) {
			m_workflowConfigs.emplace_back(std::make_shared<T>());
		}
	}
private:
	int m_iTotalWorkflows;
	bool m_bExposeTargetResults;
};

#endif // CONFIG_H