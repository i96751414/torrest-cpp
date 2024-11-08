# Rest API

## Version: 0.0.7

C++ implementation of Torrest: a torrent streaming engine with a REST api

#### Swagger

<details>
<summary><code>GET</code> <code><b>/swagger/ui</b></code> <code>Access swagger interface</code></summary>

##### Description

Access swagger interface.

##### Responses

| Code | Description |
|------|-------------|
| 200  | OK          |

</details>
<details>
<summary><code>GET</code> <code><b>/api-docs/oas-3.0.0.json</b></code> <code>Get API definition</code></summary>

##### Description

Get openapi JSON API definition.

##### Responses

| Code | Description |
|------|-------------|
| 200  | OK          |

</details>

------------------------------------------------------------------------------------------

#### Settings

<details>
<summary><code>GET</code> <code><b>/settings</b></code> <code>Get current settings</code></summary>

##### Description

Get settings as a JSON object.

##### Responses

| Code | Description |
|------|-------------|
| 200  | OK          |

</details>
<details>
<summary><code>PUT</code> <code><b>/settings</b></code> <code>Set settings</code></summary>

##### Description

Set settings given the provided JSON object.

##### Parameters

| Name  | Located in | Description    | Required | Schema  |
|-------|------------|----------------|----------|---------|
| reset | query      | Reset torrents | No       | boolean |

##### Responses

| Code | Description           |
|------|-----------------------|
| 200  | OK                    |
| 400  | Bad Request           |
| 500  | Internal Server Error |

</details>

------------------------------------------------------------------------------------------

#### Service

<details>
<summary><code>PUT</code> <code><b>/shutdown</b></code> <code>Shutdown</code></summary>

##### Description

Shutdown the application.

##### Responses

| Code | Description |
|------|-------------|
| 200  | OK          |

</details>
<details>
<summary><code>GET</code> <code><b>/status</b></code> <code>Status</code></summary>

##### Description

Get the service status.

##### Responses

| Code | Description |
|------|-------------|
| 200  | OK          |

</details>
<details>
<summary><code>PUT</code> <code><b>/pause</b></code> <code>Pause</code></summary>

##### Description

Pause the service.

##### Responses

| Code | Description |
|------|-------------|
| 200  | OK          |

</details>
<details>
<summary><code>PUT</code> <code><b>/resume</b></code> <code>Resume</code></summary>

##### Description

Resume the service.

##### Responses

| Code | Description |
|------|-------------|
| 200  | OK          |

</details>
<details>
<summary><code>POST</code> <code><b>/add/magnet</b></code> <code>Add magnet</code></summary>

##### Description

Add magnet to the service.

##### Parameters

| Name             | Located in | Description                        | Required | Schema  |
|------------------|------------|------------------------------------|----------|---------|
| uri              | query      | The magnet URI                     | Yes      | string  |
| download         | query      | Start download after adding magnet | No       | boolean |
| ignore_duplicate | query      | Ignore if duplicate                | No       | boolean |

##### Responses

| Code | Description           |
|------|-----------------------|
| 200  | OK                    |
| 400  | Bad Request           |
| 500  | Internal Server Error |

</details>
<details>
<summary><code>POST</code> <code><b>/add/torrent</b></code> <code>Add torrent file</code></summary>

##### Description

Add torrent file to the service.

##### Parameters

| Name             | Located in | Description                         | Required | Schema  |
|------------------|------------|-------------------------------------|----------|---------|
| download         | query      | Start download after adding torrent | No       | boolean |
| ignore_duplicate | query      | Ignore if duplicate                 | No       | boolean |

##### Responses

| Code | Description           |
|------|-----------------------|
| 200  | OK                    |
| 400  | Bad Request           |
| 500  | Internal Server Error |

</details>

------------------------------------------------------------------------------------------

#### Torrents

<details>
<summary><code>GET</code> <code><b>/torrents</b></code> <code>List torrents</code></summary>

##### Description

List all torrents from service.

##### Parameters

| Name   | Located in | Description         | Required | Schema  |
|--------|------------|---------------------|----------|---------|
| status | query      | Get torrents status | No       | boolean |

##### Responses

| Code | Description |
|------|-------------|
| 200  | OK          |

</details>
<details>
<summary><code>DELETE</code> <code><b>/torrents/{infoHash}</b></code> <code>Remove torrent</code></summary>

##### Description

Remove torrent from service.

##### Parameters

| Name     | Located in | Description          | Required | Schema  |
|----------|------------|----------------------|----------|---------|
| infoHash | path       | Torrent info hash    | Yes      | string  |
| delete   | query      | Delete torrent files | No       | boolean |

##### Responses

| Code | Description |
|------|-------------|
| 200  | OK          |
| 404  | Not Found   |

</details>
<details>
<summary><code>PUT</code> <code><b>/torrents/{infoHash}/resume</b></code> <code>Resume torrent</code></summary>

##### Description

Resume a paused torrent.

##### Parameters

| Name     | Located in | Description       | Required | Schema |
|----------|------------|-------------------|----------|--------|
| infoHash | path       | Torrent info hash | Yes      | string |

##### Responses

| Code | Description |
|------|-------------|
| 200  | OK          |
| 404  | Not Found   |

</details>
<details>
<summary><code>PUT</code> <code><b>/torrents/{infoHash}/pause</b></code> <code>Pause torrent</code></summary>

##### Description

Pause torrent from service.

##### Parameters

| Name     | Located in | Description       | Required | Schema |
|----------|------------|-------------------|----------|--------|
| infoHash | path       | Torrent info hash | Yes      | string |

##### Responses

| Code | Description |
|------|-------------|
| 200  | OK          |
| 404  | Not Found   |

</details>
<details>
<summary><code>GET</code> <code><b>/torrents/{infoHash}/info</b></code> <code>Torrent info</code></summary>

##### Description

Get torrent info.

##### Parameters

| Name     | Located in | Description       | Required | Schema |
|----------|------------|-------------------|----------|--------|
| infoHash | path       | Torrent info hash | Yes      | string |

##### Responses

| Code | Description |
|------|-------------|
| 200  | OK          |
| 404  | Not Found   |

</details>
<details>
<summary><code>GET</code> <code><b>/torrents/{infoHash}/status</b></code> <code>Torrent status</code></summary>

##### Description

Get torrent status.

##### Parameters

| Name     | Located in | Description       | Required | Schema |
|----------|------------|-------------------|----------|--------|
| infoHash | path       | Torrent info hash | Yes      | string |

##### Responses

| Code | Description |
|------|-------------|
| 200  | OK          |
| 404  | Not Found   |

</details>
<details>
<summary><code>GET</code> <code><b>/torrents/{infoHash}/files</b></code> <code>Torrent files</code></summary>

##### Description

Get torrent files.

##### Parameters

| Name     | Located in | Description             | Required | Schema  |
|----------|------------|-------------------------|----------|---------|
| infoHash | path       | Torrent info hash       | Yes      | string  |
| prefix   | query      | Filter result by prefix | No       | string  |
| status   | query      | Get files status        | No       | boolean |

##### Responses

| Code | Description           |
|------|-----------------------|
| 200  | OK                    |
| 404  | Not Found             |
| 500  | Internal Server Error |

</details>
<details>
<summary><code>GET</code> <code><b>/torrents/{infoHash}/items</b></code> <code>Torrent items</code></summary>

##### Description

Get torrent items.

##### Parameters

| Name     | Located in | Description       | Required | Schema  |
|----------|------------|-------------------|----------|---------|
| infoHash | path       | Torrent info hash | Yes      | string  |
| folder   | query      | Items folder      | No       | string  |
| status   | query      | Get files status  | No       | boolean |

##### Responses

| Code | Description           |
|------|-----------------------|
| 200  | OK                    |
| 404  | Not Found             |
| 500  | Internal Server Error |

</details>
<details>
<summary><code>PUT</code> <code><b>/torrents/{infoHash}/download</b></code> <code>Start download</code></summary>

##### Description

Download torrent files.

##### Parameters

| Name     | Located in | Description              | Required | Schema |
|----------|------------|--------------------------|----------|--------|
| infoHash | path       | Torrent info hash        | Yes      | string |
| prefix   | query      | Download files by prefix | No       | string |

##### Responses

| Code | Description           |
|------|-----------------------|
| 200  | OK                    |
| 404  | Not Found             |
| 500  | Internal Server Error |

</details>
<details>
<summary><code>PUT</code> <code><b>/torrents/{infoHash}/stop</b></code> <code>Stop download</code></summary>

##### Description

Stop downloading torrent files.

##### Parameters

| Name     | Located in | Description                   | Required | Schema |
|----------|------------|-------------------------------|----------|--------|
| infoHash | path       | Torrent info hash             | Yes      | string |
| prefix   | query      | Stop files download by prefix | No       | string |

##### Responses

| Code | Description           |
|------|-----------------------|
| 200  | OK                    |
| 404  | Not Found             |
| 500  | Internal Server Error |

</details>

------------------------------------------------------------------------------------------

#### Files

<details>
<summary><code>PUT</code> <code><b>/torrents/{infoHash}/files/{file}/download</b></code> <code>Download file</code></summary>

##### Description

Download file from torrent.

##### Parameters

| Name     | Located in | Description       | Required | Schema  |
|----------|------------|-------------------|----------|---------|
| infoHash | path       | Torrent info hash | Yes      | string  |
| file     | path       | File index        | Yes      | integer |
| buffer   | query      | Start buffering   | No       | boolean |

##### Responses

| Code | Description |
|------|-------------|
| 200  | OK          |
| 400  | Bad Request |
| 404  | Not Found   |

</details>
<details>
<summary><code>PUT</code> <code><b>/torrents/{infoHash}/files/{file}/stop</b></code> <code>Stop file download</code></summary>

##### Description

Stop file download from torrent.

##### Parameters

| Name     | Located in | Description       | Required | Schema  |
|----------|------------|-------------------|----------|---------|
| infoHash | path       | Torrent info hash | Yes      | string  |
| file     | path       | File index        | Yes      | integer |

##### Responses

| Code | Description |
|------|-------------|
| 200  | OK          |
| 400  | Bad Request |
| 404  | Not Found   |

</details>
<details>
<summary><code>GET</code> <code><b>/torrents/{infoHash}/files/{file}/info</b></code> <code>Get file info</code></summary>

##### Description

Get file info from torrent.

##### Parameters

| Name     | Located in | Description       | Required | Schema  |
|----------|------------|-------------------|----------|---------|
| infoHash | path       | Torrent info hash | Yes      | string  |
| file     | path       | File index        | Yes      | integer |

##### Responses

| Code | Description |
|------|-------------|
| 200  | OK          |
| 400  | Bad Request |
| 404  | Not Found   |

</details>
<details>
<summary><code>GET</code> <code><b>/torrents/{infoHash}/files/{file}/status</b></code> <code>Get file status</code></summary>

##### Description

Get file status from torrent.

##### Parameters

| Name     | Located in | Description       | Required | Schema  |
|----------|------------|-------------------|----------|---------|
| infoHash | path       | Torrent info hash | Yes      | string  |
| file     | path       | File index        | Yes      | integer |

##### Responses

| Code | Description |
|------|-------------|
| 200  | OK          |
| 400  | Bad Request |
| 404  | Not Found   |

</details>
<details>
<summary><code>GET</code> <code><b>/torrents/{infoHash}/files/{file}/serve</b></code> <code>Serve file</code></summary>

##### Description

Serve file from torrent.

##### Parameters

| Name     | Located in | Description       | Required | Schema  |
|----------|------------|-------------------|----------|---------|
| infoHash | path       | Torrent info hash | Yes      | string  |
| file     | path       | File index        | Yes      | integer |

##### Responses

| Code | Description     |
|------|-----------------|
| 206  | Partial Content |
| 400  | Bad Request     |
| 404  | Not Found       |

</details>
<details>
<summary><code>HEAD</code> <code><b>/torrents/{infoHash}/files/{file}/serve</b></code> <code>Serve file</code></summary>

##### Description

Serve file from torrent.

##### Parameters

| Name     | Located in | Description       | Required | Schema  |
|----------|------------|-------------------|----------|---------|
| infoHash | path       | Torrent info hash | Yes      | string  |
| file     | path       | File index        | Yes      | integer |

##### Responses

| Code | Description |
|------|-------------|
| 200  | OK          |
| 400  | Bad Request |
| 404  | Not Found   |

</details>

------------------------------------------------------------------------------------------
