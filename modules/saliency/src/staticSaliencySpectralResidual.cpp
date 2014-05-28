/*M///////////////////////////////////////////////////////////////////////////////////////
 //
 //  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
 //
 //  By downloading, copying, installing or using the software you agree to this license.
 //  If you do not agree to this license, do not download, install,
 //  copy or use the software.
 //
 //
 //                           License Agreement
 //                For Open Source Computer Vision Library
 //
 // Copyright (C) 2013, OpenCV Foundation, all rights reserved.
 // Third party copyrights are property of their respective owners.
 //
 // Redistribution and use in source and binary forms, with or without modification,
 // are permitted provided that the following conditions are met:
 //
 //   * Redistribution's of source code must retain the above copyright notice,
 //     this list of conditions and the following disclaimer.
 //
 //   * Redistribution's in binary form must reproduce the above copyright notice,
 //     this list of conditions and the following disclaimer in the documentation
 //     and/or other materials provided with the distribution.
 //
 //   * The name of the copyright holders may not be used to endorse or promote products
 //     derived from this software without specific prior written permission.
 //
 // This software is provided by the copyright holders and contributors "as is" and
 // any express or implied warranties, including, but not limited to, the implied
 // warranties of merchantability and fitness for a particular purpose are disclaimed.
 // In no event shall the Intel Corporation or contributors be liable for any direct,
 // indirect, incidental, special, exemplary, or consequential damages
 // (including, but not limited to, procurement of substitute goods or services;
 // loss of use, data, or profits; or business interruption) however caused
 // and on any theory of liability, whether in contract, strict liability,
 // or tort (including negligence or otherwise) arising in any way out of
 // the use of this software, even if advised of the possibility of such damage.
 //
 //M*/

#include "precomp.hpp"

namespace cv
{

/**
 * SaliencySpectralResidual
 */

/**
 * Parameters
 */

StaticSaliencySpectralResidual::Params::Params()
{
  resizedImageSize=Size(64,64);
}

StaticSaliencySpectralResidual::StaticSaliencySpectralResidual( const StaticSaliencySpectralResidual::Params &parameters ) :
    params( parameters )
{
  className = "SPECTRAL_RESIDUAL";
}

StaticSaliencySpectralResidual::~StaticSaliencySpectralResidual()
{

}

void StaticSaliencySpectralResidual::read( const cv::FileNode& fn )
{
  params.read( fn );
}

void StaticSaliencySpectralResidual::write( cv::FileStorage& fs ) const
{
  params.write( fs );
}


bool StaticSaliencySpectralResidual::computeSaliencyImpl( const Mat& image, Mat& saliencyMap )
{

  Mat grayTemp, grayDown;
  std::vector<Mat> mv;
  Mat realImage( params.resizedImageSize, CV_64F );
  Mat imaginaryImage( params.resizedImageSize, CV_64F );
  imaginaryImage.setTo( 0 );
  Mat combinedImage( params.resizedImageSize, CV_64FC2 );
  Mat imageDFT;
  Mat logAmplitude;
  Mat angle( params.resizedImageSize, CV_64F );
  Mat magnitude( params.resizedImageSize, CV_64F );
  Mat logAmplitude_blur, imageGR;

  if( image.channels() == 3 )
  {
    cvtColor( image, imageGR, COLOR_BGR2GRAY );
    resize( imageGR, grayDown, params.resizedImageSize, 0, 0, INTER_LINEAR );
  }
  else
  {
    resize( image, grayDown, params.resizedImageSize, 0, 0, INTER_LINEAR );
  }

  grayDown.convertTo( realImage, CV_64F );

  mv.push_back( realImage );
  mv.push_back( imaginaryImage );
  merge( mv, combinedImage );
  dft( combinedImage, imageDFT );
  split( imageDFT, mv );

  //-- Get magnitude and phase of frequency spectrum --//
  cartToPolar( mv.at( 0 ), mv.at( 1 ), magnitude, angle, false );
  log( magnitude, logAmplitude );
  //-- Blur log amplitude with averaging filter --//
  blur( logAmplitude, logAmplitude_blur, Size( 3, 3 ), Point( -1, -1 ), BORDER_DEFAULT );

  exp( logAmplitude - logAmplitude_blur, magnitude );
  //-- Back to cartesian frequency domain --//
  polarToCart( magnitude, angle, mv.at( 0 ), mv.at( 1 ), false );
  merge( mv, imageDFT );
  dft( imageDFT, combinedImage, DFT_INVERSE );
  split( combinedImage, mv );

  cartToPolar( mv.at( 0 ), mv.at( 1 ), magnitude, angle, false );
  GaussianBlur( magnitude, magnitude, Size( 5, 5 ), 8, 0, BORDER_DEFAULT );
  magnitude = magnitude.mul( magnitude );

  double minVal, maxVal;
  minMaxLoc( magnitude, &minVal, &maxVal );

  magnitude = magnitude / maxVal;
  magnitude.convertTo( magnitude, CV_32F );

  resize( magnitude, saliencyMap, image.size(), 0, 0, INTER_LINEAR );

#ifdef SALIENCY_DEBUG
  // visualize saliency map before and after K-means
  imshow( "Saliency Map", saliencyMap );
#endif

//TODO try the results and then delete
  /*outputMat = outputMat * 255;
  outputMat.convertTo( outputMat, CV_8U );
  saliencyMap = outputMat; */

  return true;

}

}/* namespace cv */
