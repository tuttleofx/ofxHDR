#pragma once
#include "ofxsImageEffect.h"
#include "HdrBasePluginDescriber.hpp"
#include "HdrBasePluginDefinition.hpp"
#include "../common/Image.hpp"
#include "../common/rgbCurve.hpp"
#include "../common/Presets.hpp"

namespace cameraColorCalibration {
namespace hdrBase {

/**
 * @brief HdrCalibPlugin Class
 */
class HdrBasePlugin : public OFX::ImageEffect 
{
private:
  //(!) Don't delete these, OFX::ImageEffect is managing them
  
  //Clips
  OFX::Clip *_dstClip = fetchClip(kOfxImageEffectOutputClipName); //Destination clip
  OFX::Clip *_srcClip[K_MAX_GROUPS]; //Sources clip groups
  
  //UI Groups
  OFX::GroupParam *_clipGroups[K_MAX_GROUPS];
  OFX::GroupParam *_clipGroupImages[K_MAX_GROUPS][K_MAX_IMAGES_PER_GROUP];
  
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
  
  //Connected clip index vector
  std::size_t _nbClips;
  std::vector<std::size_t> _connectedClipIdx;
  
  //Process Data
  std::vector< std::vector< cameraColorCalibration::common::Image<float> > > _ldrImageGroups;
  std::vector< std::vector<float> > _luminances;

public:
  
  /**
   * @brief Plugin Constructor
   * @param handle
   */
  HdrBasePlugin(OfxImageEffectHandle handle, std::size_t nbClips);
  
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
   * @brief Change Sources Images EV MetaData on changedParam
   * @param paramName
   * @return changedParam is a MetaData param
   */
  bool changedImageMetaData(const std::string &paramName);

  /**
   * @brief Change Target EV MetaData on changedParam
   * @param paramName
   * @return changedParam is a MetaData param
   */
  bool changedTargetMetaData(const std::string &paramName);
  
  /**
   * 
   * @param groupIndex
   * @return 
   */
  bool renderDebug(cameraColorCalibration::common::Image<float> &output, std::size_t groupIndex = 0);
  
  /**
   * 
   */
  bool loadSources();
  
  /**
   * @brief load output image pointer
   * @param outputPtr
   */
  bool loadOutput(OFX::Image *& outputPtr, double time);
  
  /**
   * @brief Update collection of connected clip indexes
   */
  void updateConnectedClipIndexCollection();
  
  /**
   * @brief Update Input Clip Parameters
   */
  void updateClipInputs();
  
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
   * @brief Get weight function
   */
  void getFunctionFromFile(const std::string &path, cameraColorCalibration::common::rgbCurve &curve);
  
  /**
   * @brief Get response function from keyframes
   */
  void getResponseFunctionFromKeyFrames(cameraColorCalibration::common::rgbCurve &response);
  
  /**
   * @brief Get weight function from keyframes
   */
  void getWeightFunctionFromKeyFrames(cameraColorCalibration::common::rgbCurve &weight);
  
  /**
   * @brief Set response function from keyframes
   */
  void setResponseFunctionKeyFrames(const cameraColorCalibration::common::rgbCurve &response);
  
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
   * @brief Define Group visibility
   * @param groupIndex
   * @param isSecretGroup
   * @param isSecretImages
   */
  void setGroupSecret(std::size_t groupIndex, bool isSecretGroup, bool isSecretImages);
  
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
  
  
  std::vector< std::vector< cameraColorCalibration::common::Image<float> > >& getAllSources()
  {
    return _ldrImageGroups;
  }
  
  std::vector< cameraColorCalibration::common::Image<float> >& getSource(std::size_t groupIndex = 0)
  {
    assert(groupIndex < getNbInputGroup());
    return _ldrImageGroups[groupIndex];
  }
  
  std::vector< std::vector<float> >& getAllExposure()
  {
    return _luminances;
  }
  
  std::vector<float>& getExposure(std::size_t groupIndex = 0)
  {
    assert(groupIndex < getNbInputGroup());
    return _luminances[groupIndex];
  }
  
  double getTargetExposure() const
  {
    return _targetShutter->getValue();
  }
  
  OFX::Clip* getInputClip(std::size_t groupIndex = 0)
  {
    assert(groupIndex < getNbInputGroup());
    return _srcClip[groupIndex];
  }
  
  std::size_t getNbInputGroup()
  {
    return _nbClips;
  }
  
  std::size_t getNbConnectedInput() const
  {
    return _connectedClipIdx.size();
  }
  
  std::size_t getFirstConnectedGroupIndex() const
  {
    return _connectedClipIdx.front();
  }
  
  std::size_t getLastConnectedGroupIndex() const
  {
    return _connectedClipIdx.back();
  }
  
  std::size_t getConnectedGroupIndex(std::size_t position) const
  {
    return _connectedClipIdx[position];
  }
  
  bool isGroupConnected(std::size_t groupIndex) const
  {
    return _srcClip[groupIndex]->isConnected();
  }
  
  bool hasInputGroup() const
  {
    return (!_connectedClipIdx.empty());
  }
  
  bool hasResponseKeyFrames() const
  {
    return (_responseRed->getNumKeys() > 0);
  }
  
  void invalidRender()
  {
    _forceInvalidation->setValue(1 + _forceInvalidation->getValue());
  }
};

} // namespace hdrBase 
} // namespace cameraColorCalibration