from asyncio import Future, Protocol, Transport, get_running_loop, wait_for
from asyncio.subprocess import Process


class TestingClientProtocol(Protocol):
    CLIENT_HELLO = b'Hello Server\n'

    def __init__(
        self,
        on_connected: Future,
        on_got_server_hello: Future,
        on_conn_lost: Future
    ) -> None:
        self.on_connected = on_connected
        self.on_got_server_hello = on_got_server_hello
        self.on_conn_lost = on_conn_lost

    def connection_made(
        self,
        transport: Transport  # type: ignore[override]
    ) -> None:
        self.transport = transport
        self.on_connected.set_result(True)

    def connection_lost(self, exc: Exception | None) -> None:
        assert exc is None  # assert protocol close transport
        self.on_conn_lost.set_result(True)

    def data_received(self, data: bytes) -> None:
        self.on_got_server_hello.set_result(True)
        assert data == b'Hello Client\n'
        self.transport.write(self.CLIENT_HELLO)
        self.transport.write_eof()
        self.transport.close()

    # def eof_received(self) -> bool | None:
    #     self.transport.close()


async def test_server(host, port, server: Process):
    loop = get_running_loop()

    on_connected = loop.create_future()
    on_got_server_hello = loop.create_future()
    on_conn_lost = loop.create_future()

    task = loop.create_task(
        loop.create_connection(
            lambda: TestingClientProtocol(
                on_connected=on_connected,
                on_got_server_hello=on_got_server_hello,
                on_conn_lost=on_conn_lost
            ),
            host=host, port=port
        )
    )

    # Wait to be connected.
    await wait_for(on_connected, 1.0)

    # Wait for client to get server hello.
    await wait_for(on_got_server_hello, 3.0)
    # Check message that server got.
    assert server.stdout is not None
    await server.stdout.readline() == b'Hello Server'

    # Wait for client to be closed.
    await wait_for(on_conn_lost, 1.0)

    # Wait for server to be finished.
    assert await server.wait() == 0

    # Wait for task to be finished.
    await wait_for(task, 1.0)
