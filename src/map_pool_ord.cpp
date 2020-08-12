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

#include <thread>
#include <chrono>
#include "pdqsort.h"
#include "map_pool_ord.hpp"

MapPoolOrd::MapPoolOrd(Conf &conf)
    : fast5s_(conf.fast5_prms),
      pool_(conf),
      channels_empty_(false) {

    channels_.resize(conf.get_num_channels());
    chunk_idx_.resize(conf.get_num_channels());
}

void MapPoolOrd::add_fast5(const std::string &fname) {
    fast5s_.add_fast5(fname);
}


void MapPoolOrd::add_read(const std::string &id) {
    fast5s_.add_read(id);
}

void MapPoolOrd::load_fast5s() {
    std::cerr << "Loading fast5s\n";
    while(!fast5s_.empty()) {
        ReadBuffer read = fast5s_.pop_read();
        channels_[read.get_channel_idx()].push_back(read);
    }

    std::cerr << "Sorting reads\n";
    for (auto &ch : channels_) {
        pdqsort(ch.begin(), ch.end());
    }
}

std::vector<Paf> MapPoolOrd::update() {
    std::vector<Paf> ret;

    channels_empty_ = true;

    for (u32 i = 0; i < channels_.size(); i++) {
        if (channels_[i].empty()) continue;

        if (pool_.is_read_finished(channels_[i].front())) {
            //std::cout << "#end " << channels_[i].front().get_id() << "\n";
            //channels_[i].pop_front();
            //chunk_idx_[i] = 0;
            //
            //if (channels_[i].empty()) continue;
            chunk_idx_[i] = channels_[i].front().chunk_count();
        }

        channels_empty_ = false;

        ReadBuffer &r = channels_[i].front();
        Chunk chunk = r.get_chunk(chunk_idx_[i]);
        
        bool empty = chunk.empty();

        if (pool_.try_add_chunk(chunk)) {
            //std::cout << "#added "
            //          << chunk.get_id() << " "
            //          << chunk_idx_[i] << "\n";
            chunk_idx_[i]++;
        }
        
        //if (empty) {//|| chunk_idx_[i] >= r.chunk_count()) {
        //    //std::cout << "#ender " << r.get_id() << "\n";
        //    channels_[i].pop_front();
        //    chunk_idx_[i] = 0;
        //}
    }

    for (const MapResult &m : pool_.update()) {
        u16 i = std::get<0>(m)-1;
        u32 nm = std::get<1>(m);

        if (!channels_[i].empty() && channels_[i].front().get_number() == nm) {
            channels_[i].pop_front();
            chunk_idx_[i] = 0;
        }

        ret.push_back(std::get<2>(m));
    }

    return ret;
}


bool MapPoolOrd::running() {
    return !(channels_empty_ && pool_.all_finished());
}

void MapPoolOrd::stop() {
    return pool_.stop_all();
}
