#!/usr/bin/env python3
# Copyright (c) 2021 SuperNET developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://www.opensource.org/licenses/mit-license.php.

import pytest
import time
from lib.pytest_util import validate_template, mine_and_waitconfirms, validate_raddr_pattern, validate_tx_pattern


@pytest.mark.usefixtures("proxy_connection", "test_params")
class TestChannelsCCBase:

    def test_channelsaddress(self, channel_instance):
        channelsaddress_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'error': {'type': 'string'},
                'ChannelsCCAddress': {'type': 'string'},
                'ChannelsCCBalance': {'type': 'number'},
                'ChannelsNormalAddress': {'type': 'string'},
                'ChannelsNormalBalance': {'type': 'number'},
                'ChannelsCC1of2Address': {'type': 'string'},
                'ChannelsCC1of2TokensAddress': {'type': 'string'},
                'PubkeyCCaddress(Channels)': {'type': 'string'},
                'PubkeyCCbalance(Channels)': {'type': 'number'},
                'myCCAddress(Channels)': {'type': 'string'},
                'myCCbalance(Channels)': {'type': 'number'},
                'myaddress': {'type': 'string'},
                'mybalance': {'type': 'number'}
            },
            'required': ['result']
        }

        rpc1 = channel_instance.rpc[0]
        pubkey2 = channel_instance.pubkey[1]

        res = rpc1.channelsaddress(pubkey2)
        validate_template(res, channelsaddress_schema)
        for key in res.keys():
            if key.find('ddress') > 0:
                assert validate_raddr_pattern(res.get(key))

    def test_channelsopen(self, channel_instance):
        channelsopen_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'error': {'type': 'string'},
                'hex': {'type': 'string'}
            },
            'required': ['result']
        }

        rpc1 = channel_instance.rpc[0]
        pubkey2 = channel_instance.pubkey[1]
        channel_instance.new_channel(rpc1, pubkey2, '10', '100000', schema=channelsopen_schema)

    def test_channelslist(self, channel_instance):
        channelsinfo_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'error': {'type': 'string'},
                'name': {'type': 'string'},
                '[0-9a-f]{64}': {'type': 'string'}  # 'txid': "channels info and payments info"
            },
            'required': ['result']
        }

        rpc1 = channel_instance.rpc[0]
        channel_instance.channelslist_get(rpc1, channelsinfo_schema)

    def test_channelsinfo(self, channel_instance):
        channelsinfo_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'error': {'type': 'string'},
                "Channel CC address": {'type': 'string'},
                "Destination address": {'type': 'string'},
                "Number of payments": {'type': 'integer'},
                "Denomination (satoshi)": {'type': 'string'},
                "Amount (satoshi)": {'type': 'string'},
                "Token id": {'type': 'string'},
                'Transactions': {
                    'type': 'array',
                    'items': {
                        'type': 'object',
                        'properties': {
                            'Open': {'type': 'string'},
                            'Payment': {'type': 'string'},
                            "Number of payments": {'type': 'integer'},
                            'Amount': {'type': 'integer'},
                            'Destination': {'type': 'string'},
                            'Secret': {'type': 'string'},
                            "Payments left": {'type': 'integer'}
                        }
                    }
                }
            },
            'required': ['result']
        }

        rpc1 = channel_instance.rpc[0]
        pubkey2 = channel_instance.pubkey[1]
        if not channel_instance.base_channel:
            channel_instance.new_channel(rpc1, pubkey2)
        res = rpc1.channelsinfo(channel_instance.base_channel.get('open_txid'))
        validate_template(res, channelsinfo_schema)

    def test_channelspayment(self, channel_instance):
        channelspayment_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'error': {'type': 'string'},
                'hex': {'type': 'string'}
            },
            'required': ['result']
        }

        rpc1 = channel_instance.rpc[0]
        pubkey2 = channel_instance.pubkey[1]
        if not channel_instance.base_channel:
            channel_instance.new_channel(rpc1, pubkey2)
            minpayment = '100000'
        else:
            minpayment = rpc1.channelsinfo(channel_instance.base_channel.get('open_txid')).get("Denomination (satoshi)")
        res = rpc1.channelspayment(channel_instance.base_channel.get('open_txid'), minpayment)
        validate_template(res, channelspayment_schema)
        txid = rpc1.sendrawtransaction(res.get('hex'))
        mine_and_waitconfirms(txid, rpc1)

    def test_channels_closenrefund(self, channel_instance):
        channels_refund_close_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'error': {'type': 'string'},
                'hex': {'type': 'string'}
            },
            'required': ['result']
        }

        rpc1 = channel_instance.rpc[0]
        pubkey2 = channel_instance.pubkey[1]
        minpayment = '100000'
        newchannel = channel_instance.new_channel(rpc1, pubkey2, '2', minpayment)

        # send 1 payment and close channel
        res = rpc1.channelspayment(newchannel.get('open_txid'), minpayment)
        txid = rpc1.sendrawtransaction(res.get('hex'))
        mine_and_waitconfirms(txid, rpc1)
        res = rpc1.channelsclose(newchannel.get('open_txid'))
        validate_template(res, channels_refund_close_schema)
        close_txid = rpc1.sendrawtransaction(res.get('hex'))
        mine_and_waitconfirms(close_txid, rpc1)

        # execute refund
        res = rpc1.channelsrefund(newchannel.get('open_txid'), close_txid)
        validate_template(res, channels_refund_close_schema)
        refund_txid = rpc1.sendrawtransaction(res.get('hex'))
        mine_and_waitconfirms(refund_txid, rpc1)


@pytest.mark.usefixtures("proxy_connection", "test_params")
class TestChannelsCC:
    def test_channels_flow(self, channel_instance):
        rpc1 = channel_instance.rpc[0]
        rpc2 = channel_instance.rpc[1]
        pubkey2 = channel_instance.pubkey[1]
        addr1 = channel_instance.address[0]
        channel = channel_instance.new_channel(rpc1, pubkey2)

        # trying to make wrong denomination channel payment
        res = rpc1.channelspayment(channel.get('open_txid'), '199000')
        assert res.get('result') == 'error'

        # trying to make 0 channel payment
        res = rpc1.channelspayment(channel.get('open_txid'), '0')
        assert res.get('result') == 'error'

        # trying to make negative channel payment
        res = rpc1.channelspayment(channel.get('open_txid'), '-100000')
        assert res.get('result') == 'error'

        # lets try payment with x2 amount to ensure that counters works correct
        res = rpc1.channelspayment(channel.get('open_txid'), '200000')
        assert res.get('result') == 'success'
        payment_tx_id = rpc1.sendrawtransaction(res.get('hex'))
        mine_and_waitconfirms(payment_tx_id, rpc1)
        assert isinstance(payment_tx_id, str)
        res = rpc1.channelsinfo(channel.get('open_txid'))
        assert res['Transactions'][-1]['Payment'] == payment_tx_id
        assert res['Transactions'][-1]["Number of payments"] == 2
        assert res['Transactions'][-1]["Payments left"] == 8  # 10 initial - 2

        # check if payment value really transferred
        raw_transaction = rpc1.getrawtransaction(payment_tx_id, 1)
        res = raw_transaction['vout'][3]['valueSat']
        assert res == 200000
        res = rpc2.validateaddress(raw_transaction['vout'][3]['scriptPubKey']['addresses'][0])['ismine']
        assert res

        # trying to initiate channels payment from node B without any secret
        res = rpc2.channelspayment(channel.get('open_txid'), "100000")
        assert res.get('result') == 'error'
        assert "invalid secret" in res.get('error')

        # trying to initiate channels payment from node B with secret from previous payment
        secret = rpc2.channelsinfo(channel.get('open_txid'))['Transactions'][-1]['Secret']
        res = rpc2.channelspayment(channel.get('open_txid'), "100000", secret)
        assert res.get('result') == 'error'
        assert "invalid secret" in res.get('error')

        # executing channel close
        res = rpc1.channelsclose(channel.get('open_txid'))
        assert res.get('result') == 'success'
        close_txid = rpc1.sendrawtransaction(res.get('hex'))
        mine_and_waitconfirms(close_txid, rpc1)

        # now in channelinfo closed flag should appear
        res = rpc1.channelsinfo(channel.get('open_txid'))
        assert res['Transactions'][-1]['Close'] == close_txid

        # executing channel refund
        res = rpc1.channelsrefund(channel.get('open_txid'), close_txid)
        assert res.get('result') == 'success'
        refund_txid = rpc1.sendrawtransaction(res.get('hex'))
        mine_and_waitconfirms(refund_txid, rpc1)

        # checking if it refunded to opener address
        raw = rpc1.getrawtransaction(refund_txid, 1)
        res = raw['vout']
        values = []
        for vout in res:  # find all txs to node1 address
            try:
                if vout.get('scriptPubKey').get('addresses')[0] == addr1 \
                        and "OP_CHECKSIG" in vout.get('scriptPubKey').get('asm'):
                    values.append(vout.get('valueSat'))
            except TypeError:  # to prevent fails on OP_RETURN // nulldata vout
                pass
        assert 800000 in values  # 10 - 2 payments, worth of 100000 satoshi each

    def test_channel_drain(self, channel_instance):
        rpc1 = channel_instance.rpc[0]
        pubkey2 = channel_instance.pubkey[1]
        payments = '3'
        pay_amount = '100000'
        channel = channel_instance.new_channel(rpc1, pubkey2, payments, pay_amount)

        # draining channel (3 payment by 100000 satoshies in total to fit full capacity)
        for i in range(3):
            res = rpc1.channelspayment(channel.get('open_txid'), '100000')
            assert res.get('result') == 'success'
            payment_tx = rpc1.sendrawtransaction(res.get("hex"))
            mine_and_waitconfirms(payment_tx, rpc1)

        # last payment should indicate that 0 payments left
        res = rpc1.channelsinfo(channel.get('open_txid'))['Transactions'][-1]["Payments left"]
        assert res == 0

        # no more payments possible
        res = rpc1.channelspayment(channel.get('open_txid'), '100000')
        assert res.get('result') == 'error'
        assert "error adding CC inputs" in res.get('error')

    def test_secret_reveal(self, channel_instance):
        # creating new channel to test the case when node B initiate payment when node A revealed secret in offline
        # 10 payments, 100000 sat denomination channel opening with second node pubkey
        rpc1 = channel_instance.rpc[0]
        rpc2 = channel_instance.rpc[1]
        pubkey2 = channel_instance.pubkey[1]
        channel = channel_instance.new_channel(rpc1, pubkey2)

        # disconnecting first node from network
        rpc1.setban("127.0.0.0/24", 'add')
        rpc2.setban("127.0.0.0/24", 'add')
        # timewait for bans to take place
        timeout = 40
        t_iter = 0
        while rpc1.getinfo()['connections'] != 0 or rpc2.getinfo()['connections'] != 0:
            time.sleep(1)
            t_iter += 1
            if t_iter >= timeout:
                raise TimeoutError("Setban timeout: ", str(t_iter), "s")

        # sending one payment to mempool to reveal the secret but not mine it
        payment_hex = rpc1.channelspayment(channel.get('open_txid'), '100000')
        res = rpc1.sendrawtransaction(payment_hex['hex'])
        assert isinstance(res, str)

        secret = rpc1.channelsinfo(channel.get('open_txid'))['Transactions'][1]['Secret']
        assert isinstance(res, str)

        # secret shouldn't be available for node B
        try:
            check = rpc2.channelsinfo(channel.get('open_txid'))['Transactions'][1]['Secret']
        except IndexError as e:
            print(e)
            check = None
        assert not check

        # trying to initiate payment from second node with revealed secret
        assert rpc1.getinfo()['connections'] == 0
        dc_payment_hex = rpc2.channelspayment(channel.get('open_txid'), '100000', secret)
        res = rpc2.sendrawtransaction(dc_payment_hex['hex'])
        assert isinstance(res, str)
        rpc1.clearbanned()
        rpc2.clearbanned()
