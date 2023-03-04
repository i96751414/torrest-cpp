#ifndef TORREST_LOG_H
#define TORREST_LOG_H

#include <utility>
#include <vector>

#include "spdlog/sinks/base_sink.h"
#include "spdlog/spdlog.h"

namespace torrest { namespace utils {

    namespace sinks {

        typedef std::function<void(const spdlog::details::log_msg &msg)> log_callback;

        template<typename Mutex>
        class callback_sink final : public spdlog::sinks::base_sink<Mutex> {
        public:
            explicit callback_sink(log_callback pCallback)
                : mCallback{std::move(pCallback)} {}

        protected:
            void sink_it_(const spdlog::details::log_msg &msg) override {
                mCallback(msg);
            }

            void flush_() override{};

        private:
            log_callback mCallback;
        };

        using callback_sink_mt = callback_sink<std::mutex>;

    }

    std::vector<spdlog::sink_ptr> &get_logger_sinks();

    void clear_sinks();

    void add_logger_sink(spdlog::sink_ptr pSink);

    void add_stdout_sink();

    void add_file_sink(const std::string &pFileName, bool pTruncate = false);

    void add_callback_sink(const sinks::log_callback &pCallback);

    std::shared_ptr<spdlog::logger> create_logger(std::string pLoggerName);

}}

#endif //TORREST_LOG_H
