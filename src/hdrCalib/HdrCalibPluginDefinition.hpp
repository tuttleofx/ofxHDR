#pragma once

/**
 * Plugin Parameters definition
 */

//Clip
#define kClip(I) std::to_string(I + 1)


//Calibration Group Parameters
#define kParamGroupCalibration "groupCalibration"

#define kParamCalibrationOutputIndex "calibrationOutputIndex"
#define kParamCalibrationCalculateResponse "calibrationCalculateResponse"

//HDR Group Parameters
#define kParamGroupHdr(G) "groupHdr_" + std::to_string(G)

//Image Group Parameters
#define kParamGroupImage(G, I) "groupImage_" + std::to_string(G) + "_" + std::to_string(I)

#define kParamImageIso(G, I) "imageParamIso_" + std::to_string(G) + "_" + std::to_string(I) //(!) nameID used for regex in ChangedParam
#define kParamImageAperture(G, I) "imageParamAperture_" + std::to_string(G) + "_" + std::to_string(I) //(!) nameID used for regex in ChangedParam
#define kParamImageShutter(G, I) "imageParamShutter_" + std::to_string(G) + "_" + std::to_string(I) //(!) nameID used for regex in ChangedParam
#define kParamImageEv(G, I) "imageEv_" + std::to_string(G) + "_" + std::to_string(I) //(!) nameID used for regex in ChangedParam


//Target Group Parameters
#define kParamGroupTarget "groupTarget"

#define kParamTargetImageIso "targetImageParamIso"
#define kParamTargetImageAperture "targetImageParamAperture" 
#define kParamTargetImageShutter "targetImageParamShutter"
#define kParamTargetImageEv "targetImageEv" 


//Algorithm Group Parameters
#define kParamGroupAlgorithm "groupAlgorithm"

#define kParamAlgorithmIterations "algorithmIterations"
#define kParamAlgorithmThreshold "algorithmThreshold"


//Weight Group Parameters
#define kParamGroupWeight "groupWeight"

#define kParamWeightPreset "weightPreset"
#define kParamWeightFilePath "weightFilePath"
#define kParamWeightLoad "weightLoad"
#define kParamWeightExport "weightExport"
#define kParamWeightRefreshKeys "weightRefreshKeys"
#define kParamWeightClearKeys "weightClearKeys"
#define kParamWeightFromKeys "weightFromKeys"
#define kParamWeightRed "weightRed"
#define kParamWeightGreen "weightGreen"
#define kParamWeightBlue "weightBlue"


//Response Group Parameters
#define kParamGroupResponse "groupResponse"

#define kParamResponseFilePath "responseFilePath"
#define kParamResponseExport "responseExport"
#define kParamResponseRed "responseRed"
#define kParamResponseGreen "responseGreen"
#define kParamResponseBlue "responseBlue"


//Invalidation Parameters
#define kParamForceInvalidation "forceInvalidation"