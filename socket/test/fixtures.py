from asyncio import create_subprocess_exec
from asyncio.subprocess import Process
from pathlib import Path
from typing import Callable, Coroutine

import pytest


@pytest.fixture
def host() -> str:
    return '127.0.0.1'


@pytest.fixture
def port(unused_tcp_port) -> int:
    return unused_tcp_port


@pytest.fixture
def client_binary_path(pytestconfig: pytest.Config) -> str:
    return str(Path(pytestconfig.invocation_params.dir) / 'bin' / 'client')


@pytest.fixture
def server_binary_path(pytestconfig: pytest.Config) -> str:
    return str(Path(pytestconfig.invocation_params.dir) / 'bin' / 'server')


@pytest.fixture
def client_factory(
    client_binary_path, host, port
) -> Callable[[], Coroutine[None, None, Process]]:
    return lambda: create_subprocess_exec(
        client_binary_path, "--host", host, "--port", str(port)
    )


@pytest.fixture
async def server(server_binary_path, host, port) -> Process:
    return await create_subprocess_exec(
        server_binary_path, "--host", host, "--port", str(port)
    )
