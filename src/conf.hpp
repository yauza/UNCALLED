/* MIT License
 *
 * Copyright (c) 2018 Sam Kovaka <skovaka@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef CONF_HPP
#define CONF_HPP

#include <iostream>
#include <vector>
#include "timer.hpp"
#include "mapper.hpp"
#include "bwa_fmi.hpp"
#include "kmer_model.hpp"
#include "event_detector.hpp"
#include "mapper.hpp"
#include "seed_tracker.hpp"
#include "read_buffer.hpp"
#include "toml.hpp"

#ifdef PYBIND
#include "pybind11/pybind11.h"
#endif

#define INDEX_SUFF ".uncl"

typedef struct {
    std::string fast5_list;
    std::string fast5_filter;
    u32 max_reads, max_buffer;
} Fast5Params;

const std::string ACTIVE_STRS[] = {"full", "even", "odd"};
const std::string MODE_STRS[] = {"deplete", "enrich"};

typedef struct {
    enum class ActiveChs {FULL, EVEN, ODD, NUM};
    ActiveChs active_chs;

    enum class Mode {DEPLETE, ENRICH, NUM};
    Mode realtime_mode;

    std::string host;
    u16 port;
    float duration;
} RealtimeParams;

#define GET_SET(T, N) T get_##N() { return N; } \
                      void set_##N(const T &v) { N = v; }

#define GET_SET_EXTERN(T, E, N) T get_##N() { return E.N; } \
                                void set_##N(const T &v) { E.N = v; }

#define DEFPRP(P) c.def_property(#P, &Conf::get_##P, &Conf::set_##P);

#define GET_NAMED_TOML(C, T, V, N) { \
    if (C.contains(N)) \
        V = toml::find<T>(C,N); \
}

#define GET_TOML_EXTERN(C, T, V, S) GET_NAMED_TOML(C, T, S.V, #V)
#define GET_TOML(C, T, V) GET_NAMED_TOML(C, T, V, #V)

class Conf {
    public:
    void load_conf(const std::string &fname) {
        const auto conf = toml::parse(fname);

        if (conf.contains("global")) {
            const auto subconf = toml::find(conf, "global");

            GET_TOML(subconf, std::string, bwa_prefix)
            GET_TOML(subconf, std::string, kmer_model)
            GET_TOML(subconf, std::string, index_preset)
            GET_TOML(subconf, u16, num_channels);
            GET_TOML(subconf, u16, threads);

            ReadBuffer::PRMS.calib_offsets.resize(num_channels);
            ReadBuffer::PRMS.calib_coefs.resize(num_channels);
        }

        if (conf.contains("fast5_params")) {
            const auto subconf = toml::find(conf, "fast5_params");

            GET_TOML_EXTERN(subconf, u32, max_buffer, fast5_prms);
            GET_TOML_EXTERN(subconf, u32, max_reads, fast5_prms);
            GET_TOML_EXTERN(subconf, std::string, fast5_list, fast5_prms);
            GET_TOML_EXTERN(subconf, std::string, fast5_filter, fast5_prms);
        }

        if (conf.contains("realtime")) {
            const auto subconf = toml::find(conf, "realtime");
            GET_TOML_EXTERN(subconf, std::string, host, realtime_prms);
            GET_TOML_EXTERN(subconf, u16, port, realtime_prms);
            GET_TOML_EXTERN(subconf, float, duration, realtime_prms);

            if (subconf.contains("realtime_mode")) {
                std::string mode_str = toml::find<std::string>(subconf, "realtime_mode");
                for (u8 i = 0; i != (u8) RealtimeParams::Mode::NUM; i++) {
                    if (mode_str == MODE_STRS[i]) {
                        realtime_prms.realtime_mode = (RealtimeParams::Mode) i;
                        break;
                    }
                }
            }

            if (subconf.contains("active_chs")) {
                std::string mode_str = toml::find<std::string>(subconf, "active_chs");
                for (u8 i = 0; i != (u8) RealtimeParams::ActiveChs::NUM; i++) {
                    if (mode_str == ACTIVE_STRS[i]) {
                        realtime_prms.active_chs = (RealtimeParams::ActiveChs) i;
                        break;
                    }
                }
            }
        }

        if (conf.contains("signal")) {
            const auto subconf = toml::find(conf, "signal");

            GET_TOML_EXTERN(subconf, u16, bp_per_sec, ReadBuffer::PRMS);
            GET_TOML_EXTERN(subconf, u16, sample_rate, ReadBuffer::PRMS);

            ReadBuffer::PRMS.bp_per_samp = (ReadBuffer::PRMS.bp_per_sec / 
                                            ReadBuffer::PRMS.sample_rate);
        }

        if (conf.contains("mapper")) {
            const auto subconf = toml::find(conf, "mapper");

            GET_TOML_EXTERN(subconf, u32, seed_len, Mapper::PRMS);
            GET_TOML_EXTERN(subconf, u32, min_rep_len, Mapper::PRMS);
            GET_TOML_EXTERN(subconf, u32, max_rep_copy, Mapper::PRMS);
            GET_TOML_EXTERN(subconf, u32, max_paths, Mapper::PRMS);
            GET_TOML_EXTERN(subconf, u32, max_consec_stay, Mapper::PRMS);
            GET_TOML_EXTERN(subconf, u32, max_events, Mapper::PRMS);
            GET_TOML_EXTERN(subconf, float, max_stay_frac, Mapper::PRMS);
            GET_TOML_EXTERN(subconf, float, min_seed_prob, Mapper::PRMS);

            GET_TOML_EXTERN(subconf, u32, max_chunks, Mapper::PRMS);
            GET_TOML_EXTERN(subconf, u32, evt_buffer_len, Mapper::PRMS);
            GET_TOML_EXTERN(subconf, u16, evt_batch_size, Mapper::PRMS);
            GET_TOML_EXTERN(subconf, float, evt_timeout, Mapper::PRMS);
            GET_TOML_EXTERN(subconf, float, chunk_size, Mapper::PRMS);
            GET_TOML_EXTERN(subconf, float, max_chunk_wait, Mapper::PRMS);
        }

        if (conf.contains("seed_tracker")) {
            const auto subconf = toml::find(conf, "seed_tracker");

            GET_TOML_EXTERN(subconf, float, min_mean_conf, SeedTracker::PRMS);
            GET_TOML_EXTERN(subconf, float, min_top_conf, SeedTracker::PRMS);
            GET_TOML_EXTERN(subconf, u32, min_aln_len, SeedTracker::PRMS);
        }

        if (conf.contains("event_detector")) {
            const auto subconf = toml::find(conf, "event_detector");
            GET_TOML_EXTERN(subconf, float, min_mean, EventDetector::PRMS);
            GET_TOML_EXTERN(subconf, float, max_mean, EventDetector::PRMS);
            GET_TOML_EXTERN(subconf, float, threshold1, EventDetector::PRMS);
            GET_TOML_EXTERN(subconf, float, threshold2, EventDetector::PRMS);
            GET_TOML_EXTERN(subconf, float, peak_height, EventDetector::PRMS);
            GET_TOML_EXTERN(subconf, u32, window_length1, EventDetector::PRMS);
            GET_TOML_EXTERN(subconf, u32, window_length2, EventDetector::PRMS);
        }
    }

    Conf() {}

    Conf(const std::string &conf_file) {
        load_conf(conf_file);
    }

    GET_SET(u16, threads)
    GET_SET(u16, num_channels)
    GET_SET(std::string, bwa_prefix)
    GET_SET(std::string, kmer_model)
    GET_SET(std::string, index_preset)

    GET_SET_EXTERN(std::string, fast5_prms, fast5_list)
    GET_SET_EXTERN(std::string, fast5_prms, fast5_filter)
    GET_SET_EXTERN(u32, fast5_prms, max_reads)
    GET_SET_EXTERN(u32, fast5_prms, max_buffer)

    GET_SET_EXTERN(std::string, realtime_prms, host)
    GET_SET_EXTERN(u16, realtime_prms, port)
    GET_SET_EXTERN(float, realtime_prms, duration)
    GET_SET_EXTERN(RealtimeParams::ActiveChs, realtime_prms, active_chs)
    GET_SET_EXTERN(RealtimeParams::Mode, realtime_prms, realtime_mode)

    GET_SET_EXTERN(u32, Mapper::PRMS, max_events)
    GET_SET_EXTERN(u32, Mapper::PRMS, max_chunks)
    GET_SET_EXTERN(float, Mapper::PRMS, chunk_size)

    #ifdef PYBIND
    static void add_pybind_vars(pybind11::class_<Conf> &c) {
        DEFPRP(threads)
        DEFPRP(num_channels)
        DEFPRP(bwa_prefix)
        DEFPRP(kmer_model)
        DEFPRP(index_preset)

        DEFPRP(fast5_list)
        DEFPRP(fast5_filter)
        DEFPRP(max_reads)
        DEFPRP(max_buffer)

        DEFPRP(host)
        DEFPRP(port)
        DEFPRP(duration)
        DEFPRP(active_chs)
        DEFPRP(realtime_mode)

        DEFPRP(max_events)
        DEFPRP(max_chunks)
        DEFPRP(chunk_size)
    }
    #endif

    void load_index_params() {
        
        std::ifstream param_file(bwa_prefix + INDEX_SUFF);
        std::string param_line;

        char *index_preset_c = (char *) index_preset.c_str();
        Mapper::PRMS.prob_threshes.resize(64);

        while (getline(param_file, param_line)) {
            char *param_name = strtok((char *) param_line.c_str(), "\t");
            char *fn_str = strtok(NULL, "\t");
            //char *path_str = strtok(NULL, "\t");
            if ( !index_preset.empty() && strcmp(param_name, index_preset_c) ) {
                    continue;
            }

            u8 fmbin = Mapper::PRMS.prob_threshes.size() - 1;
            char *prob_str;
            while ( (prob_str = strtok(fn_str, ",")) != NULL ) {
                fn_str = NULL;
                Mapper::PRMS.prob_threshes[fmbin] = atof(prob_str);
                fmbin--;
            }

            for (;fmbin < Mapper::PRMS.prob_threshes.size(); fmbin--) {
                Mapper::PRMS.prob_threshes[fmbin] = Mapper::PRMS.prob_threshes[fmbin+1];
            }
        }
    }

    u16 threads, num_channels;
    std::string bwa_prefix;
    std::string kmer_model;
    std::string index_preset;

    Fast5Params fast5_prms;
    RealtimeParams realtime_prms;
};

#endif
