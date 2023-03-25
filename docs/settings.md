# Settings

Below are described all the settings supported by torrest. One can get/set them through the `/settings` endpoint.

| name                   | type    | default                              | description                                                                                           |
|------------------------|---------|--------------------------------------|-------------------------------------------------------------------------------------------------------|
| listen_port            | int     | 6889                                 | The bittorrent service listen port                                                                    |
| listen_interfaces      | string  | 0.0.0.0:listen_port,[::]:listen_port | A comma-separated list of (IP or device name, port) pairs                                             |
| outgoing_interfaces    | string  |                                      | A comma-separated list of IP addresses and interface names                                            |
| disable_dht            | boolean | false                                | Disable the dht node                                                                                  |
| disable_upnp           | boolean | false                                | Disable the UPnP service                                                                              |
| disable_natpmp         | boolean | false                                | Disable the NAT-PMP service                                                                           |
| disable_lsd            | boolean | false                                | Disable the Local Service Discovery                                                                   |
| download_path          | string  | downloads                            | The download path                                                                                     |
| torrents_path          | string  | downloads/torrents                   | The torrents, magnets and fast-resume files download path                                             |
| user_agent             | int     | 0                                    | The client identification to the tracker                                                              |
| session_save           | int     | 30                                   | The time between save resume data calls                                                               |
| tuned_storage          | boolean | false                                | Whether to use tuned storage settings                                                                 |
| check_available_space  | boolean | true                                 | Whether to check available space on torrent download                                                  |
| connections_limit      | int     | 0                                    | The connections limit (if 0, torrest chooses what to set)                                             |
| limit_after_buffering  | boolean | false                                | Whether to only apply download/upload rate limits when not buffering                                  |
| max_download_rate      | int     | 0                                    | Max download rate, in bytes per second (0 means unlimited)                                            |
| max_upload_rate        | int     | 0                                    | Max upload rate, in bytes per second (0 means unlimited)                                              |
| share_ratio_limit      | int     | 0 [200]                              | The share ratio limit (bytes up / bytes down)                                                         |
| seed_time_ratio_limit  | int     | 0 [700]                              | The seed time ratio limite (seconds as seed / seconds as downloader)                                  |
| seed_time_limit        | int     | 0 [24 * 60 * 60]                     | The limit on the time a torrent has been an active seed                                               |
| active_downloads_limit | int     | 3                                    | How many downloading torrents the queuing mechanism allows                                            |
| active_seeds_limit     | int     | 5                                    | How many seeding torrents the queuing mechanism allows                                                |
| active_checking_limit  | int     | 1                                    | The limit of simultaneous checking torrents                                                           |
| active_dht_limit       | int     | 88                                   | The max number of torrents to announce to the DHT                                                     |
| active_tracker_limit   | int     | 1600                                 | The max number of torrents to announce to their trackers                                              |
| active_lsd_limit       | int     | 60                                   | The max number of torrents to announce to the local network over the local service discovery protocol |
| active_limit           | int     | 500                                  | Hard limit on the number of active torrents                                                           |
| encryption_policy      | int     | 0                                    | The encryption policy                                                                                 |
| proxy.type             | int     |                                      | The proxy type                                                                                        |
| proxy.port             | int     |                                      | The proxy port                                                                                        |
| proxy.hostname         | string  |                                      | The proxy hostname                                                                                    |
| proxy.username         | string  |                                      | The proxy username                                                                                    |
| proxy.passwrod         | string  |                                      | The proxy password                                                                                    |
| buffer_size            | int     | 20 * 1024 * 1024                     | The buffer size to consider when prioritizing pieces                                                  |
| piece_wait_timeout     | int     | 60                                   | The piece wait timeout (when serving files)                                                           |
| piece_expiration       | int     | 5                                    | How much time to keep an unused piece in memory (unused on legacy read piece)                         |
| service_log_level      | int     | 2                                    | The service log level                                                                                 |
| alerts_log_level       | int     | 5                                    | Alerts log level                                                                                      |
| api_log_level          | int     | 4                                    | The API log level                                                                                     |

## User agent options

| value | user-agent                                                |
|-------|-----------------------------------------------------------|
| 0     | torrest/<torrest-version> libtorrent/<libtorrent-version> |
| 1     | libtorrent/<libtorrent-version>                           |
| 2     | libtorrent (Rasterbar) 1.1.0                              |
| 3     | BitTorrent 7.5.0                                          |
| 4     | BitTorrent 7.4.3                                          |
| 5     | µTorrent 3.4.9                                            |
| 6     | µTorrent 3.2.0                                            |
| 7     | µTorrent 2.2.1                                            |
| 8     | Transmission 2.92                                         |
| 9     | Deluge 1.3.6.0                                            |
| 10    | Deluge 1.3.12.0                                           |
| 11    | Vuze 5.7.3.0                                              |

## Encryption policy

| value | encryption | description                                                                  |
|-------|------------|------------------------------------------------------------------------------|
| 0     | Enabled    | Encrypted connections are enabled, but non-encrypted connections are allowed |
| 1     | Disabled   | Only non-encrypted connections are allowed                                   |
| 2     | Forced     | Only encrypted connections are allowed                                       |

## Proxy type

| value | type                 |
|-------|----------------------|
| 0     | none                 |
| 1     | SOCKS4               |
| 2     | SOCKS5               |
| 3     | SOCKS5 with password |
| 4     | HTTP                 |
| 5     | HTTP with password   |
| 6     | i2p SAM proxy        |

## Log level

| value | level    |
|-------|----------|
| 0     | trace    |
| 1     | debug    |
| 2     | info     |
| 3     | warning  |
| 4     | error    |
| 5     | critical |
| 6     | off      |