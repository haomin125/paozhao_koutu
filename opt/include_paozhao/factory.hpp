#ifndef FACTORY_HPP
#define FACTORY_HPP

#include "classifier_result.h"

//
// template class for type view, board, and lines
// pre-condition:
//		T has method getResult() to get classificationResult
//      T has method setResult() to set classificationResult
//	

template <class T>
class Factory
{
public:
	static ClassificationResult merge(std::vector<std::shared_ptr<T>> &type);
	static ClassificationResult distribute(std::vector<std::shared_ptr<T>> &type, const std::vector<ClassificationResult> &results);
};

//
// merge classified result from bottom to the top, return merged result to the current level
// for example: merge results in view, will merge all resutls from its targets, and set back to view
//
template <class T> ClassificationResult Factory<T>::merge(std::vector<std::shared_ptr<T>> &type)
{
	ClassificationResult result = ClassifierResultConstant::Good;
	for (const auto& itr : type) {
		if (itr->getResult() != ClassifierResultConstant::Good)
			result = itr->getResult();
	}

	return result;
}

//
// distribute the classified result from the top to bottom, return result to the current level
// for example, distribute the result to the each terget, and merge them , and set back to view
//
template <class T> ClassificationResult Factory<T>::distribute(std::vector<std::shared_ptr<T>> &type, const std::vector<ClassificationResult> &results)
{
	if (type.size() != results.size())
		return ClassifierResultConstant::Classifying;

	ClassificationResult returnedResult = ClassifierResultConstant::Good;
	for (int idx = 0; idx < type.size(); idx++) {
		if (type[idx]->getResult() == ClassifierResultConstant::Classifying ||
			results[idx] != ClassifierResultConstant::Good)
		{
			type[idx]->setResult(results[idx]);

		}
		if (type[idx]->getResult() != ClassifierResultConstant::Good)
		{
			returnedResult = type[idx]->getResult();
		}
	}

	return returnedResult;
}

#endif // FACTORY_HPP