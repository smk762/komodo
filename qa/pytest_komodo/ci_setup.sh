#!/bin/bash

# chains start script params
export CLIENTS=2
export CHAIN="TONYCI"
export TEST_ADDY0="RB6fgrPX4ANGTtMUVud6aRh5cFy7TqVein"
export TEST_WIF0="UqMLYDNVtYLPKZ2CcqeBU1FcYNHkknVZaMuQRqqZWXyPmSuJw3kr"
export TEST_PUBKEY0="02e9b141e1c251a942f77df10fa4de00f53f8ab2a6d5341bbaf842c95e674e92e9"
export TEST_ADDY1="RHoTHYiHD8TU4k9rbY4Aoj3ztxUARMJikH"
export TEST_WIF1="UwmmwgfXwZ673brawUarPzbtiqjsCPWnG311ZRAL4iUCZLBLYeDu"
export TEST_PUBKEY1="0285f68aec0e2f8b5e817d71a2a20a1fda74ea9943c752a13136a3a30fa49c0149"
export CHAIN_MODE="REGULAR"
export IS_BOOTSTRAP_NEEDED="True"
export BOOTSTRAP_URL="https://sirseven.me/share/bootstrap4.tar.gz"

# starting the chains
python3 chainstart.py

# starting the tests
python3 -m pytest $@ -s -vv
