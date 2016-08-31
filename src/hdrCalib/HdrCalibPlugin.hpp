#pragma once
#include "ofxsImageEffect.h"
#include "HdrCalibPluginFactory.hpp"
#include "HdrCalibPluginDefinition.hpp"
#include "../common/RobertsonCalibrate.hpp"
#include "../common/RobertsonMerge.hpp"
#include "../common/Presets.hpp"

//Max number of HDR Group in Input
#define K_MAX_GROUPS 3

//Max number of image per HDR Group
#define K_MAX_IMAGES_PER_GROUP 20


namespace cameraColorCalibration {
namespace hdrCalibration {

/**
 * @brief HdrCalibPlugin Class
 */
class HdrCalibPlugin : public OFX::ImageEffect 
{
private:
  //(!) Don't delete these, OFX::ImageEffect is managing them

  //Clips
  OFX::Clip *_dstClip = fetchClip(kOfxImageEffectOutputClipName); //Destination clip
  OFX::Clip *_srcClip[K_MAX_GROUPS]; //Sources clip groups
  
  //UI Groups
  OFX::GroupParam *_hdrGroups[K_MAX_GROUPS];
  OFX::GroupParam *_hdrGroupImages[K_MAX_GROUPS][K_MAX_IMAGES_PER_GROUP];
  
  //Calibration Parameters
  OFX::IntParam *_hdrOutputIndex = fetchIntParam(kParamCalibrationOutputIndex); 
  OFX::PushButtonParam *_hdrCalculateResponse = fetchPushButtonParam(kParamCalibrationCalculateResponse);
  
  //Per Image MetaData Parameters
  OFX::IntParam *_iso[K_MAX_GROUPS][K_MAX_IMAGES_PER_GROUP];
  OFX::DoubleParam *_aperture[K_MAX_GROUPS][K_MAX_IMAGES_PER_GROUP];
  OFX::DoubleParam *_shutter[K_MAX_GROUPS][K_MAX_IMAGES_PER_GROUP];
  OFX::DoubleParam *_ev[K_MAX_GROUPS][K_MAX_IMAGES_PER_GROUP];
  
  //Target MetaData Parameters
  OFX::IntParam *_targetIso = fetchIntParam(kParamTargetImageIso);
  OFX::DoubleParam *_targetAperture = fetchDoubleParam(kParamTargetImageAperture);
  OFX::DoubleParam *_targetShutter = fetchDoubleParam(kParamTargetImageShutter);
  OFX::DoubleParam *_targetEv = fetchDoubleParam(kParamTargetImageEv);
  
  //Algorithm Parameters
  OFX::IntParam *_algorithmMaxIteration = fetchIntParam(kParamAlgorithmIterations);
  OFX::DoubleParam *_algorithmThreshold = fetchDoubleParam(kParamAlgorithmThreshold);
  
  //Weight Parameters
  OFX::ChoiceParam *_weightPreset = fetchChoiceParam(kParamWeightPreset);
  OFX::StringParam *_weightFilePath = fetchStringParam(kParamWeightFilePath);
  OFX::PushButtonParam *_weightLoad = fetchPushButtonParam(kParamWeightLoad);
  OFX::PushButtonParam *_weightExport = fetchPushButtonParam(kParamWeightExport);
  OFX::PushButtonParam *_weightFromKeys = fetchPushButtonParam(kParamWeightFromKeys);
  OFX::DoubleParam *_weightRed = fetchDoubleParam(kParamWeightRed);
  OFX::DoubleParam *_weightGreen = fetchDoubleParam(kParamWeightGreen);
  OFX::DoubleParam *_weightBlue = fetchDoubleParam(kParamWeightBlue); 
  
  //Response Parameters
  OFX::StringParam *_responseFilePath = fetchStringParam(kParamResponseFilePath);
  OFX::PushButtonParam *_responseExport = fetchPushButtonParam(kParamResponseExport);
  OFX::DoubleParam *_responseRed = fetchDoubleParam(kParamResponseRed);
  OFX::DoubleParam *_responseGreen = fetchDoubleParam(kParamResponseGreen);
  OFX::DoubleParam *_responseBlue = fetchDoubleParam(kParamResponseBlue); 
  
  //Invalidation Parameters
  OFX::IntParam *_forceInvalidation = fetchIntParam(kParamForceInvalidation);
  
  //Process Data
  std::vector< std::vector< cameraColorCalibration::common::Image<float> > > _ldrImageGroups;
  std::vector< std::vector<float> > _luminances;
  cameraColorCalibration::common::rgbCurve  _response;
  cameraColorCalibration::common::RobertsonCalibrate _calibration;
  cameraColorCalibration::common::RobertsonMerge _merge;
  
  //Result is uptodate for the connected clips and their parameters
  bool _uptodate = false;
  
  //User want to calculate the response
  bool _wantCalculateResponse = false;
  
  //Connected clip index vector
  std::vector<std::size_t> _connectedClipIdx;

public:
  
  /**
   * @brief Plugin Constructor
   * @param handle
   */
  HdrCalibPlugin(OfxImageEffectHandle handle);

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
   * @brief Update collection of connected clip indexes
   */
  void updateConnectedClipIndexCollection();
  
  /**
   * @brief Update Input Clip Parameters
   */
  void updateClipInputs();
  
  /**
   * @brief Update Output Index Range 
   */
  void updateOutputIndexRange();
  
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
   * @brief Update UI weight function preset
   */
  void updateWeightPreset();
  
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
   * @brief Define Group visibility
   * @param group
   * @param secret
   */
  void setGroupSecret(std::size_t groupIndex, bool isSecretGroup = true, bool isSecretImages = true);

  /**
   * @brief Define Image visibility
   * @param group
   * @param image
   * @param secret
   */
  void setImageSecret(std::size_t groupIndex, std::size_t imageIndex, bool isSecret = true);
  
  /**
   * @brief Add a group to UI
   * @param index
   */
  void addGroup(std::size_t groupIndex);
  
  /**
   * @brief Remove a group from the UI
   * @param index
   */
  void removeGroup(std::size_t groupIndex);
  
  /**
   * @brief Reset all Parameters of a group to their default values
   * @return 
   */
  void resetGroupParameters(std::size_t groupIndex);
  
  /**
   * @brief Reset all Parameters
   */
  void reset();
  
 
  std::size_t getNbConnectedInput() const
  {
    return _connectedClipIdx.size();
  }
  
  bool hasInputGroup() const
  {
    return (!_connectedClipIdx.empty());
  }
  
  void invalidRender()
  {
    _forceInvalidation->setValue(1 + _forceInvalidation->getValue());
  }
};

} // namespace hdrCalibration
} // namespace cameraColorCalibration