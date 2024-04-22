#pragma once
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>

struct inference_result {
	int class_id;
	float confidence;
	cv::Rect bounding_box;
};

class inference {
private:
	Ort::Env env;
	Ort::Session* session;
	Ort::RunOptions options;

	std::vector<const char*> input_node_names;
	std::vector<const char*> output_node_names;

	std::vector<int> image_size;
	
public:
	bool create_session( );
	void run( cv::Mat& image, std::vector<inference_result>& results );
	void tensor_process( cv::Mat& image, float* blob, std::vector<int64_t>& input_node_dims, 
		std::vector<inference_result>& results );

	std::vector<std::string> classes;
};
