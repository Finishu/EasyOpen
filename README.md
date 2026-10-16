\# EasyOpen



A Windows kernel-mode driver project that enables to access any process with `PROCESS\_ALL\_ACCESS`, developed for educational purposes and authorized security research.



\## Requirements



\- Windows operating system.

\- Visual Studio with the Windows Driver Kit (WDK).



\## Installation



Build the driver using the appropriate WDK configuration

Use a supported driver installation and loading method. Do not deploy the driver on systems without authorization.



\## Security Notice



This project is intended for educational purposes and authorized testing only. Granting `PROCESS\_ALL\_ACCESS` can expose sensitive process memory and capabilities. Restrict access to the device interface and avoid using the driver on production systems.



\## License



Licensed under the MIT License. See \[LICENSE](LICENSE) for details.



Copyright (c) 2026 Thiago.

