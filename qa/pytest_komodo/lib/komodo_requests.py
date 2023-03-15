# loosesly based on slick-bitcoinrpc https://github.com/barjomet/slick-bitcoinrpc/blob/master/slickrpc/rpc.py
import requests
from itertools import count
import ujson

DEFAULT_HTTP_TIMEOUT = 120
DEFAULT_RPC_PORT = 7777


class Proxy:
    _ids = count(0)

    def __init__(self, conf_dict: dict, timeout=DEFAULT_HTTP_TIMEOUT):
        self.timeout = timeout
        self.config = conf_dict
        if not self.config.get('rpc_port'):
            self.config['rpc_port'] = DEFAULT_RPC_PORT

    def __getattr__(self, req):
        _id = next(self._ids)

        def call(*params):
            post_val = {
                    'jsonrpc': '2.0',
                    'method': req,
                    'params': params,
                    'id': _id
                }
            url = 'http://%s:%s@%s:%s' % (self.config['rpc_user'], self.config['rpc_password'], self.config['rpc_ip'], self.config['rpc_port'])
            post_data = ujson.dumps(post_val)
            try:
                resp = requests.post(url, data=post_data, timeout=self.timeout).json()
                return resp['result']
            except ValueError:
                resp = str(requests.post(url, data=post_data, timeout=self.timeout).content())
                print(resp)
                return resp

        return call
