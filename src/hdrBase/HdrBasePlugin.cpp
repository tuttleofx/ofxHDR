#include "HdrBasePlugin.hpp"
#include "../common/RobertsonMerge.hpp"
#include "../common/Presets.hpp"
#include <stdio.h>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <regex>


namespace cameraColorCalibration {
namespace hdrBase {

HdrBasePlugin::HdrBasePlugin(OfxImageEffectHandle handle, std::size_t nbClips) : 
    OFX::ImageEffect(handle)
{   
  assert(nbClips <= K_MAX_GROUPS);
  _nbClips = nbClips;
  
  for(std::size_t group = 0; group < nbClips; ++group)
  {
    _srcClip[group] = fetchClip(kClip(group));
    _clipGroups[group] = fetchGroupParam(kParamGroupClip(group));
    
    for(std::size_t image = 0; image < K_MAX_IMAGES_PER_GROUP; ++image)
    {
      _clipGroupImages[group][image] = fetchGroupParam(kParamGroupImage(group, image));
      _iso[group][image] = fetchIntParam(kParamImageIso(group, image));
      _aperture[group][image] = fetchDoubleParam(kParamImageAperture(group, image));
      _shutter[group][image] = fetchDoubleParam(kParamImageShutter(group, image));
      _ev[group][image] = fetchDoubleParam(kParamImageEv(group, image));
    }
  }
  //reset all plugins options
  reset();
}

bool HdrBasePlugin::isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &identityTime)
{
  return false;
}

void HdrBasePlugin::changedClip(const OFX::InstanceChangedArgs &args, const std::string &clipName)
{
  if(args.reason != OFX::InstanceChangeReason::eChangeTime)
  {
    updateConnectedClipIndexCollection();
    updateClipInputs();
  }
}

void HdrBasePlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName)
{
  //check if the parameter is a MetaData
  if(changedImageMetaData(paramName))
  {
    //a target meta data has changed, ev have been update by the function
    return;
  }
  
  //check if the parameter is a MetaData
  if(changedTargetMetaData(paramName))
  {
    //a target meta data has changed, ev have been update by the function
    return;
  }

  if(paramName == kParamResponsePreset)
  {
    updateResponsePreset();
    return;
  }
  
  if(paramName == kParamWeightPreset)
  {
    updateWeightPreset();
    return;
  }
  
  if((paramName == kParamResponseExport) || (paramName == kParamWeightExport))
  {
    try 
    {
      cameraColorCalibration::common::rgbCurve curve(K_QUANTIZATION);
      if(paramName == kParamResponseExport)
      {
        getResponseFunction(curve);
        curve.write(_responseFilePath->getValue(), "Response Function");
      }
      else
      {
        getWeightFunction(curve);
        curve.write(_weightFilePath->getValue(), "Weight Function");
      }
      this->sendMessage(OFX::Message::eMessageMessage, "hdrmerge.curve.export", "Function saved in file.");
    }
    catch(std::exception &e)
    {
      this->sendMessage(OFX::Message::eMessageError, "hdrmerge.curve.export", e.what());
    }
  }
  
  if(paramName == kParamResponseRefreshKeys)
  {
    refreshResponseFunctionKeyFrames();
    return;
  }
  
  if(paramName == kParamWeightRefreshKeys)
  {
    refreshWeightFunctionKeyFrames();
    return;
  }
  
  if(paramName == kParamResponseClearKeys)
  {
    clearResponseFunctionKeyFrames();
    return;
  }
  
  if(paramName == kParamWeightClearKeys)
  {
    clearWeightFunctionKeyFrames();
    return;
  }
}

bool HdrBasePlugin::changedImageMetaData(const std::string &paramName)
{
  std::regex reg("imageParam(.*)_([0-9]*)_([0-9]*)"); //MetaData pattern
  std::smatch sm;
  std::regex_match(paramName, sm, reg); //4 matches (pattern / parameter / group / image)
    
  if(sm.size() != 4)
  {
    //the changed param is not a metaData
    return false;
  }
  
  //const std::string &parameter = sm[1];  //Iso, Aperture or Shutter
  std::size_t group = std::stoi(sm[2]); //HDR group number
  std::size_t image = std::stoi(sm[3]); //HDR image number
  
  float value = cameraColorCalibration::common::RobertsonMerge::getExposure(_shutter[group][image]->getValue(),
                                                                            _iso[group][image]->getValue(),
                                                                            _aperture[group][image]->getValue());
  _ev[group][image]->setValue(value);
  
  //the changed param is a metaData
  return true; 
}

bool HdrBasePlugin::changedTargetMetaData(const std::string &paramName)
{
  if(paramName.substr(0,16) == "targetImageParam")
  {
     //the changed param is a target metaData
    float value = cameraColorCalibration::common::RobertsonMerge::getExposure(_targetShutter->getValue(),
                                                                             _targetIso->getValue(),
                                                                            _targetAperture->getValue());
    _targetEv->setValue(value);
    return true;
  }
  //the changed param is not a metaData
  return false;
}

bool HdrBasePlugin::renderDebug(cameraColorCalibration::common::Image<float> &output, std::size_t groupIndex)
{
  if(_debugActive->getValue())
  {
    std::size_t outputIndex = _debugOutput->getValue() - 1;
    
    if(getSource(groupIndex).size() <= outputIndex)
    {
      output.setRed();
    }
    else
    {
      std::cout << "render [Debug]" << std::endl;
      output.copyFrom(getSource(groupIndex)[outputIndex]);
    }
    return true;
  }
  return false;
}

bool HdrBasePlugin::loadSources()
{
  //clear process data
  _ldrImageGroups.clear();
  _luminances.clear();
  
  std::size_t nbConnectedGroup = getNbConnectedInput();
  _ldrImageGroups = std::vector< std::vector< cameraColorCalibration::common::Image<float> > >(nbConnectedGroup);
  _luminances = std::vector< std::vector<float> >(nbConnectedGroup);
  
  std::size_t groupIndex = 0;
  
  for(std::size_t group = 0; group < getNbInputGroup(); ++group)
  {
    //Check if the current group is connected
    if(!_srcClip[group]->isConnected())
    {
      continue;
    }
    
    OFX::Clip *clip = _srcClip[group];
    std::size_t start = (std::size_t)clip->getFrameRange().min;
    std::size_t last = (std::size_t)clip->getFrameRange().max;
    
    _ldrImageGroups[groupIndex] = std::vector< cameraColorCalibration::common::Image<float> >(last - start + 1);
    _luminances[groupIndex] = std::vector<float>(last - start + 1);
    
    /*
    float targetExposure = cameraColorCalibration::common::RobertsonMerge::getExposure(_targetShutter->getValue(),
                                                                                  _targetIso->getValue(),
                                                                                  _targetAperture->getValue());
    */
    float targetExposure = _targetShutter->getValue();
    
    for(std::size_t image = start; image <= last; ++image)
    {
      std::cout << "[load] Group :  " << group << " Index : " << groupIndex << " Image :  " << image << std::endl;
      
      /*
      float exposure = cameraColorCalibration::common::RobertsonMerge::getExposure(_shutter[group][image-start]->getValue(),
                                                                                    _iso[group][image-start]->getValue(),
                                                                                    _aperture[group][image-start]->getValue());
       */
      
      //std::cout << "[load]   compute EV ("<< exposure << ") ..." << std::endl;
      //float avgLuminance = std::pow(2.f, exposure - targetExposure);
      float avgLuminance = _shutter[group][image-start]->getValue();
      std::cout << "[load]   add time ("<< avgLuminance << ") ..." << std::endl;
     
      _luminances[groupIndex][image-start] = avgLuminance;
      
      std::cout << "[load]   add Image ... " << std::endl;
      OFX::Image *imagePtr = clip->fetchImage(image);
      if(imagePtr == NULL)
      {
        std::cerr << "[load] error : can't load image " << std::endl;
        return false;
      }
      _ldrImageGroups[groupIndex][image - start].setOfxImage(imagePtr);
    }
    ++groupIndex;
  }
  return true;
}

bool HdrBasePlugin::loadOutput(OFX::Image **outputPtr, double time)
{
  *outputPtr = _dstClip->fetchImage(time);
  return (*outputPtr != NULL);
}

void HdrBasePlugin::updateConnectedClipIndexCollection()
{
  _connectedClipIdx.clear();
  for(std::size_t group = 0; group < getNbInputGroup(); ++group)
  {
    if(_srcClip[group]->isConnected())
    {
      _connectedClipIdx.push_back(group);
    }
  }
}

void HdrBasePlugin::updateClipInputs()
{
  for(std::size_t group = 0; group < getNbInputGroup(); ++group)
  {
    if(_srcClip[group]->isConnected())
    {
      addGroup(group);
    }
    else
    {
      removeGroup(group);
    }
  }
}

void HdrBasePlugin::updateResponsePreset()
{
  if(_responsePreset->getIsSecret()) // don't allow response editing
  {
    return;
  }
  
  cameraColorCalibration::common::EPresetResponse preset;
  preset = static_cast<cameraColorCalibration::common::EPresetResponse>(_responsePreset->getValue());
  
  bool custom = (preset == cameraColorCalibration::common::eResponsePresetCustom);
  bool fromFile = (preset == cameraColorCalibration::common::eResponsePresetFromFile);
  
  _responseExport->setIsSecret(!custom);
  _responseFilePath->setIsSecret(!custom && !fromFile);
  _responseRed->setEnabled(custom);
  _responseGreen->setEnabled(custom);
  _responseBlue->setEnabled(custom);
}

void HdrBasePlugin::updateWeightPreset()
{
  cameraColorCalibration::common::EPresetWeight preset;
  preset = static_cast<cameraColorCalibration::common::EPresetWeight>(_weightPreset->getValue());
  
  bool gaussianCustom = (preset == cameraColorCalibration::common::eWeightPresetGaussianCustom); 
  bool custom = (preset == cameraColorCalibration::common::eWeightPresetCustom);
  bool fromFile = (preset == cameraColorCalibration::common::eWeightPresetFromFile);
  
  _weightExport->setIsSecret(!custom);
  _weightGaussianSize->setIsSecret(!gaussianCustom);
  _weightFilePath->setIsSecret(!custom && !fromFile);
  _weightRed->setEnabled(custom);
  _weightGreen->setEnabled(custom);
  _weightBlue->setEnabled(custom);
}

void HdrBasePlugin::getResponseFunction(cameraColorCalibration::common::rgbCurve &response)
{
  cameraColorCalibration::common::EPresetResponse preset;
  preset = static_cast<cameraColorCalibration::common::EPresetResponse>(_responsePreset->getValue());
  
  bool custom = (preset == cameraColorCalibration::common::eResponsePresetCustom);
  bool fromFile = (preset == cameraColorCalibration::common::eResponsePresetFromFile);
  
  if(_responsePreset->getIsSecret() || custom)
  {
    getResponseFunctionFromKeyFrames(response);
    return;
  }
  
  if(fromFile)
  {
    getFunctionFromFile(_responseFilePath->getValue(), response);
  }
  
  cameraColorCalibration::common::initResponseFromPreset(response, preset);
}

void HdrBasePlugin::getWeightFunction(cameraColorCalibration::common::rgbCurve &weight)
{
  cameraColorCalibration::common::EPresetWeight preset;
  preset = static_cast<cameraColorCalibration::common::EPresetWeight>(_weightPreset->getValue());
  
  bool custom = (preset == cameraColorCalibration::common::eWeightPresetCustom);
  bool fromFile = (preset == cameraColorCalibration::common::eWeightPresetFromFile);
  
  if(custom)
  {
    getWeightFunctionFromKeyFrames(weight);
    return;
  }
  
  if(fromFile)
  {
    getFunctionFromFile(_weightFilePath->getValue(), weight);
  }
  
  cameraColorCalibration::common::initWeightFromPreset(weight, _weightGaussianSize->getValue(), preset);
}

void HdrBasePlugin::getFunctionFromFile(const std::string &path, cameraColorCalibration::common::rgbCurve &curve)
{
  try 
  {
    curve.read(path);
  }
  catch(std::exception &e)
  {
    this->sendMessage(OFX::Message::eMessageError, "hdrmerge.curve.load", e.what());
  }
}
  
void HdrBasePlugin::getResponseFunctionFromKeyFrames(cameraColorCalibration::common::rgbCurve &response)
{
  for(std::size_t index = 0; index < response.getSize(); ++index)
  {
    response.getCurveRed()[index] = _responseRed->getValueAtTime(index);
    response.getCurveGreen()[index] = _responseGreen->getValueAtTime(index);
    response.getCurveBlue()[index] = _responseBlue->getValueAtTime(index);
  }
}

void HdrBasePlugin::getWeightFunctionFromKeyFrames(cameraColorCalibration::common::rgbCurve &weight)
{
  for(std::size_t index = 0; index < weight.getSize(); ++index)
  {
    weight.getCurveRed()[index] = _weightRed->getValueAtTime(index);
    weight.getCurveGreen()[index] = _weightGreen->getValueAtTime(index);
    weight.getCurveBlue()[index] = _weightBlue->getValueAtTime(index);
  }
}

void HdrBasePlugin::setResponseFunctionKeyFrames(const cameraColorCalibration::common::rgbCurve &response)
{
  this->beginEditBlock("[HdrMerge] set keyframes from response function");
  
  clearResponseFunctionKeyFrames();
  
  progressStart("Create response function keyframes", "hdrmerge.response.keyframes");
  const double updateCoefficient = 1.0 / (double)response.getSize();
  for(std::size_t index = 0; index < response.getSize(); ++index)
  {
    _responseRed->setValueAtTime(index, response.getCurveRed()[index]);
    _responseGreen->setValueAtTime(index, response.getCurveGreen()[index]);
    _responseBlue->setValueAtTime(index, response.getCurveBlue()[index]);
    progressUpdate(index * updateCoefficient);
  }
  progressEnd();
  
  this->endEditBlock();
}

void HdrBasePlugin::refreshResponseFunctionKeyFrames()
{
  cameraColorCalibration::common::rgbCurve response(K_QUANTIZATION);
  getResponseFunction(response);
  setResponseFunctionKeyFrames(response);
}

void HdrBasePlugin::refreshWeightFunctionKeyFrames()
{
  cameraColorCalibration::common::rgbCurve weight(K_QUANTIZATION);
  getWeightFunction(weight);
  
  this->beginEditBlock("[HdrMerge] set keyframes from weight function");
  
  clearWeightFunctionKeyFrames();
  
  progressStart("Create weight function keyframes", "hdrmerge.weight.keyframes");
  const double updateCoefficient = 1.0 / (double)weight.getSize();
  for(std::size_t index = 0; index < weight.getSize(); ++index)
  {
    _weightRed->setValueAtTime(index, weight.getCurveRed()[index]);
    _weightGreen->setValueAtTime(index, weight.getCurveGreen()[index]);
    _weightBlue->setValueAtTime(index, weight.getCurveBlue()[index]);
    progressUpdate(index * updateCoefficient);
  }
  progressEnd();
  
  this->endEditBlock();
}

void HdrBasePlugin::clearResponseFunctionKeyFrames()
{
  _responseRed->deleteAllKeys();
  _responseGreen->deleteAllKeys();
  _responseBlue->deleteAllKeys();
  _responseRed->setValue(0);
  _responseGreen->setValue(0);
  _responseBlue->setValue(0);
}

void HdrBasePlugin::clearWeightFunctionKeyFrames()
{
  _weightRed->deleteAllKeys();
  _weightGreen->deleteAllKeys();
  _weightBlue->deleteAllKeys();
  _weightRed->setValue(0);
  _weightGreen->setValue(0);
  _weightBlue->setValue(0);
}

void HdrBasePlugin::setGroupSecret(std::size_t groupIndex, bool isSecretGroup, bool isSecretImages)
{
  _clipGroups[groupIndex]->setIsSecret(isSecretGroup);
  _clipGroups[groupIndex]->setEnabled(!isSecretGroup);

  for(std::size_t image = 0; image < K_MAX_IMAGES_PER_GROUP; ++image)
  {
    setImageSecret(groupIndex, image, isSecretImages);
  }
}

void HdrBasePlugin::setImageSecret(std::size_t groupIndex, std::size_t imageIndex, bool isSecret)
{
  _clipGroupImages[groupIndex][imageIndex]->setIsSecret(isSecret);
  _iso[groupIndex][imageIndex]->setIsSecret(isSecret);
  _aperture[groupIndex][imageIndex]->setIsSecret(isSecret);
  _shutter[groupIndex][imageIndex]->setIsSecret(isSecret);
  _ev[groupIndex][imageIndex]->setIsSecret(isSecret);
}

void HdrBasePlugin::addGroup(std::size_t groupIndex)
{
  setGroupSecret(groupIndex, false, true);
  OfxRangeD range =  _srcClip[groupIndex]->getFrameRange();
  int nbImages = range.max - range.min + 1;
  for(int image = 0; image < nbImages; ++image)
  {
    setImageSecret(groupIndex, image, false);
    //Sync Label to frame range
    _clipGroupImages[groupIndex][image]->setLabel("Image " + std::to_string(int(range.min + image)));
  }
}

void HdrBasePlugin::removeGroup(std::size_t groupIndex)
{
  resetGroupParameters(groupIndex);
  setGroupSecret(groupIndex, true, true);
}

void HdrBasePlugin::resetGroupParameters(std::size_t groupIndex)
{
  for(std::size_t image = 0; image < K_MAX_IMAGES_PER_GROUP; ++image)
  {
    _iso[groupIndex][image]->setValue(100);
    _aperture[groupIndex][image]->setValue(1.4f);
    _shutter[groupIndex][image]->setValue(0.5f);
    _ev[groupIndex][image]->setValue(0);
  }
}

void HdrBasePlugin::reset() 
{
  updateConnectedClipIndexCollection();
  updateClipInputs();
  updateResponsePreset();
  updateWeightPreset();
}

} // namespace hdrBase 
} // namespace cameraColorCalibration