#pragma once

#include "StarJson.hpp"
#include "StarOrderedMap.hpp"
#include "StarRect.hpp"
#include "StarBiMap.hpp"
#include "StarThread.hpp"
#include "StarAssetSource.hpp"
#include "StarAssetPath.hpp"

namespace Star {

STAR_CLASS(Font);
STAR_CLASS(Audio);
STAR_CLASS(Image);
STAR_STRUCT(FramesSpecification);
STAR_CLASS(Assets);

STAR_EXCEPTION(AssetException, StarException);

// The contents of an assets .frames file, which can be associated with one or
// more images, and specifies named sub-rects of those images.
struct FramesSpecification {
  // Get the target sub-rect of a given frame name (which can be an alias).
  // Returns nothing if the frame name is not found.
  Maybe<RectU> getRect(String const& frame) const;

  // The full path to the .frames file from which this was loaded.
  String framesFile;
  // Named sub-frames
  StringMap<RectU> frames;
  // Aliases for named sub-frames, always points to a valid frame name in the
  // 'frames' map.
  StringMap<String> aliases;
};

// The assets system can load image, font, json, and data assets from a set of
// sources.  Each source is either a directory on the filesystem or a single
// packed asset file.
//
// Assets is thread safe and performs TTL caching.
class Assets {
public:
  struct Settings {
    // TTL for cached assets
    float assetTimeToLive;

    // Audio under this length will be automatically decompressed
    float audioDecompressLimit;

    // Number of background worker threads
    unsigned workerPoolSize;

    // If given, if an image is unable to load, will log the error and load
    // this path instead
    Maybe<String> missingImage;

    // Same, but for audio
    Maybe<String> missingAudio;

    // When loading assets from a directory, will automatically ignore any
    // files whose asset paths matching any of the given patterns.
    StringList pathIgnore;

    // Same, but only ignores the file for the purposes of calculating the
    // digest.
    StringList digestIgnore;
  };

    enum class AssetType {
    Json,
    Image,
    Audio,
    Font,
    Bytes
  };

  enum class QueuePriority {
    None,
    Working,
    PostProcess,
    Load
  };

  struct AssetId {
    AssetType type;
    AssetPath path;

    bool operator==(AssetId const& assetId) const;
  };

  struct AssetIdHash {
    size_t operator()(AssetId const& id) const;
  };

  struct AssetData {
    virtual ~AssetData() = default;

    // Should return true if this asset is shared and still in use, so freeing
    // it from cache will not really free the resource, so it should persist in
    // the cache.
    virtual bool shouldPersist() const = 0;

    double time = 0.0;
    bool needsPostProcessing = false;
  };

  struct JsonData : AssetData {
    bool shouldPersist() const override;

    Json json;
  };

  // Image data for an image, sub-frame, or post-processed image.
  struct ImageData : AssetData {
    bool shouldPersist() const override;

    ImageConstPtr image;

    // *Optional* sub-frames data for this image, only will exist when the
    // image is a top-level image and has an associated frames file.
    FramesSpecificationConstPtr frames;

    // If this image aliases another asset entry, this will be true and
    // shouldPersist will never be true (to ensure that this alias and its
    // target can be removed from the cache).
    bool alias = false;
  };

  struct AudioData : AssetData {
    bool shouldPersist() const override;

    AudioConstPtr audio;
  };

  struct FontData : AssetData {
    bool shouldPersist() const override;

    FontConstPtr font;
  };

  struct BytesData : AssetData {
    bool shouldPersist() const override;

    ByteArrayConstPtr bytes;
  };

  struct AssetFileDescriptor {
    // The mixed case original source name;
    String sourceName;
    // The source that has the primary asset copy
    AssetSourcePtr source;
    // List of source names and sources for patches to this file.
    List<pair<String, AssetSourcePtr>> patchSources;
  };

  Assets(Settings settings, StringList assetSources);
  ~Assets();

  // Returns a list of all the asset source paths used by Assets in load order.
  StringList assetSources() const;

  // Return metadata for the given loaded asset source path
  JsonObject assetSourceMetadata(String const& sourcePath) const;

  // An imperfect sha256 digest of the contents of all combined asset sources.
  // Useful for detecting if there are mismatched assets between a client and
  // server or if assets sources have changed from a previous load.
  ByteArray digest() const;

  // Is there an asset associated with the given path?  Path must not contain
  // sub-paths or directives.
  bool assetExists(String const& path) const;

  Maybe<AssetFileDescriptor> assetDescriptor(String const& path) const;

  // The name of the asset source within which the path exists.
  String assetSource(String const& path) const;

  Maybe<String> assetSourcePath(AssetSourcePtr const& source) const;

  // Scans for all assets with the given suffix in any directory.
  StringList scan(String const& suffix) const;
  // Scans for all assets matching both prefix and suffix (prefix may be, for
  // example, a directory)
  StringList scan(String const& prefix, String const& suffix) const;
  // Scans all assets for files with the given extension, which is specially
  // indexed and much faster than a normal scan.  Extension may contain leading
  // '.' character or it may be omitted.
  StringList scanExtension(String const& extension) const;

  // Get json asset with an optional sub-path.  The sub-path portion of the
  // path refers to a key in the top-level object, and may use dot notation
  // for deeper field access and [] notation for array access.  Example:
  // "/path/to/json:key1.key2.key3[4]".
  Json json(String const& path) const;

  // Either returns the json v, or, if v is a string type, returns the json
  // pointed to by interpreting v as a string path.
  Json fetchJson(Json const& v, String const& dir = "/") const;

  // Load all the given jsons using background processing.
  void queueJsons(StringList const& paths) const;

  // Returns *either* an image asset or a sub-frame.  Frame files are JSON
  // descriptor files that reference a particular image and label separate
  // sub-rects of the image.  If the given path has a ':' sub-path, then the
  // assets system will look for an associated .frames named either
  // <full-path-minus-extension>.frames or default.frames, going up to assets
  // root.  May return the same ImageConstPtr for different paths if the paths
  // are equivalent or they are aliases of other image paths.
  ImageConstPtr image(AssetPath const& path) const;
  // Load images using background processing
  void queueImages(StringList const& paths) const;
  // Return the given image *if* it is already loaded, otherwise queue it for
  // loading.
  ImageConstPtr tryImage(AssetPath const& path) const;

  // Returns the best associated FramesSpecification for a given image path, if
  // it exists.  The given path must not contain sub-paths or directives, and
  // this function may return nullptr if no frames file is associated with the
  // given image path.
  FramesSpecificationConstPtr imageFrames(String const& path) const;

  // Returns a pointer to a shared audio asset;
  AudioConstPtr audio(String const& path) const;
  // Load audios using background processing
  void queueAudios(StringList const& paths) const;
  // Return the given audio *if* it is already loaded, otherwise queue it for
  // loading.
  AudioConstPtr tryAudio(String const& path) const;

  // Returns pointer to shared font asset
  FontConstPtr font(String const& path) const;

  // Returns a bytes asset (Reads asset as an opaque binary blob)
  ByteArrayConstPtr bytes(String const& path) const;

  // Bypass asset caching and open an asset file directly.
  IODevicePtr openFile(String const& basePath) const;

  // Clear all cached assets that are not queued, persistent, or broken.
  void clearCache();

  // Run a cleanup pass and remove any assets past their time to live.
  void cleanup();

private:
  static FramesSpecification parseFramesSpecification(Json const& frameConfig, String path);

  void queueAssets(List<AssetId> const& assetIds) const;
  shared_ptr<AssetData> tryAsset(AssetId const& id) const;
  shared_ptr<AssetData> getAsset(AssetId const& id) const;

  void workerMain();

  // All methods below assume that the asset mutex is locked when calling.

  // Do some processing that might take a long time and should not hold the
  // assets mutex during it.  Unlocks the assets mutex while the function is in
  // progress and re-locks it on return or before exception is thrown.
  template <typename Function>
  decltype(auto) unlockDuring(Function f) const;

  // Returns the best frames specification for the given image path, if it exists.
  FramesSpecificationConstPtr bestFramesSpecification(String const& basePath) const;

  IODevicePtr open(String const& basePath) const;
  ByteArray read(String const& basePath) const;

  Json readJson(String const& basePath) const;

  // Load / post process an asset and log any exception.  Returns true if the
  // work was performed (whether successful or not), false if the work is
  // blocking on something.
  bool doLoad(AssetId const& id) const;
  bool doPost(AssetId const& id) const;

  // Assets can recursively depend on other assets, so the main entry point for
  // loading assets is in this separate method, and is safe for other loading
  // methods to call recursively.  If there is an error loading the asset, this
  // method will throw.  If, and only if, the asset is blocking on another busy
  // asset, this method will return null.
  shared_ptr<AssetData> loadAsset(AssetId const& id) const;

  shared_ptr<AssetData> loadJson(AssetPath const& path) const;
  shared_ptr<AssetData> loadImage(AssetPath const& path) const;
  shared_ptr<AssetData> loadAudio(AssetPath const& path) const;
  shared_ptr<AssetData> loadFont(AssetPath const& path) const;
  shared_ptr<AssetData> loadBytes(AssetPath const& path) const;

  shared_ptr<AssetData> postProcessAudio(shared_ptr<AssetData> const& original) const;

  // Updates time on the given asset (with smearing).
  void freshen(shared_ptr<AssetData> const& asset) const;

  Settings m_settings;

  mutable Mutex m_assetsMutex;

  mutable ConditionVariable m_assetsQueued;
  mutable OrderedHashMap<AssetId, QueuePriority, AssetIdHash> m_queue;

  mutable ConditionVariable m_assetsDone;
  mutable HashMap<AssetId, shared_ptr<AssetData>, AssetIdHash> m_assetsCache;

  mutable StringMap<String> m_bestFramesFiles;
  mutable StringMap<FramesSpecificationConstPtr> m_framesSpecifications;

  // Paths of all used asset sources, in load order.
  StringList m_assetSources;

  // Maps an asset path to the loaded asset source and vice versa
  BiMap<String, AssetSourcePtr> m_assetSourcePaths;

  // Maps the source asset name to the source containing it
  CaseInsensitiveStringMap<AssetFileDescriptor> m_files;
  // Maps an extension to the files with that extension
  CaseInsensitiveStringMap<StringList> m_filesByExtension;

  ByteArray m_digest;

  List<ThreadFunction<void>> m_workerThreads;
  atomic<bool> m_stopThreads;
};

}
