/** @file MutexWrapper.hh
 *
 * This defines a simple mechanism to wrap a pointer in a shared-pointer-like
 * structure that incorporates a mutex lock
 * */

#ifndef MUTEX_WRAPPER_h
#define MUTEX_WRAPPER_h

// #include "G4Threading.hh"
// #include "G4AutoLock.hh"
#include <thread>

template <typename T>
class MutexWrapper
{
public:
#ifdef G4MULTITHREADED
  MutexWrapper(T *obj, std::recursive_mutex &mutex) : _obj(obj), _lock(mutex) {}
  // define the move constructor
  MutexWrapper(MutexWrapper<T> &&right) : _obj(right._obj), _lock(std::move(right._lock)) {}
#else
  MutexWrapper(T *obj) : _obj(obj) {}
  MutexWrapper(MutexWrapper<T> &&right) : _obj(right._obj) {}
#endif

  MutexWrapper(const MutexWrapper<T> &right) = delete;
  MutexWrapper &operator=(const MutexWrapper<T> &right) = delete;

  // access to the underlying pointer
  T *get() { return _obj; }

  T *operator->() { return _obj; }
  const T *operator->() const { return _obj; }

  operator T *() { return _obj; }

#ifdef G4MULTITHREADED
  // lock/unlock the mutex for long operations
  inline void lock() { _lock.lock(); }
  inline void unlock() { _lock.unlock(); }
#endif

private:
  T *_obj;

#ifdef G4MULTITHREADED
  std::unique_lock<std::recursive_mutex> _lock;
#endif
};

/// wrapper to make a class a protected singleton
template <class T>
MutexWrapper<T> GetWrappedSingleton()
{
  static T _obj;
#ifdef G4MULTITHREADED
  static std::recursive_mutex _mutex;
  // std::cerr<<"Mutex at "<<&_mutex<<std::endl;
  return MutexWrapper<T>(&_obj, _mutex);
#else
  return MutexWrapper<T>(&_obj);
#endif
}

#endif
