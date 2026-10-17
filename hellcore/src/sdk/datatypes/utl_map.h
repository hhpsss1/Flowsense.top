#pragma once

#include <cstdint>
#include <cstddef>
#include <optional>
template <typename K, typename V>
class c_utl_map
{
public:
	struct node_t
	{
		int m_left;
		int m_right;
		int m_parent;
		int m_tag;
		K m_key;
		V m_value;
	};

	auto begin() const
	{
		return m_data;
	}

	auto end() const
	{
		return m_data + m_size;
	}

	std::optional<V> find_by_key(K key) const
	{
		int current = m_root;
		for(int visited=0; m_data && current>=0 && current<m_size && visited<m_size; ++visited)
		{
			const node_t& element = m_data[current];
			if (element.m_key < key)
				current = element.m_right;
			else if (element.m_key > key)
				current = element.m_left;
			else
				return element.m_value;
		}

		return {};
	}

	int m_size;
	int m_unknown;
	node_t* m_data;
	int m_root;
};