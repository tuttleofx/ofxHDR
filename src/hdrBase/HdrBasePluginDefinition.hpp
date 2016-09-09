#pragma once

/**
 * Global Parameters definition
 */

//Max number of HDR Group in Input
#define K_MAX_GROUPS 10

//Max number of image per HDR Group
#define K_MAX_IMAGES_PER_GROUP 20

//Image quantization
#define K_QUANTIZATION 1 << 12 //(12 bits)


/**
 * Plugin Parameters definition
 */

//Clip
#define kClip(I) std::to_string(I + 1)


//Source Group Parameters
#define kParamGroupSources "groupSources"

//Clip Group Parameters
#define kParamGroupClip(G) "groupClip_" + std::to_string(G)


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


//Response Group Parameters
#define kParamGroupResponse "groupResponse"

#define kParamResponsePreset "responsePreset"
#define kParamResponseFilePath "responseFilePath"
#define kParamResponseExport "responseExport"
#define kParamResponseRefreshKeys "responseRefreshKeys"
#define kParamResponseClearKeys "responseClearKeys"
#define kParamResponseRed "responseRed"
#define kParamResponseGreen "responseGreen"
#define kParamResponseBlue "responseBlue"


//Advanced Group Parameters
#define kParamGroupWeight "groupWeight"

#define kParamWeightPreset "weightPreset"
#define kParamWeightGaussianCustom "weightGaussianCustom"
#define kParamWeightFilePath "weightFilePath"
#define kParamWeightExport "weightExport"
#define kParamWeightRefreshKeys "weightRefreshKeys"
#define kParamWeightClearKeys "weightClearKeys"
#define kParamWeightRed "weightRed"
#define kParamWeightGreen "weightGreen"
#define kParamWeightBlue "weightBlue"


//Debug Group
#define kParamGroupDebug "groupDebug"

#define kParamDebugActive "debugActive"
#define kParamDebugOutput "debugOutput"


//Invalidation Parameters
#define kParamForceInvalidation "forceInvalidation"