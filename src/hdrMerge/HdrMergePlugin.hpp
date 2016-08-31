#pragma once
#include "ofxsImageEffect.h"
#include "HdrMergePluginFactory.hpp"
#include "HdrMergePluginDefinition.hpp"
#include "../common/RobertsonMerge.hpp"

//Max number of source image 
#define K_MAX_SOURCE_IMAGES 20


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
  OFX::PushButtonParam *_responseLoad = fetchPushButtonParam(kParamResponseLoad);
  OFX::PushButtonParam *_responseExport = fetchPushButtonParam(kParamResponseExport);
  OFX::PushButtonParam *_responseFromKeys = fetchPushButtonParam(kParamResponseFromKeys);
  OFX::DoubleParam *_responseRed = fetchDoubleParam(kParamResponseRed);
  OFX::DoubleParam *_responseGreen = fetchDoubleParam(kParamResponseGreen);
  OFX::DoubleParam *_responseBlue = fetchDoubleParam(kParamResponseBlue); 
  
  //Weight Parameters
  OFX::ChoiceParam *_weightPreset = fetchChoiceParam(kParamWeightPreset);
  OFX::StringParam *_weightFilePath = fetchStringParam(kParamWeightFilePath);
  OFX::PushButtonParam *_weightLoad = fetchPushButtonParam(kParamWeightLoad);
  OFX::PushButtonParam *_weightExport = fetchPushButtonParam(kParamWeightExport);
  OFX::PushButtonParam *_weightFromKeys = fetchPushButtonParam(kParamWeightFromKeys);
  OFX::DoubleParam *_weightRed = fetchDoubleParam(kParamWeightRed);
  OFX::DoubleParam *_weightGreen = fetchDoubleParam(kParamWeightGreen);
  OFX::DoubleParam *_weightBlue = fetchDoubleParam(kParamWeightBlue); 
  
  //Invalidation Parameters
  OFX::IntParam *_forceInvalidation = fetchIntParam(kParamForceInvalidation);
  
  //Process Data
  std::vector< cameraColorCalibration::common::Image<float> >  _ldrImages;
  cameraColorCalibration::common::Image<float> _hdrImage;
  std::vector<float> _luminances;
  cameraColorCalibration::common::rgbCurve  _response;
  cameraColorCalibration::common::RobertsonMerge _merge;
  
  //Result parameters are uptodate 
  bool _uptodate = false;

public:
  
  /**
   * @brief Plugin Constructor
   * @param handle
   */
  HdrMergePlugin(OfxImageEffectHandle handle);

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
  bool loadGroups();
  
  /**
   * @brief Refresh response function keyframes 
   */
  void refreshResponseFunctionKeyFrames();
  
  /**
   * @brief Refresh weight function keyframes 
   */
  void refreshWeightFunctionKeyFrames();
  
  /**
   * @brief Update UI response function preset
   */
  void updateResponsePreset();
  
  /**
   * @brief Update UI weight function preset
   */
  void updateWeightPreset();
  
  /**
   * @brief Update response function from keyframes
   */
  void updateResponseFunctionFromKeyFrames(bool sendMessage = true);
  
  /**
   * @brief Update weight function from keyframes
   */
  void updateWeightFunctionFromKeyFrames(bool sendMessage = true);
  
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