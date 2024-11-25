#ifndef TENSORRT_H
#define TENSORRT_H

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <opencv2/opencv.hpp>
#include "cuda_runtime_api.h"
#include "NvInfer.h"
#include "logging.h"

#define BINDING_SIZE 3

class TensorrtClassifier
{
public:
    TensorrtClassifier(const std::string &sModelPath,  const int batchSize, const int numCategory, const int inputWidth, const int inputHeight, const int inputChannel = 3);
    ~TensorrtClassifier();
    
    //加载模型
    bool loadModel();
    void setModelParameters(const int batchSize, const int numCategory, const int inputWidth, const int inputHeight, const int inputChannel);

    //获取分类结果
    bool getClassificationResult(const std::vector<cv::Mat> &vBatchImage, std::vector<int> &vClsIndex, std::vector<float> &vSensitivity);
    bool getClassificationResult(const std::vector<cv::Mat> &vBatchImage, std::vector<int> &vClsIndex, float sensitivity);
    //获取分割结果
    bool getSegmentResult(const std::vector<cv::Mat> &vBatchImage, std::vector<cv::Mat> &vMaskImage);
    bool softmax(const float* input,float* output,int num_classes);
    
    //获取分类分割结果
    bool getMultitaskResult(const std::vector<cv::Mat> &vBatchImage, std::vector<int> &vClsIndex,  std::vector<float> &good_score,float sensitivity, std::vector<cv::Mat> &vMaskImage);

    bool inference(const std::vector<cv::Mat> &vBatchImage);
    // bool inference(const cv::Mat &vBatchImage);
    
protected:
    void normalizeImage(const cv::Mat &srcImg, cv::Mat &dstImg);
    // bool inference(const std::vector<cv::Mat> &vBatchImage);

private:
    std::string m_sModelPath;//模型路径
    cudaStream_t m_stream;

    std::shared_ptr<nvinfer1::IRuntime> m_runtime;
    std::shared_ptr<nvinfer1::ICudaEngine> m_engine;
    std::shared_ptr<nvinfer1::IExecutionContext> m_context;
    
    //float* a[3]==float [3][num_classes];
    float* m_buffers[BINDING_SIZE]; // cpu buffers for input and output data
    void* m_gpu_buffers[BINDING_SIZE]; // gpu buffers for input and output data

    //模型参数
    int m_batch_size;
    int m_numCategory;
    int m_input_w;
    int m_input_h;
    int m_input_c;
    std::vector<float> m_class_senvity;

    LoggerNv gLogger;
};


#endif //TENSORRT_H
