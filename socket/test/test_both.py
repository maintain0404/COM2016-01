from asyncio.subprocess import Process
from typing import Callable, Coroutine


async def test_both(
    client_factory: Callable[[], Coroutine[None, None, Process]],
    server: Process
):
    client: Process = await client_factory()

    # Wait client to print message from server.
    assert client.stdout is not None
    await client.stdout.readline() == b'Hello Client'

    # Wait server to print message from client.
    assert server.stdout is not None
    await server.stdout.readline() == b'Hello Server'

    # Wait to be closed
    assert await client.wait() == 0
    assert await server.wait() == 0
