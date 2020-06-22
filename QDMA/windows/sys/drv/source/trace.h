/*
 * Copyright (C) 2020 Xilinx, Inc
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

#pragma once

#if defined(DBG) || defined(ENABLE_WPP)
#define ENABLE_WPP_TRACING
#endif

/**
 * Define the tracing flags.
 *
 * Tracing GUID - 0bed1f17-aa40-5163-c038-33715b81ae49
 * Trace Name : 'Xilinx-QDMA-Debug'
 */
#define WPP_CONTROL_GUIDS                                                   \
        WPP_DEFINE_CONTROL_GUID(                                            \
            QDMATraceGuid, (0bed1f17, aa40, 5163, c038, 33715b81ae49),      \
                                                                            \
            WPP_DEFINE_BIT(TRACE_PCIE)                                      \
            WPP_DEFINE_BIT(TRACE_INTR)                                      \
            WPP_DEFINE_BIT(TRACE_THREAD)                                    \
            WPP_DEFINE_BIT(TRACE_QDMA)                                      \
            WPP_DEFINE_BIT(TRACE_DBG)                                       \
            WPP_DEFINE_BIT(TRACE_QDMA_ACCESS)                               \
            WPP_DEFINE_BIT(TRACE_DRIVER)                                    \
            WPP_DEFINE_BIT(TRACE_DEVICE)                                    \
            WPP_DEFINE_BIT(TRACE_IO)                                        \
            )

/* WPP_LEVEL_FLAGS_LOGGER and WPP_LEVEL_FLAGS_ENABLED support trace functions
   with LEVEL and FLAGS static parameters (in that order) prior to any dynamic
   parameters (such as MSG)
*/
#define WPP_LEVEL_FLAGS_LOGGER(level, flags) \
        WPP_LEVEL_LOGGER(flags)

#define WPP_LEVEL_FLAGS_ENABLED(level, flags) \
        (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= level)

/* Optimize WPP tracing call site conditional checks

   NOTE: This is only safe if we ensure no WPP tracing functions are called
         before WPP_INIT_TRACING() or after WPP_CLEANUP().
*/
#define WPP_CHECK_INIT


//
// This comment block is scanned by the trace preprocessor to define our
// Trace functions.
//
// begin_wpp config
// FUNC TraceVerbose{LEVEL=TRACE_LEVEL_VERBOSE}(FLAGS, MSG, ...);
// FUNC TraceInfo{LEVEL=TRACE_LEVEL_INFORMATION}(FLAGS, MSG, ...);
// FUNC TraceWarning{LEVEL=TRACE_LEVEL_WARNING}(FLAGS, MSG, ...);
// FUNC TraceError{LEVEL=TRACE_LEVEL_ERROR}(FLAGS, MSG, ...);
// FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
// end_wpp
//

/** WPP tracing is disabled by default in release configuration.
 *  so stub out definitions and functions
 *
 *  To Enable WPP Tracing,  Enable Run WPP Tracing in settings and
 *  define MACRO "ENABLE_WPP"
 */
#ifndef ENABLE_WPP_TRACING
#define WPP_INIT_TRACING(driver_object, registry_path)
#define WPP_CLEANUP(driver_object)
#define TraceVerbose(flags, ...)        (__VA_ARGS__)
#define TraceInfo(flags, ...)           (__VA_ARGS__)
#define TraceWarning(flags, ...)        (__VA_ARGS__)
#define TraceError(flags, ...)          (__VA_ARGS__)
#define TraceEvents(flags, ...)         (__VA_ARGS__)
#endif
