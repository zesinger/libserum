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
	std::unordered_map<uint32_t, std::vector<T>> data;
	std::vector<T> noData;
	std::vector<T> decompressedData;
	bool compression = false;

public:
	SparseVector(T noDataSignature) { noData.resize(1, noDataSignature); }

	SparseVector(T noDataSignature, bool c) : compression(c)
	{
		noData.resize(1, noDataSignature);
	}

	// Access data for a frame (decompress if necessary)
	T *operator[](const uint32_t frame)
	{
		auto it = data.find(frame);
		if (it == data.end())
			return noData.data();

		if (compression)
		{
			mz_ulong dstLen = static_cast<mz_ulong>(noData.size() * sizeof(T));
			uint8_t *tmp = (uint8_t *)malloc(sizeof(T) * noData.size());
			if (MZ_OK != mz_uncompress(tmp, &dstLen,
				reinterpret_cast<const uint8_t *>(it->second.data()),
				static_cast<mz_ulong>(it->second.size() * sizeof(T))))
			{
				free(tmp);
				return noData.data();
			}

			decompressedData.resize(dstLen / sizeof(T));
			memcpy(decompressedData.data(), tmp, dstLen);
			free(tmp);
			return decompressedData.data();		}

		return it->second.data();
	}

	bool hasData(uint32_t frame) const
	{
		auto it = data.find(frame);
		return (it != data.end());
	}

	template <typename U = T>
	void my_fread(size_t elementCount, uint32_t nframes, FILE *stream, SparseVector<U> *parent = nullptr)
	{
		size_t blockSize = elementCount * sizeof(T);

		if (noData.size() < elementCount)
		{
			noData.resize(elementCount, noData[0]);
			decompressedData.resize(elementCount, noData[0]);
		}

		std::vector<T> tmp(elementCount);

		for (uint32_t i = 0; i < nframes; ++i)
		{
			if (1 != fread(tmp.data(), blockSize, 1, stream))
			{
				fprintf(stderr, "File read error\n");
				exit(1);
			}

			if (parent == nullptr || parent->hasData(i))
			{
				if (memcmp(tmp.data(), noData.data(), blockSize) != 0)
				{
					if (compression)
					{
						mz_ulong maxLen = mz_compressBound(static_cast<mz_ulong>(blockSize));
						mz_ulong actualSize = maxLen;
						std::vector<T> compressed;
						compressed.resize((maxLen + sizeof(T) - 1) / sizeof(T)); // size in T units
						if (MZ_OK != mz_compress(reinterpret_cast<uint8_t *>(compressed.data()), &actualSize,
												 reinterpret_cast<const uint8_t *>(tmp.data()), static_cast<mz_ulong>(blockSize)))
						{
							exit(1);
						}
						compressed.resize((actualSize + sizeof(T) - 1) / sizeof(T)); // shrink to actual compressed size
						data[i] = std::move(compressed);
					}
					else
					{
						data[i] = tmp;
					}
				}
			}
		}
	}

	// Set frame directly
	void set(uint32_t frame, const T *values, size_t elementCount)
	{
		size_t blockSize = elementCount * sizeof(T);

		if (compression)
		{
			mz_ulong maxLen = mz_compressBound(static_cast<mz_ulong>(blockSize));
			std::vector<T> compressed((maxLen + sizeof(T) - 1) / sizeof(T));
			mz_ulong actualSize = maxLen;

			if (MZ_OK != mz_compress(reinterpret_cast<uint8_t *>(compressed.data()), &actualSize,
									 reinterpret_cast<const uint8_t *>(values), static_cast<mz_ulong>(blockSize)))
			{
				exit(1);
			}

			compressed.resize((actualSize + sizeof(T) - 1) / sizeof(T));

			auto it = data.find(frame);
			if (it != data.end())
			{
				it->second = std::move(compressed);
			}
			else
			{
				data[frame] = std::move(compressed);
			}
		}
		else
		{
			auto it = data.find(frame);
			if (it != data.end())
			{
				it->second.resize(elementCount);
				memcpy(it->second.data(), values, blockSize);
			}
			else
			{
				std::vector<T> tmp(values, values + elementCount);
				data[frame] = std::move(tmp);
			}
		}
	}

	void clear()
	{
		data.clear();
		noData.clear();
		decompressedData.clear();
	}
};