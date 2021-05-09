#!/usr/bin/env python3
# Copyright (c) 2021 SuperNET developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import pytest
from slickrpc.exc import RpcException as RPCError
from lib.pytest_util import validate_template, mine_and_waitconfirms, randomstring, randomhex, validate_raddr_pattern


@pytest.mark.usefixtures("proxy_connection", "test_params")
class TestTokenCCcalls:

    def test_tokencreate_list_info(self, token_instance):
        create_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'hex': {'type': 'string'},
                'error': {'type': 'string'}
            },
            'required': ['result']
        }

        list_schema = {
            'type': 'array',
            'items': {'type': 'string'}
        }

        info_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'tokenid': {'type': 'string'},
                'owner': {'type': 'string'},
                'name': {'type': 'string'},
                'supply': {'type': 'integer'},
                'description': {'type': 'string'},
                'version':  {'type': 'integer'},
                'IsMixed': {'type': 'string'}
            },
            'required': ['result']
        }

        token_instance.new_token(token_instance.rpc[0], schema=create_schema)

        res = token_instance.rpc[0].tokenlist()
        validate_template(res, list_schema)

        res = token_instance.rpc[0].tokeninfo(token_instance.base_token.get('tokenid'))
        validate_template(res, info_schema)

    def test_tokenaddress(self, token_instance):
        assetaddress_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                "GlobalPk Assets CC Address": {'type': 'string'},
                "GlobalPk Assets CC Balance": {'type': 'number'},
                "GlobalPk Assets Normal Address": {'type': 'string'},
                "GlobalPk Assets Normal Balance": {'type': 'number'},
                "GlobalPk Assets/Tokens CC Address": {'type': 'string'},
                "pubkey Assets CC Address": {'type': 'string'},
                "pubkey Assets CC Balance": {'type': 'number'},
                "mypk Assets CC Address": {'type': 'string'},
                "mypk Assets CC Balance": {'type': 'number'},
                "mypk Normal Address": {'type': 'string'},
                "mypk Normal Balance": {'type': 'number'}
            },
            'required': ['result']
        }

        tokenaddress_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                "GlobalPk Tokens CC Address": {'type': 'string'},
                "GlobalPk Tokens CC Balance": {'type': 'number'},
                "GlobalPk Tokens Normal Address": {'type': 'string'},
                "GlobalPk Tokens Normal Balance": {'type': 'number'},
                "pubkey Tokens CC Address": {'type': 'string'},
                "pubkey Tokens CC Balance": {'type': 'number'},
                "mypk Tokens CC Address": {'type': 'string'},
                "mypk Tokens CC Balance": {'type': 'number'},
                "mypk Normal Address": {'type': 'string'},
                "mypk Normal Balance": {'type': 'number'}
            },
            'required': ['result']
        }

        if not token_instance.base_token:
            token_instance.new_token(token_instance.rpc[1])

        res = token_instance.rpc[0].assetsaddress(token_instance.pubkey[0])
        validate_template(res, assetaddress_schema)

        res = token_instance.rpc[0].tokenaddress(token_instance.pubkey[0])
        validate_template(res, tokenaddress_schema)

    def test_tokentransfer_balance(self, token_instance):
        transfer_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'hex': {'type': 'string'}
            },
            'required': ['result']
        }

        balance_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'CCaddress': {'type': 'string'},
                'tokenid': {'type': 'string'},
                'balance': {'type': 'integer'}
            },
            'required': ['result']
        }

        if not token_instance.base_token:
            token_instance.new_token(token_instance.rpc[1])

        amount = 150
        res = token_instance.rpc[0].tokentransfer(token_instance.base_token.get('tokenid'),
                                                  token_instance.pubkey[1], str(amount))
        validate_template(res, transfer_schema)
        txid = token_instance.rpc[0].sendrawtransaction(res.get('hex'))
        mine_and_waitconfirms(txid, token_instance.rpc[0])

        res = token_instance.rpc[1].tokenbalance(token_instance.base_token.get('tokenid'),
                                                 token_instance.pubkey[1])
        validate_template(res, balance_schema)
        assert amount == res.get('balance')
        assert token_instance.base_token.get('tokenid') == res.get('tokenid')

    def test_tokenask_bid_orders(self, token_instance):
        askbid_schema = {
            'type': 'object',
            'properties': {
                'hex': {'type': 'string'}
            },
            'required': ['hex']
        }

        orders_schema = {
            'type': 'array',
            'items': {
                'type': 'object',
                'properties': {
                    'funcid': {'type': 'string'},
                    'txid': {'type': 'string'},
                    'vout':  {'type': 'integer'},
                    'amount': {'type': 'string'},
                    'askamount': {'type': 'string'},
                    'origaddress': {'type': 'string'},
                    'origtokenaddress': {'type': 'string'},
                    'tokenid': {'type': 'string'},
                    'totalrequired': {'type': ['string', 'integer']},
                    'price': {'type': 'string'}
                }
            }
        }

        rpc1 = token_instance.rpc[0]
        rpc2 = token_instance.rpc[1]
        pubkey2 = token_instance.pubkey[1]

        if not token_instance.base_token:
            token_instance.new_token(token_instance.rpc[1])
        amount1 = 150
        amount2 = 100
        price = 0.1

        res = rpc1.tokenask(str(amount1), token_instance.base_token.get('tokenid'), str(price))
        validate_template(res, askbid_schema)
        asktxid = rpc1.sendrawtransaction(res.get('hex'))
        mine_and_waitconfirms(asktxid, rpc1)

        res = rpc1.tokenbid(str(amount2), token_instance.base_token.get('tokenid'), str(price))
        validate_template(res, askbid_schema)
        bidtxid = rpc1.sendrawtransaction(res.get('hex'))
        mine_and_waitconfirms(bidtxid, rpc1)

        # Check orders and myorders call
        orders1 = rpc2.tokenorders(token_instance.base_token.get('tokenid'))
        validate_template(orders1, orders_schema)
        orders2 = rpc1.mytokenorders(token_instance.base_token.get('tokenid'))
        validate_template(orders2, orders_schema)
        # assert orders1 == orders2

        # Check fills and cancel calls
        res = rpc2.tokenfillask(token_instance.base_token.get('tokenid'), asktxid, str(int(amount1 * 0.5)))
        validate_template(res, askbid_schema)
        asktxid2 = rpc2.sendrawtransaction(res.get('hex'))
        mine_and_waitconfirms(asktxid2, rpc2)

        res = rpc2.tokenfillbid(token_instance.base_token.get('tokenid'), bidtxid, str(int(amount2 * 0.5)))
        validate_template(res, askbid_schema)
        bidtxid2 = rpc2.sendrawtransaction(res.get('hex'))
        mine_and_waitconfirms(bidtxid2, rpc2)

        res = rpc1.tokencancelask(token_instance.base_token.get('tokenid'), asktxid2)
        validate_template(res, askbid_schema)
        txid = rpc1.sendrawtransaction(res.get('hex'))
        mine_and_waitconfirms(txid, rpc1)

        res = rpc1.tokencancelbid(token_instance.base_token.get('tokenid'), bidtxid2)
        validate_template(res, askbid_schema)
        txid = rpc1.sendrawtransaction(res.get('hex'))
        mine_and_waitconfirms(txid, rpc1)

        # There should be no orders left after cancelling
        orders = rpc1.mytokenorders(token_instance.base_token.get('tokenid'))
        validate_template(orders, orders_schema)
        assert len(orders) == 0


@pytest.mark.usefixtures("proxy_connection", "test_params")
class TestTokenCC:

    def test_rewardsaddress(self, token_instance):
        pubkey = token_instance.pubkey[0]

        res = token_instance.rpc[0].assetsaddress(pubkey)
        for key in res.keys():
            if key.find('ddress') > 0:
                assert validate_raddr_pattern(res.get(key))

        res = token_instance.rpc[0].tokenaddress(pubkey)
        for key in res.keys():
            if key.find('ddress') > 0:
                assert validate_raddr_pattern(res.get(key))

    def test_bad_calls(self, token_instance):
        if not token_instance.base_token:
            token_instance.new_token(token_instance.rpc[1])

        rpc = token_instance.rpc[0]
        pubkey = token_instance.pubkey[0]

        name = token_instance.base_token.get('name')
        tokenid = token_instance.base_token.get('tokenid')

        # trying to create token with negative supply
        with pytest.raises(RPCError):
            res = rpc.tokencreate("NUKE", "-1987420", "no bueno supply")
            assert res.get('error')
        # creating token with name more than 32 chars
        res = rpc.tokencreate("NUKE123456789012345678901234567890", "1987420", "name too long")
        assert res.get('error')
        # getting token balance for non existing tokenid
        res = rpc.tokenbalance("", pubkey)
        assert res.get('error')
        # no info for invalid tokenid
        res = rpc.tokeninfo(pubkey)
        assert res.get('error')
        # invalid token transfer amount
        randompubkey = randomhex()
        res = rpc.tokentransfer(tokenid, randompubkey, "0")
        assert res.get('error')
        # invalid token transfer amount
        res = rpc.tokentransfer(tokenid, randompubkey, "-1")
        assert res.get('error')
        # invalid numtokens bid
        res = rpc.tokenbid("-1", tokenid, "1")
        assert res.get('error')
        # invalid numtokens bid
        res = rpc.tokenbid("0", tokenid, "1")
        assert res.get('error')
        # invalid price bid
        with pytest.raises(RPCError):
           res = rpc.tokenbid("1", tokenid, "-1")
           assert res.get('error')
        # invalid price bid
        res = rpc.tokenbid("1", tokenid, "0")
        assert res.get('error')
        # invalid tokenid bid
        res = rpc.tokenbid("100", "deadbeef", "1")
        assert res.get('error')
        # invalid numtokens ask
        res = rpc.tokenask("-1", tokenid, "1")
        assert res.get('error')
        # invalid numtokens ask
        res = rpc.tokenask("0", tokenid, "1")
        assert res.get('error')
        # invalid price ask
        with pytest.raises(RPCError):
            res = rpc.tokenask("1", tokenid, "-1")
            assert res.get('error')
        # invalid price ask
        res = rpc.tokenask("1", tokenid, "0")
        assert res.get('error')
        # invalid tokenid ask
        res = rpc.tokenask("100", "deadbeef", "1")
        assert res.get('error')
