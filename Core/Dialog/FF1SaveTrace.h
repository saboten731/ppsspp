#pragma once

// Temporary diagnostic instrumentation for the FINAL FANTASY I Savedata
// slowdown investigation. Compile with -DPPSSPP_FF1_SAVE_TRACE and remove
// this file and its call sites after the trace has been collected.

#if defined(PPSSPP_FF1_SAVE_TRACE)

#include <atomic>
#include <cstdint>
#include <string>
#include <string_view>

#include "Common/Log.h"
#include "Common/StringUtils.h"
#include "Common/Thread/ThreadUtil.h"
#include "Common/TimeUtil.h"
#include "Core/Dialog/SavedataParam.h"

inline bool FF1SaveTraceShouldLog(uint64_t callCount) {
	return callCount <= 20 || callCount % 100 == 0;
}

class FF1SaveTraceScope {
public:
	FF1SaveTraceScope(const char *function, std::atomic<uint64_t> &counter, const SceUtilitySavedataParam *param, std::string_view context = {})
		: function_(function), counter_(counter), callCount_(counter.fetch_add(1, std::memory_order_relaxed) + 1), start_(time_now_raw()), context_(context) {
		if (param) {
			mode_ = (uint32_t)param->mode;
			gameName_ = std::string(StringViewFromFixedSizeField(param->gameName));
			saveName_ = std::string(StringViewFromFixedSizeField(param->saveName));
			fileName_ = std::string(StringViewFromFixedSizeField(param->fileName));
		}
	}

	~FF1SaveTraceScope() {
		const uint64_t elapsedUs = (time_now_raw() - start_) / 1000;
		if (callCount_ <= 20 || callCount_ % 100 == 0 || elapsedUs >= 1000) {
			const size_t modeCount = sizeof(utilitySavedataTypeNames) / sizeof(utilitySavedataTypeNames[0]);
			const char *modeName = mode_ < modeCount ? utilitySavedataTypeNames[mode_] : "UNKNOWN";
			INFO_LOG(Log::sceUtility, "[FF1_SAVE_TRACE] timestamp_us=%llu thread_id=%d function=%s elapsed_us=%llu call_count=%llu mode=%s(%u) gameName=%s saveName=%s fileName=%s context=%s",
				(unsigned long long)(time_now_raw() / 1000), GetCurrentThreadIdForDebug(), function_,
				(unsigned long long)elapsedUs, (unsigned long long)callCount_, modeName, mode_,
				gameName_.c_str(), saveName_.c_str(), fileName_.c_str(), context_.c_str());
		}
	}

private:
	const char *function_;
	std::atomic<uint64_t> &counter_;
	uint64_t callCount_;
	uint64_t start_;
	uint32_t mode_ = 0xFFFFFFFF;
	std::string gameName_;
	std::string saveName_;
	std::string fileName_;
	std::string context_;
};

#define FF1_SAVE_TRACE_SCOPE(function, counter, param, context) \
	FF1SaveTraceScope ff1SaveTraceScope##__LINE__(function, counter, param, context)

#define FF1_SAVE_TRACE_EVENT(counter, ...) do { \
	const uint64_t ff1TraceEventCount = (counter).fetch_add(1, std::memory_order_relaxed) + 1; \
	if (FF1SaveTraceShouldLog(ff1TraceEventCount)) \
		INFO_LOG(Log::sceUtility, __VA_ARGS__); \
} while (false)

#else

#define FF1_SAVE_TRACE_SCOPE(function, counter, param, context) do { } while (false)
#define FF1_SAVE_TRACE_EVENT(counter, ...) do { } while (false)

#endif
