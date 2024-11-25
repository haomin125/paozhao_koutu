#ifndef CLASSIFIER_RESULT_H
#define CLASSIFIER_RESULT_H

#include <string>
#include <map>
#include <vector>

typedef int ClassificationResult;

class ClassifierResult
{
public:
	ClassifierResult(const std::string &appName, const std::string& fileName);
	virtual ~ClassifierResult() {};

	static ClassificationResult getResult(const std::string& key);
	static std::string getName(const ClassificationResult key);
	static std::string getNameByBoard(const int boardId, const ClassificationResult key);

	static int getResultSize() { return static_cast<int>(Result.size()); }
	static int getNameSize() { return static_cast<int>(Name.size()); }
	static int getDatectorSize() { return static_cast<int>(DetectorToCategory.size()); }

private:
	void loadClassifierResult();

	std::string m_appName;
	std::string m_fileName;

	static std::map<std::string, ClassificationResult> Result;
	static std::map<ClassificationResult, std::string> Name;
	static std::vector<std::vector<std::string>> DetectorToCategory;
};

// those 3 should be same in all applications, we use that for constructor only
namespace ClassifierResultConstant
{
	const ClassificationResult Error = -1;
	const ClassificationResult Classifying = 0;
	const ClassificationResult Good = 1;
};

#endif // CLASSIFIER_RESULT_H