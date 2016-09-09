#include "HdrBasePluginDescriber.hpp"
#include "HdrBasePluginDefinition.hpp"
#include "HdrBasePlugin.hpp"
#include "../common/Presets.hpp"


namespace cameraColorCalibration {
namespace hdrBase {
  
void describeClip(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context, std::size_t nbClips)
{
  //Input Clips
  for(std::size_t group = 0; group < nbClips; ++group)
  {
    OFX::ClipDescriptor *srcClip = desc.defineClip(kClip(group));
    
    if(nbClips == 1)
    {
      srcClip->setLabel("Source");
    }
    
    srcClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    srcClip->setTemporalClipAccess(true);
    srcClip->setSupportsTiles(false);
    srcClip->setIsMask(false);
    srcClip->setOptional(group > 0);
  }
  
  //Output clip
  OFX::ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
  dstClip->addSupportedComponent(OFX::ePixelComponentRGBA);
  dstClip->setSupportsTiles(false);
}
  
OFX::GroupParamDescriptor* describeInputsGroup(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context, std::size_t nbClips)
{
  OFX::GroupParamDescriptor *groupInputs = desc.defineGroupParam(kParamGroupSources);
  groupInputs->setLabel("Sources");
  
  //Clip Groups
  for(std::size_t group = 0; group < nbClips; ++group)
  {
    OFX::GroupParamDescriptor *groupClip = desc.defineGroupParam(kParamGroupClip(group));
    if(nbClips > 1)
    {
      groupClip->setLabel("Group " + kClip(group));
    }
    else
    {
      groupClip->setLabel("Group");
    }
    groupClip->setAsTab();
    groupClip->setParent(*groupInputs);

    //Image Groups
    for(std::size_t image = 0; image < K_MAX_IMAGES_PER_GROUP; ++image)
    {
      OFX::GroupParamDescriptor *groupImage = desc.defineGroupParam(kParamGroupImage(group, image));
      groupImage->setLabel("Image " + std::to_string(image + 1));
      groupImage->setOpen(false);
      groupImage->setParent(*groupClip);

      {
        OFX::IntParamDescriptor *param = desc.defineIntParam(kParamImageIso(group, image));
        param->setLabel("ISO");
        param->setHint("Image iso value");
        param->setRange(100, 500000);
        param->setDisplayRange(100, 9000);
        param->setDefault(100);
        param->setAnimates(false);
        param->setEvaluateOnChange(false);
        param->setParent(*groupImage);
      }

      {
        OFX::DoubleParamDescriptor *param = desc.defineDoubleParam(kParamImageAperture(group, image));
        param->setLabel("Aperture");
        param->setHint("Image aperture value");
        param->setRange(0, 60);
        param->setDisplayRange(0, 40);
        param->setIncrement(0.1);
        param->setDefault(1.4);
        param->setAnimates(false);
        param->setEvaluateOnChange(false);
        param->setParent(*groupImage);
      }

      {
        OFX::DoubleParamDescriptor *param = desc.defineDoubleParam(kParamImageShutter(group, image));
        param->setLabel("Shutter");
        param->setHint("Image shutter speed value");
        param->setRange(0, 7200);
        param->setDisplayRange(0, 4);
        param->setIncrement(0.001);
        param->setDefault(0.5);
        param->setAnimates(false);
        param->setEvaluateOnChange(false);
        param->setParent(*groupImage);
      }

      {
        OFX::DoubleParamDescriptor *param = desc.defineDoubleParam(kParamImageEv(group, image));
        param->setLabel("EV");
        param->setHint("Image global exposure value");
        param->setDisplayRange(-5, 15);
        param->setDefault(0);
        param->setAnimates(false);
        param->setEnabled(false);
        param->setDigits(5);
        param->setEvaluateOnChange(false);
        param->setParent(*groupImage);
      }
    }
  }
  
  return groupInputs;
}

OFX::GroupParamDescriptor* describeTargetGroup(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context)
{
  //Target Group
  OFX::GroupParamDescriptor *groupTarget = desc.defineGroupParam(kParamGroupTarget);
  groupTarget->setLabel("Target");
  groupTarget->setAsTab();

  {
    OFX::IntParamDescriptor *param = desc.defineIntParam(kParamTargetImageIso);
    param->setLabel("ISO");
    param->setHint("Target image iso value");
    param->setRange(100, 500000);
    param->setDisplayRange(100, 9000);
    param->setDefault(100);
    param->setAnimates(false);
    param->setParent(*groupTarget);
  }

  {
    OFX::DoubleParamDescriptor *param = desc.defineDoubleParam(kParamTargetImageAperture);
    param->setLabel("Aperture");
    param->setHint("Target image aperture value");
    param->setRange(0, 60);
    param->setDisplayRange(0, 40);
    param->setIncrement(0.1);
    param->setDefault(1.4);
    param->setAnimates(false);
    param->setParent(*groupTarget);
  }

  {
    OFX::DoubleParamDescriptor *param = desc.defineDoubleParam(kParamTargetImageShutter);
    param->setLabel("Shutter");
    param->setHint("Target image shutter speed value");
    param->setRange(0, 7200);
    param->setDisplayRange(0, 4);
    param->setIncrement(0.001);
    param->setDefault(0.5);
    param->setAnimates(false);
    param->setParent(*groupTarget);
  }

  {
    OFX::DoubleParamDescriptor *param = desc.defineDoubleParam(kParamTargetImageEv);
    param->setLabel("EV");
    param->setHint("Target image global exposure value");
    param->setDisplayRange(-5, 15);
    param->setDefault(0);
    param->setAnimates(false);
    param->setEnabled(false);
    param->setDigits(5);
    param->setParent(*groupTarget);
  }
  
  return groupTarget;
}


OFX::GroupParamDescriptor* describeResponseGroup(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context, bool allowEditing)
{
  //Response group
  OFX::GroupParamDescriptor *groupResponse = desc.defineGroupParam(kParamGroupResponse);
  groupResponse->setLabel("Response");
  groupResponse->setAsTab();

  {
    OFX::ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamResponsePreset);
    param->setLabel("Response Function");
    param->setHint("Type of response function to use");
    param->appendOptions(cameraColorCalibration::common::kPresetStringResponse);
    param->setDefault(cameraColorCalibration::common::eResponsePresetLinear);
    param->setEvaluateOnChange(true);
    param->setIsSecret(!allowEditing);
    param->setParent(*groupResponse);
  }

  {
    OFX::StringParamDescriptor *param = desc.defineStringParam(kParamResponseFilePath);
    param->setLabel("Response Function File Path");
    param->setHint("File path for the response function file."); 
    param->setStringType(OFX::eStringTypeFilePath);
    param->setEvaluateOnChange(false);
    param->setParent(*groupResponse);
  }

  {
    OFX::PushButtonParamDescriptor *param = desc.definePushButtonParam(kParamResponseExport);
    param->setLabel("Export Response Function");
    param->setHint("Save the response function in a file.");
    param->setEnabled(true);
    param->setParent(*groupResponse);
    param->setLayoutHint(OFX::eLayoutHintDivider);
  }

  {
    OFX::PushButtonParamDescriptor *param = desc.definePushButtonParam(kParamResponseRefreshKeys);
    param->setLabel("Refresh Keyframes");
    param->setHint("Refresh Keyframes for the current response function");
    param->setEnabled(true);
    param->setParent(*groupResponse);
    param->setLayoutHint(OFX::eLayoutHintNoNewLine);
  }

  {
    OFX::PushButtonParamDescriptor *param = desc.definePushButtonParam(kParamResponseClearKeys);
    param->setLabel("Clear Keyframes");
    param->setHint("Delete all keyframes");
    param->setEnabled(true);
    param->setParent(*groupResponse);
  }

  {
    OFX::DoubleParamDescriptor *param = desc.defineDoubleParam(kParamResponseRed);
    param->setLabel("Red");
    param->setHint("Red response function.");
    param->setDefault(0);
    param->setRange(0, 1000);
    param->setIncrement(1);
    param->setDisplayRange(0, 20);
    param->setAnimates(true);
    param->setEvaluateOnChange(true);
    param->setEnabled(allowEditing);
    param->setParent(*groupResponse);
  }

  {
    OFX::DoubleParamDescriptor *param = desc.defineDoubleParam(kParamResponseGreen);
    param->setLabel("Green");
    param->setHint("Green response function.");
    param->setDefault(0);
    param->setRange(0, 1000);
    param->setIncrement(1);
    param->setDisplayRange(0, 20);
    param->setAnimates(true);
    param->setEvaluateOnChange(true);
    param->setEnabled(allowEditing);
    param->setParent(*groupResponse);
  }

  {
    OFX::DoubleParamDescriptor *param = desc.defineDoubleParam(kParamResponseBlue);
    param->setLabel("Blue");
    param->setHint("Blue response function.");
    param->setDefault(0);
    param->setRange(0, 1000);
    param->setIncrement(1);
    param->setDisplayRange(0, 20);
    param->setAnimates(true);
    param->setEvaluateOnChange(true);
    param->setEnabled(allowEditing);
    param->setParent(*groupResponse);
  }
  
  return groupResponse;
}

OFX::GroupParamDescriptor* describeWeightGroup(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context)
{
  //Weight group
  OFX::GroupParamDescriptor *groupWeight = desc.defineGroupParam(kParamGroupWeight);
  groupWeight->setLabel("Weight");
  groupWeight->setAsTab();

  {
    OFX::ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamWeightPreset);
    param->setLabel("Weight Function");
    param->setHint("Type of weight function to use");
    param->appendOptions(cameraColorCalibration::common::kPresetStringWeight);
    param->setDefault(cameraColorCalibration::common::eWeightPresetGaussian);
    param->setEvaluateOnChange(true);
    param->setParent(*groupWeight);
  }

  {
    OFX::DoubleParamDescriptor *param = desc.defineDoubleParam(kParamWeightGaussianCustom);
    param->setLabel("Gaussian size");
    param->setHint("Gaussian size.");
    param->setDefault(4);
    param->setIncrement(1);
    param->setDisplayRange(1, 6);
    param->setAnimates(false);
    param->setEnabled(true);
    param->setEvaluateOnChange(true);
    param->setParent(*groupWeight);
  }

  {
    OFX::StringParamDescriptor *param = desc.defineStringParam(kParamWeightFilePath);
    param->setLabel("Weight Function File Path");
    param->setHint("File path for the weight function file."); 
    param->setStringType(OFX::eStringTypeFilePath);
    param->setEvaluateOnChange(true);
    param->setParent(*groupWeight);
  }

  {
    OFX::PushButtonParamDescriptor *param = desc.definePushButtonParam(kParamWeightExport);
    param->setLabel("Export Weight Function");
    param->setHint("Save the weight function in a file.");
    param->setEnabled(true);
    param->setParent(*groupWeight);
    param->setLayoutHint(OFX::eLayoutHintDivider);
  }

  {
    OFX::PushButtonParamDescriptor *param = desc.definePushButtonParam(kParamWeightRefreshKeys);
    param->setLabel("Refresh Keyframes");
    param->setHint("Refresh Keyframes for the current weight function");
    param->setEnabled(true);
    param->setParent(*groupWeight);
    param->setLayoutHint(OFX::eLayoutHintNoNewLine);
  }

  {
    OFX::PushButtonParamDescriptor *param = desc.definePushButtonParam(kParamWeightClearKeys);
    param->setLabel("Clear Keyframes");
    param->setHint("Delete all keyframes");
    param->setEnabled(true);
    param->setParent(*groupWeight);
  }

  {
    OFX::DoubleParamDescriptor *param = desc.defineDoubleParam(kParamWeightRed);
    param->setLabel("Red");
    param->setHint("Red weight function.");
    param->setDefault(0);
    param->setRange(0, 1);
    param->setIncrement(0.001);
    param->setDisplayRange(0, 1);
    param->setAnimates(true);
    param->setEnabled(false);
    param->setEvaluateOnChange(true);
    param->setParent(*groupWeight);
  }

  {
    OFX::DoubleParamDescriptor *param = desc.defineDoubleParam(kParamWeightGreen);
    param->setLabel("Green");
    param->setHint("Green weight function.");
    param->setDefault(0);
    param->setRange(0, 1);
    param->setIncrement(0.001);
    param->setDisplayRange(0, 1);
    param->setAnimates(true);
    param->setEnabled(false);
    param->setEvaluateOnChange(true);
    param->setParent(*groupWeight);
  }

  {
    OFX::DoubleParamDescriptor *param = desc.defineDoubleParam(kParamWeightBlue);
    param->setLabel("Blue");
    param->setHint("Blue weight function.");
    param->setDefault(0);
    param->setRange(0, 1);
    param->setIncrement(0.001);
    param->setDisplayRange(0, 1);
    param->setAnimates(true);
    param->setEnabled(false);
    param->setEvaluateOnChange(true);
    param->setParent(*groupWeight);
  }
  
  return groupWeight;
}

OFX::GroupParamDescriptor* describeDebugGroup(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context)
{
  //Debug group
  OFX::GroupParamDescriptor *groupDebug = desc.defineGroupParam(kParamGroupDebug);
  groupDebug->setLabel("Debug");
  groupDebug->setAsTab();

  {
    OFX::BooleanParamDescriptor *param = desc.defineBooleanParam(kParamDebugActive);
    param->setLabel("Active Debug");
    param->setHint("Output Image without modifications.");
    param->setDefault(false);
    param->setEvaluateOnChange(true);
    param->setParent(*groupDebug);
  }

  {
    OFX::IntParamDescriptor *param = desc.defineIntParam(kParamDebugOutput);
    param->setLabel("Output Image");
    param->setHint("Output Image without modifications.");
    param->setDefault(1);
    param->setRange(1, K_MAX_IMAGES_PER_GROUP);
    param->setDisplayRange(1, K_MAX_IMAGES_PER_GROUP);
    param->setEvaluateOnChange(true);
    param->setParent(*groupDebug);
  }
  
  return groupDebug;
}

void describeInvalidation(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context)
{
  OFX::IntParamDescriptor *param = desc.defineIntParam(kParamForceInvalidation);
  param->setLabel("Force Invalidation");
  param->setHint("Allow the plugin to force the host to render");
  param->setIsSecret(true);
  param->setEnabled(false);
  param->setCanUndo(false);
}

} // namespace hdrBase 
} // namespace cameraColorCalibration