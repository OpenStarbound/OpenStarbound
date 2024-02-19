#include "StarBTreeDatabase.hpp"
#include "StarSha256.hpp"
#include "StarVlqEncoding.hpp"

namespace Star {

BTreeDatabase::BTreeDatabase() {
  m_impl.parent = this;
  m_open = false;
  m_deviceSize = 0;
  m_blockSize = 2048;
  m_headFreeIndexBlock = InvalidBlockIndex;
  m_keySize = 0;
  m_autoCommit = true;
  m_indexCache.setMaxSize(64);
  m_root = InvalidBlockIndex;
  m_rootIsLeaf = false;
  m_usingAltRoot = false;
}

BTreeDatabase::BTreeDatabase(String const& contentIdentifier, size_t keySize)
  : BTreeDatabase() {
  setContentIdentifier(contentIdentifier);
  setKeySize(keySize);
}

BTreeDatabase::~BTreeDatabase() {
  close();
}

uint32_t BTreeDatabase::blockSize() const {
  ReadLocker readLocker(m_lock);
  return m_blockSize;
}

void BTreeDatabase::setBlockSize(uint32_t blockSize) {
  WriteLocker writeLocker(m_lock);
  checkIfOpen("setBlockSize", false);
  m_blockSize = blockSize;
}

uint32_t BTreeDatabase::keySize() const {
  ReadLocker readLocker(m_lock);
  return m_keySize;
}

void BTreeDatabase::setKeySize(uint32_t keySize) {
  WriteLocker writeLocker(m_lock);
  checkIfOpen("setKeySize", false);
  m_keySize = keySize;
}

String BTreeDatabase::contentIdentifier() const {
  ReadLocker readLocker(m_lock);
  return m_contentIdentifier;
}

void BTreeDatabase::setContentIdentifier(String contentIdentifier) {
  WriteLocker writeLocker(m_lock);
  checkIfOpen("setContentIdentifier", false);
  m_contentIdentifier = std::move(contentIdentifier);
}

uint32_t BTreeDatabase::indexCacheSize() const {
  SpinLocker lock(m_indexCacheSpinLock);
  return m_indexCache.maxSize();
}

void BTreeDatabase::setIndexCacheSize(uint32_t indexCacheSize) {
  SpinLocker lock(m_indexCacheSpinLock);
  m_indexCache.setMaxSize(indexCacheSize);
}

bool BTreeDatabase::autoCommit() const {
  ReadLocker readLocker(m_lock);
  return m_autoCommit;
}

void BTreeDatabase::setAutoCommit(bool autoCommit) {
  WriteLocker writeLocker(m_lock);
  m_autoCommit = autoCommit;
  if (m_autoCommit)
    doCommit();
}

IODevicePtr BTreeDatabase::ioDevice() const {
  ReadLocker readLocker(m_lock);
  return m_device;
}

void BTreeDatabase::setIODevice(IODevicePtr device) {
  WriteLocker writeLocker(m_lock);
  checkIfOpen("setIODevice", false);
  m_device = std::move(device);
}

bool BTreeDatabase::isOpen() const {
  ReadLocker readLocker(m_lock);
  return m_open;
}

bool BTreeDatabase::open() {
  WriteLocker writeLocker(m_lock);
  if (m_open)
    return false;

  if (!m_device)
    throw DBException("BlockStorage::open called with no IODevice set");

  if (!m_device->isOpen())
    m_device->open(IOMode::ReadWrite);

  m_open = true;

  if (m_device->size() > 0) {
    DataStreamIODevice ds(m_device);
    ds.seek(0);

    auto magic = ds.readBytes(VersionMagicSize);
    if (magic != ByteArray::fromCString(VersionMagic))
      throw DBException("Device is not a valid BTreeDatabase file");

    m_blockSize = ds.read<uint32_t>();

    auto contentIdentifier = ds.readBytes(ContentIdentifierStringSize);
    contentIdentifier.appendByte('\0');
    m_contentIdentifier = String(contentIdentifier.ptr());
    m_keySize = ds.read<uint32_t>();

    readRoot();

    if (m_device->isWritable())
      m_device->resize(m_deviceSize);

    return false;

  } else {
    m_deviceSize = HeaderSize;
    m_device->resize(m_deviceSize);
    m_headFreeIndexBlock = InvalidBlockIndex;

    DataStreamIODevice ds(m_device);
    ds.seek(0);

    ds.writeData(VersionMagic, VersionMagicSize);
    ds.write<uint32_t>(m_blockSize);

    if (m_contentIdentifier.empty())
      throw DBException("Opening new database and no content identifier set!");

    if (m_contentIdentifier.utf8Size() > ContentIdentifierStringSize)
      throw DBException("contentIdentifier in BTreeDatabase implementation is greater than maximum identifier length");
    if (m_keySize == 0)
      throw DBException("key size is not set opening a new BTreeDatabase");

    ByteArray contentIdentifier = m_contentIdentifier.utf8Bytes();
    contentIdentifier.resize(ContentIdentifierStringSize, 0);
    ds.writeBytes(contentIdentifier);
    ds.write(m_keySize);

    m_impl.createNewRoot();
    doCommit();

    return true;
  }
}

bool BTreeDatabase::contains(ByteArray const& k) {
  ReadLocker readLocker(m_lock);
  checkKeySize(k);
  return m_impl.contains(k);
}

Maybe<ByteArray> BTreeDatabase::find(ByteArray const& k) {
  ReadLocker readLocker(m_lock);
  checkKeySize(k);
  return m_impl.find(k);
}

List<pair<ByteArray, ByteArray>> BTreeDatabase::find(ByteArray const& lower, ByteArray const& upper) {
  ReadLocker readLocker(m_lock);
  checkKeySize(lower);
  checkKeySize(upper);
  return m_impl.find(lower, upper);
}

void BTreeDatabase::forEach(ByteArray const& lower, ByteArray const& upper, function<void(ByteArray, ByteArray)> v) {
  ReadLocker readLocker(m_lock);
  checkKeySize(lower);
  checkKeySize(upper);
  m_impl.forEach(lower, upper, std::move(v));
}

void BTreeDatabase::forAll(function<void(ByteArray, ByteArray)> v) {
  ReadLocker readLocker(m_lock);
  m_impl.forAll(std::move(v));
}

bool BTreeDatabase::insert(ByteArray const& k, ByteArray const& data) {
  WriteLocker writeLocker(m_lock);
  checkKeySize(k);
  return m_impl.insert(k, data);
}

bool BTreeDatabase::remove(ByteArray const& k) {
  WriteLocker writeLocker(m_lock);
  checkKeySize(k);
  return m_impl.remove(k);
}

uint64_t BTreeDatabase::recordCount() {
  ReadLocker readLocker(m_lock);
  return m_impl.recordCount();
}

uint8_t BTreeDatabase::indexLevels() {
  ReadLocker readLocker(m_lock);
  return m_impl.indexLevels();
}

uint32_t BTreeDatabase::totalBlockCount() {
  ReadLocker readLocker(m_lock);
  checkIfOpen("totalBlockCount", true);
  return (m_device->size() - HeaderSize) / m_blockSize;
}

uint32_t BTreeDatabase::freeBlockCount() {
  ReadLocker readLocker(m_lock);
  checkIfOpen("freeBlockCount", true);

  // Go through every FreeIndexBlock in the chain and count all of the tracked
  // free blocks.
  BlockIndex count = 0;
  BlockIndex indexBlockIndex = m_headFreeIndexBlock;
  while (indexBlockIndex != InvalidBlockIndex) {
    FreeIndexBlock indexBlock = readFreeIndexBlock(indexBlockIndex);
    count += 1 + indexBlock.freeBlocks.size();
    indexBlockIndex = indexBlock.nextFreeBlock;
  }

  count += m_availableBlocks.size() + m_pendingFree.size();

  // Include untracked blocks at the end of the file in the free count.
  count += (m_device->size() - m_deviceSize) / m_blockSize;

  return count;
}

uint32_t BTreeDatabase::indexBlockCount() {
  ReadLocker readLocker(m_lock);
  checkIfOpen("indexBlockCount", true);
  // Indexes are simply one index per block
  return m_impl.indexCount();
}

uint32_t BTreeDatabase::leafBlockCount() {
  WriteLocker writeLocker(m_lock);
  checkIfOpen("leafBlockCount", true);

  struct LeafBlocksVisitor {
    bool operator()(shared_ptr<IndexNode> const&) {
      return true;
    }

    bool operator()(shared_ptr<LeafNode> const& leaf) {
      leafBlockCount += 1 + parent->leafTailBlocks(leaf->self).size();
      return true;
    }

    BTreeDatabase* parent;
    BlockIndex leafBlockCount = 0;
  };

  LeafBlocksVisitor visitor;
  visitor.parent = this;
  m_impl.forAllNodes(visitor);

  return visitor.leafBlockCount;
}

void BTreeDatabase::commit() {
  WriteLocker writeLocker(m_lock);
  doCommit();
}

void BTreeDatabase::rollback() {
  WriteLocker writeLocker(m_lock);

  m_availableBlocks.clear();
  m_indexCache.clear();
  m_uncommitted.clear();
  m_pendingFree.clear();

  readRoot();

  if (m_device->isWritable())
    m_device->resize(m_deviceSize);
}

void BTreeDatabase::close(bool closeDevice) {
  WriteLocker writeLocker(m_lock);
  if (m_open) {
    doCommit();

    m_indexCache.clear();

    m_open = false;
    if (closeDevice && m_device && m_device->isOpen())
      m_device->close();
  }
}

BTreeDatabase::BlockIndex const BTreeDatabase::InvalidBlockIndex;
uint32_t const BTreeDatabase::HeaderSize;
char const* const BTreeDatabase::VersionMagic = "BTreeDB5";
uint32_t const BTreeDatabase::VersionMagicSize;
char const* const BTreeDatabase::IndexMagic = "II";
char const* const BTreeDatabase::LeafMagic = "LL";
char const* const BTreeDatabase::FreeIndexMagic = "FF";
size_t const BTreeDatabase::BTreeRootSelectorBit;
size_t const BTreeDatabase::BTreeRootInfoStart;
size_t const BTreeDatabase::BTreeRootInfoSize;

size_t BTreeDatabase::IndexNode::pointerCount() const {
  // If no begin pointer is set then the index is simply uninitialized.
  if (!beginPointer)
    return 0;
  else
    return pointers.size() + 1;
}

auto BTreeDatabase::IndexNode::pointer(size_t i) const -> BlockIndex {
  if (i == 0)
    return *beginPointer;
  else
    return pointers.at(i - 1).pointer;
}

void BTreeDatabase::IndexNode::updatePointer(size_t i, BlockIndex p) {
  if (i == 0)
    *beginPointer = p;
  else
    pointers.at(i - 1).pointer = p;
}

ByteArray const& BTreeDatabase::IndexNode::keyBefore(size_t i) const {
  return pointers.at(i - 1).key;
}

void BTreeDatabase::IndexNode::updateKeyBefore(size_t i, ByteArray k) {
  pointers.at(i - 1).key = k;
}

void BTreeDatabase::IndexNode::removeBefore(size_t i) {
  if (i == 0) {
    beginPointer = pointers.at(0).pointer;
    pointers.eraseAt(0);
  } else {
    pointers.eraseAt(i - 1);
  }
}

void BTreeDatabase::IndexNode::insertAfter(size_t i, ByteArray k, BlockIndex p) {
  pointers.insertAt(i, Element{k, p});
}

uint8_t BTreeDatabase::IndexNode::indexLevel() const {
  return level;
}

void BTreeDatabase::IndexNode::setIndexLevel(uint8_t indexLevel) {
  level = indexLevel;
}

void BTreeDatabase::IndexNode::shiftLeft(ByteArray const& mid, IndexNode& right, size_t count) {
  count = std::min(right.pointerCount(), count);

  if (count == 0)
    return;

  pointers.append(Element{mid, *right.beginPointer});

  ElementList::iterator s = right.pointers.begin();
  std::advance(s, count - 1);
  pointers.insert(pointers.end(), right.pointers.begin(), s);

  right.pointers.erase(right.pointers.begin(), s);
  if (right.pointers.size() != 0) {
    right.beginPointer = right.pointers.at(0).pointer;
    right.pointers.eraseAt(0);
  } else {
    right.beginPointer.reset();
  }
}

void BTreeDatabase::IndexNode::shiftRight(ByteArray const& mid, IndexNode& left, size_t count) {
  count = std::min(left.pointerCount(), count);

  if (count == 0)
    return;
  --count;

  pointers.insert(pointers.begin(), Element{mid, *beginPointer});

  ElementList::iterator s = left.pointers.begin();
  std::advance(s, left.pointers.size() - count);
  pointers.insert(pointers.begin(), s, left.pointers.end());

  left.pointers.erase(s, left.pointers.end());
  if (left.pointers.size() != 0) {
    beginPointer = left.pointers.at(left.pointers.size() - 1).pointer;
    left.pointers.eraseAt(left.pointers.size() - 1);
  } else {
    beginPointer = left.beginPointer.take();
  }
}

ByteArray BTreeDatabase::IndexNode::split(IndexNode& right, size_t i) {
  ElementList::iterator s = pointers.begin();
  std::advance(s, i - 1);

  right.beginPointer = s->pointer;
  ByteArray midKey = s->key;
  right.level = level;
  ++s;

  right.pointers.insert(right.pointers.begin(), s, pointers.end());
  --s;

  pointers.erase(s, pointers.end());

  return midKey;
}

size_t BTreeDatabase::LeafNode::count() const {
  return elements.size();
}

ByteArray const& BTreeDatabase::LeafNode::key(size_t i) const {
  return elements.at(i).key;
}

ByteArray const& BTreeDatabase::LeafNode::data(size_t i) const {
  return elements.at(i).data;
}

void BTreeDatabase::LeafNode::insert(size_t i, ByteArray k, ByteArray d) {
  elements.insertAt(i, Element{std::move(k), std::move(d)});
}

void BTreeDatabase::LeafNode::remove(size_t i) {
  elements.eraseAt(i);
}

void BTreeDatabase::LeafNode::shiftLeft(LeafNode& right, size_t count) {
  count = std::min(right.count(), count);

  if (count == 0)
    return;

  ElementList::iterator s = right.elements.begin();
  std::advance(s, count);

  elements.insert(elements.end(), right.elements.begin(), s);
  right.elements.erase(right.elements.begin(), s);
}

void BTreeDatabase::LeafNode::shiftRight(LeafNode& left, size_t count) {
  count = std::min(left.count(), count);

  if (count == 0)
    return;

  ElementList::iterator s = left.elements.begin();
  std::advance(s, left.elements.size() - count);

  elements.insert(elements.begin(), s, left.elements.end());
  left.elements.erase(s, left.elements.end());
}

void BTreeDatabase::LeafNode::split(LeafNode& right, size_t i) {
  ElementList::iterator s = elements.begin();
  std::advance(s, i);

  right.elements.insert(right.elements.begin(), s, elements.end());
  elements.erase(s, elements.end());
}

auto BTreeDatabase::BTreeImpl::rootPointer() -> Pointer {
  return parent->m_root;
}

bool BTreeDatabase::BTreeImpl::rootIsLeaf() {
  return parent->m_rootIsLeaf;
}

void BTreeDatabase::BTreeImpl::setNewRoot(Pointer pointer, bool isLeaf) {
  parent->m_root = pointer;
  parent->m_rootIsLeaf = isLeaf;

  if (parent->m_autoCommit)
    parent->doCommit();
}

auto BTreeDatabase::BTreeImpl::createIndex(Pointer beginPointer) -> Index {
  auto index = make_shared<IndexNode>();
  index->self = InvalidBlockIndex;
  index->level = 0;
  index->beginPointer = beginPointer;
  return index;
}

auto BTreeDatabase::BTreeImpl::loadIndex(Pointer pointer) -> Index {
  SpinLocker lock(parent->m_indexCacheSpinLock);
  if (auto index = parent->m_indexCache.ptr(pointer))
    return *index;
  lock.unlock();

  auto index = make_shared<IndexNode>();

  DataStreamBuffer buffer(parent->readBlock(pointer));

  if (buffer.readBytes(2) != ByteArray(IndexMagic, 2))
    throw DBException("Error, incorrect index block signature.");

  index->self = pointer;

  index->level = buffer.read<uint8_t>();
  uint32_t s = buffer.read<uint32_t>();
  index->beginPointer = buffer.read<BlockIndex>();
  index->pointers.resize(s);
  for (uint32_t i = 0; i < s; ++i) {
    auto& e = index->pointers[i];
    e.key =buffer.readBytes(parent->m_keySize);
    e.pointer = buffer.read<BlockIndex>();
  }

  lock.lock();
  parent->m_indexCache.set(pointer, index);
  return index;
}

bool BTreeDatabase::BTreeImpl::indexNeedsShift(Index const& index) {
  return index->pointerCount() < (parent->maxIndexPointers() + 1) / 2;
}

bool BTreeDatabase::BTreeImpl::indexShift(Index const& left, Key const& mid, Index const& right) {
  if (left->pointerCount() + right->pointerCount() <= parent->maxIndexPointers()) {
    left->shiftLeft(mid, *right, right->pointerCount());
    return true;
  } else {
    if (indexNeedsShift(right)) {
      right->shiftRight(mid, *left, 1);
      return true;
    } else if (indexNeedsShift(left)) {
      left->shiftLeft(mid, *right, 1);
      return true;
    } else {
      return false;
    }
  }
}

auto BTreeDatabase::BTreeImpl::indexSplit(Index const& index) -> Maybe<pair<Key, Index>> {
  if (index->pointerCount() <= parent->maxIndexPointers())
    return {};

  auto right = make_shared<IndexNode>();
  right->self = InvalidBlockIndex;
  Key k = index->split(*right, (index->pointerCount() + 1) / 2);
  return make_pair(k, right);
}

auto BTreeDatabase::BTreeImpl::storeIndex(Index index) -> Pointer {
  if (index->self != InvalidBlockIndex) {
    if (!parent->m_uncommitted.contains(index->self)) {
      parent->freeBlock(index->self);
      parent->m_indexCache.remove(index->self);
      index->self = InvalidBlockIndex;
    }
  }

  if (index->self == InvalidBlockIndex)
    index->self = parent->reserveBlock();

  DataStreamBuffer buffer(parent->m_blockSize);
  buffer.writeData(IndexMagic, 2);

  buffer.write<uint8_t>(index->level);
  buffer.write<uint32_t>(index->pointers.size());
  buffer.write<BlockIndex>(*index->beginPointer);
  for (auto i = index->pointers.begin(); i != index->pointers.end(); ++i) {
    starAssert(i->key.size() == parent->m_keySize);
    buffer.writeBytes(i->key);
    buffer.write<BlockIndex>(i->pointer);
  }

  parent->updateBlock(index->self, buffer.data());

  parent->m_indexCache.set(index->self, index);
  return index->self;
}

void BTreeDatabase::BTreeImpl::deleteIndex(Index index) {
  parent->m_indexCache.remove(index->self);
  parent->freeBlock(index->self);
}

auto BTreeDatabase::BTreeImpl::createLeaf() -> Leaf {
  auto leaf = make_shared<LeafNode>();
  leaf->self = InvalidBlockIndex;
  return leaf;
}

auto BTreeDatabase::BTreeImpl::loadLeaf(Pointer pointer) -> Leaf {
  auto leaf = make_shared<LeafNode>();
  leaf->self = pointer;

  BlockIndex currentLeafBlock = leaf->self;
  DataStreamBuffer leafBuffer;
  leafBuffer.reset(parent->m_blockSize);
  parent->readBlock(currentLeafBlock, 0, leafBuffer.ptr(), parent->m_blockSize);

  if (leafBuffer.readBytes(2) != ByteArray(LeafMagic, 2))
    throw DBException("Error, incorrect leaf block signature.");

  DataStreamFunctions leafInput([&](char* data, size_t len) -> size_t {
      size_t pos = 0;
      size_t left = len;

      while (left > 0) {
        if (leafBuffer.pos() + left < parent->m_blockSize - sizeof(BlockIndex)) {
          leafBuffer.readData(data + pos, left);
          pos += left;
          left = 0;
        } else {
          size_t toRead = parent->m_blockSize - sizeof(BlockIndex) - leafBuffer.pos();
          leafBuffer.readData(data + pos, toRead);
          pos += toRead;
          left -= toRead;
        }

        if (leafBuffer.pos() == (parent->m_blockSize - sizeof(BlockIndex)) && left > 0) {
          currentLeafBlock = leafBuffer.read<BlockIndex>();
          if (currentLeafBlock != InvalidBlockIndex) {
            leafBuffer.reset(parent->m_blockSize);
            parent->readBlock(currentLeafBlock, 0, leafBuffer.ptr(), parent->m_blockSize);

            if (leafBuffer.readBytes(2) != ByteArray(LeafMagic, 2))
              throw DBException("Error, incorrect leaf block signature.");

          } else {
            throw DBException("Leaf read off end of Leaf list.");
          }
        }
      }

      return len;
    }, {});

  uint32_t count = leafInput.read<uint32_t>();
  leaf->elements.resize(count);
  for (uint32_t i = 0; i < count; ++i) {
    auto& element = leaf->elements[i];
    element.key = leafInput.readBytes(parent->m_keySize);
    element.data = leafInput.read<ByteArray>();
  }

  return leaf;
}

bool BTreeDatabase::BTreeImpl::leafNeedsShift(Leaf const& l) {
  return parent->leafSize(l) < parent->m_blockSize / 2;
}

bool BTreeDatabase::BTreeImpl::leafShift(Leaf& left, Leaf& right) {
  if (left->count() == 0) {
    left->shiftLeft(*right, right->count());
    return true;
  }

  if (right->count() == 0)
    return true;

  uint32_t leftSize = parent->leafSize(left);
  uint32_t rightSize = parent->leafSize(right);
  if (leftSize + rightSize < parent->m_blockSize) {
    left->shiftLeft(*right, right->count());
    return true;
  }

  // TODO: Shifting algorithm is bad, could potentially want to shift more
  // than one element here.
  uint32_t rightBeginSize = parent->m_keySize + parent->dataSize(right->elements[0].data);
  uint32_t leftEndSize = parent->m_keySize + parent->dataSize(left->elements[left->elements.size() - 1].data);
  if (leftSize < rightSize - rightBeginSize && leftSize + rightBeginSize < parent->m_blockSize) {
    left->shiftLeft(*right, 1);
    return true;
  } else if (rightSize < leftSize - leftEndSize && rightSize + leftEndSize < parent->m_blockSize) {
    right->shiftRight(*left, 1);
    return true;
  }

  return false;
}

auto BTreeDatabase::BTreeImpl::leafSplit(Leaf& leaf) -> Maybe<Leaf> {
  if (leaf->elements.size() < 2)
    return {};

  uint32_t size = 6;
  bool boundaryFound = false;
  uint32_t boundary = 0;
  for (uint32_t i = 0; i < leaf->elements.size(); ++i) {
    size += parent->m_keySize;
    size += parent->dataSize(leaf->elements[i].data);
    if (size > parent->m_blockSize - sizeof(BlockIndex) && !boundaryFound) {
      boundary = i;
      boundaryFound = true;
    }
  }
  if (boundary == 0)
    boundary = 1;

  if (size < parent->m_blockSize * 2 - 2 * sizeof(BlockIndex) - 4) {
    return {};
  } else {
    auto right = make_shared<LeafNode>();
    right->self = InvalidBlockIndex;
    leaf->split(*right, boundary);
    return right;
  }
}

auto BTreeDatabase::BTreeImpl::storeLeaf(Leaf leaf) -> Pointer {
  if (leaf->self != InvalidBlockIndex) {
    List<BlockIndex> tailBlocks = parent->leafTailBlocks(leaf->self);
    for (uint32_t i = 0; i < tailBlocks.size(); ++i)
      parent->freeBlock(tailBlocks[i]);

    if (!parent->m_uncommitted.contains(leaf->self)) {
      parent->freeBlock(leaf->self);
      leaf->self = InvalidBlockIndex;
    }
  }

  if (leaf->self == InvalidBlockIndex)
    leaf->self = parent->reserveBlock();

  BlockIndex currentLeafBlock = leaf->self;
  DataStreamBuffer leafBuffer;
  leafBuffer.reset(parent->m_blockSize);
  leafBuffer.writeData(LeafMagic, 2);

  DataStreamFunctions leafOutput({}, [&](char const* data, size_t len) -> size_t {
      size_t pos = 0;
      size_t left = len;

      while (true) {
        size_t toWrite = left;
        if (toWrite > parent->m_blockSize - leafBuffer.pos() - sizeof(BlockIndex))
          toWrite = parent->m_blockSize - leafBuffer.pos() - sizeof(BlockIndex);

        if (toWrite != 0) {
          leafBuffer.writeData(data + pos, toWrite);
          left -= toWrite;
          pos += toWrite;
        }

        if (left == 0)
          break;

        if (leafBuffer.pos() == (parent->m_blockSize - sizeof(BlockIndex))) {
          BlockIndex nextBlock = parent->reserveBlock();
          leafBuffer.write<BlockIndex>(nextBlock);
          parent->updateBlock(currentLeafBlock, leafBuffer.data());
          currentLeafBlock = nextBlock;
          leafBuffer.reset(parent->m_blockSize);
          leafBuffer.writeData(LeafMagic, 2);
        }
      }

      return len;
    });

  leafOutput.write<uint32_t>(leaf->elements.size());

  for (LeafNode::ElementList::iterator i = leaf->elements.begin(); i != leaf->elements.end(); ++i) {
    starAssert(i->key.size() == parent->m_keySize);
    leafOutput.writeBytes(i->key);
    leafOutput.write(i->data);
  }

  leafBuffer.seek(parent->m_blockSize - sizeof(BlockIndex));
  leafBuffer.write<BlockIndex>(InvalidBlockIndex);
  parent->updateBlock(currentLeafBlock, leafBuffer.data());

  return leaf->self;
}

void BTreeDatabase::BTreeImpl::deleteLeaf(Leaf leaf) {
  List<BlockIndex> tailBlocks = parent->leafTailBlocks(leaf->self);
  for (uint32_t i = 0; i < tailBlocks.size(); ++i)
    parent->freeBlock(tailBlocks[i]);

  parent->freeBlock(leaf->self);
}

size_t BTreeDatabase::BTreeImpl::indexPointerCount(Index const& index) {
  return index->pointerCount();
}

auto BTreeDatabase::BTreeImpl::indexPointer(Index const& index, size_t i) -> Pointer {
  return index->pointer(i);
}

void BTreeDatabase::BTreeImpl::indexUpdatePointer(Index& index, size_t i, Pointer p) {
  index->updatePointer(i, p);
}

auto BTreeDatabase::BTreeImpl::indexKeyBefore(Index const& index, size_t i) -> Key {
  return index->keyBefore(i);
}

void BTreeDatabase::BTreeImpl::indexUpdateKeyBefore(Index& index, size_t i, Key k) {
  index->updateKeyBefore(i, k);
}

void BTreeDatabase::BTreeImpl::indexRemoveBefore(Index& index, size_t i) {
  index->removeBefore(i);
}

void BTreeDatabase::BTreeImpl::indexInsertAfter(Index& index, size_t i, Key k, Pointer p) {
  index->insertAfter(i, k, p);
}

size_t BTreeDatabase::BTreeImpl::indexLevel(Index const& index) {
  return index->indexLevel();
}

void BTreeDatabase::BTreeImpl::setIndexLevel(Index& index, size_t indexLevel) {
  index->setIndexLevel(indexLevel);
}

size_t BTreeDatabase::BTreeImpl::leafElementCount(Leaf const& leaf) {
  return leaf->count();
}

auto BTreeDatabase::BTreeImpl::leafKey(Leaf const& leaf, size_t i) -> Key {
  return leaf->key(i);
}

auto BTreeDatabase::BTreeImpl::leafData(Leaf const& leaf, size_t i) -> Data {
  return leaf->data(i);
}

void BTreeDatabase::BTreeImpl::leafInsert(Leaf& leaf, size_t i, Key k, Data d) {
  leaf->insert(i, std::move(k), std::move(d));
}

void BTreeDatabase::BTreeImpl::leafRemove(Leaf& leaf, size_t i) {
  leaf->remove(i);
}

auto BTreeDatabase::BTreeImpl::nextLeaf(Leaf const&) -> Maybe<Pointer> {
  return {};
}

void BTreeDatabase::BTreeImpl::setNextLeaf(Leaf&, Maybe<Pointer>) {}

void BTreeDatabase::readBlock(BlockIndex blockIndex, size_t blockOffset, char* block, size_t size) const {
  checkBlockIndex(blockIndex);
  rawReadBlock(blockIndex, blockOffset, block, size);
}

ByteArray BTreeDatabase::readBlock(BlockIndex blockIndex) const {
  ByteArray block(m_blockSize, 0);
  readBlock(blockIndex, 0, block.ptr(), m_blockSize);
  return block;
}

void BTreeDatabase::updateBlock(BlockIndex blockIndex, ByteArray const& block) {
  checkBlockIndex(blockIndex);
  rawWriteBlock(blockIndex, 0, block.ptr(), block.size());
}

void BTreeDatabase::rawReadBlock(BlockIndex blockIndex, size_t blockOffset, char* block, size_t size) const {
  if (blockOffset > m_blockSize || size > m_blockSize - blockOffset)
    throw DBException::format("Read past end of block, offset: {} size {}", blockOffset, size);

  if (size <= 0)
    return;

  m_device->readFullAbsolute(HeaderSize + blockIndex * (StreamOffset)m_blockSize + blockOffset, block, size);
}

void BTreeDatabase::rawWriteBlock(BlockIndex blockIndex, size_t blockOffset, char const* block, size_t size) const {
  if (blockOffset > m_blockSize || size > m_blockSize - blockOffset)
    throw DBException::format("Write past end of block, offset: {} size {}", blockOffset, size);

  if (size <= 0)
    return;

  m_device->writeFullAbsolute(HeaderSize + blockIndex * (StreamOffset)m_blockSize + blockOffset, block, size);
}

auto BTreeDatabase::readFreeIndexBlock(BlockIndex blockIndex) -> FreeIndexBlock {
  checkBlockIndex(blockIndex);

  ByteArray magic(2, 0);
  rawReadBlock(blockIndex, 0, magic.ptr(), 2);
  if (magic != ByteArray(FreeIndexMagic, 2))
    throw DBException::format("Internal exception! block {} missing free index block marker!", blockIndex);

  FreeIndexBlock freeIndexBlock;
  DataStreamBuffer buffer(max(sizeof(BlockIndex), (size_t)4));

  rawReadBlock(blockIndex, 2, buffer.ptr(), sizeof(BlockIndex));
  buffer.seek(0);
  freeIndexBlock.nextFreeBlock = buffer.read<BlockIndex>();

  rawReadBlock(blockIndex, 2 + sizeof(BlockIndex), buffer.ptr(), 4);
  buffer.seek(0);
  size_t numFree = buffer.read<uint32_t>();

  for (size_t i = 0; i < numFree; ++i) {
    rawReadBlock(blockIndex, 6 + sizeof(BlockIndex) + sizeof(BlockIndex) * i, buffer.ptr(), sizeof(BlockIndex));
    buffer.seek(0);
    freeIndexBlock.freeBlocks.append(buffer.read<BlockIndex>());
  }

  return freeIndexBlock;
}

void BTreeDatabase::writeFreeIndexBlock(BlockIndex blockIndex, FreeIndexBlock indexBlock) {
  checkBlockIndex(blockIndex);

  rawWriteBlock(blockIndex, 0, FreeIndexMagic, 2);
  DataStreamBuffer buffer(max(sizeof(BlockIndex), (size_t)4));

  buffer.seek(0);
  buffer.write<BlockIndex>(indexBlock.nextFreeBlock);
  rawWriteBlock(blockIndex, 2, buffer.ptr(), sizeof(BlockIndex));

  buffer.seek(0);
  buffer.write<uint32_t>(indexBlock.freeBlocks.size());
  rawWriteBlock(blockIndex, 2 + sizeof(BlockIndex), buffer.ptr(), 4);

  for (size_t i = 0; i < indexBlock.freeBlocks.size(); ++i) {
    buffer.seek(0);
    buffer.write<BlockIndex>(indexBlock.freeBlocks[i]);
    rawWriteBlock(blockIndex, 6 + sizeof(BlockIndex) + sizeof(BlockIndex) * i, buffer.ptr(), sizeof(BlockIndex));
  }
}

uint32_t BTreeDatabase::leafSize(shared_ptr<LeafNode> const& leaf) const {
  size_t s = 6;
  for (LeafNode::ElementList::iterator i = leaf->elements.begin(); i != leaf->elements.end(); ++i) {
    s += m_keySize;
    s += dataSize(i->data);
  }
  return s;
}

uint32_t BTreeDatabase::maxIndexPointers() const {
  // 2 for magic, 1 byte for level, sizeof(BlockIndex) for beginPointer, 4
  // for size.
  return (m_blockSize - 2 - 1 - sizeof(BlockIndex) - 4) / (m_keySize + sizeof(BlockIndex)) + 1;
}

uint32_t BTreeDatabase::dataSize(ByteArray const& d) const {
  return vlqUSize(d.size()) + d.size();
}

auto BTreeDatabase::leafTailBlocks(BlockIndex leafPointer) -> List<BlockIndex> {
  List<BlockIndex> tailBlocks;
  DataStreamBuffer pointerBuffer(sizeof(BlockIndex));
  while (leafPointer != InvalidBlockIndex) {
    readBlock(leafPointer, m_blockSize - sizeof(BlockIndex), pointerBuffer.ptr(), sizeof(BlockIndex));
    pointerBuffer.seek(0);
    leafPointer = pointerBuffer.read<BlockIndex>();
    if (leafPointer != InvalidBlockIndex)
      tailBlocks.append(leafPointer);
  }
  return tailBlocks;
}

void BTreeDatabase::freeBlock(BlockIndex b) {
  if (m_uncommitted.contains(b)) {
    m_uncommitted.remove(b);
    m_availableBlocks.add(b);
  } else {
    m_pendingFree.append(b);
  }
}

auto BTreeDatabase::reserveBlock() -> BlockIndex {
  if (m_availableBlocks.empty()) {
    if (m_headFreeIndexBlock != InvalidBlockIndex) {
      // If available, make available all the blocks in the first free index
      // block.
      FreeIndexBlock indexBlock = readFreeIndexBlock(m_headFreeIndexBlock);
      for (auto const& b : indexBlock.freeBlocks)
        m_availableBlocks.add(b);
      // We cannot make available the block itself, because we must maintain
      // atomic consistency.  We will need to free this block later and commit
      // the new free index block chain.
      m_pendingFree.append(m_headFreeIndexBlock);
      m_headFreeIndexBlock = indexBlock.nextFreeBlock;
    }

    if (m_availableBlocks.empty()) {
      // If we still don't have any available blocks, just add a block to the
      // end of the file.
      m_availableBlocks.add(makeEndBlock());
    }
  }

  BlockIndex block = m_availableBlocks.takeFirst();
  m_uncommitted.add(block);
  return block;
}

auto BTreeDatabase::makeEndBlock() -> BlockIndex {
  BlockIndex blockCount = (m_deviceSize - HeaderSize) / m_blockSize;
  m_deviceSize += m_blockSize;
  m_device->resize(m_deviceSize);
  return blockCount;
}

void BTreeDatabase::writeRoot() {
  DataStreamIODevice ds(m_device);
  // First write the root info to whichever section we are not currently using
  ds.seek(BTreeRootInfoStart + (m_usingAltRoot ? 0 : BTreeRootInfoSize));
  ds.write<BlockIndex>(m_headFreeIndexBlock);
  ds.write<StreamOffset>(m_deviceSize);
  ds.write<BlockIndex>(m_root);
  ds.write<bool>(m_rootIsLeaf);

  // Then flush all the pending changes.
  m_device->sync();

  // Then switch headers by writing the single bit that switches them
  m_usingAltRoot = !m_usingAltRoot;
  ds.seek(BTreeRootSelectorBit);
  ds.write(m_usingAltRoot);

  // Then flush this single bit write to make sure it happens before anything
  // else.
  m_device->sync();
}

void BTreeDatabase::readRoot() {
  DataStreamIODevice ds(m_device);
  ds.seek(BTreeRootSelectorBit);
  ds.read(m_usingAltRoot);

  ds.seek(BTreeRootInfoStart + (m_usingAltRoot ? BTreeRootInfoSize : 0));
  m_headFreeIndexBlock = ds.read<BlockIndex>();
  m_deviceSize = ds.read<StreamOffset>();
  m_root = ds.read<BlockIndex>();
  m_rootIsLeaf = ds.read<bool>();
}

void BTreeDatabase::doCommit() {
  if (m_availableBlocks.empty() && m_pendingFree.empty() && m_uncommitted.empty())
    return;

  if (!m_availableBlocks.empty() || !m_pendingFree.empty()) {
    // First, read the existing head FreeIndexBlock, if it exists
    FreeIndexBlock indexBlock = FreeIndexBlock{InvalidBlockIndex, {}};
    if (m_headFreeIndexBlock != InvalidBlockIndex) {
      indexBlock = readFreeIndexBlock(m_headFreeIndexBlock);
      if (indexBlock.freeBlocks.size() >= maxFreeIndexLength()) {
        // If the existing head free index block is full, then we should start a
        // new one and leave it alone
        indexBlock.nextFreeBlock = m_headFreeIndexBlock;
        indexBlock.freeBlocks.clear();
      } else {
        // If we are copying an existing free index block, the old free index
        // block will be a newly freed block
        indexBlock.freeBlocks.append(m_headFreeIndexBlock);
      }
    }

    // Then, we need to write all the available blocks, which are safe to write
    // to, and the pending free blocks, which are NOT safe to write to, to the
    // FreeIndexBlock chain.
    while (true) {
      if (indexBlock.freeBlocks.size() < maxFreeIndexLength() && (!m_availableBlocks.empty() || !m_pendingFree.empty())) {
        // If we have room on our current FreeIndexblock, just add a block to
        // it.  Prioritize the pending free blocks, because we cannot use those
        // to write to.
        BlockIndex toAdd;
        if (m_pendingFree.empty())
          toAdd = m_availableBlocks.takeFirst();
        else
          toAdd = m_pendingFree.takeFirst();

        indexBlock.freeBlocks.append(toAdd);
      } else {
        // If our index block is full OR we are out of blocks to free, then
        // need to write a new head free index block.
        if (m_availableBlocks.empty())
          m_headFreeIndexBlock = makeEndBlock();
        else
          m_headFreeIndexBlock = m_availableBlocks.takeFirst();
        writeFreeIndexBlock(m_headFreeIndexBlock, indexBlock);

        // If we're out of blocks to free, then we're done
        if (m_availableBlocks.empty() && m_pendingFree.empty())
          break;

        indexBlock.nextFreeBlock = m_headFreeIndexBlock;
        indexBlock.freeBlocks.clear();
      }
    }
  }

  writeRoot();

  m_uncommitted.clear();
}

void BTreeDatabase::checkIfOpen(char const* methodName, bool shouldBeOpen) const {
  if (shouldBeOpen && !m_open)
    throw DBException::format("BTreeDatabase method '{}' called when not open, must be open.", methodName);
  else if (!shouldBeOpen && m_open)
    throw DBException::format("BTreeDatabase method '{}' called when open, cannot call when open.", methodName);
}

void BTreeDatabase::checkBlockIndex(size_t blockIndex) const {
  BlockIndex blockCount = (m_deviceSize - HeaderSize) / m_blockSize;
  if (blockIndex >= blockCount)
    throw DBException::format("blockIndex: {} out of block range", blockIndex);
}

void BTreeDatabase::checkKeySize(ByteArray const& k) const {
  if (k.size() != m_keySize)
    throw DBException::format("Wrong key size {}", k.size());
}

uint32_t BTreeDatabase::maxFreeIndexLength() const {
  return (m_blockSize - 2 - sizeof(BlockIndex) - 4) / sizeof(BlockIndex);
}

BTreeSha256Database::BTreeSha256Database() {
  setKeySize(32);
}

BTreeSha256Database::BTreeSha256Database(String const& contentIdentifier) {
  setKeySize(32);
  setContentIdentifier(contentIdentifier);
}

bool BTreeSha256Database::contains(ByteArray const& key) {
  return BTreeDatabase::contains(sha256(key));
}

Maybe<ByteArray> BTreeSha256Database::find(ByteArray const& key) {
  return BTreeDatabase::find(sha256(key));
}

bool BTreeSha256Database::insert(ByteArray const& key, ByteArray const& value) {
  return BTreeDatabase::insert(sha256(key), value);
}

bool BTreeSha256Database::remove(ByteArray const& key) {
  return BTreeDatabase::remove(sha256(key));
}

bool BTreeSha256Database::contains(String const& key) {
  return BTreeDatabase::contains(sha256(key));
}

Maybe<ByteArray> BTreeSha256Database::find(String const& key) {
  return BTreeDatabase::find(sha256(key));
}

bool BTreeSha256Database::insert(String const& key, ByteArray const& value) {
  return BTreeDatabase::insert(sha256(key), value);
}

bool BTreeSha256Database::remove(String const& key) {
  return BTreeDatabase::remove(sha256(key));
}

}
