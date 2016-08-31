#include "HdrMergePluginFactory.hpp"
#include "HdrMergePluginDefinition.hpp"
#include "HdrMergePlugin.hpp"
#include "../common/Presets.hpp"


namespace cameraColorCalibration {
namespace hdrMerge {

void HdrMergePluginFactory::describe(OFX::ImageEffectDescriptor& desc)
{
  //Plugin Labels
  desc.setLabels(
    "HdrMerge",
    "HdrMerge",
    "HdrMerge");
  
  //Plugin grouping
  desc.setPluginGrouping("tuttle/image/process/color");

  //Plugin description
  desc.setPluginDescription("HdrMerge is used to use a response function of a couple (camera / lens) in order to do a proper color calibration of an HDR. ");

  //Supported contexts
  desc.addSupportedContext(OFX::eContextFilter);
  desc.addSupportedContext(OFX::eContextGeneral);
  desc.addSupportedContext(OFX::eContextPaint);

  //Supported pixel depths
  desc.addSupportedBitDepth(OFX::eBitDepthUByte);
  desc.addSupportedBitDepth(OFX::eBitDepthUShort);
  desc.addSupportedBitDepth(OFX::eBitDepthFloat);

  //Flags
  desc.setSingleInstance(false);
  desc.setHostFrameThreading(false);
  desc.setSupportsMultiResolution(true);
  desc.setSupportsTiles(false);
  desc.setTemporalClipAccess(true);
  desc.setRenderTwiceAlways(false);
  desc.setSupportsMultipleClipPARs(false);
}

void HdrMergePluginFactory::describeInContext(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context)
{
  //Input Clips
  OFX::ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);

  srcClip->addSupportedComponent(OFX::ePixelComponentRGBA);
  srcClip->setTemporalClipAccess(true);
  srcClip->setSupportsTiles(false);
  srcClip->setIsMask(false);
  srcClip->setOptional(false);
  
  //Output clip
  OFX::ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
  dstClip->addSupportedComponent(OFX::ePixelComponentRGBA);
  dstClip->setSupportsTiles(false);
  
  //Source Group
  {
    OFX::GroupParamDescriptor *groupSource = desc.defineGroupParam(kParamGroupSource);
    groupSource->setLabel("Source");
    groupSource->setAsTab();

    //Image Groups
    for(std::size_t image = 0; image < K_MAX_SOURCE_IMAGES; ++image)
    {
      OFX::GroupParamDescriptor *groupImage = desc.defineGroupParam(kParamGroupImage(image));
      groupImage->setLabel("Image " + std::to_string(image + 1));
      groupImage->setOpen(false);
      groupImage->setParent(*groupSource);

      {
        OFX::IntParamDescriptor *param = desc.defineIntParam(kParamImageIso(image));
        param->setLabel("ISO");
        param->setHint("Image iso value");
        param->setRange(100, 500000);
        param->setDisplayRange(100, 9000);
        param->setDefault(100);
        param->setAnimates(false);
        param->setParent(*groupImage);
      }

      {
        OFX::DoubleParamDescriptor *param = desc.defineDoubleParam(kParamImageAperture(image));
        param->setLabel("Aperture");
        param->setHint("Image aperture value");
        param->setRange(0, 60);
        param->setDisplayRange(0, 40);
        param->setIncrement(0.1);
        param->setDefault(1.4);
        param->setAnimates(false);
        param->setParent(*groupImage);
      }

      {
        OFX::DoubleParamDescriptor *param = desc.defineDoubleParam(kParamImageShutter(image));
        param->setLabel("Shutter");
        param->setHint("Image shutter speed value");
        param->setRange(0, 7200);
        param->setDisplayRange(0, 4);
        param->setIncrement(0.001);
        param->setDefault(0.5);
        param->setAnimates(false);
        param->setParent(*groupImage);
      }

      {
        OFX::DoubleParamDescriptor *param = desc.defineDoubleParam(kParamImageEv(image));
        param->setLabel("EV");
        param->setHint("Image global exposure value");
        param->setDisplayRange(-5, 15);
        param->setDefault(0);
        param->setAnimates(false);
        param->setEnabled(false);
        param->setDigits(5);
        param->setParent(*groupImage);
      }
    }
  }
  
  //Target Group
  {
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
  }

  //Response group
  {
    OFX::GroupParamDescriptor *groupResponse = desc.defineGroupParam(kParamGroupResponse);
    groupResponse->setLabel("Response");
    groupResponse->setAsTab();

    {
      OFX::ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamResponsePreset);
      param->setLabel("Response Function");
      param->setHint("Type of response function to use");
      param->appendOptions(cameraColorCalibration::common::kPresetStringResponse);
      param->setDefault(cameraColorCalibration::common::eResponsePresetLinear);
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
      OFX::PushButtonParamDescriptor *param = desc.definePushButtonParam(kParamResponseLoad);
      param->setLabel("Load Response Function");
      param->setHint("Import a response function from a file.");
      param->setEnabled(true);
      param->setParent(*groupResponse);
      param->setLayoutHint(OFX::eLayoutHintNoNewLine);
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
      OFX::PushButtonParamDescriptor *param = desc.definePushButtonParam(kParamResponseFromKeys);
      param->setLabel("Update from Keyframes");
      param->setHint("Update response function with curent keyframes");
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
      param->setEvaluateOnChange(false);
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
      param->setEvaluateOnChange(false);
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
      param->setEvaluateOnChange(false);
      param->setParent(*groupResponse);
    }
  }
  
  //Weight group
  {
    OFX::GroupParamDescriptor *groupWeight = desc.defineGroupParam(kParamGroupWeight);
    groupWeight->setLabel("Weight");
    groupWeight->setAsTab();
    
    {
      OFX::ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamWeightPreset);
      param->setLabel("Weight Function");
      param->setHint("Type of weight function to use");
      param->appendOptions(cameraColorCalibration::common::kPresetStringWeight);
      param->setDefault(cameraColorCalibration::common::eWeightPresetGaussian);
      param->setParent(*groupWeight);
    }
    
    {
      OFX::StringParamDescriptor *param = desc.defineStringParam(kParamWeightFilePath);
      param->setLabel("Weight Function File Path");
      param->setHint("File path for the weight function file."); 
      param->setStringType(OFX::eStringTypeFilePath);
      param->setEvaluateOnChange(false);
      param->setParent(*groupWeight);
    }
    
    {
      OFX::PushButtonParamDescriptor *param = desc.definePushButtonParam(kParamWeightLoad);
      param->setLabel("Load Weight Function");
      param->setHint("Import a weight function from a file.");
      param->setEnabled(true);
      param->setParent(*groupWeight);
      param->setLayoutHint(OFX::eLayoutHintNoNewLine);
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
      OFX::PushButtonParamDescriptor *param = desc.definePushButtonParam(kParamWeightFromKeys);
      param->setLabel("Update from Keyframes");
      param->setHint("Update weight function with curent keyframes");
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
      param->setEvaluateOnChange(false);
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
      param->setEvaluateOnChange(false);
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
      param->setEvaluateOnChange(false);
      param->setParent(*groupWeight);
    }
  }
  
  {
    OFX::IntParamDescriptor *param = desc.defineIntParam(kParamForceInvalidation);
    param->setLabel("Force Invalidation");
    param->setHint("Allow the plugin to force the host to render");
    param->setIsSecret(true);
    param->setEnabled(false);
    param->setCanUndo(false);
  }
}

OFX::ImageEffect* HdrMergePluginFactory::createInstance(OfxImageEffectHandle handle, OFX::ContextEnum context)
{
  return new HdrMergePlugin(handle);
}

} // namespace hdrMerge 
} // namespace cameraColorCalibration