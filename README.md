##

A simple `su` binary for KernelSU that uses a special KernelSU interfaces to get root.
The purpose is to replace built-in sucompat with a real su binary.


### Requires: 
- [KernelSU 22004](https://github.com/tiann/KernelSU/commit/562a3b9be795c7fc9ffc5802e24afbb07b4ae29a) or newer
- OR if you are on legacy prctl interface: [kernel: core_hook: add support for KernelNoSU](https://github.com/tiann/KernelSU/commit/5084a80ec4b6de56ea080628af5de15a54d8fea9)


#### Credits:
- original work by nampud


