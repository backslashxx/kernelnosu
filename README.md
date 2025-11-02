##

A simple `su` binary for KernelSU that uses a special KernelSU `prctl` to get root.
The purpose is to replace built-in sucompat, which uses detectable kprobe hooks.


Requires: [kernel: supercall: lift permissions for KernelNoSU](https://github.com/tiann/KernelSU/commit/07ffa9f3224dea0ca461c421cc8d6d860956d49b)

- original work by nampud


