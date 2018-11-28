#pragma once

#include <ctime>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Exports.h"

#ifdef _WIN32
  #define __cdecl  
#else
  #define __cdecl __attribute__((__cdecl__))
#endif

/**
 * It's log, it's log, it's better than bad, it's good.
 */
namespace KS {
    // Log Levels
    enum LogLevel:uint8_t
    {
        LOG_ALL		= 0xFF,
        LOG_FATAL	= 16,
        LOG_ERROR	= 8,
        LOG_WARNING = 4,
        LOG_INFO	= 2,
        LOG_DEBUG	= 1
    };

    /**
     * Log handler callback
     *
     * @param   LogLevel level
     * @param   const char* channel
     * @param   const char* message
     */
    typedef void(__cdecl *LogHandler)(LogLevel level, const char* channel, const char* message);

    /**
     * Static logger class. Log messages have a level and a channel string that can be used to filter log messages.
     * Channels can have parent channels. Any message a channel receives will also be sent to all of it's parent
     * channels. Channels are seperated by ".", with children channel following parents. All channels are children of
     * the root channel.
     * Example:
     *  KS::Log::Info("X.Y", "This message will be received by channels 'X.Y', 'X', and 'Root');
     */
    class Log
    {
    public:
        /**
         * Register a log handler for a specific channel and log level
         *
         * @param   const std::string& channel
         * @param   LogHandler log handler
         * @param   LogLevel log level
         * @param   bool allowBubbling - if false, logs handled by this handler will not be processed by handlers on
         *          parent channels.
         */
        EXTERNAL static void RegisterHandler(const std::string& channel, LogHandler handler, uint8_t levels, bool allowBubbling);

        /**
         * Unregister a log handler for a specific channel and log level
         *
         * @param   const std::string& channel
         * @param   LogHandler log handler
         */
        EXTERNAL static void UnregisterHandler(const std::string& channel, LogHandler handler);

        /**
         * Parses a channel into a channel hierarchy (See above) and calls the handlers for each level starting with
         * the lowest level and working up the hierarchy until 'Root'.  If a handler has been registered with 'allowBubbling'
         * set to false then all parent handlers will be skipped.
         *
         * @param   const std::string& message
         * @param   const std::string& channel
         * @param   LogLevel log level
         */
        EXTERNAL static void Write(const std::string& message, const std::string& channel, LogLevel level);

        /**
         * Write a DEBUG log message
         *
         * @param   const std::string& message to log
         * @param   const std::string& channel to log to
         */
        EXTERNAL static void Debug(const std::string& message, const std::string& channel = "Root");

        /**
         * Write an INFO log message
         *
         * @param   const std::string& message to log.
         * @param   const std::string& channel to log to
         */
        EXTERNAL static void Info(const std::string& message, const std::string& channel = "Root");

        /**
         * Write a WARNING log message
         *
         * @param   const std::string& message to log
         * @param   const std::string& channel to log to
         */
        EXTERNAL static void Warning(const std::string& message, const std::string& channel = "Root");

        /**
         * Write an ERROR log message
         *
         * @param   const std::string& message to log
         * @param   const std::string& channel to log to
         */
        EXTERNAL static void Error(const std::string& message, const std::string& channel = "Root");

        /**
         * Write a fatal log message
         *
         * @param   const std::string& message to log
         * @param   const std::string& channel to log to
         */
        EXTERNAL static void Fatal(const std::string& message, const std::string& channel = "Root");

        /**
         * Get text for a logging level
         *
         * @param   LogLevel logLevel
         * @return  std::string name of log level.
         */
        EXTERNAL static std::string GetLevelString(LogLevel logLevel);

        /**
         * Get a time string YYYY-MM-DD hh:mm:ss for the current time.
         *
         * @return  std::string current time string.
         */
        EXTERNAL static std::string GetTimeString();

    private:
        struct Logger
        {
            LogHandler handler;
            uint8_t levels;
            bool allowBubbling;
        };

        static std::unordered_map<std::string, std::vector<Logger>> m_loggers;

        /**
         * Sends a log message to all handlers on a channel.
         *
         * @param   std::string& - message
         * @param   std::string& - channel
         * @param   LogLevel log - log level
         * @param   std::string& - log handler (Channel, sub-channel, or Root)
         */
        static bool HandleLog(
            const std::string& message,
            const std::string& channel,
            LogLevel level,
            const std::string& handler);
    };
}
