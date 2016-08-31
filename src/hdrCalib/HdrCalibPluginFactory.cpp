#include "HdrCalibPluginFactory.hpp"
#include "HdrCalibPluginDefinition.hpp"
#include "HdrCalibPlugin.hpp"
#include "../common/Presets.hpp"


namespace cameraColorCalibration {
namespace hdrCalibration {

void HdrCalibPluginFactory::describe(OFX::ImageEffectDescriptor& desc)
{
  //Plugin Labels
  desc.setLabels(
    "HdrCalib",
    "HdrCalib",
    "HdrCalib");
  
  //Plugin grouping
  desc.setPluginGrouping("tuttle/image/process/color");

  //Plugin description
  desc.setPluginDescription("HdrCalib is used to compute the response function of a couple (camera / lens)  from multiple groups of multi-bracketing images. This response function is a requirement in order to do a proper color calibration of an HDR. ");

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

void HdrCalibPluginFactory::describeInContext(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context)
{
  //Input Clips
  for(std::size_t group = 0; group < K_MAX_GROUPS; ++group)
  {
    OFX::ClipDescriptor *srcClip = desc.defineClip(kClip(group));
    
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
  
  //Calibration Group
  {
    OFX::GroupParamDescriptor *groupCalibration = desc.defineGroupParam(kParamGroupCalibration);
    groupCalibration->setLabel("Calibration");
    groupCalibration->setAsTab();
    
    {
      OFX::IntParamDescriptor *param = desc.defineIntParam(kParamCalibrationOutputIndex);
      param->setLabel("HDR Output Index");
      param->setHint("The index of the input group you want as output of this node.");
      param->setRange(0, K_MAX_GROUPS);
      // DisplayRange will be updated regarding the number of connected input clips
      param->setDisplayRange(0, 0);
      param->setAnimates(false);
      param->setEnabled(false);
      param->setParent(*groupCalibration);
      param->setLayoutHint(OFX::eLayoutHintNoNewLine);
    }
    
    {
      OFX::PushButtonParamDescriptor *param = desc.definePushButtonParam(kParamCalibrationCalculateResponse);
      param->setLabel("Calculate Response");
      param->setHint("Calculate Response with all sources informations");
      param->setEnabled(false);
      param->setParent(*groupCalibration);
      param->setLayoutHint(OFX::eLayoutHintDivider);
    }
    
    //HDR Groups
    for(std::size_t group = 0; group < K_MAX_GROUPS; ++group)
    {
      OFX::GroupParamDescriptor *groupHdr = desc.defineGroupParam(kParamGroupHdr(group));
      groupHdr->setLabel("HDR " + kClip(group));
      groupHdr->setAsTab();
      groupHdr->setParent(*groupCalibration);
    
      //Image Groups
      for(std::size_t image = 0; image < K_MAX_IMAGES_PER_GROUP; ++image)
      {
        OFX::GroupParamDescriptor *groupImage = desc.defineGroupParam(kParamGroupImage(group, image));
        groupImage->setLabel("Image " + std::to_string(image + 1));
        groupImage->setOpen(false);
        groupImage->setParent(*groupHdr);
        
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

  //Algorithm options group
  {
    OFX::GroupParamDescriptor *groupAdvanced = desc.defineGroupParam(kParamGroupAlgorithm);
    groupAdvanced->setLabel("Algorithm");
    groupAdvanced->setAsTab();
    
    {
      OFX::IntParamDescriptor *param = desc.defineIntParam(kParamAlgorithmIterations);
      param->setLabel("Iterations");
      param->setHint("Maximum number of algorithm iteration");
      param->setRange(1, 5000);
      param->setDisplayRange(1, 1000);
      param->setDefault(500);
      param->setAnimates(false);
      param->setEvaluateOnChange(false);
      param->setParent(*groupAdvanced);
    }
    
    {
      OFX::DoubleParamDescriptor *param = desc.defineDoubleParam(kParamAlgorithmThreshold);
      param->setLabel("Threshold");
      param->setHint("Response precision");
      param->setRange(0, 10);
      param->setDisplayRange(0, 1);
      param->setDefault(0.01);
      param->setAnimates(false);
      param->setEvaluateOnChange(false);
      param->setParent(*groupAdvanced);
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
  
  //Response group
  {
    OFX::GroupParamDescriptor *groupResponse = desc.defineGroupParam(kParamGroupResponse);
    groupResponse->setLabel("Response");
    groupResponse->setAsTab();

    {
      OFX::StringParamDescriptor *param = desc.defineStringParam(kParamResponseFilePath);
      param->setLabel("Export path");
      param->setHint("Export path for the response file."); 
      param->setStringType(OFX::eStringTypeFilePath);
      param->setEvaluateOnChange(false);
      param->setParent(*groupResponse);
      param->setLayoutHint(OFX::eLayoutHintNoNewLine);
    }
    
    {
      OFX::PushButtonParamDescriptor *param = desc.definePushButtonParam(kParamResponseExport);
      param->setLabel("Export Response");
      param->setHint("Save the calculated response in a file.");
      param->setEnabled(false);
      param->setParent(*groupResponse);
      param->setLayoutHint(OFX::eLayoutHintDivider);
    }
    
    {
      OFX::DoubleParamDescriptor *param = desc.defineDoubleParam(kParamResponseRed);
      param->setLabel("Red");
      param->setHint("Red response.");
      param->setDefault(0);
      param->setRange(0, 4096);
      param->setIncrement(1);
      param->setDisplayRange(0, 4096);
      param->setAnimates(false);
      param->setEvaluateOnChange(false);
      param->setParent(*groupResponse);
    }
    
    {
      OFX::DoubleParamDescriptor *param = desc.defineDoubleParam(kParamResponseGreen);
      param->setLabel("Green");
      param->setHint("Green response.");
      param->setDefault(0);
      param->setRange(0, 4096);
      param->setIncrement(1);
      param->setDisplayRange(0, 4096);
      param->setAnimates(false);
      param->setEvaluateOnChange(false);
      param->setParent(*groupResponse);
    }
    
    {
      OFX::DoubleParamDescriptor *param = desc.defineDoubleParam(kParamResponseBlue);
      param->setLabel("Blue");
      param->setHint("Blue response.");
      param->setDefault(0);
      param->setRange(0, 4096);
      param->setIncrement(1);
      param->setDisplayRange(0, 4096);
      param->setAnimates(false);
      param->setEvaluateOnChange(false);
      param->setParent(*groupResponse);
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

OFX::ImageEffect* HdrCalibPluginFactory::createInstance(OfxImageEffectHandle handle, OFX::ContextEnum context)
{
  return new HdrCalibPlugin(handle);
}

} // namespace hdrCalibration
} // namespace cameraColorCalibration

