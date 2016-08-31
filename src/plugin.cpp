#include "ofxsImageEffect.h"
#include "hdrCalib/HdrCalibPluginFactory.hpp"
#include "hdrMerge/HdrMergePluginFactory.hpp"

/**
 * OFX::Plugin::getPluginIDs
 * @param ids
 */
void OFX::Plugin::getPluginIDs(OFX::PluginFactoryArray &ids)
{
  static cameraColorCalibration::hdrCalibration::HdrCalibPluginFactory hdrCalib("tuttleofx.hdrcalib", 1, 0);
  static cameraColorCalibration::hdrMerge::HdrMergePluginFactory hdrMerge("tuttleofx.hdrmerge", 1, 0);
  ids.push_back(&hdrCalib);
  ids.push_back(&hdrMerge);
}
