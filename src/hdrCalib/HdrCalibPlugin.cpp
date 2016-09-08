#include "HdrCalibPlugin.hpp"
#include "../common/RobertsonMerge.hpp"
#include <stdio.h>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <regex>


namespace cameraColorCalibration {
namespace hdrCalibration {

HdrCalibPlugin::HdrCalibPlugin(OfxImageEffectHandle handle) : 
    OFX::ImageEffect(handle),
    _response(1 << 12)
    //_merge(1 << 12)
{
  for(std::size_t group = 0; group < K_MAX_GROUPS; ++group)
  {
    _srcClip[group] = fetchClip(kClip(group));
    _hdrGroups[group] = fetchGroupParam(kParamGroupHdr(group));
    
    for(std::size_t image = 0; image < K_MAX_IMAGES_PER_GROUP; ++image)
    {
      _hdrGroupImages[group][image] = fetchGroupParam(kParamGroupImage(group, image));
      _iso[group][image] = fetchIntParam(kParamImageIso(group, image));
      _aperture[group][image] = fetchDoubleParam(kParamImageAperture(group, image));
      _shutter[group][image] = fetchDoubleParam(kParamImageShutter(group, image));
      _ev[group][image] = fetchDoubleParam(kParamImageEv(group, image));
    }
  }
  //reset all plugins options
  reset();
}

void HdrCalibPlugin::render(const OFX::RenderArguments &args)
{
  std::cout << "render : [info] time: " << args.time << std::endl;
  std::cout << "render : [info] fieldToRender: " << args.fieldToRender << std::endl;
  std::cout << "render : [info] renderQualityDraft: " << args.renderQualityDraft << std::endl;
  std::cout << "render : [info] renderScale: " << args.renderScale.x << ", " << args.renderScale.y << std::endl;
  std::cout << "render : [info] interactiveRenderStatus: " << args.interactiveRenderStatus << std::endl;
  std::cout << "render : [info] args.renderWindow: (" << args.renderWindow.x1 << ", " << args.renderWindow.y1 << "), (" << args.renderWindow.x2 << ", "  << args.renderWindow.y2 << ")" << std::endl;
  std::cout << "render : [info] output clip index : " << _hdrOutputIndex->getValue() - 1 << std::endl;
  
  if(abort())
  {
    return;
  }
  
  //Get output index
  int outputClipIndex = _hdrOutputIndex->getValue() - 1;
  if(!_srcClip[outputClipIndex]->isConnected())
  {
    std::cerr << "render : [error] invalid output index" << std::endl;
    return;
  }

  //User want calculate response
  if(_wantCalculateResponse)
  {
    _wantCalculateResponse = false;
    _uptodate = true;

    std::cout << "render : load groups ..." << std::endl;
    if(!loadGroups() || abort())
    {
      return;
    }

    std::cout << "render : [calibration]" << std::endl;
    cameraColorCalibration::common::rgbCurve weight(1 << 12);
    weight.setGaussian();
    _calibration.process(_ldrImageGroups,_luminances,weight, _response);
    std::cout << "render : [calibration] -- OK" << std::endl;

    std::cout << "render : [Display Response]" << std::endl;
    refreshResponseFunctionKeyFrames();
    std::cout << "render : [Display Response] -- OK" << std::endl;
    
    return;
  }
  
  //Write Output
  std::cout << "render : [output clip] fetch"  << std::endl;
  OFX::Image *outputPtr = _dstClip->fetchImage(args.time);
  if(outputPtr == NULL)
  {
    std::cout << "render : [output clip] is NULL" << std::endl;
    return;
  }
  cameraColorCalibration::common::Image<float> output(outputPtr);
  
  //HDRs and Response already calculate
  if(_uptodate)
  {
    std::cout << "render : HDRs and Response have been calculate" << std::endl;
    output.copyFrom(_calibration.getRadiance(_connectedClipIdx[outputClipIndex]));
  }
  else
  {
    std::cout << "render : HDRs and Response have not been calculate" << std::endl;
    output.setRed();
  }
}

bool HdrCalibPlugin::isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &identityTime)
{
  return false;
}

void HdrCalibPlugin::changedClip(const OFX::InstanceChangedArgs &args, const std::string &clipName)
{
  if(args.reason != OFX::InstanceChangeReason::eChangeTime)
  {
    //A clip changed
    _uptodate = false;
  
    updateConnectedClipIndexCollection();
    updateOutputIndexRange();
    updateClipInputs();
  }
}

void HdrCalibPlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName)
{
  //check if the parameter is a MetaData
  if(changedMetaData(paramName))
  {
    //a meta data has changed, ev have been update by the function
    _uptodate = false;
    return;
  }
  
  //Change output index
  if((paramName == kParamCalibrationOutputIndex) && (args.reason == OFX::eChangeUserEdit))
  {
    if(!_srcClip[_hdrOutputIndex->getValue() - 1]->isConnected())
    {
      int min;
      int max;
      _hdrOutputIndex->getRange(min, max);
      _hdrOutputIndex->setValue(min);
    }
    return;
  }
  
  //Calculate response
  if(paramName == kParamCalibrationCalculateResponse)
  {
    _wantCalculateResponse = true;
    invalidRender();
    return;
  }
  
  //Change algorithm max iteration
  if(paramName == kParamAlgorithmIterations)
  {
    int value = _calibration.getMaxIteration();
    _algorithmMaxIteration->getValue(value);
    _calibration.setMaxIteration(value);
    return;
  }
  
  //Change algorithm threshold
  if(paramName == kParamAlgorithmThreshold)
  {
    _calibration.setThreshold(_algorithmThreshold->getValue());
    return;
  }
  
  if(paramName == kParamWeightPreset)
  {
    updateWeightPreset();
    _uptodate = false;
    return;
  }
  
  /*
  if(paramName == kParamWeightLoad)
  {
    try 
    {
      cameraColorCalibration::common::rgbCurve fileCurve(_weightFilePath->getValue());
      _merge.setWeightFunction(fileCurve);

      this->sendMessage(OFX::Message::eMessageMessage, "hdrcalib.curve.load", "Function loaded from file.");
      _uptodate = false;
    }
    catch(std::exception &e)
    {
      this->sendMessage(OFX::Message::eMessageError, "hdrcalib.curve.load", e.what());
    }
  }
  */
  
  /*
  if((paramName == kParamResponseExport) || (paramName == kParamWeightExport))
  {
    try 
    {
      if(paramName == kParamResponseExport)
      {
        _response.write(_responseFilePath->getValue(), "Response Function");
      }
      else
      {
        _merge.getWeightFunction().write(_weightFilePath->getValue(), "Weight Function");
      }
      this->sendMessage(OFX::Message::eMessageMessage, "hdrcalib.curve.export", "Function saved in file.");
    }
    catch(std::exception &e)
    {
      this->sendMessage(OFX::Message::eMessageError, "hdrcalib.curve.export", e.what());
    }
  }
  */
  
  if(paramName == kParamWeightRefreshKeys)
  {
    refreshWeightFunctionKeyFrames();
    return;
  }
  
  if(paramName == kParamWeightClearKeys)
  {
    clearWeightFunctionKeyFrames();
    return;
  }
  
  if(paramName == kParamWeightFromKeys)
  {
    updateWeightFunctionFromKeyFrames();
    _uptodate = false;
    return;
  }
}

bool HdrCalibPlugin::changedMetaData(const std::string &paramName)
{
  std::regex reg("imageParam(.*)_([0-9]*)_([0-9]*)"); //MetaData pattern
  std::smatch sm;
  std::regex_match(paramName, sm, reg); //4 matches (pattern / parameter / group / image)
    
  if(sm.size() != 4)
  {
    //the changed param is not a source metaData
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

void HdrCalibPlugin::updateConnectedClipIndexCollection()
{
  _connectedClipIdx.clear();
  for(std::size_t group = 0; group < K_MAX_GROUPS; ++group)
  {
    if(_srcClip[group]->isConnected())
    {
      _connectedClipIdx.push_back(group);
    }
  }
}

void HdrCalibPlugin::updateClipInputs()
{
  for(std::size_t group = 0; group < K_MAX_GROUPS; ++group)
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
  _hdrCalculateResponse->setEnabled(hasInputGroup() && !_uptodate);
}

void HdrCalibPlugin::updateOutputIndexRange()
{
  std::size_t min = 0;
  std::size_t max = 0;
  
  if(_connectedClipIdx.empty())
  {
    //No connected clip
    _hdrOutputIndex->setEnabled(false);
    _hdrOutputIndex->setValue(0);
  }
  else
  {
    _hdrOutputIndex->setEnabled(true);
    min = _connectedClipIdx.front() + 1;
    max = _connectedClipIdx.back() + 1;
    
    std::size_t value = _hdrOutputIndex->getValue();
    if((value < min) || (value > max) || (!_srcClip[value - 1]->isConnected()))
    {
      _hdrOutputIndex->setValue(min);
    }
  }
  _hdrOutputIndex->setRange(min, max);
  _hdrOutputIndex->setDisplayRange(min, max);
}

bool HdrCalibPlugin::loadGroups()
{ 
  //clear process data
  _ldrImageGroups.clear();
  _luminances.clear();
  
  std::size_t nbConnectedGroup = getNbConnectedInput();
  _ldrImageGroups = std::vector< std::vector< cameraColorCalibration::common::Image<float> > >(nbConnectedGroup);
  _luminances = std::vector< std::vector<float> >(nbConnectedGroup);
  
  for(std::size_t group = 0; group < K_MAX_GROUPS; ++group)
  {
    //Check if the current group is connected
    if(!_srcClip[group]->isConnected())
    {
      continue;
    }
    
    OFX::Clip *clip = _srcClip[group];
    std::size_t start = (std::size_t)clip->getFrameRange().min;
    std::size_t last = (std::size_t)clip->getFrameRange().max;
    
    _ldrImageGroups[group] = std::vector< cameraColorCalibration::common::Image<float> >(start - last + 1);
    _luminances[group] = std::vector<float>(start - last + 1);
    
    float targetExposure = cameraColorCalibration::common::RobertsonMerge::getExposure(_targetShutter->getValue(),
                                                                                  _targetIso->getValue(),
                                                                                  _targetAperture->getValue());

    for(std::size_t image = start; image <= last; ++image)
    {
      std::cout << "[load] Group :  " << group << " Image :  " << image << std::endl;
      
      float exposure = cameraColorCalibration::common::RobertsonMerge::getExposure(_shutter[group][image-start]->getValue(),
                                                                                    _iso[group][image-start]->getValue(),
                                                                                    _aperture[group][image-start]->getValue());
      
      std::cout << "[load]   compute EV ("<< exposure << ") ..." << std::endl;
      float avgLuminance = std::pow(2.f, exposure - targetExposure);
      std::cout << "[load]   add Relative EV ("<< avgLuminance << ") ..." << std::endl;
     
      _luminances[group][image] = avgLuminance;
      
      std::cout << "[load]   add Image ... " << std::endl;
      OFX::Image *imagePtr = clip->fetchImage(image);
      if(imagePtr == NULL)
      {
        std::cerr << "[load] error : can't load image " << std::endl;
        return false;
      }
      _ldrImageGroups[group][image].setOfxImage(imagePtr);
    }
  }
  return true;
}


void HdrCalibPlugin::refreshResponseFunctionKeyFrames()
{
  clearResponseFunctionKeyFrames();
  progressStart("Create response function keyframes", "hdrcalib.response.keyframes");
  const double updateCoefficient = 1 / (double)_response.getSize();
  for(std::size_t index = 0; index < _response.getSize(); ++index)
  {
    _responseRed->setValueAtTime(index, _response.getCurveRed()[index]);
    _responseGreen->setValueAtTime(index, _response.getCurveGreen()[index]);
    _responseBlue->setValueAtTime(index, _response.getCurveBlue()[index]);
    progressUpdate(index * updateCoefficient);
  }
  progressEnd();
  _responseExport->setEnabled(true);
  _responseFilePath->setEnabled(true);
}

void HdrCalibPlugin::refreshWeightFunctionKeyFrames()
{
  /*
  clearWeightFunctionKeyFrames();
  progressStart("Create weight function keyframes", "hdrcalib.weight.keyframes");
  const cameraColorCalibration::common::rgbCurve &weight = _merge.getWeightFunction();
  const double updateCoefficient = 1 / (double)weight.getSize();
  for(std::size_t index = 0; index < weight.getSize(); ++index)
  {
    _weightRed->setValueAtTime(index, weight.getCurveRed()[index]);
    _weightGreen->setValueAtTime(index, weight.getCurveGreen()[index]);
    _weightBlue->setValueAtTime(index, weight.getCurveBlue()[index]);
    progressUpdate(index * updateCoefficient);
  }
  progressEnd();
  */
}

void HdrCalibPlugin::updateWeightPreset()
{
  /*
  cameraColorCalibration::common::EPresetWeight preset;
  preset = static_cast<cameraColorCalibration::common::EPresetWeight>(_weightPreset->getValue());
  
  bool custom = (preset == cameraColorCalibration::common::eWeightPresetCustom);
  bool fromFile = (preset == cameraColorCalibration::common::eWeightPresetFromFile);
  
  _weightLoad->setIsSecret(!fromFile);
  _weightExport->setIsSecret(!custom);
  _weightFilePath->setIsSecret(!custom && !fromFile);
  _weightFromKeys->setIsSecret(!custom);
  _weightRed->setEnabled(custom);
  _weightGreen->setEnabled(custom);
  _weightBlue->setEnabled(custom);
  
  if(!fromFile && !custom)
  {
    cameraColorCalibration::common::rgbCurve weight(_merge.getWeightFunction().getSize());
    cameraColorCalibration::common::initWeightFromPreset(weight, preset);
    _merge.setWeightFunction(weight);
  }
  */
}

void HdrCalibPlugin::updateWeightFunctionFromKeyFrames(bool sendMessage)
{
  /*
  cameraColorCalibration::common::rgbCurve weightFunction(_merge.getWeightFunction().getSize());
  for(std::size_t index = 0; index < weightFunction.getSize(); ++index)
  {
    weightFunction.getCurveRed()[index] = _weightRed->getValueAtTime(index);
    weightFunction.getCurveGreen()[index] = _weightGreen->getValueAtTime(index);
    weightFunction.getCurveBlue()[index] = _weightBlue->getValueAtTime(index);
  }
  _merge.setWeightFunction(weightFunction);
  if(sendMessage)
  {
    this->sendMessage(OFX::Message::eMessageMessage, "hdrcalib.weight.update", "Weight Function updated from keyFrames.");
  }
  */
}

void HdrCalibPlugin::clearResponseFunctionKeyFrames()
{
  _responseRed->deleteAllKeys();
  _responseGreen->deleteAllKeys();
  _responseBlue->deleteAllKeys();
  _responseRed->setValue(0);
  _responseGreen->setValue(0);
  _responseBlue->setValue(0);
}

void HdrCalibPlugin::clearWeightFunctionKeyFrames()
{
  _weightRed->deleteAllKeys();
  _weightGreen->deleteAllKeys();
  _weightBlue->deleteAllKeys();
  _weightRed->setValue(0);
  _weightGreen->setValue(0);
  _weightBlue->setValue(0);
}

void HdrCalibPlugin::setGroupSecret(std::size_t groupIndex, bool isSecretGroup, bool isSecretImages)
{
  _hdrGroups[groupIndex]->setIsSecret(isSecretGroup);
  _hdrGroups[groupIndex]->setEnabled(!isSecretGroup);

  for(std::size_t image = 0; image < K_MAX_IMAGES_PER_GROUP; ++image)
  {
    setImageSecret(groupIndex, image, isSecretImages);
  }
}

void HdrCalibPlugin::setImageSecret(std::size_t groupIndex, std::size_t imageIndex, bool isSecret)
{
  _hdrGroupImages[groupIndex][imageIndex]->setIsSecret(isSecret);
  _iso[groupIndex][imageIndex]->setIsSecret(isSecret);
  _aperture[groupIndex][imageIndex]->setIsSecret(isSecret);
  _shutter[groupIndex][imageIndex]->setIsSecret(isSecret);
  _ev[groupIndex][imageIndex]->setIsSecret(isSecret);
}

void HdrCalibPlugin::addGroup(std::size_t groupIndex)
{
  setGroupSecret(groupIndex, false, true);
  OfxRangeD range =  _srcClip[groupIndex]->getFrameRange();
  int nbImages = range.max - range.min + 1;
  for(int image = 0; image < nbImages; ++image)
  {
    setImageSecret(groupIndex, image, false);
    //Sync Label to frame range
    _hdrGroupImages[groupIndex][image]->setLabel("Image " + std::to_string(int(range.min + image)));
  }
}

void HdrCalibPlugin::removeGroup(std::size_t groupIndex)
{
  resetGroupParameters(groupIndex);
  setGroupSecret(groupIndex, true, true);
}

void HdrCalibPlugin::resetGroupParameters(std::size_t groupIndex)
{
  for(std::size_t image = 0; image < K_MAX_IMAGES_PER_GROUP; ++image)
  {
    _iso[groupIndex][image]->setValue(100);
    _aperture[groupIndex][image]->setValue(1.4f);
    _shutter[groupIndex][image]->setValue(0.5f);
    _ev[groupIndex][image]->setValue(0);
  }
}

void HdrCalibPlugin::reset() 
{
  updateConnectedClipIndexCollection();
  updateClipInputs();
  updateOutputIndexRange();
  updateWeightPreset();
  
  if(static_cast<cameraColorCalibration::common::EPresetWeight>(_weightPreset->getValue()) 
          == cameraColorCalibration::common::eWeightPresetCustom)
  {
    //Weight Function is custom
    updateWeightFunctionFromKeyFrames(false); //don't print message
  }
  
  _hdrCalculateResponse->setEnabled(hasInputGroup() && !_uptodate);
}

} // namespace hdrCalibration
} // namespace cameraColorCalibration