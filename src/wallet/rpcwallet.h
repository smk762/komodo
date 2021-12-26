// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/******************************************************************************
 * Copyright © 2014-2019 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

#ifndef BITCOIN_WALLET_RPCWALLET_H
#define BITCOIN_WALLET_RPCWALLET_H

#include "univalue.h"
#include "script/script.h"
#include "chain.h"
#include "primitives/block.h"
#include "primitives/transaction.h"

class CRPCTable;

void RegisterWalletRPCCommands(CRPCTable &tableRPC);
bool EnsureWalletIsAvailable(bool avoidException);
void EnsureWalletIsUnlocked();

extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, UniValue& entry);
extern void TxToJSONExpanded(const CTransaction& tx, const uint256 hashBlock, UniValue& entry, int nHeight = 0, int nConfirmations = 0, int nBlockTime = 0);
UniValue TxJoinSplitToJSON(const CTransaction& tx);
extern UniValue blockToJSON(const CBlock& block, const CBlockIndex* blockindex, bool txDetails = false);
extern UniValue mempoolInfoToJSON();
extern UniValue mempoolToJSON(bool fVerbose = false);
extern void ScriptPubKeyToJSON(const CScript& scriptPubKey, UniValue& out, bool fIncludeHex);
extern UniValue blockheaderToJSON(const CBlockIndex* blockindex);

#endif //BITCOIN_WALLET_RPCWALLET_H
