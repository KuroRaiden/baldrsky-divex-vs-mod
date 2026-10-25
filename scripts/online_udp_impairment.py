"""Test-only loopback proxy: deterministic lag, jitter, loss and duplication.

Only accepts the two fixed local test ports. No driver or global network rules.
Clumsy remains the independent driver-based verification path.
"""
import argparse
import heapq
import random
import socket
import time

p = argparse.ArgumentParser()
p.add_argument("--lag", type=int, default=60)
p.add_argument("--jitter", type=int, default=20)
p.add_argument("--loss", type=int, default=5)
p.add_argument("--lifetime", type=int, default=50)
a = p.parse_args()
if not (0 <= a.lag <= 1000 and 0 <= a.jitter <= 500 and 0 <= a.loss <= 50 and 1 <= a.lifetime <= 60):
    raise SystemExit("Invalid test impairment settings")
rng = random.Random(42565331)
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(("127.0.0.1", 24766))
s.setblocking(False)
queue = []
serial = 0
end = time.monotonic() + a.lifetime
while time.monotonic() < end:
    now = time.monotonic()
    for _ in range(128):
        try:
            data, source = s.recvfrom(2048)
        except BlockingIOError:
            break
        if source[0] != "127.0.0.1" or source[1] not in (24764, 24765):
            continue
        if rng.randrange(100) < a.loss:
            continue
        destination = ("127.0.0.1", 24765 if source[1] == 24764 else 24764)
        delay = max(0, a.lag + rng.randint(-a.jitter, a.jitter)) / 1000
        serial += 1
        heapq.heappush(queue, (now + delay, serial, destination, data))
        if rng.randrange(100) < 2:
            serial += 1
            heapq.heappush(queue, (now + delay + .003, serial, destination, data))
    while queue and queue[0][0] <= now:
        _, _, destination, data = heapq.heappop(queue)
        s.sendto(data, destination)
    time.sleep(.001)
s.close()
