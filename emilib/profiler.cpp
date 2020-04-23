//  Created by Emil Ernerfeldt on 2014-05-22.
//  Copyright (c) 2015 Emil Ernerfeldt. All rights reserved.
//

#include "profiler.hpp"

#include <loguru.hpp>

#ifdef __APPLE__
    #include "TargetConditionals.h"
#endif

namespace profiler {

using namespace std;
using namespace std::chrono;

const bool OUTPUT_STALLS = false;

const char* FRAME_ID = "Frame";

// ------------------------------------------------------------------------

uint64_t now_ns()
{
	using Clock = std::chrono::high_resolution_clock;
	return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
}

template<typename T>
void encode_int(Stream& out_stream, T value)
{
	out_stream.insert(out_stream.end(), (uint8_t*)&value, (uint8_t*)&value + sizeof(value));
}

void encode_time(Stream& out_stream)
{
	encode_int(out_stream, now_ns());
}

void encode_string(Stream& out_stream, const char* str)
{
	do { out_stream.push_back(*str); } while (*str++);
}

template<typename T>
T parse_int(const Stream& stream, size_t& io_offset)
{
	CHECK_LE_F(io_offset + sizeof(T), stream.size());
	auto result = *(const T*)&stream[io_offset];
	io_offset += sizeof(T);
	return result;
}

const char* parse_string(const Stream& stream, size_t& io_offset)
{
	CHECK_LE_F(io_offset, stream.size());
	const char* result = reinterpret_cast<const char*>(&stream[io_offset]);
	while (stream[io_offset] != 0) {
		++io_offset;
	}
	++io_offset;
	return result;
}

std::string format(unsigned indent, const char* id, const char* extra, NanoSeconds ns)
{
	auto indentation = std::string(4 * indent, ' ');
	return loguru::strprintf("%10.3f ms:%s %s %s", ns / 1e6, indentation.c_str(), id, extra);
}

// ------------------------------------------------------------------------

boost::optional<Scope> parse_scope(const Stream& stream, size_t offset)
{
	if (offset >= stream.size()) { return boost::none; }
	if (stream[offset] != kScopeBegin) { return boost::none; }
	++offset;

	Scope scope;
	scope.record.start_ns = parse_int<NanoSeconds>(stream, offset);
	scope.record.id       = parse_string(stream, offset);
	scope.record.extra    = parse_string(stream, offset);
	const auto scope_size = parse_int<ScopeSize>(stream, offset);
	if (scope_size == ScopeSize(-1))
	{
		// Scope started but never ended.
		return boost::none;
	}
	scope.child_idx = offset;
	scope.child_end_idx = offset + scope_size;
	CHECK_LT_F(scope.child_end_idx, stream.size());

	CHECK_EQ_F(stream[scope.child_end_idx], kScopeEnd);
	auto next_idx = scope.child_end_idx + 1;
	auto stop_ns = parse_int<NanoSeconds>(stream, next_idx);
	CHECK_LE_F(scope.record.start_ns, stop_ns);
	scope.record.duration_ns = stop_ns - scope.record.start_ns;

	scope.next_idx = next_idx;
	return scope;
}

std::vector<Scope> collectScopes(const Stream& stream, size_t offset)
{
	std::vector<Scope> result;
	while (auto scope = parse_scope(stream, offset))
	{
		result.push_back(*scope);
		offset = scope->next_idx;
	}
	return result;
}

// ------------------------------------------------------------------------

NanoSeconds check_for_stalls(const Stream& stream, const Scope& scope, NanoSeconds stall_cutoff_ns, unsigned depth)
{
	auto parent_ns = scope.record.duration_ns;

	if (OUTPUT_STALLS && parent_ns > stall_cutoff_ns) {
		LOG_S(INFO) << format(depth, scope.record.id, scope.record.extra, parent_ns);

		// Process children:
		NanoSeconds child_ns = 0;

		size_t idx = scope.child_idx;
		while (auto child = parse_scope(stream, idx)) {
			child_ns += check_for_stalls(stream, *child, stall_cutoff_ns, depth + 1);
			idx = child->next_idx;
		}
		CHECK_EQ_F(idx, scope.child_end_idx);

		if (child_ns > stall_cutoff_ns) {
			auto missing = parent_ns - child_ns;
			if (missing > stall_cutoff_ns) {
				LOG_S(INFO) << format(depth + 1, "* Unaccounted", "", missing);
			}
		}
	}

	return parent_ns;
}

// ------------------------------------------------------------------------

ProfilerMngr& ProfilerMngr::instance()
{
	static ProfilerMngr s_profile_mngr;
	return s_profile_mngr;
}

ProfilerMngr::ProfilerMngr()
{
	set_stall_cutoff(0.010);
	auto frame_str = std::to_string(_frame_counter);
	_frame_offset = get_thread_profiler().start(FRAME_ID, frame_str.c_str());
}

void ProfilerMngr::set_stall_cutoff(double secs)
{
	_stall_cutoff_ns = static_cast<NanoSeconds>(secs * 1e9);
}

void ProfilerMngr::update()
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	get_thread_profiler().stop(_frame_offset);
	_frame_counter += 1;

	for (const auto& p : _streams) {
		ERROR_CONTEXT("thread name", p.second.thread_info.name.c_str());
		size_t idx = 0;
		while (auto scope = parse_scope(p.second.stream, idx)) {
			check_for_stalls(p.second.stream, *scope, _stall_cutoff_ns, 0);
			idx = scope->next_idx;
		}

		CHECK_EQ_F(idx, p.second.stream.size());
	}

	_last_frame.swap(_streams);
	_streams.clear();

	if (_first_frame.empty()) {
		_first_frame = _last_frame;
	}

	auto frame_str = std::to_string(_frame_counter);
	_frame_offset = get_thread_profiler().start(FRAME_ID, frame_str.c_str());
}

void ProfilerMngr::report(const ThreadInfo& thread_info, const Stream& stream)
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	auto& thread_stream = _streams[thread_info.id];

	thread_stream.thread_info = thread_info;

	thread_stream.stream.insert(thread_stream.stream.end(),
		stream.begin(), stream.end());
}

// ----------------------------------------------------------------------------

ThreadProfiler::ThreadProfiler()
	: _start_time_ns(now_ns())
{
}

Offset ThreadProfiler::start(const char* id, const char* extra)
{
	_depth += 1;

	_stream.push_back(kScopeBegin);
	encode_time(_stream);
	encode_string(_stream, id);
	encode_string(_stream, extra);

	// Make room for writing size of this scope:
	auto offset = _stream.size();
	encode_int(_stream, ScopeSize(-1));
	return offset;
}

void ThreadProfiler::stop(Offset start_offset)
{
	CHECK_GT_F(_depth, 0u);
	_depth -= 1;

	auto skip = _stream.size() - (start_offset + sizeof(ScopeSize));
	CHECK_LE_F(start_offset + sizeof(ScopeSize), _stream.size());
	*reinterpret_cast<ScopeSize*>(&_stream[start_offset]) = static_cast<ScopeSize>(skip);
	_stream.push_back(kScopeEnd);
	encode_time(_stream);

	if (_depth == 0) {
		char thread_name[17];
		loguru::get_thread_name(thread_name, sizeof(thread_name), false);
		ThreadInfo thread_info {
			std::this_thread::get_id(),
			thread_name,
			_start_time_ns
		};
		ProfilerMngr::instance().report(thread_info, _stream);
		_stream.clear();
	}
}

// ----------------------------------------------------------------------------

#if !defined(__APPLE__)

	static thread_local ThreadProfiler thread_profiler_object;

	ThreadProfiler& get_thread_profiler()
	{
		return thread_profiler_object;
	}

#else // !thread_local

	static pthread_once_t s_profiler_pthread_once = PTHREAD_ONCE_INIT;
	static pthread_key_t  s_profiler_pthread_key;

	void free_thread_profiler(void* io_error_context)
	{
		delete reinterpret_cast<ThreadProfiler*>(io_error_context);
	}

	void profiler_make_pthread_key()
	{
		(void)pthread_key_create(&s_profiler_pthread_key, free_thread_profiler);
	}

	ThreadProfiler& get_thread_profiler()
	{
		(void)pthread_once(&s_profiler_pthread_once, profiler_make_pthread_key);
		auto ec = reinterpret_cast<ThreadProfiler*>(pthread_getspecific(s_profiler_pthread_key));
		if (ec == nullptr) {
			ec = new ThreadProfiler();
			(void)pthread_setspecific(s_profiler_pthread_key, ec);
		}
		return *ec;
	}
#endif // !thread_local

// ----------------------------------------------------------------------------

} // namespace profiler
