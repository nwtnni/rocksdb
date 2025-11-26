//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
//
#include "arctic.h"

#include <algorithm>
#include <memory>
#include <set>
#include <type_traits>
#include <unordered_set>

#include "db/memtable.h"
#include "memory/arena.h"
#include "memtable/stl_wrappers.h"
#include "port/port.h"
#include "rocksdb/memtablerep.h"
#include "rocksdb/utilities/options_type.h"
#include "util/mutexlock.h"

namespace ROCKSDB_NAMESPACE {
namespace {

class Map {
  void* map;

 public:
  Map() : map(arctic_new()) {}
  ~Map() { arctic_destroy(this->map); }
  void* Ref() { return arctic_ref(this->map); }
};

class ArcticRep : public MemTableRep {
 public:
  ArcticRep(Allocator* allocator);

  // Insert key into the collection. (The caller will pack key and value into a
  // single buffer and pass that in as the parameter to Insert)
  // REQUIRES: nothing that compares equal to key is currently in the
  // collection.
  void Insert(KeyHandle handle) override;

  void InsertConcurrently(KeyHandle handle) override;

  // Returns true iff an entry that compares equal to key is in the collection.
  bool Contains(const char* key) const override;

  void MarkReadOnly() override;

  size_t ApproximateMemoryUsage() override;

  void Get(const LookupKey& k, void* callback_args,
           bool (*callback_func)(void* arg, const char* entry)) override;

  void BatchPostProcess() override;

  ~ArcticRep() {}

  class Iterator : public MemTableRep::Iterator {
    void* iter;

   public:
    explicit Iterator(void* iter_);

    // Initialize an iterator over the specified collection.
    // The returned iterator is not valid.
    // explicit Iterator(const MemTableRep* collection);

    ~Iterator() { arctic_iter_destroy(this->iter); };

    // Returns true iff the iterator is positioned at a valid node.
    bool Valid() const override;

    // Returns the key at the current position.
    // REQUIRES: Valid()
    const char* key() const override;

    // Advances to the next position.
    // REQUIRES: Valid()
    void Next() override;

    // Advances to the previous position.
    // REQUIRES: Valid()
    void Prev() override;

    // Advance to the first entry with a key >= target
    void Seek(const Slice& user_key, const char* memtable_key) override;

    // Seek and do some memory validation
    Status SeekAndValidate(const Slice& internal_key, const char* memtable_key,
                           bool allow_data_in_errors,
                           bool detect_key_out_of_order,
                           const std::function<Status(const char*, bool)>&
                               key_validation_callback) override;

    // Advance to the first entry with a key <= target
    void SeekForPrev(const Slice& user_key, const char* memtable_key) override;

    // Position at the first entry in collection.
    // Final state of iterator is Valid() iff collection is not empty.
    void SeekToFirst() override;

    // Position at the last entry in collection.
    // Final state of iterator is Valid() iff collection is not empty.
    void SeekToLast() override;
  };

  // Return an iterator over the keys in this representation.
  MemTableRep::Iterator* GetIterator(Arena* arena) override;

 private:
  friend class Iterator;
  Map map;
  ThreadLocalPtr ref;

  inline void* Ref() {
    void* ref_ = this->ref.Get();
    if (ref_ == nullptr) {
      ref_ = this->map.Ref();
      this->ref.Reset(ref_);
    }
    return ref_;
  }

  static void DeleteRef(void* ptr) { arctic_ref_destroy(ptr); }
};

void ArcticRep::Insert(KeyHandle handle) {
  auto key = GetLengthPrefixedSlice((const char*)handle);
  arctic_insert(Ref(), key.data(), key.size(), handle);
}

void ArcticRep::InsertConcurrently(KeyHandle handle) {
  ArcticRep::Insert(handle);
}

// Returns true iff an entry that compares equal to key is in the collection.
bool ArcticRep::Contains(const char* key) const {
  assert(false);
  return false;
}

void ArcticRep::MarkReadOnly() {}

size_t ArcticRep::ApproximateMemoryUsage() { return 0; }

void ArcticRep::BatchPostProcess() {}

ArcticRep::ArcticRep(Allocator* allocator)
    : MemTableRep(allocator), map(), ref(ArcticRep::DeleteRef) {}

ArcticRep::Iterator::Iterator(void* iter_) : iter(iter_) {}

// Returns true iff the iterator is positioned at a valid node.
bool ArcticRep::Iterator::Valid() const {
  return arctic_iter_valid(this->iter);
}

// Returns the key at the current position.
// REQUIRES: Valid()
const char* ArcticRep::Iterator::key() const {
  return static_cast<char*>(arctic_iter_key(this->iter));
}

// Advances to the next position.
// REQUIRES: Valid()
void ArcticRep::Iterator::Next() { arctic_iter_next(this->iter); }

// Advances to the previous position.
// REQUIRES: Valid()
void ArcticRep::Iterator::Prev() { assert(false); }

// Advance to the first entry with a key >= target
void ArcticRep::Iterator::Seek(const Slice& user_key,
                               const char* memtable_key) {
  assert(false);
}

Status ArcticRep::Iterator::SeekAndValidate(
    const Slice& /* internal_key */, const char* /* memtable_key */,
    bool /* allow_data_in_errors */, bool /* detect_key_out_of_order */,
    const std::function<Status(const char*, bool)>&
    /* key_validation_callback */) {
  assert(false);
  return Status::NotSupported("SeekAndValidate() not implemented");
}

// Advance to the first entry with a key <= target
void ArcticRep::Iterator::SeekForPrev(const Slice& /*user_key*/,
                                      const char* /*memtable_key*/) {
  assert(false);
}

// Position at the first entry in collection.
// Final state of iterator is Valid() iff collection is not empty.
void ArcticRep::Iterator::SeekToFirst() {}

// Position at the last entry in collection.
// Final state of iterator is Valid() iff collection is not empty.
void ArcticRep::Iterator::SeekToLast() { assert(false); }

void ArcticRep::Get(const LookupKey& k, void* callback_args,
                    bool (*callback_func)(void* arg, const char* entry)) {
  assert(false);
}

MemTableRep::Iterator* ArcticRep::GetIterator(Arena* arena) {
  char* mem = nullptr;
  if (arena != nullptr) {
    mem = arena->AllocateAligned(sizeof(Iterator));
  }
  return new (mem) ArcticRep::Iterator(arctic_iter(Ref()));
}
}  // namespace

ArcticRepFactory::ArcticRepFactory() {}

MemTableRep* ArcticRepFactory::CreateMemTableRep(
    const MemTableRep::KeyComparator& compare, Allocator* allocator,
    const SliceTransform*, Logger* /*logger*/) {
  return new ArcticRep(allocator);
}
}  // namespace ROCKSDB_NAMESPACE
