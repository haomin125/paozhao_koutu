#include "xj_app_detector.h"
#include "xj_app_tracker.h"
#include "running_status.h"
#include "xj_app_io_manager.h"
// #include "xj_app_classifier.h"
#include "shared_utils.h"
#include "customized_json_config.h"

using namespace std;

bool AppDetector::createBoardTrackingHistory(const int plcDistance)
{
	if (m_pTracker == nullptr || m_pBoard == nullptr)
	{
		return false;
	}

	if (!m_pTracker->getHistory(m_pBoard->boardId()))
	{
		m_pTracker->setHistory(m_pBoard->boardId(), make_shared<AppTrackingHistory>(plcDistance));
	}
	m_pTrackingHistory = dynamic_pointer_cast<AppTrackingHistory>(m_pTracker->getHistory(m_pBoard->boardId()));

	return true;
}

bool AppDetector::addToTrackingHistory()
{
	if (m_pBoard == nullptr)
	{
		return false;
	}

	shared_ptr<AppTrackingObject> pObject = make_shared<AppTrackingObject>();
	pObject->setBoard(m_pBoard);
	m_pTrackingHistory->addToTrackingHistory(pObject);
	return true;
}

// app need to write its own purge method to handle classification result
//
// framework has defined purgeBoardResult() with single result, which will 
// purge eveything if result is not ClassificationResult::Good
//
// if you are using purgeBoardResult() with single result, and you can choose
// use the one in framework and remove one below or write your own
bool AppDetector::purgeBoardResult(const std::vector<std::vector<ClassificationResult>> &result, const bool needCalculationResult)
{
    int boardCount = dynamic_pointer_cast<AppWorkflow>(m_workflows[0])->getBoardCount();
    int tabletCountPerBoard = dynamic_pointer_cast<AppWorkflow>(m_workflows[0])->getTabletCountPerBoard();

    int getRowCount = dynamic_pointer_cast<AppWorkflow>(m_workflows[0])->getRowCount();
    int getColCount = RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::TABLET_ROI_COL_COUNT);
    int lastColCount = dynamic_pointer_cast<AppWorkflow>(m_workflows[0])->getlastColCount();
    bool is_pihao = dynamic_pointer_cast<AppWorkflow>(m_workflows[0])->getpihao();

    int lotNumCount = RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::LOTNUM_COUNT_PER_BOARD);
    m_usemodel = RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::USE_MODEL);

    int mergeRes = ClassifierResultConstant::Good;
    bool signal_convert = CustomizedJsonConfig::instance().get<bool>("signal_convert");
    std::array<bool, 16> signals{false};
    std::vector<int> mergeResults(8);

    // cout << result.size() << endl;
    // cout << result << endl;
    
    int tabletIdx(0);
    for (int viewIdx = 0; viewIdx < result.size(); viewIdx++)       //2个相机
    {
        for (int tabletBoardIdx = 0; tabletBoardIdx < boardCount; tabletBoardIdx++)         //每个相机2板药
        {
            int latColCount_ = viewIdx==0 ? lastColCount: getColCount -lastColCount;
            int tabletCountPerBoard_ = tabletBoardIdx==0 ? tabletCountPerBoard:getRowCount * latColCount_;
            for (int tabletIdxPerBoard = 0; tabletIdxPerBoard < tabletCountPerBoard_; tabletIdxPerBoard++)           //每板药有8粒
            {
                if (tabletIdx >= result[viewIdx].size())
                    continue;
                //cout <<  "tabletBoardIdx: " << tabletBoardIdx << " tabletIdx: " << tabletIdx << " boardId: " << m_pBoard->boardId() << " viewId: " << viewIdx << " targetId: " << tabletIdx << " result: " << result[viewIdx][tabletBoardIdx*tabletCountPerBoard +tabletIdx] << endl;
                if (result[viewIdx][tabletBoardIdx*tabletCountPerBoard +tabletIdx] > ClassifierResultConstant::Good)
                {
                    mergeRes = result[viewIdx][tabletBoardIdx*tabletCountPerBoard +tabletIdx];
                }
                if (tabletIdx % tabletCountPerBoard_ == tabletCountPerBoard_ - 1)
                {
                    if(is_pihao)
                    { 
                        if((viewIdx+tabletBoardIdx)!=2)
                            if(result[viewIdx][tabletCountPerBoard*(boardCount-1) +getRowCount * (getColCount-3)+tabletBoardIdx] > ClassifierResultConstant::Good)
                                mergeRes = result[viewIdx][tabletCountPerBoard*(boardCount-1) +getRowCount * (getColCount-3)+tabletBoardIdx];
                    }
                    mergeResults[boardCount * (m_pBoard->boardId()*result.size() + viewIdx) + tabletBoardIdx] = mergeRes;
                    mergeRes = ClassifierResultConstant::Good;
                    tabletIdx = 0;
                }
                else
                {
                    tabletIdx++;
                }
            }
        }
    }
    if(mergeResults[3] != ClassifierResultConstant::Good)
        mergeResults[1] = mergeResults[3];
    int signal_type = RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::TEST_TYPE);
    int channel = RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::TEST_CHANNEL);
    bool signal_anable = CustomizedJsonConfig::instance().get<bool>("signal_convert");
    if(signal_type == 0)
    {
        int signal_num  = 0;
        for (int i = 0; i < 3; i++)
        {
            // cout << "mergeResults[ " << i << " ] result: " << mergeResults[i] << endl;
            RunningInfo::instance().GetRunningData().ProcessClassifyResult(m_pBoard->boardId(), mergeResults[i], productCount());
            if (mergeResults[i] == ClassifierResultConstant::Good)
            {
                signals[i] = true;
                signal_num +=1;
            }  
        }
        if(signal_anable)
        {
            for(int i = 0; i < 3; i++)
                signals[i] = signal_num !=3; 
        }
        cout<<"productCount: "<<productCount()<<" , signal: ";
        for (size_t i = 0; i < 3; i++)
        {
            cout<<signals[i]<<" ";
        }
        
        cout<<endl;
    }
    else
    {
        std::vector<int> mergeResults(12);
        signal_test(signals,mergeResults,signal_type,channel);
    }

    return dynamic_pointer_cast<AppIoManager>(ioManager())->sendGoodSignals(0, signals);
    // return true;
}


bool AppDetector::signal_test(std::array<bool, 16>  &signals,std::vector<int> mergeResults,const int signal_type,const int channel)
{

    if(signal_type==1)
    {
        dynamic_pointer_cast<AppIoManager>(ioManager())->sendGoodSignals_horse();
    }
    if(signal_type == 2)
    {
        cout<<"signal:";                        
        for(int i=0 ;i< mergeResults.size();i++)
        {
            if(i==channel)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
                signals[i]=true;
            cout<<" "<<signals[i];
        }
        cout<<endl;
    }
     if(signal_type == 3)
    {
        cout<<"signal:";                        
        for(int i=0 ;i< channel;i++)
        {
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
            signals[i]=true;
            cout<<" "<<signals[i];
        }
        cout<<endl;
    }

}


bool AppDetector::reconfigWorkflow()
{
    for (const auto &itr : m_workflows)
    {
        dynamic_pointer_cast<AppWorkflow>(itr)->reconfigParameters();
    }
    return true;
}

// bool AppDetector::decideToSave(const cv::Mat &img, const ClassificationResult result, const std::string &imgName)
// {
//     cout << "imgName: " << imgName << endl;
    
//     return true;

// }


bool AppDetector::customizedSavedImageName(const std::string &name, const int viewIdx, const int targetIdx, std::string &returned)
{
    // cout << "name: " << name << endl;
    returned = name;
    returned = returned.replace(returned.find("TYPE"), string("TYPE").length(), string("C"));
    returned = returned.replace(returned.find("TIME"), string("TIME").length(), string("TM"));
    returned = returned.replace(returned.find("BOARD"), string("BOARD").length(), string("B"));
    returned = returned.replace(returned.find("ID"), string("ID").length(), string("V"));
    returned = returned.replace(returned.find("TARGET"), string("TARGET").length(), string("T"));
    returned = returned.substr(0, returned.find("PROD")) + RunningInfo::instance().GetProductionInfo().GetCurrentProd() + "-" + RunningInfo::instance().GetProductionInfo().GetCurrentLot() + ".png";
    vector<vector<float>> score_list;
    

    vector<float> score_1 = dynamic_pointer_cast<AppWorkflow>(m_workflows[0])->getGoodCategoryScores();
    vector<float> score_2 = dynamic_pointer_cast<AppWorkflow>(m_workflows[1])->getGoodCategoryScores();
    score_list.emplace_back(score_1);
    score_list.emplace_back(score_2);

    int socre_size = score_1.size() - score_2.size();// if score_1.size()>score_2.size() else score_2.size();
    for(size_t i=0; i < abs(socre_size);i++)
    {  
        if(socre_size<0)
            score_1.emplace_back(0); 
        else    
            score_2.emplace_back(0);
    }

    // cout<<returned<<endl;
    returned = shared_utils::getImageFileNameWithGS(returned, score_list, viewIdx, targetIdx);
    

    return true;
}

bool AppDetector::decideToSave(const cv::Mat &img, const ClassificationResult result, const std::string &imgName)
{
    // logDEBUG<<"start to save Image";

    string newImageName(imgName);
    if (result == ClassifierResultConstant::Good)
    {
        double locateScore = CustomizedJsonConfig::instance().get<double> ("GOOD_SCORE");
        
        double goodScore = atoi((newImageName.substr(newImageName.size()-10,3)).c_str())/1000.0;
        if(!m_usemodel)
            return BaseDetector::decideToSave(img,result,imgName);
        if (goodScore > locateScore || goodScore ==0)
        {
            return true;
        }    
    }
    return BaseDetector::decideToSave(img,result,imgName);
}
