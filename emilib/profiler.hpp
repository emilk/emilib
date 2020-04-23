//  Created for ghostel
//
//  Created by Emil Ernerfeldt on 2016-03-05.
//  Copyright © 2016 Emil Ernerfeldt. All rights reserved.

#pragma once

#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <boost/optional.hpp>
#include <loguru.hpp>  // LOGURU_ANONYMOUS_VARIABLE

namespace profiler {

using NanoSeconds = uint64_t;
using ScopeSize   = uint32_t;
using Offset      = size_t;
/// Profile events are encododed as Streams.
using Stream      = std::vector<uint8_t>;

struct ThreadInfo
{
    std::thread::id id;
    std::string     name;
    /// When the first even was recorded. Useful for ordering ThreadStreams.
    uint64_t        start_time_ns = 0;
};

struct ThreadStream
{
    ThreadInfo thread_info;
    Stream     stream;
};

using ThreadStreams = std::unordered_map<std::thread::id, ThreadStream>;

// ----------------------------------------------------------------------------

static const char kScopeBegin = 'B';
static const char kScopeEnd   = 'E';

struct Record
{
    NanoSeconds start_ns;
    NanoSeconds duration_ns;
    const char* id;
    const char* extra;
};

// Used when parsing a Stream.
struct Scope
{
    Record record;
    size_t child_idx;     // Stream offset for first child (if kBegin).
    size_t child_end_idx; // Stream offset after last child.
    size_t next_idx;      // Stream offset for next siblimg (if any).
};

boost::optional<Scope> parse_scope(const Stream& stream, size_t offset);

std::vector<Scope> collectScopes(const Stream& stream, size_t offset);

// ----------------------------------------------------------------------------

class ProfilerMngr
{
public:
    static ProfilerMngr& instance();

    /// Call once per frame
    void update();

    /// Report profile data for a thread. Called by ThreadProfiler.
    void report(const ThreadInfo& thread_info, const Stream& stream);

    /// Higher stall than this will be warned about.
    /// Set to e.g. 1.0 / 60.0 to warn about frame spikes
    void set_stall_cutoff(double secs);

    const ThreadStreams& first_frame() const { return _first_frame; }
    const ThreadStreams& last_frame() const { return _last_frame; }

private:
    ProfilerMngr();

    std::recursive_mutex _mutex;
    NanoSeconds          _stall_cutoff_ns;
    uint64_t             _frame_counter = 0;
    Offset               _frame_offset;
    ThreadStreams        _streams;
    ThreadStreams        _first_frame
    ThreadStreams        _last_frame;
};

// ----------------------------------------------------------------------------

class ThreadProfiler
{
public:
    ThreadProfiler();

    Offset start(const char* id, const char* extra);
    void stop(Offset start_offset);

private:
    Stream   _stream;
    size_t   _depth = 0;
    uint64_t _start_time_ns;
};

ThreadProfiler& get_thread_profiler();

// ----------------------------------------------------------------------------

class ProfileScope
{
public:
    ProfileScope(const char* id, const char* extra)
    {
        _offset = get_thread_profiler().start(id, extra);
    }
    ~ProfileScope() { get_thread_profiler().stop(_offset); }

private:
    ProfileScope(const ProfileScope&) = delete;
    ProfileScope(ProfileScope&&) = delete;
    ProfileScope& operator=(const ProfileScope&) = delete;
    ProfileScope& operator=(ProfileScope&&) = delete;

    Offset _offset;
};

// ----------------------------------------------------------------------------
// This is what you'll actually use:

// Overhead for one of these calls is about 140 ns.
#define PROFILE2(id, extra) profiler::ProfileScope LOGURU_ANONYMOUS_VARIABLE(profiler_RAII_)(id, extra)
#define PROFILE_FUNCTION()  PROFILE2(__PRETTY_FUNCTION__, "")
#define PROFILE(id)         PROFILE2(id, "")

// ----------------------------------------------------------------------------

} // namespace profiler
