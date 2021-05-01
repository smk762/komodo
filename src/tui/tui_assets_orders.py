#!/usr/bin/env python3

from lib import rpclib, tuilib
import os
import time

# test creates fungible and non-fungible tokens
# performs transfers and multiple transfers
# then runs assets cc ask/bids tests

header = "komodo assets cc tokenask/bid v2 test\n"

def get_result_error(r):
    if isinstance(r, dict) :
        if r.get('result') == "error" :
            return r.get('error')
    return ''

def check_result(r):
    if isinstance(r, str) :
        # print("r is str, true")
        return True
    elif isinstance(r, dict) :
        # print("r is dict")
        if isinstance(r.get('hex'), str) :
            # print("r has hex, true")
            return True
        if r.get('result') == "error" :
            # print("r has error, false")
            return False
    # print("r has unknown, true")
    return True
    
def run_tokens_create(rpc):

    # set your own two node params
    # DIMXY20 
    rpc1 = rpclib.rpc_connect("user2135429985", "passe26e9bce922bb0005fff3c41c20e7ea033399104aab3f148c515a2fa72fa4a9272", 14723)
    rpc2 = rpclib.rpc_connect("user3701990598", "pass6df4dc57b2ee49b9e591ac6c8cb6aa89f0a06056ce67c6c45bbb14c0d63170e8a0", 15723)

    #tokenid = "b08775cf3b3b371f97256b427af005d5573a2868106a8c4dd4e8c76e87d476d7" # fungible
    #tokenid = "3b0003561fb98b332452e71208f13e4379fa6987577c3dcd2aef6199b54111ae" # NFT 0 royalty
    #tokenid = "03b0018da699af63a40595cc93b5e3b097a88225e51e767efbe1425aa4698a65" # NFT 1/1000 royalty - spent
    #tokenid = "9f051912b13b707b64b65bb179649901449f7ae78d74e78a813dffb2cf7705f8" # NFT eval=00

    print("creating fungible token 1...")
    result = rpc1.tokenv2create("T1", str(0.000001))
    assert(check_result(result))
    tokenid1 = rpc1.sendrawtransaction(result['hex'])
    print("created token:", tokenid1)

    print("creating fungible token 2...")
    result = rpc1.tokenv2create("T2", str(0.1))
    assert(check_result(result))
    tokenid2 = rpc1.sendrawtransaction(result['hex'])
    print("created token:", tokenid2)

    print("creating NFT 1 with 00...")
    result = rpc1.tokenv2create("NFT-00-1", str(0.00000001), "nft eval 00", "00010203")
    assert(check_result(result))
    nft_00_id1 = rpc1.sendrawtransaction(result['hex'])
    print("created token:", nft_00_id1)

    print("creating NFT 2 with 00...")
    result = rpc1.tokenv2create("NFT-00-2", str(0.00000001), "nft eval 00", "00010203")
    assert(check_result(result))
    nft_00_id2 = rpc1.sendrawtransaction(result['hex'])
    print("created token:", nft_00_id2)

    # reserved for tokel nft data F7 evalcode
    """ 
    print("creating NFT with F7, no royaly, with arbitary data...")
    result = rpc1.tokenv2create("NFT-F7-1", str(0.00000001), "nft eval=f7 arbitrary=hello", "F70101ee020d687474703a2f2f6d792e6f7267040568656c6c6f")
    assert(check_result(result))
    nft_f7_id1 = rpc1.sendrawtransaction(result['hex'])
    print("created token:", nft_f7_id1)

    print("creating NFT with F7 and royaly 0xAA...")
    result = rpc1.tokenv2create("NFT-F7-2", str(0.00000001), "nft eval=f7 roaylty=AA", "F70101ee020d687474703a2f2f6d792e6f726703AA")
    assert(check_result(result))
    nft_f7_id2 = rpc1.sendrawtransaction(result['hex'])
    print("created token:", nft_f7_id2)
    """

    # first try transfer tokens to a pk and back, then run assets tests
    print("starting token transfers tests...")
    run_transfers(rpc1, rpc2, tokenid1, tokenid2, nft_00_id1, nft_00_id2)
    print("token transfers tests finished okay")
    time.sleep(3)

    print("starting assets tests...")
    run_assets_orders(rpc1, rpc2, tokenid1, 10, 8)
    run_assets_orders(rpc1, rpc2, nft_00_id1, 1, 1)
    print("assets tests finished okay")
    time.sleep(3)
    exit

def call_token_rpc(rpc, rpcname, stop_error, *args) :
    retries = 24
    delay = 10
    rpcfunc = getattr(rpc, rpcname)
    for i in range(retries):
        print("calling " + rpcname)
        result = rpcfunc(*args)
        print(rpcname + " create tx result:", result)
        if  check_result(result):
            break
        if stop_error and get_result_error(result) == stop_error :  
            print(rpcname + " retrying stopped because of stop error received: " + stop_error)
            return result
        if i < retries-1:
            print("retrying " + rpcname + '...')
            time.sleep(delay)                
    assert(check_result(result))
    print(rpcname + " tx created")
    txid = rpc.sendrawtransaction(result['hex'])
    print("sendrawtransaction result: ", txid)
    assert(check_result(txid))            
    print(rpcname + " tx sent")
    return txid


def run_transfers(rpc1, rpc2, tokenid1, tokenid2, nftid1, nftid2):

    amount  = 1
   
    pubkey1 = rpc1.getinfo()['pubkey']
    pubkey2 = rpc2.getinfo()['pubkey']

    # rpc1.setgenerate(True, 2)

    for i in range(1):
        try:
            name = "none"
        except KeyboardInterrupt:
            break
        else:
            print("creating tokenv2transfer tokenid1 tx...")
            call_token_rpc(rpc1, "tokenv2transfer", '', tokenid1, pubkey2,  str(amount))

            print("creating tokenv2transfer tokenid1 tx back...")
            call_token_rpc(rpc2, "tokenv2transfer", '', tokenid1, pubkey1,  str(amount))

            print("creating tokenv2transfer nftid1 tx...")
            call_token_rpc(rpc1, "tokenv2transfer", '', nftid1, pubkey2,  str(amount))

            print("creating tokenv2transfer nftid1 tx back...")
            call_token_rpc(rpc2, "tokenv2transfer", '', nftid1, pubkey1,  str(amount))

            print("creating tokenv2transfermany tokenid1 tokenid2 tx...")
            call_token_rpc(rpc1, "tokenv2transfermany", '', tokenid1, tokenid2, pubkey2,  str(amount))

            print("creating tokenv2transfermany tokenid1 tokenid2 tx back...")
            call_token_rpc(rpc2, "tokenv2transfermany", '', tokenid1, tokenid2, pubkey1,  str(amount))

            print("creating tokenv2transfermany nftid1 nftid2 tx...")
            call_token_rpc(rpc1, "tokenv2transfermany", '', nftid1, nftid2, pubkey2,  str(amount))

            print("creating tokenv2transfermany nftid1 nftid2 tx back...")
            call_token_rpc(rpc2, "tokenv2transfermany", '', nftid1, nftid2, pubkey1,  str(amount))



def run_assets_orders(rpc1, rpc2, tokenid, total, units):

    retries = 24
    delay = 10
   
    #total = 1 
    askunits = bidunits = units
    unitprice = 0.0001

    # get initial balance
    result = rpc1.tokenv2balance(tokenid)
    assert(check_result(result))
    initial_balance = int(result["balance"])
    print("initial balance for tokenid " + tokenid + " = " + str(initial_balance))

    # create tokenask
    print("creating tokenv2ask tx...")
    askid = call_token_rpc(rpc1, 'tokenv2ask', '', str(total), tokenid, str(unitprice))

    # fill ask 
    print("creating tokenv2fillask tx...")
    fillaskid = call_token_rpc(rpc2, "tokenv2fillask", '', tokenid, askid, str(askunits))

    # cancel ask
    print("creating tokenv2cancelask tx...")
    cancelid = call_token_rpc(rpc1, 'tokenv2cancelask', 'ask is empty', tokenid, fillaskid)
    
    # create tokenbid to buy back tokens - same amount
    print("creating tokenv2bid tx...")
    bidid = call_token_rpc(rpc1, 'tokenv2bid', '', str(askunits), tokenid, str(unitprice))

    # fill bid 
    print("creating tokenv2fillbid tx...")
    fillbidid = call_token_rpc(rpc2, 'tokenv2fillbid', '', tokenid, bidid, str(bidunits))

    # cancel bid
    print("creating tokenv2cancelbid tx...")
    cancelid = call_token_rpc(rpc1, 'tokenv2cancelbid', 'bid is empty', tokenid, fillbidid)

    # check balance after 
    retries = 24
    delay = 10
    for i in range(retries):
        print("calling " + "tokenv2balance")
        finresult = rpc1.tokenv2balance(tokenid)
        assert(check_result(result))
        if int(finresult["balance"]) == initial_balance :
            break
        if i < retries-1:
            print("retrying " + "tokenv2balance" + '...')
            time.sleep(delay)                

    assert(int(finresult["balance"]) == initial_balance)

menuItems = [
    {"run token create/transfers and assets orders": run_tokens_create},
    {"Exit": exit}
]


def main():
    while True:
        os.system('clear')
        print(tuilib.colorize(header, 'pink'))
        print(tuilib.colorize('CLI version 0.2\n', 'green'))
        for item in menuItems:
            print(tuilib.colorize("[" + str(menuItems.index(item)) + "] ", 'blue') + list(item.keys())[0])
        choice = input(">> ")
        try:
            if int(choice) < 0:
                raise ValueError
            # Call the matching function
            if list(menuItems[int(choice)].keys())[0] == "Exit":
                list(menuItems[int(choice)].values())[0]()
            else:
                list(menuItems[int(choice)].values())[0](rpc_connection)
        except (ValueError, IndexError):
            pass


if __name__ == "__main__":
    print("starting assets orders test (remember to set rpc params for two nodes for your chain)")
    time.sleep(2)   
    rpc_connection = ""
    main()

