// Copyright 2020 The Khronos Group
// SPDX-License-Identifier: Apache-2.0

#ifndef UsdBridgeUsdWriter_h
#define UsdBridgeUsdWriter_h

#include "usd.h"
PXR_NAMESPACE_USING_DIRECTIVE

#include "UsdBridgeData.h"
#include "UsdBridgeCaches.h"
#include "UsdBridgeVolumeWriter.h"
#include "UsdBridgeConnection.h"
#include "UsdBridgeTimeEvaluator.h"

#include <memory>
#include <functional>

//Includes detailed usd translation interface of Usd Bridge
class UsdBridgeUsdWriter
{
public:
  using MaterialDMI = UsdBridgeMaterialData::DataMemberId;
  using SamplerDMI = UsdBridgeSamplerData::DataMemberId;
  using AtNewRefFunc = std::function<void(UsdBridgePrimCache* parentCache, UsdBridgePrimCache* childCache)>;
  using AtRemoveRefFunc = std::function<void(UsdBridgePrimCache* parentCache, UsdBridgePrimCache* childCache)>;

  struct RefModFuncs
  {
    AtNewRefFunc AtNewRef;
    AtRemoveRefFunc AtRemoveRef;
  };

  UsdBridgeUsdWriter(const UsdBridgeSettings& settings);
  ~UsdBridgeUsdWriter();

  void SetExternalSceneStage(UsdStageRefPtr sceneStage);
  void SetEnableSaving(bool enableSaving);

  int FindSessionNumber();
  bool CreateDirectories();
#ifdef CUSTOM_PBR_MDL 
  bool CreateMdlFiles();
#endif
  bool InitializeSession();
  void ResetSession();

  bool OpenSceneStage();
  UsdStageRefPtr GetSceneStage() const;
  UsdStageRefPtr GetTimeVarStage(UsdBridgePrimCache* cache
#ifdef TIME_CLIP_STAGES
    , bool useClipStage = false, const char* clipPf = nullptr, double timeStep = 0.0
    , std::function<void (UsdStageRefPtr)> initFunc = [](UsdStageRefPtr){}
#endif
    ) const;
#ifdef VALUE_CLIP_RETIMING
  void CreateManifestStage(const char* name, const char* primPostfix, UsdBridgePrimCache* cacheEntry);
  void RemoveManifestAndClipStages(const UsdBridgePrimCache* cacheEntry);

  const UsdStagePair& FindOrCreatePrimStage(UsdBridgePrimCache* cacheEntry, const char* namePostfix) const;
  const UsdStagePair& FindOrCreateClipStage(UsdBridgePrimCache* cacheEntry, const char* namePostfix, double timeStep, bool& exists) const;
  const UsdStagePair& FindOrCreatePrimClipStage(UsdBridgePrimCache* cacheEntry, const char* namePostfix, bool isClip, double timeStep, bool& exists) const;
#endif
  void AddRootPrim(UsdBridgePrimCache* primCache, const char* primPathCp, const char* layerId = nullptr);
  void RemoveRootPrim(UsdBridgePrimCache* primCache, const char* primPathCp);

  const std::string& CreatePrimName(const char* name, const char* category);
  const std::string& GetResourceFileName(const std::string& basePath, double timeStep, const char* fileExtension);
  const std::string& GetResourceFileName(const char* folderName, const std::string& objectName, double timeStep, const char* fileExtension);
  const std::string& GetResourceFileName(const char* folderName, const char* optionalObjectName, const std::string& defaultObjectName, double timeStep, const char* fileExtension);

  bool CreatePrim(const SdfPath& path);
  void DeletePrim(const UsdBridgePrimCache* cacheEntry);
#ifdef TIME_BASED_CACHING
  void InitializePrimVisibility(UsdStageRefPtr stage, const SdfPath& primPath, const UsdTimeCode& timeCode,
    UsdBridgePrimCache* parentCache, UsdBridgePrimCache* childCache);
  void SetPrimVisible(UsdStageRefPtr stage, const SdfPath& primPath, const UsdTimeCode& timeCode,
    UsdBridgePrimCache* parentCache, UsdBridgePrimCache* childCache);
  void PrimRemoveIfInvisibleAnytime(UsdStageRefPtr stage, const UsdPrim& prim, bool timeVarying, const UsdTimeCode& timeCode, AtRemoveRefFunc atRemoveRef,
    UsdBridgePrimCache* parentCache, UsdBridgePrimCache* primCache);
  void ChildrenRemoveIfInvisibleAnytime(UsdStageRefPtr stage, UsdBridgePrimCache* parentCache, const SdfPath& parentPath, bool timeVarying, const UsdTimeCode& timeCode, AtRemoveRefFunc atRemoveRef, const SdfPath& exceptPath = SdfPath());
#endif

#ifdef VALUE_CLIP_RETIMING
  void InitializeClipMetaData(const UsdPrim& clipPrim, UsdBridgePrimCache* childCache, double parentTimeStep, double childTimeStep, bool clipStages, const char* clipPostfix);
  void UpdateClipMetaData(const UsdPrim& clipPrim, UsdBridgePrimCache* childCache, double parentTimeStep, double childTimeStep, bool clipStages, const char* clipPostfix);
#endif

  SdfPath AddRef_NoClip(UsdBridgePrimCache* parentCache, UsdBridgePrimCache* childCache, const char* refPathExt,
    bool timeVarying, double parentTimeStep, bool instanceable,
    const RefModFuncs& refModCallbacks);
  SdfPath AddRef(UsdBridgePrimCache* parentCache, UsdBridgePrimCache* childCache, const char* refPathExt,
    bool timeVarying, bool valueClip, bool clipStages, const char* clipPostFix,
    double parentTimeStep, double childTimeStep, bool instanceable,
    const RefModFuncs& refModCallbacks);
  // Timevarying means that the existence of a reference between prim A and B can differ over time, valueClip denotes the addition of clip metadata for re-timing, 
  // and clipStages will carve up the referenced clip asset files into one for each timestep (stages are created as necessary). clipPostFix is irrelevant if !clipStages.
  // Returns the path to the parent's subprim which represents/holds the reference to the child's main class prim.
  SdfPath AddRef_Impl(UsdBridgePrimCache* parentCache, UsdBridgePrimCache* childCache, const char* refPathExt,
    bool timeVarying, bool valueClip, bool clipStages, const char* clipPostFix,
    double parentTimeStep, double childTimeStep, bool instanceable,
    const RefModFuncs& refModCallbacks);
  void RemoveAllRefs(UsdBridgePrimCache* parentCache, const char* refPathExt, bool timeVarying, double timeStep, AtRemoveRefFunc atRemoveRef);
  void RemoveAllRefs(UsdStageRefPtr stage, UsdBridgePrimCache* parentCache, SdfPath childBasePath, bool timeVarying, double timeStep, AtRemoveRefFunc atRemoveRef);
  void ManageUnusedRefs(UsdBridgePrimCache* parentCache, const UsdBridgePrimCacheList& newChildren, const char* refPathExt, bool timeVarying, double timeStep, AtRemoveRefFunc atRemoveRef);
  void ManageUnusedRefs(UsdStageRefPtr stage, UsdBridgePrimCache* parentCache, const UsdBridgePrimCacheList& newChildren, const char* refPathExt, bool timeVarying, double timeStep, AtRemoveRefFunc atRemoveRef);

  void InitializeUsdTransform(const UsdBridgePrimCache* cacheEntry);
  UsdPrim InitializeUsdGeometry(UsdStageRefPtr geometryStage, const SdfPath& geomPath, const UsdBridgeMeshData& meshData, bool uniformPrim);
  UsdPrim InitializeUsdGeometry(UsdStageRefPtr geometryStage, const SdfPath& geomPath, const UsdBridgeInstancerData& instancerData, bool uniformPrim);
  UsdPrim InitializeUsdGeometry(UsdStageRefPtr geometryStage, const SdfPath& geomPath, const UsdBridgeCurveData& curveData, bool uniformPrim);
  UsdPrim InitializeUsdVolume(UsdStageRefPtr volumeStage, const SdfPath& volumePath, bool uniformPrim) const;
  UsdShadeMaterial InitializeUsdMaterial(UsdStageRefPtr materialStage, const SdfPath& matPrimPath, bool uniformPrim) const;
  void InitializeUsdSampler(UsdStageRefPtr samplerStage,const SdfPath& samplerPrimPath, UsdBridgeSamplerData::SamplerType type, bool uniformPrim) const;
  void InitializeUsdCamera(UsdStageRefPtr cameraStage, const SdfPath& geomPath);
  UsdShadeShader GetOrCreateAttributeReader() const;

#ifdef USE_INDEX_MATERIALS
  UsdShadeMaterial InitializeIndexVolumeMaterial_Impl(UsdStageRefPtr volumeStage, 
    const SdfPath& volumePath, bool uniformPrim, const UsdBridgeTimeEvaluator<UsdBridgeVolumeData>* timeEval = nullptr) const;
#endif

#ifdef VALUE_CLIP_RETIMING
  void UpdateUsdGeometryManifest(const UsdBridgePrimCache* cacheEntry, const UsdBridgeMeshData& meshData);
  void UpdateUsdGeometryManifest(const UsdBridgePrimCache* cacheEntry, const UsdBridgeInstancerData& instancerData);
  void UpdateUsdGeometryManifest(const UsdBridgePrimCache* cacheEntry, const UsdBridgeCurveData& curveData);
  void UpdateUsdVolumeManifest(const UsdBridgePrimCache* cacheEntry, const UsdBridgeVolumeData& volumeData);
  void UpdateUsdMaterialManifest(const UsdBridgePrimCache* cacheEntry, const UsdBridgeMaterialData& matData);
  void UpdateUsdSamplerManifest(const UsdBridgePrimCache* cacheEntry, const UsdBridgeSamplerData& samplerData);
#endif

  void BindMaterialToGeom(const SdfPath& refGeomPath, const SdfPath& refMatPath);
  void ConnectSamplersToMaterial(UsdStageRefPtr materialStage, const SdfPath& matPrimPath, const SdfPrimPathList& refSamplerPrimPaths, 
    const UsdBridgePrimCacheList& samplerCacheEntries, const UsdSamplerRefData* samplerRefDatas, size_t numSamplers, double worldTimeStep); // Disconnect happens automatically at parameter overwrite

  void UnbindMaterialFromGeom(const SdfPath & refGeomPath);

  void UpdateUsdTransform(const SdfPath& transPrimPath, const float* transform, bool timeVarying, double timeStep);
  void UpdateUsdGeometry(const UsdStagePtr& timeVarStage, const SdfPath& meshPath, const UsdBridgeMeshData& geomData, double timeStep);
  void UpdateUsdGeometry(const UsdStagePtr& timeVarStage, const SdfPath& instancerPath, const UsdBridgeInstancerData& geomData, double timeStep);
  void UpdateUsdGeometry(const UsdStagePtr& timeVarStage, const SdfPath& curvePath, const UsdBridgeCurveData& geomData, double timeStep);
  void UpdateUsdMaterial(UsdStageRefPtr timeVarStage, const SdfPath& matPrimPath, const UsdBridgeMaterialData& matData, const UsdGeomPrimvarsAPI& boundGeomPrimvars, double timeStep);
  void UpdatePsShader(UsdStageRefPtr timeVarStage, const SdfPath& matPrimPath, const UsdBridgeMaterialData& matData, const UsdGeomPrimvarsAPI& boundGeomPrimvars, double timeStep);
  void UpdateMdlShader(UsdStageRefPtr timeVarStage, const SdfPath& matPrimPath, const UsdBridgeMaterialData& matData, const UsdGeomPrimvarsAPI& boundGeomPrimvars, double timeStep);
  void UpdateUsdVolume(UsdStageRefPtr timeVarStage, UsdBridgePrimCache* cacheEntry, const UsdBridgeVolumeData& volumeData, double timeStep);
  void UpdateUsdSampler(UsdStageRefPtr timeVarStage, UsdBridgePrimCache* cacheEntry, const UsdBridgeSamplerData& samplerData, double timeStep);
  void UpdateUsdCamera(UsdStageRefPtr timeVarStage, const SdfPath& cameraPrimPath, 
    const UsdBridgeCameraData& cameraData, double timeStep, bool timeVarHasChanged);
  void UpdateUsdInstancerPrototypes(const SdfPath& instancerPath, const UsdBridgeInstancerRefData& geomRefData, const SdfPrimPathList& refProtoGeomPrimPaths, const char* protoShapePathRp);
  void UpdateAttributeReader(UsdStageRefPtr timeVarStage, const SdfPath& matPrimPath, MaterialDMI dataMemberId, const char* newName, const UsdGeomPrimvarsAPI& boundGeomPrimvars, double timeStep, MaterialDMI timeVarying);
  void UpdateInAttribute(UsdStageRefPtr timeVarStage, const SdfPath& samplerPrimPath, const char* newName, double timeStep, SamplerDMI timeVarying);
  void UpdateBeginEndTime(double timeStep);

#ifdef USE_INDEX_MATERIALS
  void UpdateIndexVolumeMaterial(UsdStageRefPtr sceneStage, UsdStageRefPtr timeVarStage, const SdfPath& volumePath, const UsdBridgeVolumeData& volumeData, double timeStep);
#endif

  void ResetSharedResourceModified();

  TfToken& AttributeNameToken(const char* attribName);

  friend void ResourceCollectVolume(UsdBridgePrimCache* cache, UsdBridgeUsdWriter& usdWriter);
  friend void ResourceCollectSampler(UsdBridgePrimCache* cache, UsdBridgeUsdWriter& usdWriter);
  friend void RemoveResourceFiles(UsdBridgePrimCache* cache, UsdBridgeUsdWriter& usdWriter, 
    const char* resourceFolder, const char* extension);

  // Settings 
  UsdBridgeSettings Settings;
  UsdBridgeConnectionSettings ConnectionSettings;

  UsdBridgeLogObject LogObject;

protected:
  // Connect
  std::unique_ptr<UsdBridgeConnection> Connect = nullptr;

  // Volume writer
  std::shared_ptr<UsdBridgeVolumeWriterI> VolumeWriter; // shared - requires custom deleter

  // Shared resource cache (ie. resources shared between UsdBridgePrimCache entries)
  // Maps keys to a refcount and modified flag
  using SharedResourceValue = std::pair<int, bool>;
  using SharedResourceKV = std::pair<UsdBridgeResourceKey, SharedResourceValue>;
  using SharedResourceContainer = std::vector<SharedResourceKV>;
  SharedResourceContainer SharedResourceCache; 

  void AddSharedResourceRef(const UsdBridgeResourceKey& key);
  bool RemoveSharedResourceRef(const UsdBridgeResourceKey& key);
  // Sets modified flag and returns whether the shared resource has been modified since ResetSharedResourceModified()
  bool SetSharedResourceModified(const UsdBridgeResourceKey& key);

#ifdef USE_INDEX_MATERIALS
  void WriteTfPrimvars(const float* tfOpacities, const UsdBridgeVolumeData& volumeData, UsdAttribute& outAttrib, UsdTimeCode outTimeCode);
#endif

  // Token cache for attribute names
  std::vector<TfToken> AttributeTokens;

  // Session specific info
  int SessionNumber = -1;
  UsdStageRefPtr SceneStage;
  UsdStageRefPtr ExternalSceneStage;
  bool EnableSaving = true;
  std::string SceneFileName;
  std::string SessionDirectory;
  std::string RootName;
  std::string RootClassName;
#ifdef CUSTOM_PBR_MDL 
  SdfAssetPath MdlOpaqueRelFilePath; // relative from Scene Folder
  SdfAssetPath MdlTranslucentRelFilePath; // relative from Scene Folder
#endif

  double StartTime = 0.0;
  double EndTime = 0.0;

  std::string TempNameStr;
  std::vector<unsigned char> TempImageData;
};

void RemoveResourceFiles(UsdBridgePrimCache* cache, UsdBridgeUsdWriter& usdWriter, 
  const char* resourceFolder, const char* fileExtension);
void ResourceCollectVolume(UsdBridgePrimCache* cache, UsdBridgeUsdWriter& usdWriter);
void ResourceCollectSampler(UsdBridgePrimCache* cache, UsdBridgeUsdWriter& usdWriter);

#endif