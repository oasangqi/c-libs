//============================================================================
// Name        : Buffer.h
// Author      : kdjie
// Version     : 1.0
// Copyright   : @2015
// Description : 14166097@qq.com
//============================================================================

#pragma once

#include <stdint.h>

namespace evwork
{

	class CBuffer
	{
	public:
		CBuffer(uint32_t _blocksize, uint32_t _minblock)
			: BlockSize(_blocksize)
			, MinBlock(_minblock)
			, m_data(NULL)
		{
			__reset();
		}

		~CBuffer()
		{
			__reset();
		}

		// 获取数据
		const char* data() const
		{
			if (m_data == NULL)
				return NULL;

			return m_data + m_head_off;
		}

		// 获取数据大小
		uint32_t size() const
		{
			return m_tail_off - m_head_off;
		}
		
		// 追加数据
		void append(const char* _data, uint32_t _size)
		{
			if (_size == 0)
				return;

			__try_increment(_size);

			memcpy(m_data + m_tail_off, _data, _size);
			m_tail_off += _size;
		}

		// 擦除部分数据
		void erase(uint32_t _size)
		{
			if (_size == 0)
				return;

			uint32_t datasize = (m_tail_off - m_head_off);

			if (_size > datasize)
				_size = datasize;

			m_head_off += _size;

			// 擦除数据累计大于1个块，则进行缩小调整
			if (m_head_off >= BlockSize)
			{
				// __try_decrement();

				memmove(m_data, m_data + m_head_off + _size, datasize - _size);
				m_head_off = 0;
				m_tail_off = datasize - _size;
			}
		}

		void reset()
		{
			__reset();
		}

		// 扩容
		void inc_capacity(uint32_t _size)
		{
			__try_increment(_size);
		}

		// 获取空闲大小
		uint32_t freesize()
		{
			return m_capacity - m_tail_off;
		}

		// 数据尾
		char* tail()
		{
			if (m_data == NULL)
				return NULL;

			return m_data + m_tail_off;
		}

		// 更新数据大小(追加数据之后必须执行该操作)
		void inc_size(uint32_t _size)
		{
			m_tail_off += _size;

			if (m_tail_off > m_capacity)
				m_tail_off = m_capacity;
		}

		CBuffer & operator = (const CBuffer& _obj)
		{
			__reset();

			append(_obj.data(), _obj.size());
			return *this;
		}

	private:

		void __reset()
		{
			if (m_data)
			{
				delete[] m_data;
			}

			m_data = NULL;
			m_capacity = 0;
			m_tail_off = m_head_off = 0;
		}

		void __try_increment(uint32_t _size)
		{
			uint32_t freesize = (m_capacity - m_tail_off);
			if (freesize >= _size)
				return;

			uint32_t datasize = (m_tail_off - m_head_off);
			uint32_t require_size = datasize + _size;
			if (m_capacity >= require_size)
			{
				memcpy(m_data, m_data + m_head_off, datasize);
				m_head_off = 0;
				m_tail_off = datasize;
				return;
			}

			uint32_t capacity = __calc_capacity(require_size);

			char* pNew = new char[capacity];

			if (m_data)
			{
				memcpy(pNew, m_data + m_head_off, datasize);
				delete[] m_data;
			}

			m_data = pNew;
			m_capacity = capacity;
			m_head_off = 0;
			m_tail_off = datasize;
		}

		void __try_decrement()
		{
			uint32_t datasize = (m_tail_off - m_head_off);

			uint32_t capacity = __calc_capacity(datasize);

			if (capacity < m_capacity)
			{
				if (capacity == 0)
				{
					__reset();
				}
				else
				{
					char* pNew = new char[capacity];

					memcpy(pNew, m_data + m_head_off, datasize);
					delete[] m_data;

					m_data = pNew;
					m_capacity = capacity;
					m_head_off = 0;
					m_tail_off = datasize;
				}
			}
		}

		// 分配向上取整个块大小
		uint32_t __calc_capacity(uint32_t _size)
		{
			uint32_t block_count = _size / BlockSize;
			if (_size % BlockSize > 0)
				++block_count;

			if (block_count < MinBlock)
				block_count = MinBlock;

			return BlockSize * block_count;
		}

	private:
		uint32_t BlockSize; // 单个块大小
		uint32_t MinBlock; // 缩小时最小保留块

		char* m_data; // 内存位置
		uint32_t m_capacity; // 容量

		uint32_t m_head_off; // 数据头偏离内存头的位置
		uint32_t m_tail_off; // 数据尾偏离内存头的位置
	};

}
