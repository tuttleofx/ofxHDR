#pragma once
#include "ofxsImageEffect.h"
#include "HdrMergePluginFactory.hpp"
#include "HdrMergePluginDefinition.hpp"
#include "../common/RobertsonMerge.hpp"

//Max number of source image 
#define K_MAX_SOURCE_IMAGES 20

//Quantization
#define K_QUANTIZATION 1 << 12 //(12 bits)


namespace cameraColorCalibration {
namespace hdrMerge {

/**
 * @brief HdrCalibPlugin Class
 */
class HdrMergePlugin : public OFX::ImageEffect 
{
private:
  //(!) Don't delete these, OFX::ImageEffect is managing them

  //Clips
  OFX::Clip *_srcClip = fetchClip(kOfxImageEffectSimpleSourceClipName); //Source clip 
  OFX::Clip *_dstClip = fetchClip(kOfxImageEffectOutputClipName); //Destination clip

  //UI Groups
  OFX::GroupParam *_sourceImages[K_MAX_SOURCE_IMAGES];
  
  //Sources MetaData Parameters
  OFX::IntParam *_iso[K_MAX_SOURCE_IMAGES];
  OFX::DoubleParam *_aperture[K_MAX_SOURCE_IMAGES];
  OFX::DoubleParam *_shutter[K_MAX_SOURCE_IMAGES];
  OFX::DoubleParam *_ev[K_MAX_SOURCE_IMAGES];
  
  //Target MetaData Parameters
  OFX::IntParam *_targetIso = fetchIntParam(kParamTargetImageIso);
  OFX::DoubleParam *_targetAperture = fetchDoubleParam(kParamTargetImageAperture);
  OFX::DoubleParam *_targetShutter = fetchDoubleParam(kParamTargetImageShutter);
  OFX::DoubleParam *_targetEv = fetchDoubleParam(kParamTargetImageEv);
  
  //Response Parameters
  OFX::ChoiceParam *_responsePreset = fetchChoiceParam(kParamResponsePreset);
  OFX::StringParam *_responseFilePath = fetchStringParam(kParamResponseFilePath);
  OFX::PushButtonParam *_responseExport = fetchPushButtonParam(kParamResponseExport);
  OFX::DoubleParam *_responseRed = fetchDoubleParam(kParamResponseRed);
  OFX::DoubleParam *_responseGreen = fetchDoubleParam(kParamResponseGreen);
  OFX::DoubleParam *_responseBlue = fetchDoubleParam(kParamResponseBlue); 
  
  //Weight Parameters
  OFX::ChoiceParam *_weightPreset = fetchChoiceParam(kParamWeightPreset);
  OFX::DoubleParam *_weightGaussianSize = fetchDoubleParam(kParamWeightGaussianCustom);
  OFX::StringParam *_weightFilePath = fetchStringParam(kParamWeightFilePath);
  OFX::PushButtonParam *_weightExport = fetchPushButtonParam(kParamWeightExport);
  OFX::DoubleParam *_weightRed = fetchDoubleParam(kParamWeightRed);
  OFX::DoubleParam *_weightGreen = fetchDoubleParam(kParamWeightGreen);
  OFX::DoubleParam *_weightBlue = fetchDoubleParam(kParamWeightBlue); 
  
  //Debug Parameters
  OFX::BooleanParam *_debugActive = fetchBooleanParam(kParamDebugActive);
  OFX::IntParam *_debugOutput = fetchIntParam(kParamDebugOutput);
  
  //Invalidation Parameters
  OFX::IntParam *_forceInvalidation = fetchIntParam(kParamForceInvalidation);
  
  //Process Data
  std::vector< cameraColorCalibration::common::Image<float> >  _ldrImages;
  cameraColorCalibration::common::Image<float> _hdrImage;
  std::vector<float> _luminances;
  
  //Result parameters are uptodate 
//  bool _uptodate = false;

public:
  
  /**
   * @brief Plugin Constructor
   * @param handle
   */
  HdrMergePlugin(OfxImageEffectHandle handle);

  /** 
   * @brief the get frames needed action
   * If the effect wants change the frames needed on an input clip from the default values (which is the same as the frame to be renderred)
   * it should do so by calling the OFX::FramesNeededSetter::setFramesNeeded function on the \em frames argument.
  */
  virtual void getFramesNeeded(const OFX::FramesNeededArguments &args, OFX::FramesNeededSetter &frames);

  /**
   * @brief Override render method
   * @param[in] args
   */
  virtual void render(const OFX::RenderArguments &args);

  /**
   * @brief Override isIdentity method
   * @param[in] args
   * @param[in,out] identityClip
   * @param[in,out] identityTime
   * @return 
   */
  virtual bool isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &identityTime);
  
  /**
   * @brief Override changedClip method
   * @param[in] args
   * @param[in] clipName
   */
  virtual void changedClip(const OFX::InstanceChangedArgs &args, const std::string &clipName);

  /**
   * @brief Override changedParam method
   * @param[in] args
   * @param[in] paramName
   */
  virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName);
  
  /**
   * @brief Change EV MetaData on changedParam
   * @param paramName
   * @return changedParam is a MetaData param
   */
  bool changedMetaData(const std::string &paramName);
  
  /**
   * @brief Update Source  Parameters
   */
  void updateSourceParameters();
  
  /**
   * @brief Load images and metadatas in memory
   * @return true if all has been loaded
   */
  bool loadGroup();
  
  /**
   * @brief Update UI response function preset
   */
  void updateResponsePreset();
  
  /**
   * @brief Update UI weight function preset
   */
  void updateWeightPreset();
  
 /**
   * @brief Set response function 
   */
  void getResponseFunction(cameraColorCalibration::common::rgbCurve &response);
  
  /**
   * @brief Set weight function
   */
  void getWeightFunction(cameraColorCalibration::common::rgbCurve &weight);
  
  /**
   * @brief Set weight function
   */
  void getFunctionFromFile(const std::string &path, cameraColorCalibration::common::rgbCurve &curve);
  
  /**
   * @brief Set response function from keyframes
   */
  void getResponseFunctionFromKeyFrames(cameraColorCalibration::common::rgbCurve &response);
  
  /**
   * @brief Set weight function from keyframes
   */
  void getWeightFunctionFromKeyFrames(cameraColorCalibration::common::rgbCurve &weight);
  
  /**
   * @brief Refresh response function keyframes 
   */
  void refreshResponseFunctionKeyFrames();
  
  /**
   * @brief Refresh weight function keyframes 
   */
  void refreshWeightFunctionKeyFrames();
  
  /**
   * @brief Clear response function keyframes
   */
  void clearResponseFunctionKeyFrames();

  /**
   * @brief Clear weight function keyframes
   */
  void clearWeightFunctionKeyFrames();

  /**
   * @brief Define Image visibility
   * @param image
   * @param secret
   */
  void setImageOptionsSecret(std::size_t imageIndex, bool isSecret = true);
  
  /**
   * @brief Reset all Parameters
   */
  void reset();
  
  
  void invalidRender()
  {
    _forceInvalidation->setValue(1 + _forceInvalidation->getValue());
  }
};

} // namespace hdrMerge 
} // namespace cameraColorCalibration