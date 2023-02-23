import pytest
import json
import os
import time
from lib.komodo_requests import Proxy
from lib.pytest_util import OraclesCC, TokenCC, DiceCC, ChannelsCC, RewardsCC

@pytest.fixture(scope='session')
def proxy_connection():
    proxy_connected = []

    def _proxy_connection(node_params_dictionary):
        try:
            proxy = Proxy((node_params_dictionary), timeout=360)
            proxy_connected.append(proxy)
        except ConnectionAbortedError as e:
            raise Exception("Connection error! Probably no daemon on selected port. Error: ", e)
        return proxy

    yield _proxy_connection

    for each in proxy_connected:
        print("\nStopping created proxies...")
        time.sleep(10)  # time wait for tests to finish correctly before stopping daemon
        try:  # while using AuthServiceProxy, stop method results in connection aborted error
            each.stop()
        except ConnectionAbortedError as e:
            print(e)


@pytest.fixture(scope='session')
def test_params(proxy_connection):
    with open('nodesconfig.json', 'r') as f:
        params_dict = json.load(f)
    nodelist_raw = list(params_dict.keys())
    nodelist = []
    if os.environ['CLIENTS']:
        numclients = int(os.environ['CLIENTS'])
        for i in range(numclients):
            nodelist.append(nodelist_raw[i])
    else:
        nodelist_raw.pop()  # escape extra param in dict -- is_fresh_chain
        nodelist = nodelist_raw
    test_params = {}
    for node in nodelist:
        node_params = params_dict[node]
        rpc = proxy_connection(node_params)
        test_params.update({node: node_params})
        test_params[node].update({'rpc': rpc})
    return test_params


@pytest.fixture(scope='session')
def oracle_instance(test_params):
    oracle = OraclesCC(test_params)
    return oracle


@pytest.fixture(scope='session')
def token_instance(test_params):
    token = TokenCC(test_params)
    return token


@pytest.fixture(scope='session')
def dice_casino(test_params):
    dice = DiceCC(test_params)
    return dice


@pytest.fixture(scope='session')
def channel_instance(test_params):
    channel = ChannelsCC(test_params)
    return channel


@pytest.fixture(scope='session')
def rewards_plan(test_params):
    rewards_plan = RewardsCC(test_params)
    return rewards_plan
