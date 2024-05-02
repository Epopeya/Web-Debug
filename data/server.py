import asyncio
import websockets
import json
import random

async def send_data(ws, path):
    while True:
        await ws.send(json.dumps({
            "pos": [0, -1000],
            "rot": 0,
            "trot": 0,
            "msgs": ["Message 1", "Message 2", "Message 3"],
            "bat": 3.7,
            "route": [[1000, -1000], [1000, 1000], [-1000, 1000], [-1000, -1000]],
            "points": [[random.gauss(0, 100), random.gauss(-500, 5)], [random.gauss(0, 100), random.gauss(-1500, 5)]],
        }))
        await asyncio.sleep(0.1)


async def main():
    async with websockets.serve(send_data, "localhost", 1234):
        await asyncio.Future()

asyncio.run(main())
