#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <miniz/miniz.h>

template <typename T>
class SparseVector
{
	static_assert(std::is_trivial<T>::value && std::is_standard_layout<T>::value,
				  "SparseVector only supports trivial types like uint8_t or uint16_t");

protected:
	std::vector<std::vector<T>> index;
	std::unordered_map<uint32_t, std::vector<T>> data;
	std::unordered_map<uint32_t, std::vector<uint8_t>> compressedData;
	std::vector<T> noData;
	bool use_index = false;
	bool use_compression = false;
	mz_ulong blockSize = 0;

public:
	SparseVector(T noDataSignature) { noData.resize(1, noDataSignature); }

	SparseVector(T noDataSignature, bool c) : use_compression(c)
	{
		noData.resize(1, noDataSignature);
	}

	T *operator[](const uint32_t elementId)
	{
		if (use_index)
		{
			if (elementId >= index.size())
				return noData.data();
			return index[elementId].data();
		}
		else if (use_compression)
		{
			auto it = compressedData.find(elementId);
			if (it == compressedData.end())
				return noData.data();

			mz_ulong dstSize = blockSize;
			uint8_t *tmp = static_cast<uint8_t *>(malloc(dstSize));
			if (!tmp)
				return noData.data();

			if (MZ_OK != mz_uncompress(tmp, &dstSize,
									   it->second.data(),
									   static_cast<mz_ulong>(it->second.size())))
			{
				free(tmp);
				return noData.data();
			}

			data[0].resize(dstSize / sizeof(T));
			memcpy(data[0].data(), tmp, dstSize);
			free(tmp);
			return data[0].data();
		}
		else
		{
			auto it = data.find(elementId);
			if (it == data.end())
				return noData.data();
			return it->second.data();
		}
	}

	bool hasData(uint32_t elementId) const
	{
		return elementId < index.size() && !index[elementId].empty() && index[elementId][0] != noData[0];
	}

	template <typename U = T>
	void set(uint32_t elementId, const T *values, size_t elementSize, SparseVector<U> *parentIndex = nullptr)
	{
		if (elementSize == 1 && parentIndex == nullptr)
		{
			use_index = true;
		}
		else if (elementSize >= 128)
		{
			//use_compression = true;
		}

		blockSize = elementSize * sizeof(T);

		if (use_index)
		{
			index.resize(std::max<size_t>(index.size(), elementId + 1));
			index[elementId].assign(values, values + elementSize);
		}
		else if (parentIndex == nullptr || parentIndex->hasData(elementId))
		{
			if (memcmp(values, noData.data(), blockSize) != 0)
			{
				if (use_compression)
				{
					mz_ulong maxSize = mz_compressBound(blockSize);
					mz_ulong actualSize = maxSize;
					std::vector<uint8_t> compressed(maxSize);
					if (MZ_OK != mz_compress(compressed.data(), &actualSize,
											 reinterpret_cast<const uint8_t *>(values), blockSize))
					{
						exit(1);
					}
					compressed.resize(actualSize);
					compressedData[elementId] = std::move(compressed);
				}
				else
				{
					data[elementId].assign(values, values + elementSize);
				}
			}
		}
	}

	template <typename U = T>
	void my_fread(size_t elementSize, uint32_t numElements, FILE *stream, SparseVector<U> *parentIndex = nullptr)
	{
		blockSize = elementSize * sizeof(T);
		std::vector<T> tmp(elementSize);

		for (uint32_t i = 0; i < numElements; ++i)
		{
			if (fread(tmp.data(), blockSize, 1, stream) != 1)
			{
				fprintf(stderr, "File read error\n");
				exit(1);
			}
			set(i, tmp.data(), elementSize, parentIndex);
		}
	}

	void clear()
	{
		index.clear();
		data.clear();
		compressedData.clear();
		noData.clear();
	}
};
