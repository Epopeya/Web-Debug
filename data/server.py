import asyncio
import websockets
import json

async def send_data(ws, path):
    while True:
        await ws.send(json.dumps({
            "pos": [-1200, -100],
            "rot": 20,
            "trot": -20,
            "msgs": ["Message 1", "Message 2", "Message 3"],
            "bat": 3.7,
            "route": [[-1000, 1000], [0, 800], [1000, 1000], [1000, -1000]],
            "points": [[100, 100], [-100, 100], [100, 0]],
        }))
        await asyncio.sleep(1)


async def main():
    async with websockets.serve(send_data, "localhost", 1234):
        await asyncio.Future()

asyncio.run(main())
