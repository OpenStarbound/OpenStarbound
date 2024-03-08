INCLUDE_DIRECTORIES (
    ${STAR_EXTERN_INCLUDES}
    ${STAR_CORE_INCLUDES}
  )

SET (star_core_HEADERS
    StarAStar.hpp
    StarAlgorithm.hpp
    StarArray.hpp
    StarAssetPath.hpp
    StarAtomicSharedPtr.hpp
    StarAudio.hpp
    StarBTree.hpp
    StarBTreeDatabase.hpp
    StarBiMap.hpp
    StarBlockAllocator.hpp
    StarBuffer.hpp
    StarByteArray.hpp
    StarBytes.hpp
    StarCasting.hpp
    StarColor.hpp
    StarCompression.hpp
    StarConfig.hpp
    StarCurve25519.hpp
    StarDataStream.hpp
    StarDataStreamDevices.hpp
    StarDataStreamExtra.hpp
    StarDirectives.hpp
    StarDynamicLib.hpp
    StarEither.hpp
    StarEncode.hpp
    StarException.hpp
    StarFile.hpp
    StarFlatHashMap.hpp
    StarFlatHashSet.hpp
    StarFont.hpp
    StarFormat.hpp
    StarHash.hpp
    StarHostAddress.hpp
    StarIODevice.hpp
    StarIdMap.hpp
    StarImage.hpp
    StarImageProcessing.hpp
    StarInputEvent.hpp
    StarInterpolation.hpp
    StarRefPtr.hpp
    StarIterator.hpp
    StarJson.hpp
    StarJsonBuilder.hpp
    StarJsonExtra.hpp
    StarJsonParser.hpp
    StarJsonPath.hpp
    StarJsonPatch.hpp
    StarJsonRpc.hpp
    StarFormattedJson.hpp
    StarLexicalCast.hpp
    StarLine.hpp
    StarList.hpp
    StarListener.hpp
    StarLockFile.hpp
    StarLogging.hpp
    StarLruCache.hpp
    StarLua.hpp
    StarLuaConverters.hpp
    StarMap.hpp
    StarMathCommon.hpp
    StarMatrix3.hpp
    StarMaybe.hpp
    StarMemory.hpp
    StarMultiArray.hpp
    StarMultiArrayInterpolator.hpp
    StarMultiTable.hpp
    StarNetElement.hpp
    StarNetElementBasicFields.hpp
    StarNetElementContainers.hpp
    StarNetElementDynamicGroup.hpp
    StarNetElementFloatFields.hpp
    StarNetElementGroup.hpp
    StarNetElementSignal.hpp
    StarNetElementSyncGroup.hpp
    StarNetElementSystem.hpp
    StarNetElementTop.hpp
    StarNetImpl.hpp
    StarObserverStream.hpp
    StarOptionParser.hpp
    StarOrderedMap.hpp
    StarOrderedSet.hpp
    StarOutputProxy.hpp
    StarParametricFunction.hpp
    StarPch.hpp
    StarPeriodic.hpp
    StarPeriodicFunction.hpp
    StarPerlin.hpp
    StarPoly.hpp
    StarPythonic.hpp
    StarRandom.hpp
    StarRandomPoint.hpp
    StarRect.hpp
    StarRpcPromise.hpp
    StarRpcThreadPromise.hpp
    StarSectorArray2D.hpp
    StarSecureRandom.hpp
    StarSet.hpp
    StarSha256.hpp
    StarShellParser.hpp
    StarSignalHandler.hpp
    StarSocket.hpp
    StarSpatialHash2D.hpp
    StarSpline.hpp
    StarStaticRandom.hpp
    StarStaticVector.hpp
    StarString.hpp
    StarStringView.hpp
    StarStrongTypedef.hpp
    StarTcp.hpp
    StarText.hpp
    StarThread.hpp
    StarTickRateMonitor.hpp
    StarTime.hpp
    StarTtlCache.hpp
    StarUdp.hpp
    StarUnicode.hpp
    StarUuid.hpp
    StarVector.hpp
    StarVlqEncoding.hpp
    StarWeightedPool.hpp
    StarWorkerPool.hpp
    StarXXHash.hpp
  )

SET (star_core_SOURCES
    StarAudio.cpp
    StarAssetPath.cpp
    StarBTreeDatabase.cpp
    StarBuffer.cpp
    StarByteArray.cpp
    StarColor.cpp
    StarCompression.cpp
    StarCurve25519.cpp
    StarDataStream.cpp
    StarDataStreamDevices.cpp
    StarDirectives.cpp
    StarEncode.cpp
    StarFile.cpp
    StarFont.cpp
    StarHostAddress.cpp
    StarIODevice.cpp
    StarImage.cpp
    StarImageProcessing.cpp
    StarInputEvent.cpp
    StarJson.cpp
    StarJsonBuilder.cpp
    StarJsonExtra.cpp
    StarJsonPath.cpp
    StarJsonPatch.cpp
    StarJsonRpc.cpp
    StarFormattedJson.cpp
    StarListener.cpp
    StarLogging.cpp
    StarLua.cpp
    StarLuaConverters.cpp
    StarMemory.cpp
    StarNetElement.cpp
    StarNetElementBasicFields.cpp
    StarNetElementGroup.cpp
    StarNetElementSyncGroup.cpp
    StarOptionParser.cpp
    StarPerlin.cpp
    StarRandom.cpp
    StarSha256.cpp
    StarShellParser.cpp
    StarSocket.cpp
    StarString.cpp
    StarStringView.cpp
    StarTcp.cpp
    StarText.cpp
    StarThread.cpp
    StarTime.cpp
    StarTickRateMonitor.cpp
    StarUdp.cpp
    StarUnicode.cpp
    StarUuid.cpp
    StarWorkerPool.cpp
  )

IF (STAR_SYSTEM_FAMILY_UNIX)
  SET (star_core_SOURCES ${star_core_SOURCES}
      StarDynamicLib_unix.cpp
      StarException_unix.cpp
      StarFile_unix.cpp
      StarLockFile_unix.cpp
      StarSecureRandom_unix.cpp
      StarSignalHandler_unix.cpp
      StarThread_unix.cpp
      StarTime_unix.cpp
    )
ELSEIF (STAR_SYSTEM_FAMILY_WINDOWS)
  SET (star_core_HEADERS ${star_core_HEADERS}
      StarString_windows.hpp
    )

  SET (star_core_SOURCES ${star_core_SOURCES}
      StarDynamicLib_windows.cpp
      StarFile_windows.cpp
      StarLockFile_windows.cpp
      StarSignalHandler_windows.cpp
      StarString_windows.cpp
      StarThread_windows.cpp
      StarTime_windows.cpp
      StarException_windows.cpp
      StarSecureRandom_windows.cpp
    )

ENDIF ()

ADD_LIBRARY (star_core OBJECT ${star_core_SOURCES} ${star_core_HEADERS})

IF(STAR_PRECOMPILED_HEADERS)
  TARGET_PRECOMPILE_HEADERS (star_core PUBLIC StarPch.hpp)
ENDIF()

IF(STAR_USE_JEMALLOC AND JEMALLOC_IS_PREFIXED)
  SET_SOURCE_FILES_PROPERTIES(StarMemory.cpp PROPERTIES
          COMPILE_DEFINITIONS STAR_JEMALLOC_IS_PREFIXED
          )
ENDIF()