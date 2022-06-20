cpu_limit_run - run program with cpu limit
---

cpu_limit_run is a program that can runs a program with a cpu limit, without cgroup or root privileges.

```shell
# build
make
# run program a.out with cpu limit of 20%
./target/cpu_limit_run --percent 20 -- a.out arg1 arg2
```
