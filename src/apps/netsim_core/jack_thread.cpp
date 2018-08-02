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

