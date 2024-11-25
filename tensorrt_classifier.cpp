#include "tensorrt_classifier.h"
// #include "logger.h"
#include <iostream>
using namespace std;
using namespace cv;
template <typename _T>
shared_ptr<_T> make_nvshared(_T *ptr) {
	return shared_ptr<_T>(ptr, [](_T* p){p->destroy();});
}
#define INPUT_INDEX 0
#define CLS_OR_SEG_OUTPUT_INDEX 1
#define CLS_AND_SEG_OUTPUT_INDEX 2
#define CREATE_BUFFERS   10
#define COPY_BUFFERS     11
#define CHECK(status) \
    do\
    {\
        auto ret = (status);\
        if (ret != 0)\
        {\
            std::cerr << "Cuda failure: " << ret << std::endl;\
            abort();\
        }\
    } while (0)
vector<unsigned char> load_file(const string& file) 
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
		in.read((char*)&data[0], length);
	}
	in.close();
	return data;
}
TensorrtClassifier::TensorrtClassifier(const string &sModelPath,const TensorrtOutputType tensorrtOutputType, const int batchSize, const int numCategory, const int inputWidth, const int inputHeight, const int inputChannel):
                m_sModelPath(sModelPath),
                m_tensorrtOutputType(tensorrtOutputType),
                m_batch_size(batchSize),
                m_numCategory(numCategory),
				m_input_w(inputWidth),
				m_input_h(inputHeight),
				m_input_c(inputChannel),
                m_runtime(nullptr),
                m_engine(nullptr),
                m_context(nullptr)
{

}

TensorrtClassifier::~TensorrtClassifier()
{
    if(m_stream != NULL)
        cudaStreamDestroy(m_stream);
    for (int i = 0; i < BINDING_SIZE ; i++)
    {
        if (m_buffers[i] != nullptr)
        {
            free(m_buffers[i]);
        }
        if (m_gpu_buffers[i] != nullptr)
        {
            CHECK(cudaFree(m_gpu_buffers[i]));
        }
    }
}
bool TensorrtClassifier::createOrCopyBuffers(const int method)
{
    auto cls_output_size = m_numCategory * m_batch_size * sizeof(float);
    auto seg_output_size = m_input_w * m_input_h * m_batch_size * sizeof(float);
    std::vector<int> vOutputSize;
    int number = CLS_OR_SEG_OUTPUT_INDEX;
    if (TensorrtOutputType::CATEGORY == m_tensorrtOutputType)
    {
        vOutputSize.emplace_back(cls_output_size);
    }else if(TensorrtOutputType::CATEGORY_SEGMENT == m_tensorrtOutputType)
    {
        number = CLS_AND_SEG_OUTPUT_INDEX;
        vOutputSize.emplace_back(cls_output_size);
        vOutputSize.emplace_back(seg_output_size);
    }else
    {
        vOutputSize.emplace_back(seg_output_size);
    }
    for (size_t i = CLS_OR_SEG_OUTPUT_INDEX; i <= number; i++)
    {
        if(method == COPY_BUFFERS)
        {
            CHECK(cudaMemcpyAsync(m_buffers[i], m_gpu_buffers[i],vOutputSize.at(i - 1),cudaMemcpyDeviceToHost, m_stream));
        }else
        {
            m_buffers[i] = (float*)malloc(vOutputSize.at(i - 1));
            CHECK(cudaMalloc(&m_gpu_buffers[i], vOutputSize.at(i - 1)));
        }
    }
    return true;
}
bool TensorrtClassifier::hasDynamicDim()
{
    int numBindings = m_engine->getNbBindings();
    for (int i = 0; i < numBindings; i++)
    {
        nvinfer1::Dims dims = m_engine->getBindingDimensions(i);
        for (int j = 0; j < dims.nbDims; ++j)
        {
            if(dims.d[j] == -1)
                return true;
        }
    }
    return false;
}
bool TensorrtClassifier::setRunDims(int ibinding,const std::vector<int> & dims)
{
    nvinfer1::Dims d;
    std::memcpy(d.d,dims.data(),sizeof(int) * dims.size());
    d.nbDims = dims.size();
    return m_context->setBindingDimensions(ibinding,d);
}
template<typename T>
void TensorrtClassifier::softmax(const T* src,T * dst,const int numCategory)
{
    const T max_val = *std::max_element(src,src + numCategory);
    T denominator{0};
    for(auto i = 0; i < numCategory; i++)
    {
        dst[i] = std::exp(src[i] - max_val);
        denominator += dst[i];
    }
    for (auto i = 0; i < numCategory; i++)
    {
        dst[i] /= denominator;
    }
}
bool TensorrtClassifier::loadModel()
{     
    //step1: set gpu device No.
    cudaSetDevice(0);
    //step2:load file
    vector<unsigned char> engine_data = load_file(m_sModelPath);
    if(engine_data.size() == 0)
    {
        cout << "engine model is empty!!!";  
        return false;
    }
    //step3:deserialize the engine
    m_runtime.reset(nvinfer1::createInferRuntime(LoggerNV::instance()));
    m_engine.reset(m_runtime->deserializeCudaEngine(engine_data.data(), engine_data.size(), nullptr));
    m_context.reset(m_engine->createExecutionContext());
    //step4:申请输入内存和显存
    CHECK(cudaStreamCreate(&m_stream));
    auto input_size = m_input_w * m_input_h * m_input_c * m_batch_size * sizeof(float);
    m_buffers[INPUT_INDEX] = (float*)malloc(input_size);
    CHECK(cudaMalloc(&m_gpu_buffers[INPUT_INDEX], input_size));
    // TensorrtClassifier::print();
    if(hasDynamicDim())
        setRunDims(0,{m_batch_size,m_input_c,m_input_h,m_input_w});
    //step5:申请输出内存和显存
    createOrCopyBuffers(CREATE_BUFFERS);
    //step6:推理预热
    Mat image = Mat::zeros(Size(m_input_h, m_input_w), CV_8UC3);
    inference({image});
    return true;
}

void TensorrtClassifier::setModelParameters(const TensorrtOutputType tensorrtOutputType,const int batchSize, const int numCategory, const int inputWidth, const int inputHeight, const int inputChannel)
{
    m_tensorrtOutputType = tensorrtOutputType;
    m_batch_size = batchSize;
    m_numCategory = numCategory;
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
	dstImg.convertTo(dstImg, CV_32FC3, 1.0/255);
    //step4:减去均值
    subtract(dstImg, Scalar(0.485, 0.456, 0.406), dstImg);
    //step5:除以方差
    divide(dstImg, Scalar(0.229, 0.224, 0.225), dstImg);
}

bool TensorrtClassifier::inference(const vector<Mat> &vBatchImage)
{
    if(vBatchImage.empty())
    {
        cout << "inference no images input!!!";
        return false;
    }
    //step1:对图像进行前处理，并将图像拷贝到指针cpu数组m_buffers[0]当中
    for(size_t i = 0; i < vBatchImage.size(); ++i)
    {
        Mat inputImg;
        normalizeImage(vBatchImage[i], inputImg);
        vector<Mat> chw;
        for (size_t j = 0; j < m_input_c; ++ j) 
        {
           chw.emplace_back(cv::Mat(Size(m_input_w, m_input_h), CV_32FC1, (float*)m_buffers[INPUT_INDEX] + (j + i * m_input_c) * m_input_w * m_input_h));
        }
        split(inputImg, chw);
    }
    //step2:从内存到显存, 从CPU到GPU, 将输入数据拷贝到显存
    CHECK(cudaMemcpyAsync(m_gpu_buffers[INPUT_INDEX], 
                          m_buffers[INPUT_INDEX],
                          m_input_w * m_input_h * m_input_c * m_batch_size * sizeof(float),
                          cudaMemcpyHostToDevice, m_stream));
    //step3:推理
    m_context->enqueueV2(m_gpu_buffers, m_stream, nullptr);
    //step4:显存拷贝到内存
    createOrCopyBuffers(COPY_BUFFERS);
    //step5:这个是为了同步不同的流
    cudaStreamSynchronize(m_stream);
    return true;
}

bool TensorrtClassifier::getClassificationResult(const vector<Mat> &vBatchImage,const vector<float> &vSensitivity,vector<int> &vClsIndex,std::vector<float> & vClsScore,bool is_softMax)
{
    if(m_tensorrtOutputType != TensorrtOutputType::CATEGORY)
        return false;
    if(vSensitivity.empty())
    {
        cout << "getClassificationResult sensitivity is empty!!!";
        return false;
    }
    try
    {
        //step1: inference
        if(!inference(vBatchImage))
        {
            cout << "getClassificationResult tensorrt dl inference failed!!!";
            return false;
        }
        //step2: post-process
        if(!classifyProcess(vBatchImage,vSensitivity,vClsIndex,vClsScore,is_softMax))
        {
            cout <<"getClassificationResult classifyProcess failed!!!";
            return false;
        }
    }
    catch(const std::exception& e)
    {
        cout << e.what();
    }
	return true;
}

bool TensorrtClassifier::getMultiClassificationResult(const vector<Mat> &vBatchImage,const vector<float> &vSensitivity,vector<int> &vClsIndex,std::vector<float> & vClsScore,ScoreData &inferenceScore,bool is_softMax)
{
    if(m_tensorrtOutputType != TensorrtOutputType::CATEGORY)
        return false;
    if(vSensitivity.empty())
    {
        cout << "getMultiClassificationResult sensitivity is empty!!!";
        return false;
    }
    try
    {
        //step1: inference
        if(!inference(vBatchImage))
        {
            cout << "getMultiClassificationResult tensorrt dl inference failed!!!";
            return false;
        }
        //step2: post-process
        if(!multiClassifyProcess(vBatchImage,vSensitivity,vClsIndex,vClsScore,inferenceScore,is_softMax))
        {
            cout <<"getMultiClassificationResult classifyProcess failed!!!";
            return false;
        }
    }
    catch(const std::exception& e)
    {
        cout << e.what();
    }
	return true;
}

bool TensorrtClassifier::getSegmentResult(const vector<Mat> &vBatchImage, vector<Mat> &vMaskImage)
{
    if(m_tensorrtOutputType != TensorrtOutputType::SEGMENT)
    {
        return false;
    }
    try
    {
        //step1: inference
        if(!inference(vBatchImage))
        {
            cout << "getSegmentResult tensorrt dl inference failed!!!" ;
            return false;
        }
        //step2: post-process
        if(!segmentProcess(vBatchImage,vMaskImage))
        {
            cout <<"getSegmentResult segmentProcess failed!!!";
            return false;
        }
    }
    catch(const std::exception& e)
    {
        cout << e.what();
    }
	return vMaskImage.size() == vBatchImage.size();
}


void TensorrtClassifier::print() 
{
    int num_input = 0;
    int num_output = 0;
    auto engine = this->m_engine;
    for (int i = 0; i < engine->getNbBindings(); ++i) {
        if (engine->bindingIsInput(i))
            num_input++;
        else
            num_output++;
    }

    printf("Inputs: %d", num_input);
    for (int i = 0; i < num_input; ++i) {
        auto name = engine->getBindingName(i);
        auto dim = engine->getBindingDimensions(i);
        printf("\t%d.%s", i, name);
        for (int j = 0; j < 4; j++)
        {
            // LogINFO<<dim.d[j]<<" ";
        }
    }

    printf("Outputs: %d", num_output);
    for (int i = 0; i < num_output; ++i) {
        auto name = engine->getBindingName(i + num_input);
        auto dim = engine->getBindingDimensions(i + num_input);
        printf("\t%d.%s : shape {%s}", i, name);
        for (int j = 0; j < 4; j++)
        {
            // LogINFO<<dim.d[j]<<" ";
        }
    }
}


bool TensorrtClassifier::classifyProcess(const vector<Mat> &vBatchImage,const vector<float> &vSensitivity,vector<int> & vClsIndex,std::vector<float> & vClsScore,bool is_softMax)
{
    vClsIndex.clear();
    vClsScore.clear();
    float * cls_ptr = m_buffers[CLS_OR_SEG_OUTPUT_INDEX];//获取分类头指针
    float   cls_dst[m_numCategory];//分类结果输出
    if(NULL == cls_ptr)
        return false;
    for(int i = 0; i < vBatchImage.size(); i++)
    {
        int index_max = 0;
        int badMaxScore = 0;
        cls_ptr += i * m_numCategory;
        if (is_softMax)
        {
            softmax(cls_ptr,cls_dst,m_numCategory);
        }else
        {
            for (int c = 0; c < m_numCategory; c++)
                cls_dst[c] = cls_ptr[c];
        }
        //0号索引是GOOD分数
        if (cls_dst[0] > vSensitivity.at(0))
        {
            vClsIndex.emplace_back(0);
            vClsScore.emplace_back(cls_dst[0]);
            continue;
        }
        else
        {
            //寻找所有类别中最大的bad分数
            for (int c = 1; c < m_numCategory; c++)
            {
                if(cls_dst[c] >= badMaxScore)
                {
                    badMaxScore = cls_dst[c];
                    index_max = c;
                }
            }
        }
        if (vSensitivity.size() > 1 && m_numCategory == vSensitivity.size() && badMaxScore < vSensitivity.at(index_max))
            index_max = 0;//满足条件为GOOD
        vClsIndex.emplace_back(index_max);
        vClsScore.emplace_back(badMaxScore);
    }
    return true;
}

bool TensorrtClassifier::multiClassifyProcess(const vector<Mat> &vBatchImage,const vector<float> &vSensitivity,vector<int> & vClsIndex,std::vector<float> & vClsScore,bool is_softMax)
{
    vClsIndex.clear();
    vClsScore.clear();
    float * cls_ptr = m_buffers[CLS_OR_SEG_OUTPUT_INDEX];//获取分类头指针
    float   cls_dst[m_numCategory];//分类结果输出
    if(NULL == cls_ptr)
        return false;
    for(int i = 0; i < vBatchImage.size(); i++)
    {
        int index_max = 0;
        float badMaxScore = 0.0;
        cls_ptr += i * m_numCategory;
        if (is_softMax)
        {
            softmax(cls_ptr,cls_dst,m_numCategory);
        }else
        {
            for (int c = 0; c < m_numCategory; c++)
                cls_dst[c] = cls_ptr[c];
        }
        //寻找所有类别中最大的bad分数
        for (int c = 1; c < m_numCategory; c++)
        {
            if (cls_dst[c] >= vSensitivity.at(c))
            {
                if(cls_dst[c] >= badMaxScore)
                {
                    badMaxScore = cls_dst[c];
                    index_max = c;
                }
            }    
        }
        //0号索引是GOOD分数
        if (cls_dst[0] > vSensitivity.at(0) || badMaxScore == 0.0)
        {
            vClsIndex.emplace_back(0);
            vClsScore.emplace_back(cls_dst[0]);
            continue;
        }
        else
        {
            vClsIndex.emplace_back(index_max);
            vClsScore.emplace_back(badMaxScore);
        }  
        
    }
    return true;
}

bool TensorrtClassifier::multiClassifyProcess(const vector<Mat> &vBatchImage,const vector<float> &vSensitivity,vector<int> & vClsIndex,std::vector<float> & vClsScore,ScoreData &inferenceScore,bool is_softMax)
{
    vClsIndex.clear();
    vClsScore.clear();

    float * cls_ptr = m_buffers[CLS_OR_SEG_OUTPUT_INDEX];//获取分类头指针
    float   cls_dst[m_numCategory];//分类结果输出
    if(NULL == cls_ptr)
        return false;
    for(int i = 0; i < vBatchImage.size(); i++)
    {
        int index_max = 0;
        int actual_index_max = 0;
        float badMaxScore = 0.0;
        float actualBadMaxScore = 0.0;
        cls_ptr += i * m_numCategory;
        if (is_softMax)
        {
            softmax(cls_ptr,cls_dst,m_numCategory);
        }else
        {
            for (int c = 0; c < m_numCategory; c++)
                cls_dst[c] = cls_ptr[c];
        }
        //寻找所有类别中最大的bad分数
        for (int c = 1; c < m_numCategory; c++)
        {
            if (cls_dst[c] >= vSensitivity.at(c))
            {
                if(cls_dst[c] >= badMaxScore)
                {
                    badMaxScore = cls_dst[c];
                    index_max = c;
                }
            }
            if(cls_dst[c] >= actualBadMaxScore)
            {
                actualBadMaxScore = cls_dst[c];
                actual_index_max = c;
            }    
        }
        //得分数据记录
        inferenceScore.goodScore.emplace_back(cls_dst[0]);
        inferenceScore.badClass.emplace_back(actual_index_max);
        inferenceScore.maxBadScore.emplace_back(actualBadMaxScore);
        //0号索引是GOOD分数
        if (cls_dst[0] > vSensitivity.at(0) || badMaxScore == 0.0)
        {
            vClsIndex.emplace_back(0);
            vClsScore.emplace_back(cls_dst[0]);
            continue;
        }
        else
        {
            vClsIndex.emplace_back(index_max);
            vClsScore.emplace_back(badMaxScore);
        }  
        
    }
    return true;
}

bool TensorrtClassifier::segmentProcess(const vector<Mat> &vBatchImage, vector<Mat> &vMaskImage)
{
    vMaskImage.clear();
    //step2: post-process
    const int output_channel = 1;
    const int output_height = m_input_h;
    const int output_width = m_input_w;
    int index = CLS_OR_SEG_OUTPUT_INDEX;
    if(m_tensorrtOutputType == TensorrtOutputType::CATEGORY_SEGMENT)
        index = CLS_AND_SEG_OUTPUT_INDEX;
	for(int i = 0; i < vBatchImage.size(); ++i)
	{
		Mat maskImg(Size(output_width, output_height), CV_32FC1, (float*)m_buffers[index ] + i * (output_height * output_width * output_channel));
        resize(maskImg, maskImg, vBatchImage[i].size());     
		vMaskImage.emplace_back(maskImg);
	}
    return true;
}

bool TensorrtClassifier::getClassifyAndSegmentResult(const vector<Mat> &vBatchImage,const vector<float> &vSensitivity,vector<int> &vClsIndex,vector<Mat> &vSegmentMaskImage,vector<float> & vClsScore,bool is_softMax)
{
    if(m_tensorrtOutputType != TensorrtOutputType::CATEGORY_SEGMENT)
        return false;
    if(vSensitivity.empty())
    {
        cout << "getClassifyAndSegmentResult sensitivity is empty!!!";
        return false;
    }
    try
    {
        //step1: inference
        if(!inference(vBatchImage))
        {
            cout << "getClassifyAndSegmentResult tensorrt dl inference failed!!!";
            return false;
        }
        //step2: cls-process
        if(!classifyProcess(vBatchImage,vSensitivity,vClsIndex,vClsScore,is_softMax))
        {
            cout<<"getClassifyAndSegmentResult classifyProcess failed!!!";
            return false;
        }
        //step3: seg-process
        if(!segmentProcess(vBatchImage,vSegmentMaskImage))
        {
            cout<<"getClassifyAndSegmentResult segmentProcess failed!!!";
            return false;
        }
    }
    catch(const std::exception& e)
    {
        cout << e.what();
    }
    return true;
}

bool TensorrtClassifier::getMultiClassifyAndSegmentResult(const vector<Mat> &vBatchImage,const vector<float> &vSensitivity,vector<int> &vClsIndex,vector<Mat> &vSegmentMaskImage,vector<float> & vClsScore,ScoreData &inferenceScore,bool is_softMax)
{
    if(m_tensorrtOutputType != TensorrtOutputType::CATEGORY_SEGMENT)
        return false;
    if(vSensitivity.empty())
    {
        cout << "getClassifyAndSegmentResult sensitivity is empty!!!";
        return false;
    }
    try
    {
        //step1: inference
        if(!inference(vBatchImage))
        {
            cout << "getClassifyAndSegmentResult tensorrt dl inference failed!!!";
            return false;
        }
        //step2: cls-process
        // if(!classifyProcess(vBatchImage,vSensitivity,vClsIndex,vClsScore,is_softMax))
        if(!multiClassifyProcess(vBatchImage,vSensitivity,vClsIndex,vClsScore,inferenceScore,is_softMax))
        {
            cout<<"getClassifyAndSegmentResult classifyProcess failed!!!";
            return false;
        }
        //step3: seg-process
        if(!segmentProcess(vBatchImage,vSegmentMaskImage))
        {
            cout<<"getClassifyAndSegmentResult segmentProcess failed!!!";
            return false;
        }
    }
    catch(const std::exception& e)
    {
        cout << e.what();
    }
    return true;
}