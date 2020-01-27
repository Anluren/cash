#pragma once

#include "ioport.h"
#include "streams.h"

namespace ch {
namespace internal {

class deviceimpl;
class context;

class device_base {
public:

  device_base();

  virtual ~device_base();

  device_base(const device_base& other);

  device_base(device_base&& other);

  device_base& operator=(const device_base& other);

  device_base& operator=(device_base&& other);

  auto impl() const {
    return impl_;
  }

  std::string name() const;

protected:

  device_base(const std::type_index& signature, bool is_pod, const std::string& name);

  template <typename T, typename... Args>
  auto load(const source_info& srcinfo, Args&&... args) {
    auto is_dup = this->begin();
    auto obj = new T(std::forward<Args>(args)...);
    if (!is_dup) {
      this->begin_build();
      obj->describe();
      ch_cout.flush();
      this->end_build();
    }
    this->end(srcinfo);
    return obj;
  }

  bool begin();

  void begin_build();

  void end_build();

  void end(const source_info& srcinfo);

  deviceimpl* impl_;

  template <typename T> friend class io_loader;
};

///////////////////////////////////////////////////////////////////////////////

template<typename T>
using detect_describe_t = decltype(std::declval<T&>().describe());

template <typename T, typename... Args>
inline constexpr bool is_pod_module_v = (0 == sizeof...(Args))
                                     && (sizeof(T) == sizeof(decltype(T::io)));

///////////////////////////////////////////////////////////////////////////////

template <typename T = void>
class ch_device final : public device_base {
protected:

  ch_device& operator=(const ch_device& other) = delete;

  ch_device& operator=(ch_device&& other) = delete;

  std::shared_ptr<T> obj_;

public:

  static_assert(is_logic_io_v<decltype(T::io)>, "missing io port");
  static_assert(is_detected_v<detect_describe_t, T>, "missing describe() method");
  using base = device_base;
  using value_type = T;
  using io_type = ch_flip_io<ch_system_io<decltype(T::io)>>;

  io_type io;

  template <typename... Args,
            CH_REQUIRE(std::is_constructible_v<T, Args...>)>
  ch_device(Args&&... args)
    : base(std::type_index(typeid(T)), is_pod_module_v<T, Args...>, idname<T>(true))
    , obj_(this->load<T>(source_info(), std::forward<Args>(args)...))
    , io(obj_->io)
  {}

  template <typename... Args,
            CH_REQUIRE(std::is_constructible_v<T, Args...>)>
  ch_device(const std::string& name, Args&&... args)
    : base(std::type_index(typeid(T)), is_pod_module_v<T, Args...>, name)
    , obj_(this->load<T>(source_info(), std::forward<Args>(args)...))
    , io(obj_->io)
  {}

  ch_device(const ch_device& other) 
    : base(other)
    , obj_(other.obj_)
    , io(other.io)
  {}

  ch_device(ch_device&& other)
    : base(std::move(other))
    , obj_(std::move(other.obj_))
    , io(std::move(other.io))
  {}
};

///////////////////////////////////////////////////////////////////////////////

void ch_stats(std::ostream& out, const device_base& device);

}
}
