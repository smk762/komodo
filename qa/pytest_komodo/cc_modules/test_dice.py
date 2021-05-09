#!/usr/bin/env python3
# Copyright (c) 2020 SuperNET developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://www.opensource.org/licenses/mit-license.php.

import pytest
from lib.pytest_util import validate_template, mine_and_waitconfirms, randomstring,\
     randomhex, wait_blocks, validate_raddr_pattern, check_synced
from decimal import *


@pytest.mark.usefixtures("proxy_connection", "test_params")
class TestDiceCCBase:

    def test_diceaddress(self, dice_casino):
        diceaddress_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'DiceCCAddress': {'type': 'string'},
                'DiceCCBalance': {'type': 'number'},
                'DiceNormalAddress': {'type': 'string'},
                'DiceNormalBalance': {'type': 'number'},
                'DiceCCTokensAddress': {'type': 'string'},
                'myCCAddress(Dice)': {'type': 'string'},
                'myCCBalance(Dice)': {'type': 'number'},
                'myaddress': {'type': 'string'},
                'mybalance': {'type': 'number'},
                'error': {'type': 'string'}
            },
            'required': ['result']
        }

        rpc = dice_casino.rpc[0]
        pubkey = dice_casino.pubkey[0]

        res = rpc.diceaddress()
        validate_template(res, diceaddress_schema)
        for key in res.keys():
            if key.find('ddress') > 0:
                assert validate_raddr_pattern(res.get(key))
        res = rpc.diceaddress('')
        validate_template(res, diceaddress_schema)
        res = rpc.diceaddress(pubkey)
        validate_template(res, diceaddress_schema)
        for key in res.keys():
            if key.find('ddress') > 0:
                assert validate_raddr_pattern(res.get(key))

    def test_dicefund(self, dice_casino):
        dicefund_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'hex': {'type': 'string'},
                'error': {'type': 'string'}
            },
            'required': ['result']
        }

        dice_casino.new_casino(dice_casino.rpc[0], schema=dicefund_schema)

    def test_dicelist(self, dice_casino):
        dicelist_schema = {
            'type': 'array',
            'items': {'type': 'string'}
        }

        rpc = dice_casino.rpc[0]
        res = rpc.dicelist()
        validate_template(res, dicelist_schema)

    def test_diceinfo(self, dice_casino):
        diceinfo_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'error': {'type': 'string'},
                'fundingtxid': {'type': 'string'},
                'name': {'type': 'string'},
                'sbits': {'type': 'integer'},
                'minbet': {'type': 'string'},
                'maxbet': {'type': 'string'},
                'maxodds': {'type': 'integer'},
                'timeoutblocks': {'type': 'integer'},
                'funding': {'type': 'string'}
            },
            'required': ['result']
        }

        rpc = dice_casino.rpc[0]
        if not dice_casino.open_casino:
            dice_casino.new_casino(dice_casino.rpc[0])
        fundtxid = dice_casino.open_casino.get('fundingtxid')
        dice_casino.diceinfo_maincheck(rpc, fundtxid, diceinfo_schema)

    def test_diceaddfunds(self, dice_casino):
        diceaddfunds_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'hex': {'type': 'string'},
                'error': {'type': 'string'}
            },
            'required': ['result']
        }

        rpc = dice_casino.rpc[0]
        amount = '15'
        if not dice_casino.open_casino:
            dice_casino.new_casino(dice_casino.rpc[0])
        fundtxid = dice_casino.open_casino.get('fundingtxid')
        dice_casino.diceaddfunds_maincheck(rpc, amount, fundtxid, diceaddfunds_schema)

    def test_dicebet_dicestatus_dicefinish(self, dice_casino):
        dicebet_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'hex': {'type': 'string'},
                'error': {'type': 'string'}
            },
            'required': ['result']
        }
        dicestatus_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'status': {'type': 'string'},
                'error': {'type': 'string'}
            },
            'required': ['result']
        }
        dicefinish_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'hex': {'type': 'string'},
                'error': {'type': 'string'}
            },
            'required': ['result']
        }

        rpc1 = dice_casino.rpc[0]
        rpc2 = dice_casino.rpc[1]
        casino = dice_casino.new_casino(dice_casino.rpc[1])
        dice_casino.create_entropy(rpc2, casino)
        bettxid = dice_casino.dicebet_maincheck(rpc1, casino, dicebet_schema)
        dice_casino.dicestatus_maincheck(rpc1, casino, bettxid, dicestatus_schema)
        wait_blocks(rpc1, 5)  # 5 here is casino's block timeout
        dice_casino.dicefinsish_maincheck(rpc1, casino, bettxid, dicefinish_schema)


@pytest.mark.usefixtures("proxy_connection", "test_params")
class TestDiceCC:

    def test_dice_errors(self, dice_casino):
        rpc1 = dice_casino.rpc[0]
        dicename = randomstring(4)

        # creating dice plan with too long name (>8 chars)
        res = rpc1.dicefund("THISISTOOLONG", "10000", "10", "10000", "10", "5")
        assert res.get('result') == 'error'

        # creating dice plan with < 100 funding
        res = rpc1.dicefund(dicename, "10", "1", "10000", "10", "5")
        assert res.get('result') == 'error'

        # adding negative and zero funds to plan
        try:
            fundtxid = rpc1.dicelist()[0]
            name = rpc1.diceinfo(fundtxid).get('name')
            res = rpc1.diceaddfunds(name, fundtxid, '0')
            assert res.get('result') == 'error'
            res = rpc1.diceaddfunds(name, fundtxid, '-123')
            assert res.get('result') == 'error'
        except IndexError:
            casino = TestDiceCCBase.new_casino(rpc1)
            fundtxid = casino.get('fundingtxid')
            name = rpc1.diceinfo(fundtxid).get('name')
            res = rpc1.diceaddfunds(name, fundtxid, '0')
            assert res.get('result') == 'error'
            res = rpc1.diceaddfunds(name, fundtxid, '-123')
            assert res.get('result') == 'error'

        # not valid dice info checking
        res = rpc1.diceinfo("invalid")
        assert res.get('result') == 'error'

    def test_dice_badbets(self, dice_casino):

        if not dice_casino.open_casino:
            dice_casino.new_casino(dice_casino.rpc[1])
        casino = dice_casino.open_casino

        rpc1 = dice_casino.rpc[0]
        rpc2 = dice_casino.rpc[1]

        # before betting nodes should be synced
        check_synced(rpc1, rpc2)

        try:
            fundtxid = rpc2.dicelist()[0]
        except IndexError:
            casino = TestDiceCCBase.new_casino(rpc1)
            fundtxid = casino.get('fundingtxid')

        dname = str(casino.get('name'))
        minbet = str(casino.get('minbet'))
        maxbet = str(casino.get('maxbet'))
        maxodds = str(casino.get('maxodds'))
        funding = str(casino.get('funding'))

        res = rpc1.dicebet(dname, fundtxid, '0', maxodds)  # 0 bet
        assert res.get('result') == 'error'

        res = rpc1.dicebet(dname, fundtxid, minbet, '0')  # 0 odds
        assert res.get('result') == 'error'

        res = rpc1.dicebet(dname, fundtxid, '-1', maxodds)  # negative bet
        assert res.get('result') == 'error'

        res = rpc1.dicebet(dname, fundtxid, minbet, '-1')  # negative odds
        assert res.get('result') == 'error'

        # placing bet more than maxbet
        dmb = Decimal(maxbet)
        dmb += 1
        res = rpc1.dicebet(dname, fundtxid, "{0:.8f}".format(dmb), maxodds)
        assert res.get('result') == 'error'

        # placing bet with odds more than allowed
        dmo = Decimal(maxodds)
        dmo += 1
        res = rpc1.dicebet(dname, fundtxid, minbet, "{0:.8f}".format(dmo))
        assert res.get('result') == 'error'

        # placing bet with amount more than funding
        betamount = funding
        res = rpc1.dicebet(dname, fundtxid, str(betamount), maxodds)
        assert res.get('result') == 'error'

        # placing bet with not correct dice name
        name = randomstring(5)
        res = rpc1.dicebet(name, fundtxid, minbet, maxodds)
        assert res.get('result') == 'error'

        # placing bet with not correct dice id
        badtxid = randomhex()
        res = rpc1.dicebet(dname, badtxid, minbet, maxodds)
        assert res.get('result') == 'error'
