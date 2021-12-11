/******************************************************************************
 * Copyright © 2021 The SuperNET Developers.                             *
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

#include <iostream>
#include "CCupgrades.h"

namespace CCUpgrades {

    class CUpgradesContainer {
    public:
        CUpgradesContainer()  {

            // default upgrades: always enable all fixes
            defaultUpgrades.IsAllEnabled = true;

            // TOKEL
            ChainUpgrades tokel;
            tokel.setActivationHeight(CCASSETS_OPDROP_VALIDATE_FIX, CCASSETS_OPDROP_FIX_TOKEL_HEIGHT, UPGRADE_ACTIVE);
            mChainUpgrades["TOKEL"] = tokel;

            // TKLTEST
            ChainUpgrades tkltest = tokel;
            mChainUpgrades["TKLTEST"] = tkltest;

            // add more chains here...
            // ...
        }

    public:
        std::map<std::string, ChainUpgrades> mChainUpgrades;
        ChainUpgrades defaultUpgrades;
    } ccChainsUpgrades;

    static const ChainUpgrades *pSelectedUpgrades = &ccChainsUpgrades.defaultUpgrades;

    // return ref to chain upgrades list by chain name: 
    void SelectUpgrades(const std::string &chainName) {
        std::map<std::string, ChainUpgrades>::const_iterator it = ccChainsUpgrades.mChainUpgrades.find(chainName);
        if (it != ccChainsUpgrades.mChainUpgrades.end())  {
            pSelectedUpgrades = &it->second;
        }
        else {
            pSelectedUpgrades = &ccChainsUpgrades.defaultUpgrades;
        }
    }

    const ChainUpgrades &GetUpgrades()
    {
        return *pSelectedUpgrades;
    }


    bool IsUpgradeActive(int32_t nHeight, const ChainUpgrades &chainUpgrades, UPGRADE_ID id) {
        if (chainUpgrades.IsAllEnabled)
            return true;
        else {
            std::map<UPGRADE_ID, UpgradeInfo>::const_iterator it = chainUpgrades.mUpgrades.find(id);
            if (it != chainUpgrades.mUpgrades.end())
                return nHeight >= it->second.nActivationHeight ? it->second.status == UPGRADE_ACTIVE : false;
            return false;
        }
    }

}; // namespace CCUpgrades
