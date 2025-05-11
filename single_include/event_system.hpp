#pragma once
#include <array>
#include <algorithm>
#include <functional>
#include <stdexcept>

#include "entt/core/fwd.hpp"
#include "entt/core/type_info.hpp"

namespace ev
{
	template <class T>
	concept is_container = requires(T c)
	{
		typename T::value_type;
		typename T::iterator;
		c.begin();
		c.end();
		c.size();
	};

	template<class Key, class Value, size_t Size>
	struct event_map {
		std::array<std::pair<Key, Value>, Size> data;
		size_t offset = 0;

		using value_type = Value;

		constexpr void set_offset(const size_t new_offset) noexcept
		{
			this->offset = new_offset;
		}

		constexpr void insert(const Key& key, const Value& value)
		{
			if (offset >= Size) return;

			data[offset] = std::make_pair(key, value);
			offset++;
		}

		[[nodiscard]] constexpr Value at(const Key& key) const {
			const auto it = std::ranges::find_if(data, [key](const auto& some_it) constexpr {
				return some_it.first == key;
				});
			if (it != data.end()) {
				return it->second;
			}

			throw std::invalid_argument("Wrong key is given");
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

		constexpr void for_each(auto callback)
		{
			std::ranges::for_each(data, callback);
		}

		constexpr void clear() noexcept {
			data = decltype(data) {};
		}
	};

	template <class T>
	concept is_event_map = requires(T c)
	{
		typename T::value_type;
		(&T::insert);
		(&T::at);
		(&T::optional_at);
		(&T::clear);
		requires is_container<std::decay_t<decltype(c.data)>>;
		{ c.offset } -> std::same_as<size_t&>;
	};

	template <class T>
	concept is_event_array = requires(T c)
	{
		(&T::for_each);
		(&T::clear);
		requires is_container<std::decay_t<decltype(c.data)>>;
	};

	
	struct event_mark {};
	struct cancelable_event
	{
		bool canceled = false;
	};
	struct no_copying
	{
		no_copying() = default;
		~no_copying() = default;

		no_copying(const no_copying&) = delete;
		no_copying& operator=(const no_copying&) = delete;
		no_copying(no_copying&&) = delete;
		no_copying& operator=(no_copying&&) = delete;
	};

	template <class T>
	concept is_event = std::derived_from<std::decay_t<T>, event_mark>;

	template <is_event... Events>
	struct registered_events
	{
		static constexpr auto size = sizeof...(Events);
		static constexpr std::array<entt::id_type, size> hashes{ entt::type_hash<Events>::value()... };

		static constexpr bool exists(const entt::id_type my_hash) noexcept
		{
			return std::ranges::any_of(hashes, [my_hash](const auto hash) constexpr
				{
					return my_hash == hash;
				});
		}

		static_assert(size > 0, "Registered events cannot be empty");
	};

	template <class T>
	concept is_registered_events = requires()
	{
		requires is_container<std::decay_t<decltype(T::hashes)>>;
		(&T::exists);
		{ T::size } -> std::same_as<const std::size_t&>;
	};

	template <size_t N>
	using priority_type = event_map<entt::id_type, unsigned char, N>;

	template <is_registered_events RegisteredEvents, is_container auto Table>
	struct table_t
	{
		static constexpr auto table = Table;

		[[nodiscard]] static constexpr bool event_exists(const entt::id_type hash) noexcept
		{
			return RegisteredEvents::exists(hash);
		}
	};
	
	template <class T>
	concept is_table = requires()
	{
		requires is_container<std::decay_t<decltype(T::table)>>;
		(&T::event_exists);
	};

	template <is_event Event, unsigned char Priority>
	consteval auto make_priority() noexcept
	{
		return std::make_pair(entt::type_hash<Event>::value(), Priority);
	}

	template <is_registered_events RegisteredEvents>
	struct priority_traits
	{
		static consteval auto get_zero_priority() noexcept
		{
			event_map<entt::id_type, unsigned char, RegisteredEvents::size> my_map{};
			for (const auto& hash : RegisteredEvents::hashes)
			{
				my_map.insert(hash, 0);
			}

			return my_map;
		}

		template <class Listener>
		static consteval auto get_or_make_priority() noexcept
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

		template <is_event_map auto Priority>
		static consteval auto zero_or_priority(const entt::id_type hash)
		{
			if (auto res = Priority.optional_at(hash); !res)
				return static_cast<typename decltype(Priority)::value_type>(0);
			return Priority.at(hash);
		}

		template <is_event_map auto Priority>
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

	template <is_registered_events RegisteredEvents, is_event_map auto Priority, size_t Pos>
	consteval void set_table(is_event_map auto& table)
	{
		for (const auto& priority : priority_traits<RegisteredEvents>::template get_normalized_priority<Priority>().data)
		{
			table.insert(Pos, std::make_pair(priority.first, priority.second));
		}
	}

	template <is_registered_events RegisteredEvents, is_event_map auto... Priorities, size_t... Indexes>
	consteval void set_table_with_indices(is_event_map auto& table, std::index_sequence<Indexes...>)
	{
		(set_table<RegisteredEvents, Priorities, Indexes>(table), ...);
	}

	template <is_registered_events RegisteredEvents, is_event_map auto... Priorities>
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

	template <is_registered_events RegisteredEvents, class... Listeners>
	consteval auto make_static_table_t()
	{
		return table_t<RegisteredEvents, create_sorted_table<RegisteredEvents, priority_traits<RegisteredEvents>::template get_or_make_priority<Listeners>()...>()>{};
	}

	template <is_registered_events RegisteredEvents, class... Listeners>
	constexpr auto make_static_table = make_static_table_t<RegisteredEvents, Listeners...>();

	template <is_event Event, is_table auto Table, class... Args> requires requires()
	{
		requires std::constructible_from<Event, Args...>;
		requires Table.event_exists(entt::type_hash<Event>::value());
	}
	constexpr auto fire_emplace_event(is_container auto& listeners, Args&&... args)
	{
		Event event{ std::forward<Args>(args)... };

		std::ranges::for_each(Table.table, [&event, &listeners](const auto& mod) constexpr
			{
				if (mod.second.first == entt::type_hash<Event>::value())
				{
					listener_traits::call_event(event, listeners[mod.first]);
				}
			});
	}

	template <is_event Event, is_table auto Table> requires requires()
	{
		requires Table.event_exists(entt::type_hash<Event>::value());
	}
	constexpr auto fire_event(is_container auto& listeners, Event& event)
	{
		std::ranges::for_each(Table.table, [&event, &listeners](const auto& mod) constexpr
			{
				if (mod.second.first == entt::type_hash<Event>::value())
				{
					listener_traits::call_event(event, listeners[mod.first]);
				}
			});
	}
}