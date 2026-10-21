# PS5 MicroPython v0.1.0

First native homebrew development build, not yet hardware-tested.

- Official MicroPython C embedding core, parser, compiler and GC.
- Native PS5 controller-operated script selection and output view.
- Built-in Hello, Fibonacci and Collections Python examples.
- Custom script scanning from the writable scripts directory.
- Persistent execution logs and native launcher artwork.

Known limitations: scripts execute on the UI thread (long or infinite
scripts may freeze the interface); minimal MicroPython configuration, no
CPython packages and no on-screen code editor. Hardware compatibility
including firmware 5.10 remains unverified.
