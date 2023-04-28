from asyncio import Future, get_running_loop, Protocol, timeout, Transport
from asyncio.subprocess import Process


class TestingClientProtocol(Protocol):
    CLIENT_HELLO = b'Hello Server\n'

    def __init__(
        self,
        on_connected: Future,
        on_got_server_hello: Future,
        on_say_client_hello: Future,
        on_conn_lost: Future
    ) -> None:
        self.on_connected = on_connected
        self.on_got_server_hello = on_got_server_hello
        self.on_say_client_hello = on_say_client_hello
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
    on_say_client_hello = loop.create_future()
    on_conn_lost = loop.create_future()

    async with timeout(5):

        task = loop.create_task(
            loop.create_connection(
                lambda: TestingClientProtocol(
                    on_connected=on_connected,
                    on_got_server_hello=on_got_server_hello,
                    on_say_client_hello=on_say_client_hello,
                    on_conn_lost=on_conn_lost
                ),
                host=host, port=port
            )
        )

        # Wait to be connected
        await on_connected

        # Wait to get server hello
        await on_got_server_hello

        # Wait to client say hello
        await on_say_client_hello

        # Check message that server got
        assert server.stdout is not None
        await server.stdout.readline() == b'Hello Server'

        # Wait to client close
        await on_conn_lost

        # wait server finished
        assert await server.wait() == 0

        # Wait task finished
        await task
