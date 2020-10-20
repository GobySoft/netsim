// Copyright 2018:
//   GobySoft, LLC (2017-)
//   Massachusetts Institute of Technology (2017-)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//
//
// This file is part of the NETSIM Binaries.
//
// The NETSIM Binaries are free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// The NETSIM Binaries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with NETSIM.  If not, see <http://www.gnu.org/licenses/>.

#include "jack_thread.h"

int jack_process(jack_nframes_t nframes, void* arg)
{
    return ((JackThread*)arg)->jack_process(nframes);
}


void jack_shutdown (void *arg)
{
    ((JackThread*)arg)->jack_shutdown();
}

int jack_xrun(void* arg)
{
    return ((JackThread*)arg)->jack_xrun();
}

int jack_buffer_size_change(jack_nframes_t nframes, void *arg)
{
    return ((JackThread*)arg)->jack_buffer_size_change(nframes);
}

