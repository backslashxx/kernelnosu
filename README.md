##

A simple `su` binary for KernelSU that uses a special KernelSU `prctl` to get root.
The purpose is to replace built-in sucompat, which uses detectable kprobe hooks.


Requires: [kernel: core_hook: add support for KernelNoSU](https://github.com/backslashxx/KernelSU/commit/5084a80ec4b6de56ea080628af5de15a54d8fea9)

- original work by nampud


