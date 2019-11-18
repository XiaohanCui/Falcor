/***************************************************************************
# Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
#pragma once

namespace Falcor
{
    /** Container class for logging messages. 
    *   To enable log messages, make sure _LOG_ENABLED is set to true in FalcorConfig.h.
    *   Messages are printed to a log file in the application directory. Using Logger#ShowBoxOnError() you can control if a message box will be shown as well.
    */
    class dlldecl Logger
    {
    public:
        /** Log messages severity
        */
        enum class Level
        {
            Info = 0,       ///< Informative messages
            Warning = 1,    ///< Warning messages
            Error = 2,      ///< Error messages. Application might be able to continue running, but incorrectly.
            Fatal = 3,      ///< Unrecoverable error. Will assert in debug builds
            Disabled = -1
        };

        /** Message box behavior
        */
        enum class MsgBox
        {
            Auto,   ///< Show a message box only if the verbosity is Error or higher **and** `isBoxShownOnError()` returns `true`
            Show,   ///< Show a message box
            Nope    ///< Don't show a message box
        };

        /** Shutdown the logger and close the log file.
        */
        static void shutdown();

        /** Controls weather or not to show message box on log messages.
            \param[in] showBox true to show a message box, false to disable it.
        */
        static void showBoxOnError(bool showBox);

        /** Returns weather or not the message box is shown on log messages.
            returns true if a message box is shown, false otherwise.
        */
        static bool isBoxShownOnError();

        /** Check if the logger is enabled
        */
        static constexpr bool enabled() { return _LOG_ENABLED != 0; }

        /** Set the logger verbosity
        */
        static void setVerbosity(Level level);

    private:
        friend void logInfo(const std::string& msg, MsgBox mbox);
        friend void logWarning(const std::string& msg, MsgBox mbox);
        friend void logError(const std::string& msg, MsgBox mbox);
        friend void logErrorAndExit(const std::string& msg, MsgBox mbox);

        static void log(Level L, const std::string& msg, MsgBox mbox = Logger::MsgBox::Auto);
        Logger() = delete;
    };

    inline void logInfo(const std::string& msg, Logger::MsgBox mbox = Logger::MsgBox::Auto) { Logger::log(Logger::Level::Info, msg, mbox); }
    inline void logWarning(const std::string& msg, Logger::MsgBox mbox = Logger::MsgBox::Auto) { Logger::log(Logger::Level::Warning, msg, mbox); }
    inline void logError(const std::string& msg, Logger::MsgBox mbox = Logger::MsgBox::Auto) { Logger::log(Logger::Level::Error, msg, mbox); }
    inline void logErrorAndExit(const std::string& msg, Logger::MsgBox mbox = Logger::MsgBox::Auto) { Logger::log(Logger::Level::Error, msg + "\nTerminating...", mbox); exit(1); }
}
