from asyncio import Future, get_running_loop, Protocol, timeout, Transport
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
    on_say_hello = loop.create_future()
    on_got_hello = loop.create_future()
    on_finished = loop.create_future()
    on_conn_lost = loop.create_future()

    server = await loop.create_server(
        lambda: TestingServerProtocol(
            on_connected=on_connected,
            on_say_hello=on_say_hello,
            on_got_hello=on_got_hello,
            on_finished=on_finished,
            on_conn_lost=on_conn_lost
        ),
        host, port
    )

    async with timeout(5):
        async with server:
            await server.start_serving()
            client: Process = await client_factory()
            # Wait to be connected
            await on_connected

            # Wait server say hello
            await on_say_hello

            # Check message that client got
            assert client.stdout is not None
            assert await client.stdout.readline() == b'Hello Client'

            # Wait server got hello(client sends)
            await on_got_hello

            # Wait client ends
            assert await client.wait() == 0

            # Wait to server finished
            await on_finished

            # Wait to connection lost
            await on_conn_lost

            server.close()
            await server.wait_closed()
