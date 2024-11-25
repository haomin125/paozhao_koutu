#ifndef TRACKER_H
#define TRACKER_H

#include <string>
#include <deque>
#include <vector>
#include <mutex>
#include <memory>

#include "classifier_result.h"
//
//  each app should have its derived tracking object class, which should defined its own 
//  data object, please refer the example in tracker_test.cpp
//
class BaseTrackingObject {
public:
	BaseTrackingObject();
	BaseTrackingObject(const BaseTrackingObject &object);
	virtual ~BaseTrackingObject() {};
	  
	int objectIndex() { return m_iObjectIndex; }
	void objectIndex(const int index) { m_iObjectIndex = index; }

	// for backward compatible, getBatchedResult need to be overriden in app 
	// if app detector turns on exposeTargetResults!!!
	virtual ClassificationResult getResult() = 0;
	virtual bool getBatchedResult(std::vector<std::vector<ClassificationResult>> &result);
private:
	int m_iObjectIndex;
};

//
//  each app should have its own tracking queue, and using its own data object created in shared_ptr
//  c++ polymorphism and dynamic binding will let you stay in your own data type, not use the base
//
class BaseTrackingHistory
{
public:
	BaseTrackingHistory(const int plcDistance) : 
		m_bNeedCalibrate(false), 
		m_iPlcDistance(plcDistance) { m_objects.clear(); };
	virtual ~BaseTrackingHistory() {};

	// makeDecision should be always at the front of the queue, after make decision, 
	// it should pop the element off the queue to cleanup 
	virtual bool makeDecision(ClassificationResult &result);
	virtual bool makeDecision(std::vector<std::vector<ClassificationResult>> &result);

	// re-arrange the existing queue if required
	virtual bool calibrate() = 0;

	// update queue and check if we have any missed one before or after
	virtual void addToTrackingHistory(const std::shared_ptr<BaseTrackingObject> object_ptr);

	void resetCalibrate(bool flag) { m_bNeedCalibrate = flag; }
	bool needCalibrate() { return m_bNeedCalibrate; }

	int plcDistance() { return m_iPlcDistance; }

	size_t trackingSize() { return m_objects.size(); }
	int firstObjectIndex() { return m_objects.front()->objectIndex(); }
protected:
	// make this one protected, so subclass can use all deque functions to manipulate it
	// BaseTrackingObject can be the any derived tracking objects when you defined
	// this queue saves the tracking objects history
	std::deque<std::shared_ptr<BaseTrackingObject>> m_objects;

	virtual void removeFromTrackingHistory() { m_objects.pop_front(); }
private:
	bool m_bNeedCalibrate;
	int m_iPlcDistance;
};

//
// this is almost empty tracker class, which presents how many boards we will use per line for tracking
//
class BaseTracker {
public:
	BaseTracker(const int boards);
	virtual ~BaseTracker() {}

	int totalTrackingHistories() { return m_iTotalTrackingHistories; }

	virtual void setHistory(const int boardId, const std::shared_ptr<BaseTrackingHistory> queue_ptr);
	virtual std::shared_ptr<BaseTrackingHistory> getHistory(const int boardId);
	virtual size_t historySize(const int boardId);
protected:
	std::vector<std::shared_ptr<BaseTrackingHistory>> m_histories;

	// add mutex here, in case if we need to compare histoies in 2 or more boards, since usually each board may run in one thread
	static std::mutex m_mutex;
private:
	int m_iTotalTrackingHistories;
};

#endif // TRACKER_H