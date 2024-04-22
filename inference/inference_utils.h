#pragma once
#include <opencv2/opencv.hpp>

template<typename T>
char* blob_from_image( cv::Mat& input_image, T& input_blob ) {
    int channels = input_image.channels( );
    int image_height = input_image.rows;
    int image_width = input_image.cols;

    for ( int c = 0; c < channels; c++ )
    {
        for ( int h = 0; h < image_height; h++ )
        {
            for ( int w = 0; w < image_width; w++ )
            {
                input_blob[c * image_width * image_height + h * image_width + w] = typename std::remove_pointer<T>::type(
                    ( input_image.at<cv::Vec3b>( h, w )[c] ) / 255.0f );
            }
        }
    }
    return nullptr;
}