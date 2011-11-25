#pragma once
#include "../geometry/rect2d.hpp"

#include "../base/mutex.hpp"

#include "../std/deque.hpp"
#include "../std/string.hpp"
#include "../std/utility.hpp"
#include "../std/vector.hpp"


// Information about stored mwm.
class MwmInfo
{
public:
  m2::RectD m_limitRect;    // Limit rect of mwm.
  uint8_t m_minScale;       // Min zoom level of mwm.
  uint8_t m_maxScale;       // Max zoom level of mwm.

  // Does this MwmInfo represent a valid Mwm?
  bool isValid() const { return m_status == STATUS_ACTIVE; }
private:
  friend class MwmSet;

  enum Status { STATUS_ACTIVE = 0, STATUS_TO_REMOVE = 1, STATUS_REMOVED = 2 };
  uint8_t m_lockCount;      // Number of locks.
  uint8_t m_status;         //
};

class MwmSet
{
public:
  typedef size_t MwmId;

  explicit MwmSet(size_t cacheSize = 5);
  virtual ~MwmSet() = 0;

  class MwmValueBase
  {
  public:
    virtual ~MwmValueBase() {}
  };

  // Mwm lock, which is used to lock mwm when its FileContainer is used.
  class MwmLock
  {
  public:
    MwmLock(MwmSet const & mwmSet, MwmId mwmId);
    ~MwmLock();

    inline MwmValueBase * GetValue() const { return m_pValue; }
    string GetCountryName() const;

  private:
    MwmSet const & m_mwmSet;
    MwmId m_id;
    MwmValueBase * m_pValue;
  };

  /// Add new mwm. Returns false, if mwm with given fileName already exists.
  /// @param[in]  fileName  File name (without full path) of country.
  /// @param[out] r         Limit rect of country.
  /// @return Map format version or -1 if not added
  int Add(string const & fileName, m2::RectD & r);
  inline bool Add(string const & fileName)
  {
    m2::RectD dummy;
    return (-1 != Add(fileName, dummy));
  }

  // Remove mwm.
  void Remove(string const & fileName);

  // Get ids of all mwms. Some of them may be marked to remove.
  void GetMwmInfo(vector<MwmInfo> & info) const;

  // Clear caches.
  void ClearCache();

protected:
  /// @return mwm format version
  virtual int GetInfo(string const & name, MwmInfo & info) const = 0;
  virtual MwmValueBase * CreateValue(string const & name) const = 0;

  void Cleanup();

private:
  static const MwmId INVALID_MWM_ID = static_cast<MwmId>(-1);

  typedef deque<pair<MwmId, MwmValueBase *> > CacheType;

  // Update given MwmInfo.
  inline static void UpdateMwmInfo(MwmInfo & info);

  MwmValueBase * LockValue(MwmId id) const;
  void UnlockValue(MwmId id, MwmValueBase * p) const;

  // Find first removed mwm or add a new one.
  MwmId GetFreeId();

  // Find mwm with a given name.
  MwmId GetIdByName(string const & name);

  // Do the cleaning for [beg, end) without acquiring the mutex.
  void ClearCacheImpl(CacheType::iterator beg, CacheType::iterator end);

  mutable vector<MwmInfo> m_info;
  mutable vector<string> m_name;
  mutable CacheType m_cache;
  size_t m_cacheSize;
  mutable threads::Mutex m_lock;
};
