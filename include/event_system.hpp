#pragma once
#include <array>
#include <algorithm>
#include <functional>
#include <stdexcept>

#include "entt/core/fwd.hpp"
#include "entt/core/type_info.hpp"

namespace ev
{
	template<class Key, class Value, size_t Size>
	struct event_map {
		std::array<std::pair<Key, Value>, Size> data;
		size_t offset = 0;

		using value_type = Value;

		constexpr void insert(const Key& key, const Value& value)
		{
			if (offset >= Size) return;

			data._Elems[offset] = std::make_pair(key, value);
			offset++;
		}

		[[nodiscard]] constexpr Value at(const Key& key) const {
			const auto it = std::ranges::find_if(data, [key](const auto& some_it) constexpr {
				return some_it.first == key;
				});
			if (it != data.end()) {
				return it->second;
			}

			throw std::invalid_argument("Wrong key (or value exists)");
		}

		[[nodiscard]] constexpr std::optional<Value> optional_at(const Key& key) const noexcept {
			const auto it = std::ranges::find_if(data, [key](const auto& new_it) constexpr {
				return new_it.first == key;
				});
			if (it != data.end()) {
				return it->second;
			}

			return std::nullopt;
		}

		constexpr void clear() noexcept {
			data = decltype(data) {};
		}
	};

	template<class Value, size_t Size>
	struct event_array {
		std::array<Value, Size> data;

		constexpr void for_each(const std::function<void(Value& value)>& callback)
		{
			std::ranges::for_each(data, callback);
		}

		constexpr void for_each(const std::function<void(const Value& value)>& callback)
		{
			std::ranges::for_each(data, callback);
		}

		constexpr void clear() noexcept {
			data = decltype(data) {};
		}
	};

	struct event_mark {};
	struct just_event : event_mark {};
	struct cancelable_event : event_mark
	{
		bool canceled = false;
	};

	template <class... Events>
	struct registered_events
	{
		static constexpr auto size = sizeof...(Events);
		static constexpr std::array<entt::id_type, size> hashes{ entt::type_hash<Events>::value()... };

		static constexpr bool exists(const entt::id_type my_hash)
		{
			return std::ranges::all_of(hashes, [my_hash](const auto hash) constexpr
				{
					return my_hash == hash;
				});
		}
	};

	template <size_t N>
	using priority_type = ev::event_map<entt::id_type, unsigned char, N>;

	template <class Event, unsigned char Priority>
	consteval auto make_priority()
	{
		return std::make_pair(entt::type_hash<Event>::value(), Priority);
	}

	template <class RegisteredEvents>
	struct priority_traits
	{
		static_assert(!RegisteredEvents::hashes.empty(), "No events");

		static consteval auto get_zero_priority()
		{
			event_map<entt::id_type, unsigned char, RegisteredEvents::size> my_map{};
			for (const auto& hash : RegisteredEvents::hashes)
			{
				my_map.insert(hash, 0);
			}

			return my_map;
		}

		template <class Listener>
		static consteval auto get_or_make_priority()
		{
			if constexpr (requires()
			{
				Listener::priority;
			})
			{
				return Listener::priority;
			}
			return get_zero_priority();
		}

		template <auto Priority>
		static consteval auto zero_or_priority(const entt::id_type hash)
		{
			if (auto res = Priority.optional_at(hash); !res)
				return static_cast<typename std::decay_t<decltype(Priority)>::value_type>(0);
			return Priority.at(hash);
		}

		template <auto Priority>
		static consteval auto get_normalized_priority()
		{
			if (Priority.data.empty()) return get_zero_priority();
			event_map<entt::id_type, unsigned char, RegisteredEvents::size> my_map{};
			for (const auto& hash : RegisteredEvents::hashes)
			{
				my_map.insert(hash, zero_or_priority<Priority>(hash));
			}

			return my_map;
		}
	};

	struct listener_traits
	{
		static constexpr void call_event(auto& event, auto& listener)
		{
			if constexpr (requires() { listener->on_event(event); })
			{
				listener->on_event(event);
			}
			else if constexpr (requires() { listener.on_event(event); })
			{
				listener.on_event(event);
			}
		}
	};

	template <size_t N>
	constexpr auto event_index_sequence_for = std::make_index_sequence<N>();

	template <class RegisteredEvents, auto Priority, size_t Pos>
	consteval void set_table(auto& table)
	{
		for (const auto& priority : priority_traits<RegisteredEvents>::template get_normalized_priority<Priority>().data)
		{
			table.insert(Pos, std::make_pair(priority.first, priority.second));
		}
	}

	template <class RegisteredEvents, auto... Priorities, size_t... Indexes>
	consteval void set_table_with_indices(auto& table, std::index_sequence<Indexes...>)
	{
		(set_table<RegisteredEvents, Priorities, Indexes>(table), ...);
	}

	template <class RegisteredEvents, auto... Priorities>
	consteval auto create_sorted_table()
	{
		event_map<size_t, std::pair<entt::id_type, unsigned char>, sizeof...(Priorities)* RegisteredEvents::size> table{};
		set_table_with_indices<RegisteredEvents, Priorities...>(table, event_index_sequence_for<sizeof...(Priorities)>);

		std::ranges::sort(table.data, [](const auto& a, const auto& b)
			{
				return a.second.second > b.second.second;
			});

		return table.data;
	}

	template <class RegisteredEvents, class... Listeners>
	consteval auto make_static_table()
	{
		return create_sorted_table<RegisteredEvents, priority_traits<RegisteredEvents>::template get_or_make_priority<Listeners>()...>();
	}

	template <class Event, class... Args>
	constexpr auto fire_emplace_event(auto& listeners, const auto& table, Args&&... args)
	{
		Event event{ std::forward<Args>(args)... };

		std::ranges::for_each(table, [&event, &listeners](const auto& mod) constexpr
			{
				if (mod.second.first == entt::type_hash<Event>::value())
				{
					listener_traits::call_event(event, listeners[mod.first]);
				}
			});
	}

	template <class Event>
	constexpr auto fire_event(auto& listeners, const auto& table, Event& event)
	{
		std::ranges::for_each(table, [&event, &listeners](const auto& mod) constexpr
			{
				if (mod.second.first == entt::type_hash<Event>::value())
				{
					listener_traits::call_event(event, listeners[mod.first]);
				}
			});
	}
}