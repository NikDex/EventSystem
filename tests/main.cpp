#include <gtest/gtest.h>
#include <event_system.hpp>

#include <print>
#include <entt/poly/poly.hpp>

struct smth1_event;
struct smth2_event;
struct smth3_event;

class listener_base
{
public:
	explicit listener_base(const char* name): name_(name)
	{
	}

	[[nodiscard]] constexpr auto get_name() const
	{
		return name_;
	}

protected:
	const char* name_;
};
class listener_interface: public entt::type_list<>
{
public:
	template <class Poly>
	struct type: Poly
	{
		void on_event(smth1_event& e)
		{
			this->template invoke<0>(*this, e);
		}
		void on_event(smth2_event& e)
		{
			this->template invoke<1>(*this, e);
		}
		void on_event(smth3_event& e)
		{
			this->template invoke<2>(*this, e);
		}
	};

	template <class Derived>
	using impl = entt::value_list<
		entt::overload<void(smth1_event&)>(&Derived::on_event),
		entt::overload<void(smth2_event&)>(&Derived::on_event),
		entt::overload<void(smth3_event&)>(&Derived::on_event)
	>;
};

using listener = entt::poly<listener_interface>;

struct smth1_event
{
	int& value;
};
struct smth2_event
{
	float& value;
};
struct smth3_event
{
	std::string& value;
};

class test1: public listener_base
{
public:
	using listener_base::listener_base;

	void on_event(smth1_event& ev)
	{
		ev.value = 1;
		std::println("{}: smth1", name_);
	}

	void on_event(smth2_event& ev)
	{
		ev.value = 0.3f;
		std::println("{}: smth2", name_);
	}

	void on_event(smth3_event& ev)
	{
		ev.value = name_;
		std::println("{}: smth3", name_);
	}
};
class test2 : public listener_base
{
public:
	using listener_base::listener_base;

	void on_event(smth1_event& ev)
	{
		ev.value = 2;
		std::println("{}: smth1", name_);
	}

	void on_event(smth2_event& ev)
	{
		ev.value = 0.2f;
		std::println("{}: smth2", name_);
	}

	void on_event(smth3_event& ev)
	{
		ev.value = name_;
		std::println("{}: smth3", name_);
	}
};
class test3: public listener_base
{
public:
	using listener_base::listener_base;

	void on_event(smth1_event& ev)
	{
		ev.value = 3;
		std::println("{}: smth1", name_);
	}

	void on_event(smth2_event& ev)
	{
		ev.value = 0.1f;
		std::println("{}: smth2", name_);
	}

	void on_event(smth3_event& ev)
	{
		ev.value = name_;
		std::println("{}: smth3", name_);
	}
};

class test1_with_same_priority : public test1
{
public:
	using test1::test1;

	static constexpr ev::priority_type<3> priority{ {
		ev::make_priority<smth1_event, 0>(),
		ev::make_priority<smth2_event, 1>(),
		ev::make_priority<smth3_event, 2>(), // test1_with_eq_some_priority:smth3_event (2) == test3_with_priority:smth3_event (2)
	} };
};
class test1_with_priority : public test1
{
public:
	using test1::test1;

	static constexpr ev::priority_type<3> priority{ {
		ev::make_priority<smth1_event, 0>(),
		ev::make_priority<smth2_event, 2>(),
		ev::make_priority<smth3_event, 1>(),
	} };
};
class test2_with_priority : public test2
{
public:
	using test2::test2;

	static constexpr ev::priority_type<3> priority{ {
		ev::make_priority<smth1_event, 2>(),
		ev::make_priority<smth2_event, 1>(),
		ev::make_priority<smth3_event, 0>(),
	} };
};
class test2_with_one_priority : public test2
{
public:
	using test2::test2;

	static constexpr ev::priority_type<3> priority{ {
		ev::make_priority<smth2_event, 1>(),
	} };
};
class test3_with_priority : public test3
{
public:
	using test3::test3;

	static constexpr ev::priority_type<3> priority{ {
		ev::make_priority<smth1_event, 1>(),
		ev::make_priority<smth2_event, 3>(),
		ev::make_priority<smth3_event, 2>(),
	} };
};

TEST(Listeners, DefaultWithNoPriority)
{
	auto [listeners] = ev::event_array{ std::array {
		listener(std::in_place_type<test1>, "test 1"),
		listener(std::in_place_type<test2>, "test 2"),
		listener(std::in_place_type<test3>, "test 3")
	} };

	constexpr auto table = ev::make_static_table<ev::registered_events<smth1_event, smth2_event, smth3_event>, test1, test2, test3>();

	int int_data = 0;
	ev::fire_emplace_event<smth1_event>(listeners, table, int_data);

	float float_data = 0.f;
	ev::fire_emplace_event<smth2_event>(listeners, table, float_data);

	std::string name_data = "nothing";
	ev::fire_emplace_event<smth3_event>(listeners, table, name_data);

	EXPECT_EQ(int_data, 3);
	EXPECT_EQ(float_data, 0.1f);
	EXPECT_EQ(name_data, "test 3");
}

TEST(ListenersAndPriority, AllWithPriority)
{
	auto [listeners] = ev::event_array{ std::array {
		listener(std::in_place_type<test1_with_priority>, "test 1"),
		listener(std::in_place_type<test2_with_priority>, "test 2"),
		listener(std::in_place_type<test3_with_priority>, "test 3")
	} };

	constexpr auto table = ev::make_static_table<ev::registered_events<smth1_event, smth2_event, smth3_event>, test1_with_priority, test2_with_priority, test3_with_priority>();

	int int_data = 0;
	ev::fire_emplace_event<smth1_event>(listeners, table, int_data);

	float float_data = 0.f;
	ev::fire_emplace_event<smth2_event>(listeners, table, float_data);

	std::string name_data = "nothing";
	ev::fire_emplace_event<smth3_event>(listeners, table, name_data);

	EXPECT_EQ(int_data, 1);
	EXPECT_EQ(float_data, 0.2f);
	EXPECT_EQ(name_data, "test 2");
}

TEST(ListenersAndPriority, ListenerWithOnePriority)
{
	auto [listeners] = ev::event_array{std::array {
		listener(std::in_place_type<test1>, "test 1"),
		listener(std::in_place_type<test2_with_priority>, "test 2"),
		listener(std::in_place_type<test3>, "test 3")
	} };

	constexpr auto table = ev::make_static_table<ev::registered_events<smth1_event, smth2_event, smth3_event>, test1, test2_with_priority, test3>();

	int int_data = 0;
	ev::fire_emplace_event<smth1_event>(listeners, table, int_data);

	float float_data = 0.f;
	ev::fire_emplace_event<smth2_event>(listeners, table, float_data);

	std::string name_data = "nothing";
	ev::fire_emplace_event<smth3_event>(listeners, table, name_data);

	EXPECT_EQ(int_data, 3);
	EXPECT_EQ(float_data, 0.1f);
	EXPECT_EQ(name_data, "test 3");
}

TEST(ListenersAndPriority, ListenerWithOnePriorityEvent)
{
	auto [listeners] = ev::event_array{std::array {
		listener(std::in_place_type<test1>, "test 1"),
		listener(std::in_place_type<test2_with_one_priority>, "test 2"),
		listener(std::in_place_type<test3>, "test 3")
	} };

	constexpr auto table = ev::make_static_table<ev::registered_events<smth1_event, smth2_event, smth3_event>, test1, test2_with_one_priority, test3>();

	int int_data = 0;
	ev::fire_emplace_event<smth1_event>(listeners, table, int_data);

	float float_data = 0.f;
	ev::fire_emplace_event<smth2_event>(listeners, table, float_data);

	std::string name_data = "nothing";
	ev::fire_emplace_event<smth3_event>(listeners, table, name_data);

	EXPECT_EQ(int_data, 3);
	EXPECT_EQ(float_data, 0.1f);
	EXPECT_EQ(name_data, "test 3");
}

TEST(ListenersAndPriority, ListenersWithSamePriority)
{
	auto [listeners] = ev::event_array{std::array {
		listener(std::in_place_type<test1_with_same_priority>, "test 1"),
		listener(std::in_place_type<test2_with_priority>, "test 2"),
		listener(std::in_place_type<test3_with_priority>, "test 3")
	} };

	constexpr auto table = ev::make_static_table<ev::registered_events<smth1_event, smth2_event, smth3_event>, test1_with_same_priority, test2_with_priority, test3_with_priority>();

	int int_data = 0;
	ev::fire_emplace_event<smth1_event>(listeners, table, int_data);

	float float_data = 0.f;
	ev::fire_emplace_event<smth2_event>(listeners, table, float_data);

	std::string name_data = "nothing";
	ev::fire_emplace_event<smth3_event>(listeners, table, name_data);

	EXPECT_EQ(int_data, 1);
	EXPECT_EQ(float_data, 0.2f);
	EXPECT_EQ(name_data, "test 2");
}