/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "fd-util.h"
#include "fuzz.h"
#include "fuzz-journald.h"
#include "journald-stream.h"

static int stream_fds[2] = PIPE_EBADF;

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
        Server s;
        StdoutStream *stream;
        int v;

        if (outside_size_range(size, 1, 65536))
                return 0;

        fuzz_setup_logging();

        assert_se(socketpair(AF_UNIX, SOCK_STREAM|SOCK_CLOEXEC|SOCK_NONBLOCK, 0, stream_fds) >= 0);
        dummy_server_init(&s, NULL, 0);
        assert_se(stdout_stream_install(&s, stream_fds[0], &stream) >= 0);
        assert_se(write(stream_fds[1], data, size) == (ssize_t) size);
        while (ioctl(stream_fds[0], SIOCINQ, &v) == 0 && v)
                sd_event_run(s.event, UINT64_MAX);
        if (s.n_stdout_streams)
                stdout_stream_destroy(stream);
        server_done(&s);
        stream_fds[1] = safe_close(stream_fds[1]);

        return 0;
}
