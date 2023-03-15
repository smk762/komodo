#!/usr/bin/env python3
# Copyright (c) 2021 SuperNET developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://www.opensource.org/licenses/mit-license.php.

import pytest
import time
from lib.pytest_util import validate_template, validate_raddr_pattern, randomhex


@pytest.mark.usefixtures("proxy_connection", "test_params")
class TestRewardsCC:

    def test_rewardscreatefunding(self, rewards_plan):
        createfunding_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'hex': {'type': 'string'},
                'error': {'type': 'string'}
            },
            'required': ['result']
        }

        rpc = rewards_plan.rpc[0]
        rewards_plan.new_rewardsplan(rpc, schema=createfunding_schema)

    def test_rewardsaddress(self, rewards_plan):
        rewardsaddress_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'error': {'type': 'string'},
                'RewardsCCAddress': {'type': 'string'},
                'RewardsCCBalance': {'type': 'number'},
                'RewardsNormalAddress': {'type': 'string'},
                'RewardsNormalBalance': {'type': 'number'},
                'RewardsCCTokensAddress': {'type': 'string'},
                'PubkeyCCaddress(Rewards)': {'type': 'string'},
                'PubkeyCCbalance(Rewards)': {'type': 'number'},
                'myCCAddress(Rewards)': {'type': 'string'},
                'myCCbalance(Rewards)': {'type': 'number'},
                'myaddress': {'type': 'string'},
                'mybalance': {'type': 'number'}
            },
            'required': ['result']
        }

        rpc = rewards_plan.rpc[0]
        pubkey = rewards_plan.pubkey[0]
        res = rpc.rewardsaddress()
        validate_template(res, rewardsaddress_schema)
        assert res.get('result') == 'success'

        res = rpc.rewardsaddress()
        for key in res.keys():
            if key.find('ddress') > 0:
                assert validate_raddr_pattern(res.get(key))

        res = rpc.rewardsaddress(pubkey)
        for key in res.keys():
            if key.find('ddress') > 0:
                assert validate_raddr_pattern(res.get(key))

    def test_rewarsdlist(self, rewards_plan):
        rewadslist_schema = {
            'type': 'array',
            'items': {'type': 'string'}
        }

        rpc = rewards_plan.rpc[0]
        res = rpc.rewardslist()
        validate_template(res, rewadslist_schema)

    def test_rewardsinfo(self, rewards_plan):
        rewardsinfo_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'error': {'type': 'string'},
                'fundingtxid': {'type': 'string'},
                'name': {'type': 'string'},
                'sbits': {'type': 'integer'},
                'APR': {'type': 'string'},
                'minseconds': {'type': 'integer'},
                'maxseconds': {'type': 'integer'},
                'mindeposit': {'type': 'string'},
                'funding': {'type': 'string'},
                'locked': {'type': 'string'}
            },
            'required': ['result']
        }

        rpc = rewards_plan.rpc[0]
        pubkey = rewards_plan.pubkey[1]
        if not rewards_plan.base_plan:
            rewards_plan.new_rewardsplan(rpc, pubkey)
        rewards_plan.rewardsinfo_maincheck(rpc, rewards_plan.base_plan.get('fundingtxid'), rewardsinfo_schema)

    def test_rewardsaddfunding(self, rewards_plan):
        addfunding_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'hex': {'type': 'string'},
                'error': {'type': 'string'}
            },
            'required': ['result']
        }

        rpc = rewards_plan.rpc[0]
        pubkey = rewards_plan.pubkey[1]
        if not rewards_plan.base_plan:
            rewards_plan.new_rewardsplan(rpc, pubkey)
        rewards_plan.rewardsaddfunding_maincheck(rpc, rewards_plan.base_plan.get('fundingtxid'), addfunding_schema)

    def test_lock_unlock(self, rewards_plan):
        lock_unlock_schema = {
            'type': 'object',
            'properties': {
                'result': {'type': 'string'},
                'hex': {'type': 'string'},
                'error': {'type': 'string'}
            },
            'required': ['result']
        }

        rpc = rewards_plan.rpc[0]
        pubkey = rewards_plan.pubkey[1]
        if not rewards_plan.base_plan:
            rewards_plan.new_rewardsplan(rpc, pubkey)
        rewards_plan.un_lock_maincheck(rpc, rewards_plan.base_plan.get('fundingtxid'), lock_unlock_schema)


@pytest.mark.usefixtures("proxy_connection", "test_params")
class TestRewardsCCExtras:

    def test_bad_calls(self, rewards_plan):
        rpc = rewards_plan.rpc[0]
        pubkey = rewards_plan.pubkey[1]
        if not rewards_plan.base_plan:
            rewards_plan.new_rewardsplan(rpc, pubkey)
        fundtxid = rewards_plan.base_plan.get('fundingtxid')

        name = rpc.rewardsinfo(fundtxid).get('name')
        invalidfunding = randomhex()
        # looking up non-existent reward should return error
        res = rpc.rewardsinfo(invalidfunding)
        assert res.get('result') == 'error'

        # creating rewards plan with name > 8 chars, should return error
        res = rpc.rewardscreatefunding("STUFFSTUFF", "7777", "25", "0", "10", "10")
        assert res.get('result') == 'error'

        # creating rewards plan with 0 funding
        res = rpc.rewardscreatefunding("STUFF", "0", "25", "0", "10", "10")
        assert res.get('result') == 'error'

        # creating rewards plan with 0 maxdays
        res = rpc.rewardscreatefunding("STUFF", "7777", "25", "0", "10", "0")
        assert res.get('result') == 'error'

        # creating rewards plan with > 25% APR
        res = rpc.rewardscreatefunding("STUFF", "7777", "30", "0", "10", "10")
        assert res.get('result') == 'error'

        # creating reward plan with already existing name, should return error
        res = rpc.rewardscreatefunding(name, "777", "25", "0", "10", "10")
        assert res.get('result') == 'error'

        # add funding amount must be positive
        res = rpc.rewardsaddfunding(name, fundtxid, "-1")
        assert res.get('result') == 'error'

        # add funding amount must be positive
        res = rpc.rewardsaddfunding(name, fundtxid, "0")
        assert res.get('result') == 'error'

        # trying to lock funds, locking funds amount must be positive
        res = rpc.rewardslock(name, fundtxid, "-5")
        assert res.get('result') == 'error'

        # trying to lock funds, locking funds amount must be positive
        res = rpc.rewardslock(name, fundtxid, "0")
        assert res.get('result') == 'error'

        # trying to lock less than the min amount is an error
        res = rpc.rewardslock(name, fundtxid, "7")
        assert res.get('result') == 'error'
