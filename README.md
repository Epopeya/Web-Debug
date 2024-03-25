# Communications chip
This repo contains all the necessary files for the auxiliary debugging chip

## Protocol
We communicate through WebSockets using JSON packets. The schema that defines such packets can be found [here](schema.json)

## Filesystem
We currently use [LittleFS](https://github.com/littlefs-project/littlefs) for storing the client page (which uses static HTML with some Javascript for the WebSocket connection). All client files are under `data/`

See also:
- [Filesystem](https://docs.platformio.org/en/latest/platforms/espressif32.html#uploading-files-to-file-system)
- [Custom partition tables](https://docs.platformio.org/en/latest/platforms/espressif32.html#partition-tables)
