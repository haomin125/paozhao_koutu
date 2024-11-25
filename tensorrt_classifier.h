#ifndef TENSORRT_H
#define TENSORRT_H

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <opencv2/opencv.hpp>
#include "cuda_runtime_api.h"
#include "NvInfer.h"
#include <functional>
#include <mutex>
#define BINDING_SIZE 3
enum  class TensorrtOutputType : int
{
    CATEGORY         = 1,
    SEGMENT          = 2,
    CATEGORY_SEGMENT = 3
};

class LoggerNV : public nvinfer1::ILogger
{
public:
    static LoggerNV &instance()
    {
        static LoggerNV instance;
        return instance;
    }
private:
    LoggerNV() {}
    virtual ~LoggerNV() {}
    using Severity = nvinfer1::ILogger::Severity;
    void log(Severity severity, const char *msg) noexcept override
    {
        // suppress info-level messages
        if (severity <= Severity::kWARNING)
        {
            std::cout << msg << std::endl;
        }
    }
};

class TensorrtClassifier
{
public:
    struct ScoreData
    {
        std::vector<int> badClass;
        std::vector<float> maxBadScore;
        std::vector<float> goodScore;
    };
    /**
     * @brief 构造函数 
     * @param[in] {sModelPath 模型路径}
     * @param[in] {tensorrtOutputType 模型输出类型如:分类,分割,多任务}
     * @param[in] {batchSize 模型一次推理多少张图片}
     * @param[in] {numCategory 模型分类数}
     * @param[in] {inputWidth 模型输入图片宽度}
     * @param[in] {inputHeight 模型输入图片高度}
     * @param[in] {inputChannel 模型输入图片通道数(默认３通道)}
     */    
    TensorrtClassifier(const std::string &sModelPath,const TensorrtOutputType tensorrtOutputType,const int batchSize,const int numCategory,const int inputWidth, const int inputHeight, const int inputChannel = 3);
    ~TensorrtClassifier();
    /**
     * @brief 加载模型
     * @return {模型加载是否成功}
     */    
    bool loadModel(); 
    /**
     * @brief  设置模型参数
     * @param {tensorrtOutputType 模型输出格式}
     * @param {batchSize 一次推理大小}
     * @param {numCategory 模型分类数}
     * @param {inputWidth 输入图片宽度}
     * @param {inputHeight 输入图片高度}
     * @param {inputChannel 输入图片通道数}
     */     
    void setModelParameters(const TensorrtOutputType tensorrtOutputType,const int batchSize,const int numCategory,const int inputWidth,const int inputHeight, const int inputChannel);
    /**
     * @brief 获取分类模型结果(适用于纯分类模型)
     * @param[in] {vBatchImage  输入图片}
     * @param[in] {vSensitivity 分类灵敏度}
     * @param[out] {vClsIndex   输出分类类别0 GOOD >0 BAD}
     * @param[out] {vClsScore   输出分类分数}
     * @param[in] {is_softMax  是否需要对分类结果做softMax,默认需要}
     * @return {获取结果是否成功}
     */    
    bool getClassificationResult(const std::vector<cv::Mat> &vBatchImage,const std::vector<float> &vSensitivity, std::vector<int> &vClsIndex, std::vector<float> & vClsScore,bool is_softMax = true);
    /**
     * @brief 获取多分类模型结果(适用于纯分类模型) 后处理逻辑优化
     * @param[in] {vBatchImage  输入图片}
     * @param[in] {vSensitivity 分类灵敏度}
     * @param[out] {vClsIndex   输出分类类别0 GOOD >0 BAD}
     * @param[out] {vClsScore   输出分类分数}
     * @param[out] {inferenceScore 得分数据集}
     * @param[in] {is_softMax  是否需要对分类结果做softMax,默认需要}
     * @return {获取结果是否成功}
     */    
    bool getMultiClassificationResult(const std::vector<cv::Mat> &vBatchImage,const std::vector<float> &vSensitivity, std::vector<int> &vClsIndex, std::vector<float> & vClsScore,ScoreData &inferenceScore,bool is_softMax = true);
    /**
     * @brief 获取分类跟分割结果(适用于多任务模型)
     * @param[in]  {vBatchImage  　输入图片}
     * @param[in]  {vSensitivity 　分类灵敏度}
     * @param[out] {vClsIndex   输出分类类别0 GOOD >0 BAD}
     * @param[out] {vSegmentMaskImage 输出分割mask结果图}
     * @param[out] {vClsScore  输出分类分数}
     * @param[out] {is_softMax 是否需要对分类结果做softMax,默认需要}
     * @return {获取分类分割结果是否成功}
     */    
    bool getClassifyAndSegmentResult(const std::vector<cv::Mat> &vBatchImage,const std::vector<float> &vSensitivity,std::vector<int> &vClsIndex,std::vector<cv::Mat> &vSegmentMaskImage,std::vector<float> & vClsScore,bool is_softMax = true);
    /**
     * @brief 获取分类跟分割结果(适用于多任务模型) 后处理逻辑优化
     * @param[in]  {vBatchImage  　输入图片}
     * @param[in]  {vSensitivity 　分类灵敏度}
     * @param[out] {vClsIndex   输出分类类别0 GOOD >0 BAD}
     * @param[out] {vSegmentMaskImage 输出分割mask结果图}
     * @param[out] {vClsScore  输出分类分数}
     * @param[out] {inferenceScore 得分数据集}
     * @param[out] {is_softMax 是否需要对分类结果做softMax,默认需要}
     * @return {获取分类分割结果是否成功}
     */  
    bool getMultiClassifyAndSegmentResult(const std::vector<cv::Mat> &vBatchImage,const std::vector<float> &vSensitivity,std::vector<int> &vClsIndex,std::vector<cv::Mat> &vSegmentMaskImage,std::vector<float> & vClsScore,ScoreData &inferenceScore,bool is_softMax = true);
    /**
     * @brief 获取分割结果(适用于纯分割模型)
     * @param[in]  {vBatchImage  　输入图片}
     * @param[out] {vSegmentMaskImage 输出分割mask结果图}
     * @return {获取分割结果是否成功}
     */    
    bool getSegmentResult(const std::vector<cv::Mat> &vBatchImage, std::vector<cv::Mat> &vMaskImage);   
    /**
     * @brief 获取batchSize
     * @return {返回batchSize}
     */    
    int getBatchSize(){return m_batch_size;};
    /**
     * @brief 获取输入图片宽度
     * @return {返回输入图片宽度}
     */ 
    int getImageWidth(){return m_input_w;};
    /**
     * @brief 获取输入图片高度
     * @return {返回输入图片高度}
     */ 
    int getImageHeight(){return m_input_h;};
    /**
     * @brief 获取模型分类数
     * @return {返回模型分类数}
     */ 
    int getImageCategory(){return m_numCategory;};
private:
    /**
     * @brief 创建或者拷贝内存
     * @param[in] {method 选择创建还是拷贝内存方法}
     * @return {内存操作是否成功}
     */    
    bool createOrCopyBuffers(const int method);
    //判断engine模型是否为动态模型
    /**
     * @brief 判断engine模型是否为动态模型
     * @return {返回模型是否为动态模型}
     */    
    bool hasDynamicDim();
    /**
     * @brief 根据动态batch size重新设置input dim维度 
     * @param[in] {ibinding 模型的ibinding}
     * @param[in] {dims     模型的维度}
     * @return {是否设置成功}
     */    
    bool setRunDims(int ibinding,const std::vector<int> & dims);
    /**
     * @brief 对分类结果做softmax
     * @param[in] {src 输入数据}
     * @param[out] {dst 输出数据}
     * @param[in] {numCategory 模型分类数}
     */    
    template<typename T>
    void softmax(const T* src,T * dst,const int numCategory);
    /**
     * @brief 打印模型内部参数
     */    
    void print();
protected:
    /**
     * @brief 对输入图片进行归一化
     * @param[in] {srcImg 输入图片}
     * @param[out] {dstImg 输出图片}
     */    
    void normalizeImage(const cv::Mat &srcImg, cv::Mat &dstImg);
    /**
     * @brief 模型推理 
     * @param[in]  {vBatchImage  　输入图片}
     * @return {推理是否成功}
     */ 
    bool inference(const std::vector<cv::Mat> &vBatchImage);
    /**
     * @brief 分类后处理
     * @param[in] {vBatchImage  输入图片}
     * @param[in] {vSensitivity 分类灵敏度}
     * @param[out] {vClsIndex   输出分类类别0 GOOD >0 BAD}
     * @param[out] {vClsScore   输出分类分数}
     * @param[out] {is_softMax  是否需要对分类结果做softMax,默认需要}
     * @return {后处理是否成功}
     */  
    bool classifyProcess(const std::vector<cv::Mat> &vBatchImage,const std::vector<float> &vSensitivity,std::vector<int> &vClsIndex,std::vector<float> & vClsScore,bool is_softMax = true);
    /**
     * @brief 分类后处理
     * @param[in] {vBatchImage  输入图片}
     * @param[in] {vSensitivity 分类灵敏度}
     * @param[out] {vClsIndex   输出分类类别0 GOOD >0 BAD}
     * @param[out] {vClsScore   输出分类分数}
     * @param[out] {is_softMax  是否需要对分类结果做softMax,默认需要}
     * @return {后处理是否成功}
     */  
    bool multiClassifyProcess(const std::vector<cv::Mat> &vBatchImage,const std::vector<float> &vSensitivity,std::vector<int> &vClsIndex,std::vector<float> & vClsScore,bool is_softMax = true);
    /**
     * @brief 分类后处理
     * @param[in] {vBatchImage  输入图片}
     * @param[in] {vSensitivity 分类灵敏度}
     * @param[out] {vClsIndex   输出分类类别0 GOOD >0 BAD}
     * @param[out] {vClsScore   输出分类分数}
     * @param[out] {is_softMax  是否需要对分类结果做softMax,默认需要}
     * @return {后处理是否成功}
     */  
    bool multiClassifyProcess(const std::vector<cv::Mat> &vBatchImage,const std::vector<float> &vSensitivity,std::vector<int> &vClsIndex,std::vector<float> & vClsScore,ScoreData &inferenceScore,bool is_softMax = true);
    /**
     * @brief 分割后处理(适用于纯分割模型)
     * @param[in]  {vBatchImage  　输入图片}
     * @param[out] {vSegmentMaskImage 输出分割mask结果图}
     * @return {分割后处理是否成功}
     */ 
    bool segmentProcess(const std::vector<cv::Mat> &vBatchImage, std::vector<cv::Mat> &vMaskImage);
private:
    std::string m_sModelPath;//模型路径
    cudaStream_t m_stream;
    std::shared_ptr<nvinfer1::IRuntime> m_runtime;
    std::shared_ptr<nvinfer1::ICudaEngine> m_engine;
    std::shared_ptr<nvinfer1::IExecutionContext> m_context;
    float* m_buffers[BINDING_SIZE] = {nullptr}; // cpu buffers for input and output data
    void* m_gpu_buffers[BINDING_SIZE] = {nullptr};; // gpu buffers for input and output data
    //模型参数
    int m_batch_size;
    int m_numCategory;
    int m_input_w;
    int m_input_h;
    int m_input_c;
    TensorrtOutputType m_tensorrtOutputType;//模型输出类型
    // LoggerNv gLogger;
};


#endif //TENSORRT_H
