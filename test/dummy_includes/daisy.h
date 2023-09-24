#pragma once

namespace daisy {
struct QspiHandle {};

template <typename T> struct PersistentStorage {
  PersistentStorage(QspiHandle handle);

  void Save() {}

  T GetSettings() {}
};
} // namespace daisy