from asyncio import Future, Protocol, Transport, get_running_loop, wait_for
from asyncio.subprocess import Process


class TestingServerProtocol(Protocol):
    SERVER_HELLO = b'Hello Client\n'

    def __init__(
        self,
        on_finished: Future,
        on_say_server_hello: Future,
        on_got_client_hello: Future,
        on_connected: Future,
        on_conn_lost: Future
    ):
        self.did_hello = False
        self.on_connected = on_connected
        self.on_say_server_hello = on_say_server_hello
        self.on_got_client_hello = on_got_client_hello
        self.on_finished = on_finished
        self.on_conn_lost = on_conn_lost

    def connection_made(
        self,
        transport: Transport  # type: ignore[override]
    ) -> None:
        self.on_connected.set_result(True)
        self.transport = transport
        self.transport.write(self.SERVER_HELLO)
        self.did_hello = True
        self.on_say_server_hello.set_result(True)

    def connection_lost(self, exc: Exception | None) -> None:
        assert exc is None  # assert protocol received eof
        self.transport.close()

    def data_received(self, data: bytes) -> None:
        if not self.did_hello:
            print(data)
            self.on_got_client_hello.set_result(True)

    def eof_received(self) -> bool | None:
        self.transport.write_eof()
        self.on_finished.set_result(True)

        return True


async def test_client(host, port, client_factory):
    loop = get_running_loop()
    on_connected = loop.create_future()
    on_say_server_hello = loop.create_future()
    on_got_client_hello = loop.create_future()
    on_finished = loop.create_future()
    on_conn_lost = loop.create_future()

    server = await loop.create_server(
        lambda: TestingServerProtocol(
            on_connected=on_connected,
            on_say_server_hello=on_say_server_hello,
            on_got_client_hello=on_got_client_hello,
            on_finished=on_finished,
            on_conn_lost=on_conn_lost
        ),
        host, port
    )

    async with server:
        await wait_for(server.start_serving(), 1.0)
        client: Process = await client_factory()
        # Wait to be connected.
        await wait_for(on_connected, 1.0)

        # Wait for server to say hello.
        await wait_for(on_say_server_hello, 1.0)

        # Check message that client got.
        assert client.stdout is not None
        assert await client.stdout.readline() == b'Hello Client'

        # Wait for server to get hello that client sends.
        await wait_for(on_got_client_hello, 1.0)

        # Wait for client to be finished.
        assert await client.wait() == 0

        # Wait for server to be finished.
        await wait_for(on_finished, 1.0)

        # Wait for server to connection lost.
        await wait_for(on_conn_lost, 1.0)

        server.close()
        await wait_for(server.wait_closed(), 1.0)
