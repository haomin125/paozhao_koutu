#include "tensorrt_classifier.h"
#include <iostream>
#include "timer_utils.hpp"
#include "customized_json_config.h"
#include "running_status.h"

using namespace std;
using namespace cv;

template <typename _T>
shared_ptr<_T> make_nvshared(_T *ptr)
{
    return shared_ptr<_T>(ptr, [](_T *p) { p->destroy(); });
}

#define INPUT_INDEX 0
#define CLS_OUTPUT_INDEX 1
#define SEG_OUTPUT_INDEX 2

#define CHECK(status)                                          \
    do                                                         \
    {                                                          \
        auto ret = (status);                                   \
        if (ret != 0)                                          \
        {                                                      \
            std::cerr << "Cuda failure: " << ret << std::endl; \
            abort();                                           \
        }                                                      \
    } while (0)

// static Logger gLogger;

vector<unsigned char> load_file(const string &file)
{
    vector<uint8_t> data;
    ifstream in(file, ios::in | ios::binary);
    if (!in.is_open())
    {
        return data;
    }

    in.seekg(0, ios::end);
    size_t length = in.tellg();
    if (length > 0)
    {
        in.seekg(0, ios::beg);
        data.resize(length);
        in.read((char *)&data[0], length);
    }
    in.close();

    return data;
}

TensorrtClassifier::TensorrtClassifier(const string &sModelPath, const int batchSize, const int numCategory, const int inputWidth, const int inputHeight, const int inputChannel) : m_sModelPath(sModelPath),
                                                                                                                                                                                    m_batch_size(batchSize),
                                                                                                                                                                                    m_numCategory(numCategory),
                                                                                                                                                                                    m_input_w(inputWidth),
                                                                                                                                                                                    m_input_h(inputHeight),
                                                                                                                                                                                    m_input_c(inputChannel),
                                                                                                                                                                                    m_runtime(nullptr),
                                                                                                                                                                                    m_engine(nullptr),
                                                                                                                                                                                    m_context(nullptr)
{
    string strProdName = RunningInfo::instance().GetProductionInfo().GetCurrentProd();
    m_class_senvity = CustomizedJsonConfig::instance().getVector<float>(strProdName+"_sentivity");
}

TensorrtClassifier::~TensorrtClassifier()
{
    cudaStreamDestroy(m_stream);
    for (int i = 0; i < BINDING_SIZE; i++)
    {
        free(m_buffers[i]);
        CHECK(cudaFree(m_gpu_buffers[i]));
    }
}

bool TensorrtClassifier::loadModel()
{
    //step1: set gpu device No.
    cudaSetDevice(0);

    //step2:load file
    vector<unsigned char> engine_data = load_file(m_sModelPath);
    if (engine_data.size() == 0)
    {
        cout << "engine model is empty!!!" << endl;
        return false;
    }

    //step3:deserialize the engine
    //m_runtime = make_nvshared(nvinfer1::createInferRuntime(gLogger));
    // m_runtime.reset(nvinfer1::createInferRuntime(gLogger));
    
    m_runtime.reset(nvinfer1::createInferRuntime(TensorrtClassifier::gLogger));
    m_engine.reset(m_runtime->deserializeCudaEngine(engine_data.data(), engine_data.size(), nullptr));
    m_context.reset(m_engine->createExecutionContext());
    
    //step4:申请输入内存和显存
    CHECK(cudaStreamCreate(&m_stream));
    // const int maxBatchSize = m_batch_size;
    auto input_size = m_input_w * m_input_h * m_input_c * m_batch_size * sizeof(float);
    m_buffers[INPUT_INDEX] = (float *)malloc(input_size);
    CHECK(cudaMalloc(&m_gpu_buffers[INPUT_INDEX], input_size));

    //step5:申请输出内存和显存
    auto cls_output_size = m_numCategory * m_batch_size * sizeof(float);
    m_buffers[CLS_OUTPUT_INDEX] = (float *)malloc(cls_output_size);
    CHECK(cudaMalloc(&m_gpu_buffers[CLS_OUTPUT_INDEX], cls_output_size));

    auto seg_output_size = m_input_w * m_input_h * m_batch_size * sizeof(float);
    m_buffers[SEG_OUTPUT_INDEX] = (float *)malloc(seg_output_size);
    CHECK(cudaMalloc(&m_gpu_buffers[SEG_OUTPUT_INDEX], seg_output_size));

    return true;
}

void TensorrtClassifier::setModelParameters(const int batchSize, const int numCategory, const int inputWidth, const int inputHeight, const int inputChannel)
{
    m_batch_size = batchSize;
    m_numCategory = numCategory;
    // cout<<"numCategory:"<<numCategory<<endl;
    m_input_w = inputWidth;
    m_input_h = inputHeight;
    m_input_c = inputChannel;
}

void TensorrtClassifier::normalizeImage(const Mat &srcImg, Mat &dstImg)
{
    //step1:缩放到模型输入尺寸
    resize(srcImg, dstImg, Size(m_input_w, m_input_h));
    //step2:BGR转RGB
    cvtColor(dstImg, dstImg, COLOR_BGR2RGB);
    //step3:归一化到0-1
    dstImg.convertTo(dstImg, CV_32FC3, 1.0 / 255);
    //step4:减去均值
    subtract(dstImg, Scalar(0.485, 0.456, 0.406), dstImg);
    //step5:除以方差
    divide(dstImg, Scalar(0.229, 0.224, 0.225), dstImg);
}

bool TensorrtClassifier::inference(const vector<Mat> &vBatchImage)
{
    const int batch_size = vBatchImage.size(); //cgc
    if (batch_size == 0)
    {
        cout << "no images input!!!" << endl;
        return false;
    }
    //step1:对图像进行前处理，并将图像拷贝到指针cpu数组m_buffers[0]当中
    for (size_t i = 0; i < vBatchImage.size(); ++i) //cgc
    {
        Mat inputImg;
        normalizeImage(vBatchImage[i], inputImg); //[i] cgc
        vector<Mat> chw;
        for (size_t j = 0; j < m_input_c; ++j)
        {
            // chw.emplace_back(cv::Mat(Size(m_input_w, m_input_h), CV_32FC1, (float*)m_buffers[INPUT_INDEX] + (j + i * batch_size * m_input_c) * m_input_w * m_input_h));
            chw.emplace_back(cv::Mat(Size(m_input_w, m_input_h), CV_32FC1, (float *)m_buffers[INPUT_INDEX] + (j + i * m_input_c) * m_input_w * m_input_h));
        }
        split(inputImg, chw);
    }
    //step2:从内存到显存, 从CPU到GPU, 将输入数据拷贝到显存
    CHECK(cudaMemcpyAsync(m_gpu_buffers[INPUT_INDEX],
                          m_buffers[INPUT_INDEX],
                          m_input_w * m_input_h * m_input_c * batch_size * sizeof(float),
                          cudaMemcpyHostToDevice, m_stream));

    //step3:推理
    //m_context->enqueue(batch_size, m_gpu_buffers, m_stream, nullptr);
    m_context->enqueueV2(m_gpu_buffers, m_stream, nullptr);

    //step4:显存拷贝到内存
    CHECK(cudaMemcpyAsync(m_buffers[CLS_OUTPUT_INDEX],
                          m_gpu_buffers[CLS_OUTPUT_INDEX],
                          m_numCategory * batch_size * sizeof(float),
                          cudaMemcpyDeviceToHost, m_stream));

    CHECK(cudaMemcpyAsync(m_buffers[SEG_OUTPUT_INDEX],
                          m_gpu_buffers[SEG_OUTPUT_INDEX],
                          m_input_w * m_input_h * batch_size * sizeof(float),
                          cudaMemcpyDeviceToHost, m_stream));

    //step5:这个是为了同步不同的流
    cudaStreamSynchronize(m_stream);

    return true;
}

bool TensorrtClassifier::getClassificationResult(const vector<Mat> &vBatchImage, vector<int> &vClsIndex, vector<float> &vSensitivity)
{
    //step1: inference
    if (!inference(vBatchImage))
    {
        cout << "tensorrt dl inference failed!!!" << endl;
        return false;
    }

    if (vSensitivity.empty())
    {
        cout << "sensitivity is empty!!!" << endl;
        return false;
    }

    //step2: post-process
    for (int i = 0; i < vBatchImage.size(); ++i)
    {
        if ((float)m_buffers[CLS_OUTPUT_INDEX][i * m_numCategory] > vSensitivity[m_numCategory])
        {
            vClsIndex.emplace_back(0);
        }
        else
        {
            int max = 0;
            int index_max = 0;
            for (int j = 1; j < m_numCategory; ++j)
            {
                if ((float)m_buffers[CLS_OUTPUT_INDEX][j + i * m_numCategory] > max)
                {
                    max = (float)m_buffers[CLS_OUTPUT_INDEX][j + i * m_numCategory];
                    index_max = j;
                }
            }
            if ((vSensitivity.size() > 1) && (m_numCategory == vSensitivity.size()))
            {
                if (max < vSensitivity[index_max])
                {
                    index_max = 0;
                    // break;
                }
            }
            vClsIndex.emplace_back(index_max);
        }
    }
    return true;
}

bool TensorrtClassifier::getClassificationResult(const vector<Mat> &vBatchImage, vector<int> &vClsIndex, float sensitivity)
{
    //step1: inference
    if (!inference(vBatchImage))
    {
        cout << "tensorrt dl inference failed!!!" << endl;
        return false;
    }

    //step2: post-process
    for (int i = 0; i < vBatchImage.size(); ++i)
    {
        // cout<<"----"<<(float)m_buffers[CLS_OUTPUT_INDEX][ i * m_numCategory] <<endl;
        if ((float)m_buffers[CLS_OUTPUT_INDEX][i * m_numCategory] > sensitivity)
        {
            vClsIndex.emplace_back(0);
        }
        else
        {
            int max = 0;
            int index_max = 0;
            for (int j = 1; j < m_numCategory; ++j)
            {
                // if((float)m_buffers[CLS_OUTPUT_INDEX][j + i * m_numCategory] < sensitivity)
                // {
                //     index_max = 0;
                //     break;
                // }
                // cout<<"----"<<(float)m_buffers[CLS_OUTPUT_INDEX][j + i * m_numCategory] <<endl;
                if ((float)m_buffers[CLS_OUTPUT_INDEX][j + i * m_numCategory] > max)
                {
                    max = (float)m_buffers[CLS_OUTPUT_INDEX][j];
                    index_max = j;
                }
            }
            vClsIndex.emplace_back(index_max);
        }
    }
    return true;
}

bool TensorrtClassifier::getSegmentResult(const vector<Mat> &vBatchImage, vector<Mat> &vMaskImage)
{
    //step1: inference
    if (!inference(vBatchImage))
    {
        cout << "tensorrt dl inference failed!!!" << endl;
        return false;
    }
    //step2: post-process
    const int output_channel = 1;
    const int output_height = m_input_h;
    const int output_width = m_input_w;
    for (int i = 0; i < vBatchImage.size(); ++i)
    {
        Mat maskImg(Size(output_width, output_height), CV_32FC1, (float *)m_buffers[SEG_OUTPUT_INDEX] + i * (output_height * output_width * output_channel));
        // maskImg.convertTo(maskImg, CV_8UC1, 255);
        // imwrite("maskImg.png", maskImg);
        // resize(maskImg, maskImg, vBatchImage[i].size());
        vMaskImage.emplace_back(maskImg);
    }

    return vMaskImage.size() == vBatchImage.size();
}

// template <typename T>
// void softmax(const T *src, T *dst, int length,int index)
// {
//     const T max_val = *std::max_element(src+index*length, src +length);
//     T denominator{0};
//     for (auto i=0;i<length;i++)
//     {
//         dst[i] = std::exp(src[i+index*length] - max_val);
//         denominator += dst[i+index*length];
//     }
//     for (auto i=0;i<length;i++)
//     {
//         dst[i+index*length] /= denominator;
//     }
// }

// bool TensorrtClassifier::softmax(const float* m_buffers,float* sfotmax_buffer,int numCategory)
bool TensorrtClassifier::softmax(const float* input,float* output,const int num_classes)
{
    /*
    softmax=e_x_n/sum(0->n)(e^x_n)
    input:array[data1,data2,....]
    output:array[data1,data2,....]
    */
    float denominator =0;
    // cout<<"---------------------------"<<endl;
    const float max_val = *std::max_element(input, input +num_classes);
    for (auto i=0;i<num_classes;i++)
    {
        // std::cout<< std::exp(input[i] - max_val)<<std::endl;
        output[i] = std::exp(input[i] - max_val);
        denominator += output[i];
    }

    for (auto i=0;i<m_numCategory;i++)
    {
        output[i] /= denominator;   
    }

}

/*
[cls_head_ptr,seg_head]
*/
bool TensorrtClassifier::getMultitaskResult(const vector<Mat> &vBatchImage, vector<int> &vClsIndex, vector<float> &good_score,float sensitivity, vector<Mat> &vMaskImage)
{

    //step1: inference
   
    if (!inference(vBatchImage))
    {
        cout << "tensorrt dl inference failed!!!" << endl;
        return false;
    }
    
    // cout<<"numCategory: "<<m_numCategory<<endl;
    //step2: post-process (classification)
    float* cls_head_ptr=m_buffers[CLS_OUTPUT_INDEX];
    // float* seg_head=m_buffers[SEG_OUTPUT_INDEX];
    for (int i = 0; i < vBatchImage.size(); ++i)
    {

        float* curr_cls_ptr=&cls_head_ptr[i*m_numCategory];
        float curr_cls_softmax_ptr[m_numCategory];

        softmax(curr_cls_ptr,curr_cls_softmax_ptr,m_numCategory);
        good_score.emplace_back(curr_cls_softmax_ptr[0]);
        
        if (curr_cls_softmax_ptr[0] > sensitivity)
        {    
            vClsIndex.emplace_back(0);
            // cout<<"0:  "<<curr_cls_softmax_ptr[0]<<endl;
        }
        else
        {
            float max = 0.0;
            int index_max = 0;
            for (int j = 1; j < m_numCategory; ++j)
            {
                if (curr_cls_softmax_ptr[j] > max)
                {
                    
                    max = curr_cls_softmax_ptr[j];
                    index_max = j;
                }
                
            }
            if(max > m_class_senvity[index_max])
                vClsIndex.emplace_back(index_max);
            else
                vClsIndex.emplace_back(0);
        }
    }
    
    // cout<<vClsIndex<<endl;
    

    //step3: post-process (segment)
    const int output_channel = 1;
    const int output_height = m_input_h;
    const int output_width = m_input_w;
    for (int i = 0; i < vBatchImage.size(); ++i)
    {
        Mat maskImg(Size(output_width, output_height), CV_32FC1, (float *)m_buffers[SEG_OUTPUT_INDEX] + i * (output_height * output_width * output_channel));
        // maskImg.convertTo(maskImg, CV_8UC1, 255);
        // imwrite("maskImg.png", maskImg);
        // resize(maskImg, maskImg, vBatchImage[i].size());
        vMaskImage.emplace_back(maskImg);
    }

    return vMaskImage.size() == vBatchImage.size();
}