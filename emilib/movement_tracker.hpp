// By Emil Ernerfeldt 2012-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
// HISTORY:
//   Created in 2012-11-11

#pragma once

#include <deque>
#include <utility>
#include <vector>

#include <loguru.hpp>

namespace emilib {

/// Tracks movement and gives info about velocity.
/// 'T' is the type of movement we track (float for scoll, Vec2f for position, ...)
template<typename T>
class MovementTracker
{
protected:
	struct TimePosPair
	{
		double when;
		T where;
	};
	using TimePosList = std::deque<TimePosPair>;

public:
	using value_type = T;

	MovementTracker() { }
	virtual ~MovementTracker() { }

	// ------------------------------------------------

	void clear()
	{
		_list.clear();
		_has_start = false;
	}

	void add(const T& pos, double time)
	{
		if (!_has_start) {
			_start = TimePosPair{time, pos};
			_has_start = true;
		}

		_list.push_back(TimePosPair{time, pos});

		flush(time);
	}

	std::vector<T> points() const
	{
		std::vector<T> points;
		for (auto&& p : _list) {
			points.push_back(p.where);
		}
		return points;
	}

	bool empty() const { return _list.empty(); }
	size_t size() const { return _list.size(); }

	double start_time() const
	{
		CHECK_F(_has_start);
		return _start.when;
	}

	T start_pos() const
	{
		CHECK_F(_has_start);
		return _start.where;
	}

	double latest_time() const
	{
		CHECK_F(!empty());
		return _list.back().when;
	}

	T latest_pos() const
	{
		CHECK_F(!empty());
		return _list.back().where;
	}

	/// Last movement delta
	T rel() const
	{
		CHECK_GE_F(size(), 2);
		return _list[_list.size()-1].where - _list[_list.size()-2].where;
	}

	double duration() const
	{
		CHECK_F(!empty());
		return latest_time() - start_time();
	}

	/// Return T() on fail
	/// Calculates the average velocity over the last VelocityTime() seconds.
	virtual T velocity(double now) const
	{
		size_t begin;
		if (!velocity_calc_begin(begin, now)) {
			return T();
		}

		double dt = _list.back().when - _list[begin].when;

		if (dt <= 0) {
			return T();
		}

		T dx = _list.back().where - _list[begin].where;

		return dx / (float)dt;
	}

	T velocity() const
	{
		return velocity(latest_time());
	}

	/// Has all movement been within "max_dist" radius, during the last "duration" seconds?
	template<typename F>
	bool is_still(F max_dist, double duration) const
	{
		CHECK_F(!empty());
		const double now = latest_time(); // well.. whatever

		for (size_t i=0; i<_list.size(); ++i) {
			if (now - _list[i].when < duration) {
				if (distance(_list[i].where, _list.back().where) > max_dist) {
					return false;
				}
			}
		}

		return true;
	}

	// ------------------------------------------------

	/// Flush out oldest entries.
	void flush(double now)
	{
		while (!_list.empty() && _list.front().when < now - _max_history_time) {
			_list.pop_front();
		}
	}

protected:
	/// From where shall we calculate velocity? Return false on "not at all"
	bool velocity_calc_begin(size_t& out_index, double now) const
	{
		if (_list.size() < 2) {
			return false; // Not enough data
		}

		auto duration = now - start_time();

		if (duration < min_velocity_time()) {
			return false; // Not enough data
		}

		double vel_time = velocity_time();

		for (size_t i=0; i<_list.size()-1; ++i) {
			if (now - _list[i].when < vel_time) {
				if (_list.size() - i < min_velocity_samples()) {
					return false; // Too few samples
				}

				out_index = i;
				return true;
			}
		}

		return false;
	}

	/// The minimum number of samples for there to be any velocity calculated.
	static constexpr size_t min_velocity_samples()
	{
		return 3;
	}

	/// Minimum time before we have a good velocity
	static constexpr double min_velocity_time()
	{
		return 0.01f;
	}

	/// The time over which we calculate velocity.
	static constexpr double velocity_time()
	{
		return 0.1f;
	}

	// ------------------------------------------------

	bool        _has_start        = false;
	TimePosPair _start;           // Since it can be pruned from _list
	TimePosList _list;
	double      _max_history_time = 10; // Don't keep points older than this
};

// ------------------------------------------------

/// Made to take into account the cyclic nature of angles (in radians, btw)
class RotationTracker : public MovementTracker<float>
{
	using base = MovementTracker<float>;
public:
	virtual float velocity(double now) const override;
};

// ------------------------------------------------

// Example:
// using PositionTracker = MovementTracker<Vec2f>;

} // namespace emilib
