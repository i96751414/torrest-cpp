# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Added `/torrents/{infoHash}/items` endpoint to allow getting items by folders.
- Added `prefix` query parameter to get files API. By providing a prefix, one can filter results by path.
- Added `prefix` query parameter to torrent download/stop API. By providing a prefix, one can filter which files are downloaded.

## [v0.0.6] - 24/09/2024

### Fixed

- Fix listen_interfaces port matching

### Changed

- Updated nlohmann JSON to v3.11.3
- Updated spdlog to v1.14.1
- Updated openssl to 1.1.1w
- Updated Boost to 1.86.0

### Added

- Add support to `write_mode` setting. It supports `0: auto`, `1: pwrite`, `2: mmap_write` and `3:force_pread_pwrite`
  values, defaulting to `0: auto`.
- Automatically find interfaces addresses when `listen_interfaces` is set to `auto:<interface-name[:port]>` (for
  instance, `auto:wlan0` or `auto:wlan0:61000,wlan1`). Multiple values are supported and must be comma separated.
- Add version method to libtorrest and bindings

## [v0.0.5] - 03/01/2024

### Fixed

- Always include "Accept-Ranges" header in serve responses.
- Update to libtorrent 1.2.19

### Changed

- Make info hash case-insensitive

## [v0.0.4] - 30/04/2023

### Changed

- Allow setting a custom user-agent
- Downgrade to last libtorrent v1.2 (1.2.18)

## [v0.0.3] - 08/04/2023

### Added

- Allow building different major versions of libtorrent.
- New safer read piece mechanism (around piece alerts). Legacy read piece is still supported for libtorrent v1.
- Internal piece cache with a configurable piece expiration (defaults to 5 seconds).
- Allow building as a shared library. The below methods are now available:
    - int start_with_env()
    - int start(uint16_t port, String settings_path, int global_log_level)
    - void stop()
    - void clear_logging_sinks()
    - void add_logging_stdout_sink()
    - void add_logging_file_sink(String file_path, bool truncate)
    - void add_logging_callback_sink(log_callback_fn callback)
- C and Python bindings to the shared library.
- Environment variables support.
- New options to command line invocation: log pattern and log path.

### Changed

- Update libtorrent to v2.0.8.
- Update nlohmann json to v3.11.0.
- Improve piece wait timeout so it is always respected.
- Updated all API verbs for correctness.
- Only set max single core connections if connections_limit is not explicitly set (arm devices only).
- Improved proxy settings validation.

### Fixed

- Correctly close HTTP connections on server shutdown.
- Fix query parameters default values.

## [v0.0.2] - 05/06/2022

Hotfix release.

### Fixed

- Fix server error DTOs.
- Do not handle duplicate torrents as error when loading torrents.
- Disable atomic linking on oatpp for android and linux builds.

## [v0.0.1] - 29/05/2022

First release.

### Added

- Libtorrent 1.2.16 support.
- Multi-platform support with cross build environments: android-arm, android-arm64, android-x64, android-x86,
  darwin-x64, linux-armv7, linux-arm64, linux-x64, linux-x86, windows-x64 and windows-x86.
- REST API with swagger containing settings, service, torrents and files management endpoints.
- Configurable service with a comprehensive list of settings.
- Buffering functionality to prioritize certain pieces making them available first.

[Unreleased]: https://github.com/i96751414/torrest-cpp/compare/v0.0.6...master

[v0.0.6]: https://github.com/i96751414/torrest-cpp/compare/v0.0.5...v0.0.6

[v0.0.5]: https://github.com/i96751414/torrest-cpp/compare/v0.0.4...v0.0.5

[v0.0.4]: https://github.com/i96751414/torrest-cpp/compare/v0.0.3...v0.0.4

[v0.0.3]: https://github.com/i96751414/torrest-cpp/compare/v0.0.2...v0.0.3

[v0.0.2]: https://github.com/i96751414/torrest-cpp/compare/v0.0.1...v0.0.2

[v0.0.1]: https://github.com/i96751414/torrest-cpp/commits/v0.0.1