#include "inference.h"
#include "inference_utils.h"

bool inference::create_session() {
    try {
        // create environment for onnxruntime
        this->env = Ort::Env(ORT_LOGGING_LEVEL_WARNING, "Yolo");
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }

    Ort::SessionOptions session_options;

    OrtCUDAProviderOptions cudaOption;
    cudaOption.device_id = 0; // use gpu 0

    try {
        // add cuda provider to session options
        session_options.AppendExecutionProvider_CUDA(cudaOption);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }

    // set graph optimization level
    session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

    // set intra op num threads
    session_options.SetIntraOpNumThreads(1);

    // set log severity level
    session_options.SetLogSeverityLevel(3);

    try {
        this->session = new Ort::Session(this->env, L"C:\\Users\\apex.onnx", session_options);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }

    this->input_node_names = { "images" };
    this->output_node_names = { "output0" };

    this->options = Ort::RunOptions{ nullptr };

    this->image_size = { 640, 640 };

    this->classes = { "Body", "Head" };

    return true;
}

void inference::run(cv::Mat& image, std::vector<inference_result>& results) {

    cv::Mat image_copy = image.clone();

    float* blob = new float[image_copy.total() * 3];

    // convert image to blob
    blob_from_image(image_copy, blob);

    std::vector<int64_t> input_node_dims = { 1, 3, 640, 640 };

    tensor_process(image_copy, blob, input_node_dims, results);
}

void inference::tensor_process(cv::Mat& image, float* blob, std::vector<int64_t>& input_node_dims, std::vector<inference_result>& results) {

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU), blob, 3 * 640 * 640,
        input_node_dims.data(), input_node_dims.size());

    // run inference
    auto output_tensor = session->Run(options, input_node_names.data(), &input_tensor, 1, output_node_names.data(),
        output_node_names.size());

    Ort::TypeInfo type_info = output_tensor.front().GetTypeInfo();

    // get tensor type and shape info
    auto tensor_info = type_info.GetTensorTypeAndShapeInfo();

    // get output tensor shape
    std::vector<int64_t> outputNodeDims = tensor_info.GetShape();

    auto output = output_tensor.front().GetTensorMutableData<float>();

    delete[] blob; // free memory

    int stride_num = outputNodeDims[1];//8400
    int signal_result_num = outputNodeDims[2];//84
    std::vector<int> class_ids;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;
    cv::Mat raw_data;

    raw_data = cv::Mat(stride_num, signal_result_num, CV_32F, output);

    // transpose the matrix
    raw_data = raw_data.t();

    float* data = (float*)raw_data.data;

    for (int i = 0; i < signal_result_num; ++i)
    {
        float* classesScores = data + 4;
        cv::Mat scores(1, this->classes.size(), CV_32FC1, classesScores);
        cv::Point class_id;
        double max_class_score;
        cv::minMaxLoc(scores, 0, &max_class_score, 0, &class_id);

        if (max_class_score > 0.1)
        {
            confidences.push_back(max_class_score);
            class_ids.push_back(class_id.x);
            float x = data[0];
            float y = data[1];
            float w = data[2];
            float h = data[3];

            int left = int((x - 0.5 * w));
            int top = int((y - 0.5 * h));

            int width = int(w);
            int height = int(h);

            boxes.push_back(cv::Rect(left, top, width, height));
        }
        data += stride_num;
    }

    std::vector<int> nmsResult;
    cv::dnn::NMSBoxes(boxes, confidences, 0.1, 0.5, nmsResult);
    for (int i = 0; i < nmsResult.size(); ++i)
    {
        int idx = nmsResult[i];
        inference_result result;
        result.class_id = class_ids[idx];
        result.confidence = confidences[idx];
        result.bounding_box = boxes[idx];
        results.push_back(result);
    }


}
