# Native ARM64 guest tests

Build and run on Linux ARM64 with 4 KiB host pages. Enable `BUILD_TESTING` and
`LICHII_TESTS` when configuring Lichii, then run:

```sh
cmake --build build --target arm_cpu_guest_tests
ctest --test-dir build -L '^arm_cpu_guest$' --output-on-failure --no-tests=error
```

The runner writes an assembly-built, keyless NRO to a private temporary directory
and boots it through the normal application loader, with null graphics and audio.
The NCE and Dynarmic cases use the same guest code to check SVC arguments and
results, TLS across thread switches, the guest counter and sleep, exclusive
reservations, and involuntary preemption. The NCE-only fault cases require
an unmapped load and a store to a read-only page to suspend the faulting worker
without executing its post-fault marker. A backend check combines the loader's
trampoline state with a TLS marker execution wrote into the NCE thread state.
The native cases check both physical and virtual counter reads against
`GetSystemTick` at the 19.2 MHz and 31.25 MHz guest frequencies.
They also test a delayed fault after a cache mark is cleared, without lifting
the guest's read-only protection or retrying an unrelated access.

The null renderer cannot exercise a tracked write trap. For a separate Vulkan
integration check, back two GPU device addresses with the same guest page.
Have the GPU write a pattern through the first alias and fence it. Verify the
page is cache-marked before allowing the guest to overwrite one word. Re-read
through the second GPU alias and check that the guest's word and the adjacent
GPU-written words survive. Require the guest to finish under a watchdog, and
record the page's cache-mark transition so an unarmed page cannot pass.

The benchmark is opt-in. Run the following both ways on the same native ARM64
host and with the same affinity, alternating the order between pairs:

```sh
build/bin/arm_cpu_guest_tests nce --benchmark
build/bin/arm_cpu_guest_tests dynarmic --benchmark
```

Each loop runs once before its measured sample. The reported times use the
guest counter backed by the host's ARM virtual counter, and exclude emulator
startup. Report the spread along with the result. Do not use cross-architecture
emulation for timing, or treat these loops as game performance estimates.
