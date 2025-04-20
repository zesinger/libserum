#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include <unordered_map>
#include <vector>

template<typename T>
class SparseVector
{
  static_assert(std::is_trivial<T>::value && std::is_standard_layout<T>::value,
                "SparseVector only supports trivial types like uint8_t or uint16_t");

 protected:
  std::unordered_map<uint32_t, std::vector<T>> data;
  std::vector<T> noData;

 public:
  SparseVector(T noDataSignature) {
	noData.resize(1, noDataSignature);
  }

  // Access data for a frame (returns pointer to noData if not found)
  T* operator[](const uint32_t frame) {
    auto it = data.find(frame);
    if (it != data.end()) return it->second.data();
    return noData.data();
  }

  // Load data (only store if not matching the noData signature)
  void my_fread(size_t elementCount, uint32_t nframes, FILE* stream) {
    size_t blockSize = elementCount * sizeof(T);

    if (noData.size() < elementCount) noData.resize(elementCount, noData[0]);

	std::vector<T> tmp(elementCount);

    for (uint32_t i = 0; i < nframes; ++i) {
      if (1 != fread(tmp.data(), blockSize, 1, stream)) {
		// Error reading file
		exit(1);
	  }

      if (memcmp(tmp.data(), noData.data(), noData.size() * sizeof(T)) != 0)
        data[i] = tmp;
    }
  }

  void set(uint32_t frame, const T* values, size_t elementCount) {
	auto it = data.find(frame);
	if (it != data.end()) {
	  it->second.resize(elementCount);
	  memcpy(it->second.data(), values, elementCount * sizeof(T));
	} else {
	  std::vector<T> tmp(values, values + elementCount);
	  data[frame] = tmp;
	}
  }

  // Clear all stored frames
  void clear() {
    data.clear();
  }
};
