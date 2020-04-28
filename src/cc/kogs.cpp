/******************************************************************************
* Copyright � 2014-2019 The SuperNET Developers.                             *
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

#include "CCKogs.h"
#include <algorithm>    // shuffle
#include <random>       // default_random_engine

#include <rpc/server.h>

#ifndef KOMODO_ADDRESS_BUFSIZE
#define KOMODO_ADDRESS_BUFSIZE 64
#endif

// helpers
static std::map<uint8_t, std::string> objectids = {
    { KOGSID_GAMECONFIG, "KOGSID_GAMECONFIG" },
    { KOGSID_GAME, "KOGSID_GAME" },
    { KOGSID_PLAYER, "KOGSID_PLAYER" },
    { KOGSID_KOG, "KOGSID_KOG" },
    { KOGSID_SLAMMER, "KOGSID_SLAMMER" },
    { KOGSID_PACK, "KOGSID_PACK" },
    { KOGSID_CONTAINER, "KOGSID_CONTAINER" },
    { KOGSID_BATON , "KOGSID_BATON" },
    { KOGSID_SLAMPARAMS, "KOGSID_SLAMPARAMS" },
    { KOGSID_GAMEFINISHED, "KOGSID_GAMEFINISHED" },
    { KOGSID_ADVERTISING, "KOGSID_ADVERTISING" }
};


static CPubKey GetSystemPubKey()
{
    std::string spubkey = GetArg("-kogssyspk", "");
    if (!spubkey.empty())
    {
        vuint8_t vpubkey = ParseHex(spubkey.c_str());
        CPubKey pk = pubkey2pk(vpubkey);
        if (pk.size() == CPubKey::COMPRESSED_PUBLIC_KEY_SIZE)
            return pk;
    }
    return CPubKey();
}

static bool CheckSysPubKey()
{
    // TODO: use remote pk
    CPubKey mypk = pubkey2pk(Mypubkey());
    CPubKey syspk = GetSystemPubKey();
    if (!syspk.IsValid() || syspk.size() != CPubKey::COMPRESSED_PUBLIC_KEY_SIZE)
    {
        CCerror = "invalid kogssyspk";
        return false;
    }
    if (mypk != syspk)
    {
        CCerror = "operation disabled for your pubkey";
        return false;
    }
    return true;
}

// calculate how many kogs exist with this appearanceId
static int32_t AppearanceIdCount(int32_t appearanceId)
{
    // TODO: return count of nfts with this appearanceId
    return 0;
}

// get NFT unspent transaction
static bool GetNFTUnspentTx(uint256 tokenid, CTransaction &unspenttx)
{
    uint256 txid, spenttxid, dummytxid;
    int32_t vini, height, dummyvout;
    uint256 hashBlock;
    int32_t nvout = 1; // cc vout with token value in the tokenbase tx

	struct CCcontract_info *cpTokens, C;
	cpTokens = CCinit(&C, EVAL_TOKENS);

    txid = tokenid;
    while (CCgetspenttxid(spenttxid, vini, height, txid, nvout) == 0 && !myIsutxo_spentinmempool(dummytxid, dummyvout, txid, nvout))
    {
        txid = spenttxid;
        uint256 hashBlock;
        int32_t i;
        // nvout = 0; // cc vout with token value in the subsequent txns
        // get next vout for this tokenid
        if (!myGetTransaction(txid, unspenttx, hashBlock))
            return false;
        for(i = 0; i < unspenttx.vout.size(); i ++) {
            if (IsTokensvout(true, true, cpTokens, NULL, unspenttx, i, tokenid) > 0)    {
                nvout = i;
                break;
            }
        }
        if (i == unspenttx.vout.size())
            return false;  // no token vouts
    }

    // use non-locking ver as this func could be called from validation code
    // also check the utxo is not spent in mempool
    if (txid == tokenid)    {
        if (myGetTransaction(txid, unspenttx, hashBlock) && !myIsutxo_spentinmempool(dummytxid, dummyvout, txid, nvout))  
            return true;
        else
            return false;
    }
    return true;
}

// get previous token tx
static bool GetNFTPrevVout(const CTransaction &tokentx, CTransaction &prevtxout, int32_t &nvout, std::vector<CPubKey> &vpks)
{
    uint8_t evalcode;
    uint256 tokenid;
    std::vector<CPubKey> pks;
    std::vector<vscript_t> oprets;

    if (tokentx.vout.size() > 0 && DecodeTokenOpRetV1(tokentx.vout.back().scriptPubKey, tokenid, pks, oprets) != 0)
    {
        struct CCcontract_info *cpTokens, C;
        cpTokens = CCinit(&C, EVAL_TOKENS);

        for (auto vin : tokentx.vin)
        {
            if (cpTokens->ismyvin(vin.scriptSig))
            {
                uint256 hashBlock, tokenIdOpret;
                uint8_t version;
                vscript_t opret;
                std::vector<vscript_t> oprets;
                CTransaction prevtx;

                // get spent token tx
                if (GetTransaction(vin.prevout.hash, prevtx, hashBlock, true) &&
                    prevtx.vout.size() > 1 &&
                    DecodeTokenOpRetV1(prevtx.vout.back().scriptPubKey, tokenIdOpret, pks, oprets) != 0)
                {
                    for (int32_t v = 0; v < prevtx.vout.size(); v++)
                    {
                        if (IsTokensvout(false, true, cpTokens, NULL, prevtx, v, tokenid))  // if true token vout
                        {
                            prevtxout = prevtx;
                            nvout = v;
                            vpks = pks; // validation pubkeys
                            return true;
                        }
                    }
                }
                else
                {
                    CCerror = "can't load or decode prev token tx";
                    LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "could not load prev token tx txid=" << tokenid.GetHex() << std::endl);
                }
                break;
            }
        }
    }
    return false;
}


static bool IsNFTSpkMine(uint256 txid, const CScript &spk, const CPubKey &mypk, uint256 &tokenidOut)
{
    if (IsEqualScriptPubKeys(spk, MakeTokensCC1vout(EVAL_KOGS, 0, mypk).scriptPubKey))  
    {
        CScript opret;
        // check op_drop data
        if (!MyGetCCopretV2(spk, opret))    {
            CTransaction tx;
            uint256 hashBlock;
            if (myGetTransaction(txid, tx, hashBlock))  {
                if (tx.vout.size() > 0) {
                    opret = tx.vout.back().scriptPubKey;
                }
            }
        }
        if (opret.size() > 0)   {
            // check opreturn if no op_drop
            uint256 tokenid;
            std::vector<CPubKey> pks;
            std::vector<vscript_t> oprets;
            if (DecodeTokenOpRetV1(opret, tokenid, pks, oprets) != 0) {
                tokenidOut == tokenid;
                return true;
            }
        }
    }
    return false;
}

// check if token is on mypk
static bool IsNFTmine(uint256 reftokenid, const CPubKey &mypk)
{
// bad: very slow:
//    return GetTokenBalance(mypk, tokenid, true) > 0;
/*
    CTransaction lasttx;
    //CPubKey mypk = pubkey2pk(Mypubkey());

    if (GetNFTUnspentTx(tokenid, lasttx) &&
        lasttx.vout.size() > 1)
    {
        for (const auto &v : lasttx.vout)
        {
            if (IsEqualVouts (v, MakeTokensCC1vout(EVAL_KOGS, v.nValue, mypk)))
                return true;
        }
    }
    return false; */
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspents;

    struct CCcontract_info *cpKogs, C; 
    cpKogs = CCinit(&C, EVAL_KOGS);

    char tokenaddr[KOMODO_ADDRESS_BUFSIZE];
    GetTokensCCaddress(cpKogs, tokenaddr, mypk);    
    SetCCunspentsWithMempool(unspents, tokenaddr, true); 

    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it = unspents.begin(); it != unspents.end(); it++) 
    {
        uint256 tokenid;
        if (IsNFTSpkMine(it->first.txhash, it->second.script, mypk, tokenid) && tokenid == reftokenid)
            return true;
    }
    return false;
}

// check if enclosure is on mypk
static bool IsEnclosureMine(uint256 refid, const CPubKey &mypk)
{
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspents;

    struct CCcontract_info *cp, C; 
    cp = CCinit(&C, EVAL_KOGS);

    char ccaddr[KOMODO_ADDRESS_BUFSIZE];
    GetCCaddress(cp, ccaddr, mypk);    
    SetCCunspentsWithMempool(unspents, ccaddr, true); 

    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it = unspents.begin(); it != unspents.end(); it++) 
    {
        if (IsEqualScriptPubKeys (it->second.script, MakeCC1vout(EVAL_KOGS, it->second.satoshis, mypk).scriptPubKey))  {
            CScript opret;
			// check op_drop data
            if (!MyGetCCopretV2(it->second.script, opret))    {
                CTransaction tx;
                uint256 hashBlock;
                if (myGetTransaction(it->first.txhash, tx, hashBlock))  {
                    if (tx.vout.size() > 0) {
                        opret = tx.vout.back().scriptPubKey;
                    }
                }
            }
            if (opret.size() > 0)   {
				// check opreturn if no op_drop
				vuint8_t vopret;
				GetOpReturnData(opret, vopret);
				KogsEnclosure encl;
				if (E_UNMARSHAL(vopret, ss >> encl))	{
                    if (encl.creationtxid == refid)
                        return true;
                }
            }
        }
    }
    return false;
}

// check if token has been burned
static bool IsNFTBurned(uint256 tokenid, CTransaction &lasttx)
{
    if (GetNFTUnspentTx(tokenid, lasttx) &&
        // lasttx.vout.size() > 1 &&
        HasBurnedTokensvouts(lasttx, tokenid) > 0)
        return true;
    else
        return false;
}

static bool IsTxSigned(const CTransaction &tx)
{
    for (const auto &vin : tx.vin)
        if (vin.scriptSig.size() == 0)
            return false;

    return true;
}

// checks if game finished
static bool IsGameFinished(const KogsGameConfig &gameconfig, const KogsBaton *pbaton) 
{ 
    return  pbaton->prevturncount > 0 && pbaton->kogsInStack.empty() || pbaton->prevturncount >= pbaton->playerids.size() * gameconfig.maxTurns;
}

// create game object NFT by calling token cc function
static UniValue CreateGameObjectNFT(const CPubKey &remotepk, struct KogsBaseObject *baseobj)
{
    vscript_t vnftdata = baseobj->Marshal(); // E_MARSHAL(ss << baseobj);
    if (vnftdata.empty())
    {
        CCerror = std::string("can't marshal object with id=") + std::string(1, (char)baseobj->objectType);
        return NullUniValue; // return empty obj
    }

    UniValue sigData = CreateTokenExt(remotepk, 0, 1, baseobj->nameId, baseobj->descriptionId, vnftdata, EVAL_KOGS, true);

    if (!ResultHasTx(sigData))
        return NullUniValue;  // return empty obj

    /* feature with sending tx inside the rpc
    // decided not todo this - for reviewing by the user
    // send the tx:
    // unmarshal tx to get it txid;
    vuint8_t vtx = ParseHex(hextx);
    CTransaction matchobjtx;
    if (!E_UNMARSHAL(vtx, ss >> matchobjtx)) {
        return std::string("error: can't unmarshal tx");
    }

    //RelayTransaction(matchobjtx);
    UniValue rpcparams(UniValue::VARR), txparam(UniValue::VOBJ);
    txparam.setStr(hextx);
    rpcparams.push_back(txparam);
    try {
        sendrawtransaction(rpcparams, false);  // NOTE: throws error!
    }
    catch (std::runtime_error error)
    {
        return std::string("error: bad parameters: ") + error.what();
    }
    catch (UniValue error)
    {
        return std::string("error: can't send tx: ") + error.getValStr();
    }

    std::string hextxid = matchobjtx.GetHash().GetHex();
    return hextxid;
    */

    return sigData;
}

// create enclosure tx (similar but not exactly like NFT as enclosure could be changed) with game object inside
static UniValue CreateEnclosureTx(const CPubKey &remotepk, KogsBaseObject *baseobj, bool isSpendable, bool needBaton)
{
    const CAmount  txfee = 10000;
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());

    bool isRemote = IS_REMOTE(remotepk);
    CPubKey mypk = isRemote ? remotepk : pubkey2pk(Mypubkey());

    struct CCcontract_info *cp, C;
    cp = CCinit(&C, EVAL_KOGS);

    KogsEnclosure enc(mypk);  //'zeroid' means 'for creation'

    enc.vdata = baseobj->Marshal();
    enc.name = baseobj->nameId;
    enc.description = baseobj->descriptionId;

    CAmount normals = txfee + KOGS_NFT_MARKER_AMOUNT;
    if (needBaton)
        normals += KOGS_BATON_AMOUNT;

    if (AddNormalinputs(mtx, mypk, normals, 0x10000, isRemote) > 0)
    {
        mtx.vout.push_back(MakeCC1vout(EVAL_KOGS, 1, mypk)); // spendable vout for transferring the enclosure ownership
        mtx.vout.push_back(MakeCC1vout(EVAL_KOGS, KOGS_NFT_MARKER_AMOUNT, GetUnspendable(cp, NULL)));  // kogs cc marker
        if (needBaton)
            mtx.vout.push_back(MakeCC1vout(EVAL_KOGS, KOGS_BATON_AMOUNT, GetUnspendable(cp, NULL))); // initial marker for miners who will create a baton indicating whose turn is first

        CScript opret;
        opret << OP_RETURN << enc.EncodeOpret();
        UniValue sigData = FinalizeCCTxExt(isRemote, 0, cp, mtx, mypk, txfee, opret);
        if (ResultHasTx(sigData))
            return sigData;
        else
            CCerror = "can't finalize or sign tx";
    }
    else
        CCerror = "can't find normals for 2 txfee";
    return NullUniValue;
}

// create baton tx to pass turn to the next player
// called by a miner
static void AddBatonVouts(CMutableTransaction &mtx, struct CCcontract_info *cp, uint256 prevtxid, int32_t prevn, const KogsBaseObject *pbaton, const CPubKey &destpk, CScript &opret)
{
    const CAmount  txfee = 10000;
    //CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
    CPubKey minerpk = pubkey2pk(Mypubkey());  // we have mypk in the wallet, no remote call for baton

    KogsEnclosure enc(minerpk);  // 'zeroid' means 'for creation'
    enc.vdata = pbaton->Marshal();
    enc.name = pbaton->nameId;
    enc.description = pbaton->descriptionId;

    //if (AddNormalinputs(mtx, minerpk, txfee, 0x10000, false) > 0)
    //{
    mtx.vin.push_back(CTxIn(prevtxid, prevn));  // spend the prev game or slamparam baton
    mtx.vout.push_back(MakeCC1vout(EVAL_KOGS, KOGS_BATON_AMOUNT, destpk)); // baton to indicate whose turn is now

    // add probe cc and kogs priv to spend from kogs global pk
    struct CCcontract_info *cpKogs, C;
    cpKogs = CCinit(&C, EVAL_KOGS);
    uint8_t kogspriv[32];
    CPubKey kogsPk = GetUnspendable(cpKogs, kogspriv);

    CC* probeCond = MakeCCcond1(EVAL_KOGS, kogsPk);
    CCAddVintxCond(cp, probeCond, kogspriv);
    cc_free(probeCond);

    //CScript opret;
    opret << OP_RETURN << enc.EncodeOpret();  // create opreturn
        /*std::string hextx = FinalizeCCTx(0, cp, mtx, minerpk, txfee, opret);  // TODO why was destpk here (instead of minerpk)?
        if (hextx.empty())
        {
            LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "can't create baton for txid=" << prevtxid.GetHex() << " could not finalize tx" << std::endl);
            return CTransaction(); // empty tx
        }
        else
        {
            return mtx;
        }*/
    ///}
    //LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "can't normal inputs for txfee" << std::endl);
    //return CTransaction(); // empty tx
}

// create baton tx to pass turn to the next player
// called by a miner
static CTransaction CreateBatonTx(uint256 prevtxid, int32_t prevn, const KogsBaseObject *pbaton, CPubKey destpk)
{
    const CAmount  txfee = 10000;
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
    CPubKey minerpk = pubkey2pk(Mypubkey());  // we have mypk in the wallet, no remote call for baton

    struct CCcontract_info *cp, C;
    cp = CCinit(&C, EVAL_KOGS);

    KogsEnclosure enc(minerpk);  // 'zeroid' means 'for creation'
    enc.vdata = pbaton->Marshal();
    enc.name = pbaton->nameId;
    enc.description = pbaton->descriptionId;

    if (AddNormalinputs(mtx, minerpk, txfee, 0x10000, false) > 0)
    {
        mtx.vin.push_back(CTxIn(prevtxid, prevn));  // spend the prev game or slamparam baton
        mtx.vout.push_back(MakeCC1vout(EVAL_KOGS, KOGS_BATON_AMOUNT, destpk)); // baton to indicate whose turn is now

        CScript opret;
        opret << OP_RETURN << enc.EncodeOpret();
        std::string hextx = FinalizeCCTx(0, cp, mtx, minerpk, txfee, opret);  // TODO why was destpk here (instead of minerpk)?
        if (hextx.empty())
        {
            LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "can't create baton for txid=" << prevtxid.GetHex() << " could not finalize tx" << std::endl);
            return CTransaction(); // empty tx
        }
        else
        {
            return mtx;
        }
    }
    LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "can't normal inputs for txfee" << std::endl);
    return CTransaction(); // empty tx
}


static bool LoadTokenData(const CTransaction &tx, int32_t nvout, uint256 &creationtxid, vuint8_t &vorigpubkey, std::string &name, std::string &description, std::vector<vscript_t> &oprets)
{
    uint256 tokenid;
    uint8_t funcid, evalcode, version;
    std::vector<CPubKey> pubkeys;
    CTransaction createtx;

    if (tx.vout.size() > 0)
    {
        CScript opret;
        if (!MyGetCCopretV2(tx.vout[nvout].scriptPubKey, opret)) 
            opret = tx.vout.back().scriptPubKey;

        if ((funcid = DecodeTokenOpRetV1(opret, tokenid, pubkeys, oprets)) != 0)
        {
            if (IsTokenTransferFuncid(funcid))
            {
                uint256 hashBlock;
                if (!myGetTransaction(tokenid, createtx, hashBlock) /*|| hashBlock.IsNull()*/)  //use non-locking version, check that tx not in mempool
                {
                    return false;
                }
                creationtxid = tokenid;
            }
            else if (IsTokenCreateFuncid(funcid))
            {
                createtx = tx;
                creationtxid = createtx.GetHash();
            }

            if (!createtx.IsNull())
            {
                if (DecodeTokenCreateOpRetV1(createtx.vout.back().scriptPubKey, vorigpubkey, name, description, oprets) != 0)
                    return true;
            }
        }
    }
    else
        LOGSTREAMFN("kogs", CCLOG_DEBUG1, stream << "no opret in token" << std::endl);
    return false;
}


static struct KogsBaseObject *DecodeGameObjectOpreturn(const CTransaction &tx, int32_t nvout)
{
    CScript opret;
    vscript_t vopret;

    if (tx.vout.size() < 1) {
        LOGSTREAMFN("kogs", CCLOG_INFO, stream << "cant find vouts in txid=" << tx.GetHash().GetHex() << std::endl);
        return nullptr;
    }
    if (nvout < 0 || nvout >= tx.vout.size()) {
        LOGSTREAMFN("kogs", CCLOG_INFO, stream << "nvout out of bounds txid=" << tx.GetHash().GetHex() << std::endl);
        return nullptr;
    }

    if (MyGetCCopretV2(tx.vout[nvout].scriptPubKey, opret)) 
        GetOpReturnData(opret, vopret);
    else
        GetOpReturnData(tx.vout.back().scriptPubKey, vopret);
        
    if (vopret.size() < 2)
    {
        LOGSTREAMFN("kogs", CCLOG_INFO, stream << "cant find opret in txid=" << tx.GetHash().GetHex() << std::endl);
        return nullptr;
    }

    if (vopret.begin()[0] == EVAL_TOKENS)
    {
        vuint8_t vorigpubkey;
        std::string name, description;
        std::vector<vscript_t> oprets;
        uint256 tokenid;

        // parse tokens:
        // find CREATION TX and get NFT data
        if (LoadTokenData(tx, nvout, tokenid, vorigpubkey, name, description, oprets))
        {
            vscript_t vnftopret;
            if (GetOpReturnCCBlob(oprets, vnftopret))
            {
                uint8_t objectType;
                CTransaction dummytx;
                if (!KogsBaseObject::DecodeObjectHeader(vnftopret, objectType)) {
                    LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "DecodeObjectHeader tokens returned null tokenid=" << tokenid.GetHex() << std::endl);
                    return nullptr;
                }

                // TODO: why to check here whether nft is burned?
                // we need to load burned nfts too!
                // if (IsNFTBurned(creationtxid, dummytx))
                //    return nullptr;

                KogsBaseObject *obj = KogsFactory::CreateInstance(objectType);
                if (obj == nullptr) {
                    LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "CreateInstance tokens returned null tokenid=" << tokenid.GetHex() << std::endl);
                    return nullptr;
                }

                if (obj->Unmarshal(vnftopret))
                {
                    obj->creationtxid = tokenid;
                    obj->nameId = name;
                    obj->descriptionId = description;
                    obj->encOrigPk = pubkey2pk(vorigpubkey);
                    obj->istoken = true;
					obj->funcid = vopret[1];
                    return obj;
                }
                else
                    LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "cant unmarshal nft to GameObject for tokenid=" << tokenid.GetHex() << std::endl);
            }
            else
                LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "cant find nft opret in token opret for tokenid=" << tokenid.GetHex() << std::endl);
        }
        else
            LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "cant load token data for tokenid=" << tokenid.GetHex() << std::endl);
    }
    else if (vopret.begin()[0] == EVAL_KOGS)
    {
        KogsEnclosure enc;

        // parse kogs enclosure:
        if (KogsEnclosure::DecodeLastOpret(tx, enc))   // finds THE FIRST and LATEST TX and gets data from the oprets
        {
            uint8_t objectType;

            if (!KogsBaseObject::DecodeObjectHeader(enc.vdata, objectType)) {
                LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "DecodeObjectHeader kogs returned null txid=" << tx.GetHash().GetHex() << std::endl);
                return nullptr;
            }

            KogsBaseObject *obj = KogsFactory::CreateInstance(objectType);
            if (obj == nullptr) {
                LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "CreateInstance kogs returned null txid=" << tx.GetHash().GetHex() << std::endl);
                return nullptr;
            }
            if (obj->Unmarshal(enc.vdata))
            {
                obj->creationtxid = enc.creationtxid;
                obj->nameId = enc.name;
                obj->descriptionId = enc.description;
                obj->encOrigPk = enc.origpk;
                //obj->latesttx = enc.latesttx;
                obj->istoken = false;
				obj->funcid = vopret[1];
                return obj;
            }
            else
                LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "cant unmarshal non-nft kogs object to GameObject txid=" << tx.GetHash().GetHex() << std::endl);
        }
        else
            LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "cant decode opret of non-nft kogs object txid=" << tx.GetHash().GetHex() << std::endl);
    }
    else
    {
        LOGSTREAMFN("kogs", CCLOG_INFO, stream << "not kogs or token opret" << std::endl);
    }
    return nullptr;
}


// load any kogs game object for any ot its txids
static struct KogsBaseObject *LoadGameObject(uint256 txid, int32_t nvout)
{
    uint256 hashBlock;
    CTransaction tx;

    if (myGetTransaction(txid, tx, hashBlock)/* && !hashBlock.IsNull()*/)  //use non-locking version, check not in mempool
    {
        if (nvout == 10e8)
            nvout = tx.vout.size() - 1;
        KogsBaseObject *pBaseObj = DecodeGameObjectOpreturn(tx, nvout);   
		// check if only sys key allowed to create object
		if (pBaseObj && (KogsIsSysCreateObject(pBaseObj->objectType) && GetSystemPubKey() != pBaseObj->encOrigPk))
			return nullptr;
		return pBaseObj;
    }
    return nullptr;
}

static struct KogsBaseObject *LoadGameObject(uint256 txid)
{
    return LoadGameObject(txid, 10e8);  // 10e8 means use last vout opreturn
}



// game object checker if NFT is mine
class IsNFTMineChecker : public KogsObjectFilterBase  {
public:
    IsNFTMineChecker(const CPubKey &_mypk) { mypk = _mypk; }
    virtual bool operator()(KogsBaseObject *obj) {
        return obj != NULL && IsNFTmine(obj->creationtxid, mypk);
    }
private:
    CPubKey mypk;
};

class GameHasPlayerIdChecker : public KogsObjectFilterBase {
public:
    GameHasPlayerIdChecker(uint256 _playerid) { playerid = _playerid; }
    virtual bool operator()(KogsBaseObject *obj) {
        if (obj != NULL && obj->objectType == KOGSID_GAME)
        {
            KogsGame *game = (KogsGame*)obj;
            return std::find(game->playerids.begin(), game->playerids.end(), playerid) != game->playerids.end();
        }
        else
            return false;
    }
private:
    uint256 playerid;
};

static void ListGameObjects(const CPubKey &remotepk, uint8_t objectType, bool onlyMine, KogsObjectFilterBase *pObjFilter, std::vector<std::shared_ptr<KogsBaseObject>> &list)
{
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > addressUnspents;
    bool isRemote = IS_REMOTE(remotepk);
    CPubKey mypk = isRemote ? remotepk : pubkey2pk(Mypubkey());

    struct CCcontract_info *cp, C; 
    cp = CCinit(&C, EVAL_KOGS);

    LOGSTREAMFN("kogs", CCLOG_DEBUG1, stream << "getting all objects with objectType=" << (char)objectType << std::endl);
    if (onlyMine == false) {
        // list all objects by marker:
        SetCCunspentsWithMempool(addressUnspents, cp->unspendableCCaddr, true);    // look all tx on cc addr 
    }
    else {
        // list my objects by utxos on token+kogs or kogs address
		// TODO: add check if this is nft or enclosure
		// if this is nfts:
        char tokenaddr[KOMODO_ADDRESS_BUFSIZE];
        GetTokensCCaddress(cp, tokenaddr, mypk);    
        SetCCunspentsWithMempool(addressUnspents, tokenaddr, true); 

		// if this is kogs 'enclosure'
        char kogsaddr[KOMODO_ADDRESS_BUFSIZE];
        GetCCaddress(cp, kogsaddr, mypk);    
        SetCCunspentsWithMempool(addressUnspents, kogsaddr, true);         
    }

    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it = addressUnspents.begin(); it != addressUnspents.end(); it++) 
    {
        if (onlyMine || it->second.satoshis == KOGS_NFT_MARKER_AMOUNT) // check for marker==10000 to differenciate it from batons with 20000
        {
            struct KogsBaseObject *obj = LoadGameObject(it->first.txhash, it->first.index); // parse objectType and unmarshal corresponding gameobject
            if (obj != nullptr && obj->objectType == objectType && (pObjFilter == NULL || (*pObjFilter)(obj)))
                list.push_back(std::shared_ptr<KogsBaseObject>(obj)); // wrap with auto ptr to auto-delete it
        }
    }
    LOGSTREAMFN("kogs", CCLOG_DEBUG1, stream << "found=" << list.size() << " objects with objectType=" << (char)objectType << std::endl);
}

// loads tokenids from 1of2 address (kogsPk, containertxidPk) and adds the tokenids to container object
static void ListContainerTokenids(KogsContainer &container)
{
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > addressUnspents;

    struct CCcontract_info *cp, C;
    cp = CCinit(&C, EVAL_KOGS);

    char txidaddr[KOMODO_ADDRESS_BUFSIZE];
    char tokenaddr[KOMODO_ADDRESS_BUFSIZE];
    CPubKey kogsPk = GetUnspendable(cp, NULL);
    CPubKey containertxidPk = CCtxidaddr_tweak(txidaddr, container.creationtxid);
    GetTokensCCaddress1of2(cp, tokenaddr, kogsPk, containertxidPk);

    SetCCunspentsWithMempool(addressUnspents, tokenaddr, true);    // look all tx on 1of2 addr
    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it = addressUnspents.begin(); it != addressUnspents.end(); it++) 
    {
        uint256 dummytxid;
        int32_t dummyvout;
        //if (!myIsutxo_spentinmempool(dummytxid, dummyvout, it->first.txhash, it->first.index))
        //{
        std::shared_ptr<KogsBaseObject> spobj(LoadGameObject(it->first.txhash, it->first.index)); // load and unmarshal gameobject for this txid
        if (spobj != nullptr && KogsIsMatchObject(spobj->objectType))
            container.tokenids.push_back(spobj->creationtxid);
        //}
    }
}

// create slam param tx to send slam height and strength to the chain
static UniValue CreateSlamParamTx(const CPubKey &remotepk, uint256 prevtxid, int32_t prevn, const KogsSlamParams &slamparam)
{
    const CAmount  txfee = 10000;
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());

    bool isRemote = IS_REMOTE(remotepk);
    CPubKey mypk = isRemote ? remotepk : pubkey2pk(Mypubkey());

    struct CCcontract_info *cp, C;
    cp = CCinit(&C, EVAL_KOGS);

    KogsEnclosure enc(mypk);  // 'zeroid' means 'for creation'
    enc.vdata = slamparam.Marshal();
    enc.name = slamparam.nameId;
    enc.description = slamparam.descriptionId;

    mtx.vin.push_back(CTxIn(prevtxid, prevn));  // spend the prev baton

    if (AddNormalinputs(mtx, mypk, 3*txfee, 0x10000, isRemote) > 0)
    {
        mtx.vout.push_back(MakeCC1vout(EVAL_KOGS, KOGS_BATON_AMOUNT, GetUnspendable(cp, NULL))); // marker to find batons

        CScript opret;
        opret << OP_RETURN << enc.EncodeOpret();
        UniValue sigData = FinalizeCCTxExt(isRemote, 0, cp, mtx, mypk, txfee, opret);  // TODO why was destpk here (instead of minerpk)?
        if (!ResultHasTx(sigData)) {
            CCerror = "could not finalize or sign slam param transaction";
            return NullUniValue;
        }
        else
        {
            return sigData;
        }
    }
    else
    {
        CCerror = "could not find normal inputs for txfee";
        return NullUniValue; // empty 
    }
}

// create an advertising tx to make known the player is ready to play
static UniValue CreateAdvertisingTx(const CPubKey &remotepk, const KogsAdvertising &ad)
{
    const CAmount  txfee = 10000;
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());

    bool isRemote = IS_REMOTE(remotepk);
    CPubKey mypk = isRemote ? remotepk : pubkey2pk(Mypubkey());

    struct CCcontract_info *cp, C;
    cp = CCinit(&C, EVAL_KOGS);

    KogsEnclosure enc(mypk);  
    enc.vdata = ad.Marshal();
    enc.name = ad.nameId;
    enc.description = ad.descriptionId;

    if (AddNormalinputsRemote(mtx, mypk, txfee, 0x10000) > 0)   // add always from mypk because it will be checked who signed this advertising tx
    {
        mtx.vout.push_back(MakeCC1vout(EVAL_KOGS, KOGS_ADVERISING_AMOUNT, GetUnspendable(cp, NULL))); // baton for miner to indicate the slam data added

        CScript opret;
        opret << OP_RETURN << enc.EncodeOpret();
        UniValue sigData = FinalizeCCTxExt(isRemote, 0, cp, mtx, mypk, txfee, opret);
        if (!ResultHasTx(sigData)) {
            CCerror = "could not finalize or sign advertising transaction";
            return NullUniValue;
        }
        return sigData;
    }
    else
    {
        CCerror = "could not find normal inputs for txfee";
        return NullUniValue; // empty 
    }
}

// loads container ids deposited on gameid 1of2 addr
static void KogsDepositedContainerListImpl(uint256 gameid, std::vector<std::shared_ptr<KogsContainer>> &containers)
{
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > addressUnspents;

    struct CCcontract_info *cp, C;
    cp = CCinit(&C, EVAL_KOGS);

    char txidaddr[KOMODO_ADDRESS_BUFSIZE];
    char tokenaddr[KOMODO_ADDRESS_BUFSIZE];
    CPubKey kogsPk = GetUnspendable(cp, NULL);
    CPubKey gametxidPk = CCtxidaddr_tweak(txidaddr, gameid);
    GetTokensCCaddress1of2(cp, tokenaddr, kogsPk, gametxidPk);

    SetCCunspentsWithMempool(addressUnspents, tokenaddr, true);    // look all tx on 1of2 addr
    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it = addressUnspents.begin(); it != addressUnspents.end(); it++)
    {
        KogsBaseObject* pobj = LoadGameObject(it->first.txhash, it->first.index); // load and unmarshal gameobject for this txid
        if (pobj != nullptr && pobj->objectType == KOGSID_CONTAINER)
        {
            std::shared_ptr<KogsContainer> spcontainer((KogsContainer*)pobj);
            containers.push_back(spcontainer);
        }
    }
}

// if playerId set returns found adtxid and nvout
// if not set returns all advertisings (checked if signed correctly) in adlist
static bool FindAdvertisings(uint256 playerId, uint256 &adtxid, int32_t &nvout, std::vector<KogsAdvertising> &adlist)
{
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > addressUnspents;
    struct CCcontract_info *cp, C;
    cp = CCinit(&C, EVAL_KOGS);

    char kogsaddr[KOMODO_ADDRESS_BUFSIZE];
    GetCCaddress(cp, kogsaddr, GetUnspendable(cp, NULL));

    LOGSTREAMFN("kogs", CCLOG_DEBUG1, stream << "searching my advertizing marker" << std::endl);

    // check if advertising is already on kogs global:
    SetCCunspentsWithMempool(addressUnspents, kogsaddr, true);    // look for baton on my cc addr 
    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it = addressUnspents.begin(); it != addressUnspents.end(); it++)
    {
        uint256 dummytxid;
        int32_t dummyvout;
        // its very important to check if the baton not spent in mempool, otherwise we could pick up a previous already spent baton
        if (it->second.satoshis == KOGS_ADVERISING_AMOUNT /*&& !myIsutxo_spentinmempool(dummytxid, dummyvout, it->first.txhash, it->first.index)*/) // picking markers==5000
        {
            std::shared_ptr<KogsBaseObject> spadobj(LoadGameObject(it->first.txhash, it->first.index));
            if (spadobj != nullptr && spadobj->objectType == KOGSID_ADVERTISING)
            {
                KogsAdvertising* padobj = (KogsAdvertising*)spadobj.get();
                CTransaction tx;
                uint256 hashBlock;

                if (playerId.IsNull() || padobj->playerId == playerId)
                {
                    // not very good: second time tx load
                    if (myGetTransaction(it->first.txhash, tx, hashBlock) && TotalPubkeyNormalInputs(tx, padobj->encOrigPk) > 0) // check if player signed
                    {
                        if (!playerId.IsNull()) // find a specific player ad object
                        {
                            //if (padobj->encOrigPk == mypk) we already checked in the caller that playerId is signed with mypk
                            //{
                            adtxid = it->first.txhash;
                            nvout = it->first.index;
                            return true;
                            //}
                        }
                        else
                            adlist.push_back(*padobj);
                    }
                }
            }
        }
    }
    return false;
}

// get percentage range for the given value (height or strength)
static int getRange(const std::vector<KogsSlamRange> &range, int32_t val)
{
    if (range.size() < 2) {
        LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "incorrect range size" << std::endl);
        return -1;
    }

    if (val < range[0].upperValue)
        return 0;

    for (int i = 1; i < range.size(); i++)
        if (val < range[i].upperValue)
            return i;
    if (val <= range[range.size() - 1].upperValue)
        return range.size() - 1;

    LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "height or strength value too big=" << val << std::endl);
    return -1;
}

// flip kogs based on slam data and height and strength ranges
// flip kogs based on slam data and height and strength ranges
static bool FlipKogs(const KogsGameConfig &gameconfig, const KogsSlamParams &slamparams, KogsBaton &newbaton, const KogsBaton *ptestbaton)
{
    std::vector<KogsSlamRange> heightRanges = heightRangesDefault;
    std::vector<KogsSlamRange> strengthRanges = strengthRangesDefault;
    if (gameconfig.heightRanges.size() > 0)
        heightRanges = gameconfig.heightRanges;
    if (gameconfig.strengthRanges.size() > 0)
        strengthRanges = gameconfig.strengthRanges;

    int iheight = getRange(heightRanges, slamparams.armHeight);
    int istrength = getRange(strengthRanges, slamparams.armStrength);

    LOGSTREAMFN("kogs", CCLOG_DEBUG1, stream << "iheight=" << iheight << " heightRanges.size=" << heightRanges.size() << " istrength=" << istrength  << " strengthRanges.size=" << strengthRanges.size() << std::endl);

    if (iheight < 0 || istrength < 0)   {
        LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "slamparam out of range: iheight=" << iheight << " heightRanges.size=" << heightRanges.size() << " istrength=" << istrength  << " strengthRanges.size=" << strengthRanges.size() << std::endl);
        return false;
    }
    // calc percentage of flipped based on height or ranges
    int randH;
    int randS;
    if (ptestbaton == nullptr)  {
        // make random range offset
        newbaton.randomHeightRange = rand();
        newbaton.randomStrengthRange = rand();  
    }
    else {
        // use existing randoms to validate
        newbaton.randomHeightRange = ptestbaton->randomHeightRange;
        newbaton.randomStrengthRange = ptestbaton->randomStrengthRange;
    }
    int heightFract = heightRanges[iheight].left + newbaton.randomHeightRange % (heightRanges[iheight].right - heightRanges[iheight].left);
    int strengthFract = strengthRanges[istrength].left + newbaton.randomStrengthRange % (strengthRanges[istrength].right - strengthRanges[istrength].left);
    int totalFract = heightFract + strengthFract;

    LOGSTREAMFN("kogs", CCLOG_DEBUG1, stream << "heightFract=" << heightFract << " strengthFract=" << strengthFract << std::endl);

    int countFlipped = 0;
    if (ptestbaton == nullptr)  {

        // how many kogs would flip:
        int countFlipped = (newbaton.kogsInStack.size() * totalFract) / 100;
        if (countFlipped > newbaton.kogsInStack.size())
            countFlipped = newbaton.kogsInStack.size();
        // set limit for 1st turn: no more than 50% flipped
        if (newbaton.prevturncount == 1 && countFlipped > newbaton.kogsInStack.size() / 2)
            countFlipped = newbaton.kogsInStack.size() / 2; //no more than 50%
    }
    else {
        // test countFlipped
        // get countFlipped from stack in testbaton and prev stack:
        countFlipped = 0;
        for (int i = 0; i < newbaton.kogsInStack.size(); i ++)    {
            if (i >= ptestbaton->kogsInStack.size() || newbaton.kogsInStack[i] != ptestbaton->kogsInStack[i])
            countFlipped ++;
        }
        // check countFlipped for min max:
        if (ptestbaton->prevturncount == 1 && countFlipped > newbaton.kogsInStack.size() / 2)
            // set countFlipped for the first turn
            countFlipped = newbaton.kogsInStack.size() / 2; //no more than 50%
        else {
            // check countFlipped for min max:
            int countFlippedMin = (heightRanges[iheight].left + strengthRanges[istrength].left) * newbaton.kogsInStack.size() / 100;
            int countFlippedMax = (heightRanges[iheight].right + strengthRanges[istrength].right) * newbaton.kogsInStack.size() / 100;
            if (countFlipped < countFlippedMin || countFlipped > countFlippedMax)   {
                LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "countFlipped out of range: countFlipped=" << countFlipped << " countFlippedMin=" << countFlippedMin << " countFlippedMax=" << countFlippedMax << std::endl);
                return false;
            }
        }
    }

    LOGSTREAMFN("kogs", CCLOG_DEBUG1, stream << "countFlipped=" << countFlipped << std::endl);

    // find previous turn index
    int32_t prevturn = newbaton.nextturn - 1;
    if (prevturn < 0)
        prevturn = newbaton.playerids.size() - 1; 

    // randomly select flipped kogs:
    while (countFlipped--)
    {
        //int i = rand() % baton.kogsInStack.size();
        int i = newbaton.kogsInStack.size() - 1;           // remove kogs from top
        
        newbaton.kogsFlipped.push_back(std::make_pair(newbaton.playerids[prevturn], newbaton.kogsInStack[i]));
        LOGSTREAMFN("kogs", CCLOG_DEBUG1, stream << "flipped kog id=" << newbaton.kogsInStack[i].GetHex() << std::endl);
        newbaton.kogsInStack.erase(newbaton.kogsInStack.begin() + i);
    }

    return true;
}
 

// adding kogs to stack from the containers, check if kogs are not in the stack already or not flipped
static bool AddKogsToStack(const KogsGameConfig &gameconfig, KogsBaton &baton, const std::vector<std::shared_ptr<KogsContainer>> &spcontainers)
{
    // int remainder = 4 - baton.kogsInStack.size(); 
    // int kogsToAdd = remainder / spcontainers.size(); // I thought first that kogs must be added until stack max size (it was 4 for testing)

    int kogsToAddFromPlayer; 
    if (baton.prevturncount == 0) // first turn
        kogsToAddFromPlayer = gameconfig.numKogsInStack / baton.playerids.size();  //first time add until max stack
    else
        kogsToAddFromPlayer = gameconfig.numKogsToAdd;

    for (auto c : spcontainers)
    {
        // get kogs that are not yet in the stack or flipped
        std::vector<uint256> freekogs;
        for (const auto &t : c->tokenids)
        {
            // check if kog in stack
            if (std::find(baton.kogsInStack.begin(), baton.kogsInStack.end(), t) != baton.kogsInStack.end())
                continue;
            // check if kog in flipped 
            if (std::find_if(baton.kogsFlipped.begin(), baton.kogsFlipped.end(), [&](std::pair<uint256, uint256> f) { return f.second == t;}) != baton.kogsFlipped.end())
                continue;
            freekogs.push_back(t);
        }
        if (kogsToAddFromPlayer > freekogs.size())
        {
            LOGSTREAMFN("kogs", CCLOG_INFO, stream << "kogs number remaining in container=" << freekogs.size() << " is less than needed to add to stack=" << kogsToAddFromPlayer << ", won't add kogs" << std::endl);
            return false;
        }

        int added = 0;
        while (added < kogsToAddFromPlayer && freekogs.size() > 0)
        {
            int i = rand() % freekogs.size(); // take random pos to add to the stack
            baton.kogsInStack.push_back(freekogs[i]);  // add to stack
            LOGSTREAMFN("kogs", CCLOG_DEBUG1, stream << "added kog to stack kogid=" << freekogs[i].GetHex() << std::endl);
            freekogs.erase(freekogs.begin() + i);
            added++;

            /*            
            int i = rand() % c->tokenids.size(); // take random pos to add to the stack
            if (std::find(kogsInStack.begin(), kogsInStack.end(), c->tokenids[i]) == kogsInStack.end()) //add only if no such kog id in stack yet
            {
                kogsInStack.push_back(c->tokenids[i]);  // add to stack
                added++;
            }*/
        }
    }

    // shuffle kogs in the stack:
    std::shuffle(baton.kogsInStack.begin(), baton.kogsInStack.end(), std::default_random_engine(time(NULL)));  // TODO: check if any interference with rand() in KogsCreateMinerTransactions

    return true;
}

static bool KogsManageStack(const KogsGameConfig &gameconfig, KogsBaseObject *pGameOrParams, KogsBaton *prevbaton, KogsBaton &newbaton, const KogsBaton *ptestbaton, std::vector<std::shared_ptr<KogsContainer>> &containers)
{   
    if (pGameOrParams->objectType != KOGSID_GAME && pGameOrParams->objectType != KOGSID_SLAMPARAMS)
    {
        LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "incorrect objectType=" << (char)pGameOrParams->objectType << std::endl);
        return false;
    }

    uint256 gameid;
    std::vector<uint256> playerids;
    if (pGameOrParams->objectType == KOGSID_GAME) {
        gameid = pGameOrParams->creationtxid;
        playerids = ((KogsGame *)pGameOrParams)->playerids;
    }
    else
    {
        if (prevbaton == nullptr) // check for internal logic error
        {
            LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "prevbaton is null" << (char)pGameOrParams->objectType << std::endl);
            return false;
        }
        gameid = ((KogsSlamParams*)pGameOrParams)->gameid;
        playerids = prevbaton->playerids;
    }

    KogsDepositedContainerListImpl(gameid, containers);

    //find kog tokenids on container 1of2 address
    for (const auto &c : containers)
        ListContainerTokenids(*c);

    // store containers' owner playerids
    std::set<uint256> owners;
    for(const auto &c : containers)
        owners.insert(c->playerid);

    // check deposited containers
    bool IsSufficientContainers = true;
    if (containers.size() != playerids.size())
    {
        static thread_local std::map<uint256, bool> gameid_container_num_errlogs;  // store flag if already warned 

        if (!gameid_container_num_errlogs[gameid])   // prevent logging this message on each loop when miner creates transactions
        {
            LOGSTREAMFN("kogs", CCLOG_INFO, stream << "warning: not all players deposited containers yet, gameid=" << gameid.GetHex() << std::endl);
            for (const auto &c : containers)
                LOGSTREAMFN("kogs", CCLOG_DEBUG1, stream << "deposited container=" << c->creationtxid.GetHex() << std::endl);
            for (const auto &p : playerids)
                LOGSTREAMFN("kogs", CCLOG_DEBUG1, stream << "player=" << p.GetHex() << std::endl);
            gameid_container_num_errlogs[gameid] = true;
        }

        //if (pGameOrParams->objectType != KOGSID_GAME)
        //    LOGSTREAMFN("kogs", CCLOG_INFO, stream << "some containers transferred back, gameid=" << gameid.GetHex() << std::endl);  // game started and not all containers, seems some already transferred
        IsSufficientContainers = false;
    }

    if (containers.size() != owners.size())
    {
        for (auto const &c : containers) std::cerr << __func__ << " container=" << c->creationtxid.GetHex() << std::endl;
        for (auto const &o : owners) std::cerr << __func__ << " owner=" << o.GetHex() << std::endl;
        for (auto const &p : playerids) std::cerr << __func__ << " playerid=" << p.GetHex() << std::endl;

        LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "containers number != container owners number, some containers are from the same owner" << std::endl);
        IsSufficientContainers = false;
    }

    // TODO: check that the pubkeys are from this game's players

    /* should be empty if now prev baton
    if (pGameOrParams->objectType == KOGSID_GAME)  // first turn
    {
        newbaton.kogsFlipped.clear();
        newbaton.kogsInStack.clear();
    }*/

    if (pGameOrParams->objectType == KOGSID_SLAMPARAMS)  // process slam data 
    {
        KogsSlamParams* pslamparams = (KogsSlamParams*)pGameOrParams;
        if (!FlipKogs(gameconfig, *pslamparams, newbaton, ptestbaton))   // before the call newbaton must contain the prev baton state 
            return false;
    }
    if (IsSufficientContainers)
        AddKogsToStack(gameconfig, newbaton, containers);

    return IsSufficientContainers;
}


// RPC implementations:

// wrapper to load container ids deposited on gameid 1of2 addr 
void KogsDepositedContainerList(uint256 gameid, std::vector<uint256> &containerids)
{
    std::vector<std::shared_ptr<KogsContainer>> containers;
    KogsDepositedContainerListImpl(gameid, containers);

    for (const auto &c : containers)
        containerids.push_back(c->creationtxid);
}

// returns all objects' creationtxid (tokenids or kog object creation txid) for the object with objectType
void KogsCreationTxidList(const CPubKey &remotepk, uint8_t objectType, bool onlymy, KogsObjectFilterBase *pFilter, std::vector<uint256> &creationtxids)
{
    std::vector<std::shared_ptr<KogsBaseObject>> objlist;
    //IsNFTMineChecker checker( IS_REMOTE(remotepk) ? remotepk : pubkey2pk(Mypubkey()) );

    // get all objects with this objectType
    ListGameObjects(remotepk, objectType, onlymy, pFilter, objlist);

    for (const auto &o : objlist)
    {
        creationtxids.push_back(o->creationtxid);
    }
} 

// returns game list, either in which playerid participates or all
void KogsGameTxidList(const CPubKey &remotepk, uint256 playerid, std::vector<uint256> &creationtxids)
{
    std::vector<std::shared_ptr<KogsBaseObject>> objlist;
    GameHasPlayerIdChecker checker(playerid);

    // get all objects with this objectType
    ListGameObjects(remotepk, KOGSID_GAME, false, !playerid.IsNull() ? &checker : nullptr, objlist);

    for (auto &o : objlist)
    {
        creationtxids.push_back(o->creationtxid);
    }
}


// iterate match object params and call NFT creation function
std::vector<UniValue> KogsCreateMatchObjectNFTs(const CPubKey &remotepk, std::vector<KogsMatchObject> & matchobjects)
{
    std::vector<UniValue> results;

    // TODO: do we need to check remote pk or suppose we are always in local mode with sys pk in the wallet?
    if (!CheckSysPubKey())
        return NullResults;
    
    LockUtxoInMemory lockutxos;  //activate in-mem utxo locking

    for (auto &obj : matchobjects) {

        int32_t borderColor = rand() % 26 + 1; // 1..26
        int32_t borderGraphic = rand() % 13;   // 0..12
        int32_t borderWidth = rand() % 15 + 1; // 1..15

        // generate the border appearance id:
        obj.appearanceId = borderColor * 13 * 16 + borderGraphic * 16 + borderWidth;
        obj.printId = AppearanceIdCount(obj.appearanceId);
             
        if (obj.objectType == KOGSID_SLAMMER)
            obj.borderId = rand() % 2 + 1; // 1..2

        UniValue sigData = CreateGameObjectNFT(remotepk, &obj);
        if (!ResultHasTx(sigData)) {
            results = NullResults;
            break;
        }
        else
            results.push_back(sigData);
    }

    return results;
}

// create pack of 'packsize' kog ids and encrypt its content
// pack content is tokenid list (encrypted)
// pack use case:
// when pack is purchased, the pack's NFT is sent to the purchaser
// then the purchaser burns the pack NFT and this means he unseals it.
// after this the system user sends the NFTs from the pack to the puchaser.
// NOTE: for packs we cannot use more robust algorithm of sending kogs on the pack's 1of2 address (like in containers) 
// because in such a case the pack content would be immediately available for everyone
UniValue KogsCreatePack(const CPubKey &remotepk, KogsPack newpack)
{
    // TODO: do we need to check remote pk or suppose we are always in local mode with sys pk in the wallet?
    if (!CheckSysPubKey())
        return NullUniValue;

    return CreateGameObjectNFT(remotepk, &newpack);
}

// create game config object
UniValue KogsCreateGameConfig(const CPubKey &remotepk, KogsGameConfig newgameconfig)
{
    return CreateEnclosureTx(remotepk, &newgameconfig, false, false);
}

// create player object with player's params
UniValue KogsCreatePlayer(const CPubKey &remotepk, KogsPlayer newplayer)
{
    return CreateEnclosureTx(remotepk, &newplayer, true, false);
}

UniValue KogsStartGame(const CPubKey &remotepk, KogsGame newgame)
{
    std::shared_ptr<KogsBaseObject> spGameConfig(LoadGameObject(newgame.gameconfigid));
    if (spGameConfig == nullptr || spGameConfig->objectType != KOGSID_GAMECONFIG)
    {
        CCerror = "can't load game config";
        return NullUniValue;
    }

    // check if all players advertised:
    for (auto const &playerid : newgame.playerids) 
    {
        uint256 adtxid;
        int32_t advout;
        std::vector<KogsAdvertising> dummy;

        if (!FindAdvertisings(playerid, adtxid, advout, dummy)) {
            CCerror = "playerid did not advertise: " + playerid.GetHex();
            return NullUniValue;
        }
    }

    /*KogsGameConfig *pGameConfig = (KogsGameConfig*)spGameConfig.get();
    std::vector<std::shared_ptr<KogsContainer>> spcontainers;
    if(!KogsManageStack(pGameConfig, nullptr, &newgame, spcontainers))
    {
        CCerror = "could not manage stack: " + CCerror; // try use the CCerror from ManageStack
        return NullUniValue;
    }*/

    return CreateEnclosureTx(remotepk, &newgame, false, true);
}

// container is an NFT token
// to add tokens to it we just send them to container 1of2 addr (kogs-global, container-create-txid)
// it's better for managing NFTs inside the container, easy to deposit: if container NFT is sent to some adddr that would mean all nfts inside it are also on this addr
// and the kogs validation code would allow to spend nfts from 1of2 addr to whom who owns the container nft
// simply and effective
// returns hextx list of container creation tx and token transfer tx
std::vector<UniValue> KogsCreateContainerV2(const CPubKey &remotepk, KogsContainer newcontainer, const std::set<uint256> &tokenids)
{
    std::vector<UniValue> results;

    std::shared_ptr<KogsBaseObject>spobj( LoadGameObject(newcontainer.playerid) );
    if (spobj == nullptr || spobj->objectType != KOGSID_PLAYER)
    {
        CCerror = "could not load this playerid";
        return NullResults;
    }

    bool isRemote = IS_REMOTE(remotepk);
    CPubKey mypk = isRemote ? remotepk : pubkey2pk(Mypubkey());

    if (((KogsPlayer*)spobj.get())->encOrigPk != mypk)
    {
        CCerror = "not your playerid";
        return NullResults;
    }

    //call this before txns creation
    LockUtxoInMemory lockutxos;  

    UniValue sigData = CreateGameObjectNFT(remotepk, &newcontainer);
    if (!ResultHasTx(sigData)) 
        return NullResults;
    

    results.push_back(sigData);

    /* this code does not work in NSPV mode (containerid is unknown at this time)
    // unmarshal tx to get it txid;
    vuint8_t vtx = ParseHex(ResultGetTx(sigData));
    CTransaction containertx;
    if (E_UNMARSHAL(vtx, ss >> containertx)) 
    {
        struct CCcontract_info *cp, C;
        cp = CCinit(&C, EVAL_KOGS);
        CPubKey kogsPk = GetUnspendable(cp, NULL);

        uint256 containertxid = containertx.GetHash();
        char txidaddr[KOMODO_ADDRESS_BUFSIZE];
        CPubKey createtxidPk = CCtxidaddr_tweak(txidaddr, containertxid);

        char tokenaddr[KOMODO_ADDRESS_BUFSIZE];
        GetTokensCCaddress(cp, tokenaddr, mypk);

        for (auto t : tokenids)
        {
            UniValue sigData = TokenTransferExt(remotepk, 0, t, tokenaddr, std::vector<std::pair<CC*, uint8_t*>>(), std::vector<CPubKey> {kogsPk, createtxidPk}, 1);
            if (!ResultHasTx(sigData)) {
                results = NullResults;
                break;
            }
            results.push_back(sigData);
        }
    }
    else
    {
        CCerror = "can't unmarshal container tx";
        return NullResults;
    } */

    return results;
}

// transfer container to destination pubkey
/*
static UniValue SpendEnclosure(const CPubKey &remotepk, int64_t txfee, KogsEnclosure enc, CPubKey destpk)
{
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());

    struct CCcontract_info *cp, C;
    cp = CCinit(&C, EVAL_KOGS);

    bool isRemote = IS_REMOTE(remotepk);
    CPubKey mypk = isRemote ? remotepk : pubkey2pk(Mypubkey());

    if (AddNormalinputs(mtx, mypk, txfee, 0x10000, isRemote) > 0)
    {
        if (enc.latesttxid.IsNull()) {
            CCerror = strprintf("incorrect latesttx in container");
            return NullUniValue;
        }

        mtx.vin.push_back(CTxIn(enc.latesttxid, 0));
        mtx.vout.push_back(MakeCC1vout(EVAL_KOGS, 1, destpk));  // container has value = 1

        CScript opret;
        opret << OP_RETURN << enc.EncodeOpret();
        UniValue sigData = FinalizeCCTxExt(true, 0, cp, mtx, mypk, txfee, opret);
        if (!ResultHasTx(sigData))
        {
            CCerror = strprintf("could not finalize transfer container tx");
            return nullUnivalue;
        }

        return sigData;

    }
    else
    {
        CCerror = "insufficient normal inputs for tx fee";
    }
    return NullUniValue;
}
*/

// deposit (send) container to 1of2 game txid pubkey
UniValue KogsDepositContainerV2(const CPubKey &remotepk, int64_t txfee, uint256 gameid, uint256 containerid)
{
    bool isRemote = IS_REMOTE(remotepk);
    CPubKey mypk = isRemote ? remotepk : pubkey2pk(Mypubkey());

    std::shared_ptr<KogsBaseObject>spgamebaseobj(LoadGameObject(gameid));
    if (spgamebaseobj == nullptr || spgamebaseobj->objectType != KOGSID_GAME) {
        CCerror = "can't load game data";
        return NullUniValue;
    }
    KogsGame *pgame = (KogsGame *)spgamebaseobj.get();

    std::shared_ptr<KogsBaseObject>spgameconfigbaseobj(LoadGameObject(pgame->gameconfigid));
    if (spgameconfigbaseobj == nullptr || spgameconfigbaseobj->objectType != KOGSID_GAMECONFIG) {
        CCerror = "can't load game config data";
        return NullUniValue;
    }
    KogsGameConfig *pgameconfig = (KogsGameConfig *)spgameconfigbaseobj.get();

    std::shared_ptr<KogsBaseObject>spcontbaseobj(LoadGameObject(containerid));
    if (spcontbaseobj == nullptr || spcontbaseobj->objectType != KOGSID_CONTAINER) {
        CCerror = "can't load container";
        return NullUniValue;
    }
    KogsContainer *pcontainer = (KogsContainer *)spcontbaseobj.get();
    ListContainerTokenids(*pcontainer);

    // TODO: check if this player has already deposited a container. Seems the doc states only one container is possible
    if (pcontainer->tokenids.size() != pgameconfig->numKogsInContainer)     {
        CCerror = "kogs number in container does not match game config";
        return NullUniValue;
    }

    struct CCcontract_info *cp, C;
    cp = CCinit(&C, EVAL_KOGS);
    CPubKey kogsPk = GetUnspendable(cp, NULL);
    //CPubKey mypk = pubkey2pk(Mypubkey());

    char txidaddr[KOMODO_ADDRESS_BUFSIZE];
    CPubKey gametxidPk = CCtxidaddr_tweak(txidaddr, gameid);

    char tokenaddr[KOMODO_ADDRESS_BUFSIZE];
    GetTokensCCaddress(cp, tokenaddr, mypk);

    // passing remotepk for TokenTransferExt to correctly call FinalizeCCTx
    UniValue sigData = TokenTransferExt(remotepk, 0, containerid, tokenaddr, std::vector<std::pair<CC*, uint8_t*>>{ }, std::vector<CPubKey>{ kogsPk, gametxidPk }, 1, true); // amount = 1 always for NFTs
    return sigData;
}

// claim sending container from 1of2 game txid to origin pubkey
UniValue KogsClaimDepositedContainer(const CPubKey &remotepk, int64_t txfee, uint256 gameid, uint256 containerid)
{
    std::shared_ptr<KogsBaseObject>spgamebaseobj(LoadGameObject(gameid));
    if (spgamebaseobj == nullptr || spgamebaseobj->objectType != KOGSID_GAME) {
        CCerror = "can't load game data";
        return NullUniValue;
    }

    std::shared_ptr<KogsBaseObject>spcontbaseobj(LoadGameObject(containerid));
    if (spcontbaseobj == nullptr || spcontbaseobj->objectType != KOGSID_CONTAINER) {
        CCerror = "can't load container";
        return NullUniValue;
    }

    KogsContainer *pcontainer = (KogsContainer *)spcontbaseobj.get();
    CTransaction lasttx;
    bool isRemote = IS_REMOTE(remotepk);
    CPubKey mypk = isRemote ? remotepk : pubkey2pk(Mypubkey());

    if (pcontainer->encOrigPk == mypk)  // We do not allow to transfer container to other users, so only primary origPk is considered as user pk
    {
        if (GetNFTUnspentTx(pcontainer->creationtxid, lasttx))
        {
            // send container back to the sender:
            struct CCcontract_info *cp, C;
            cp = CCinit(&C, EVAL_KOGS);
            uint8_t kogsPriv[32];
            CPubKey kogsPk = GetUnspendable(cp, kogsPriv);

            char txidaddr[KOMODO_ADDRESS_BUFSIZE];
            CPubKey gametxidPk = CCtxidaddr_tweak(txidaddr, gameid);

            char tokensrcaddr[KOMODO_ADDRESS_BUFSIZE];
            GetTokensCCaddress1of2(cp, tokensrcaddr, kogsPk, gametxidPk);

            CC* probeCond = MakeTokensCCcond1of2(EVAL_KOGS, kogsPk, gametxidPk);  // make probe cc for signing 1of2 game txid addr

            UniValue sigData = TokenTransferExt(remotepk, 0, containerid, tokensrcaddr, std::vector<std::pair<CC*, uint8_t*>>{ std::make_pair(probeCond, kogsPriv) }, std::vector<CPubKey>{ mypk }, 1, true); // amount = 1 always for NFTs

            cc_free(probeCond); // free probe cc
            return sigData;
        }
        else
            CCerror = "cant get last tx for container";

    }
    else
        CCerror = "not my container";

    return NullUniValue;
}

// check if container NFT is on 1of2 address (kogsPk, gametxidpk)
static bool IsContainerDeposited(KogsGame game, KogsContainer container)
{
    CTransaction lasttx;
    if (!GetNFTUnspentTx(container.creationtxid, lasttx))  // TODO: optimize getting lasttx vout in LoadGameObject()
        return false;
    if (lasttx.vout.size() < 1)
        return false;

    struct CCcontract_info *cp, C;
    cp = CCinit(&C, EVAL_KOGS);
    CPubKey kogsPk = GetUnspendable(cp, NULL);

    char txidaddr[KOMODO_ADDRESS_BUFSIZE];
    CPubKey gametxidPk = CCtxidaddr_tweak(txidaddr, game.creationtxid);
    if (lasttx.vout[0] == MakeCC1of2vout(EVAL_TOKENS, 1, kogsPk, gametxidPk))  // TODO: bad, use IsTokenVout
        return true;
    return false;
}

// checks if container deposited to gamepk and gamepk is mypk or if it is not deposited and on mypk
static int CheckIsMyContainer(const CPubKey &remotepk, uint256 gameid, uint256 containerid)
{
    bool isRemote = IS_REMOTE(remotepk);
    CPubKey mypk = isRemote ? remotepk : pubkey2pk(Mypubkey());

    std::shared_ptr<KogsBaseObject>spcontbaseobj( LoadGameObject(containerid) );
    if (spcontbaseobj == nullptr || spcontbaseobj->objectType != KOGSID_CONTAINER) {
        CCerror = "can't load container";
        return -1;
    }
    KogsContainer *pcontainer = (KogsContainer *)spcontbaseobj.get();

    if (!gameid.IsNull())
    {
        std::shared_ptr<KogsBaseObject>spgamebaseobj( LoadGameObject(gameid) );
        if (spgamebaseobj == nullptr || spgamebaseobj->objectType != KOGSID_GAME) {
            CCerror = "can't load container";
            return -1;
        }
        KogsGame *pgame = (KogsGame *)spgamebaseobj.get();
        if (IsContainerDeposited(*pgame, *pcontainer)) 
        {
            if (mypk != pgame->encOrigPk) {  // TODO: why is only game owner allowed to modify container?
                CCerror = "can't add or remove kogs: container is deposited and you are not the game creator";
                return 0;
            }
            else
                return 1;
        }
    }
    
    CTransaction lasttx;
    if (!GetNFTUnspentTx(containerid, lasttx)) {
        CCerror = "container is already burned or not yours";
        return -1;
    }
    if (lasttx.vout.size() < 1) {
        CCerror = "incorrect nft last tx";
        return -1;
    }
    for(const auto &v : lasttx.vout)    {
        if (IsEqualVouts(v, MakeTokensCC1vout(EVAL_KOGS, 1, mypk))) 
            return 1;
    }
    
    CCerror = "this is not your container to add or remove kogs";
    return 0;
}

// add kogs to the container by sending kogs to container 1of2 address
std::vector<UniValue> KogsAddKogsToContainerV2(const CPubKey &remotepk, int64_t txfee, uint256 containerid, const std::set<uint256> &tokenids)
{
    std::vector<UniValue> result;

    if (CheckIsMyContainer(remotepk, zeroid, containerid) <= 0)
        return NullResults;

    struct CCcontract_info *cp, C;
    cp = CCinit(&C, EVAL_KOGS);
    CPubKey kogsPk = GetUnspendable(cp, NULL);
    bool isRemote = IS_REMOTE(remotepk);
    CPubKey mypk = isRemote ? remotepk : pubkey2pk(Mypubkey());

    char txidaddr[KOMODO_ADDRESS_BUFSIZE];
    CPubKey containertxidPk = CCtxidaddr_tweak(txidaddr, containerid);

    LockUtxoInMemory lockutxos; // activate locking

	// create copret with containerid
	KogsContainerOps containerOps(KOGSID_ADDTOCONTAINER);
	containerOps.Init(containerid);
 	KogsEnclosure enc(mypk);  //'zeroid' means 'for creation'

    enc.vdata = containerOps.Marshal();
    enc.name = containerOps.nameId;
    enc.description = containerOps.descriptionId;
	CScript opret;
    opret << OP_RETURN << enc.EncodeOpret();

    char tokenaddr[KOMODO_ADDRESS_BUFSIZE];
    GetTokensCCaddress(cp, tokenaddr, mypk);

    CMutableTransaction mtx;
    struct CCcontract_info *cpTokens, CTokens;
    cpTokens = CCinit(&CTokens, EVAL_TOKENS);
    UniValue beginResult = TokenBeginTransferTx(mtx, cpTokens, remotepk, 10000);
    if (ResultIsError(beginResult)) {
        CCerror = ResultGetError(beginResult);
        return NullResults;
    }

    for (const auto &tokenid : tokenids)
    {
        //UniValue sigData = TokenTransferExt(remotepk, 0, tokenid, tokenaddr, std::vector<std::pair<CC*, uint8_t*>>{ }, std::vector<CPubKey>{ kogsPk, containertxidPk }, 1, true); // amount = 1 always for NFTs
        UniValue addtxResult = TokenAddTransferVout(mtx, cpTokens, remotepk, tokenid, tokenaddr, std::vector<CPubKey>{ kogsPk, containertxidPk }, {nullptr, nullptr}, 1, true);
        if (ResultIsError(addtxResult)) {
            CCerror = ResultGetError(addtxResult);
            return NullResults;
        }
    }
    UniValue sigData = TokenFinalizeTransferTx(mtx, cpTokens, remotepk, 10000, opret);
    if (ResultHasTx(sigData))   {
        result.push_back(sigData);
    }
    else
    {
        CCerror = ResultGetError(sigData);
    }
    
    return result;
}

// remove kogs from the container by sending kogs from the container 1of2 address to self
std::vector<UniValue> KogsRemoveKogsFromContainerV2(const CPubKey &remotepk, int64_t txfee, uint256 gameid, uint256 containerid, const std::set<uint256> &tokenids)
{
    std::vector<UniValue> results;
    KogsBaseObject *baseobj;

    bool isRemote = IS_REMOTE(remotepk);
    CPubKey mypk = isRemote ? remotepk : pubkey2pk(Mypubkey());

    if (CheckIsMyContainer(remotepk, gameid, containerid) <= 0)
        return NullResults;

    struct CCcontract_info *cp, C;
    cp = CCinit(&C, EVAL_KOGS);
    uint8_t kogspriv[32];
    CPubKey kogsPk = GetUnspendable(cp, kogspriv);
    char txidaddr[KOMODO_ADDRESS_BUFSIZE];
    CPubKey createtxidPk = CCtxidaddr_tweak(txidaddr, containerid);
    CC *probeCond = MakeTokensCCcond1of2(EVAL_KOGS, kogsPk, createtxidPk);
    char tokenaddr[KOMODO_ADDRESS_BUFSIZE];
    GetTokensCCaddress1of2(cp, tokenaddr, kogsPk, createtxidPk);

    LockUtxoInMemory lockutxos; // activate locking

	// create copret with containerid
	KogsContainerOps containerOps(KOGSID_REMOVEFROMCONTAINER);
	containerOps.Init(containerid);
 	KogsEnclosure enc(mypk);  //'zeroid' means 'for creation'

    enc.vdata = containerOps.Marshal();
    enc.name = containerOps.nameId;
    enc.description = containerOps.descriptionId;
	CScript opret;
    opret << OP_RETURN << enc.EncodeOpret();

    /*for (auto tokenid : tokenids)
    {
        UniValue sigData = TokenTransferExt(remotepk, 0, tokenid, tokenaddr, std::vector<std::pair<CC*, uint8_t*>>{ std::make_pair(probeCond, kogspriv) }, std::vector<CPubKey>{ mypk }, 1, true); // amount = 1 always for NFTs
        if (!ResultHasTx(sigData)) {
            results = NullResults;
            break;
        }
        results.push_back(sigData);
    }*/

    CMutableTransaction mtx;
    struct CCcontract_info *cpTokens, CTokens;
    cpTokens = CCinit(&CTokens, EVAL_TOKENS);
    UniValue beginResult = TokenBeginTransferTx(mtx, cpTokens, remotepk, 10000);
    if (ResultIsError(beginResult)) {
        CCerror = ResultGetError(beginResult);
        return NullResults;
    }

    for (const auto &tokenid : tokenids)
    {
        UniValue addtxResult = TokenAddTransferVout(mtx, cpTokens, remotepk, tokenid, tokenaddr, std::vector<CPubKey>{ mypk }, { std::make_pair(probeCond, kogspriv) }, 1, true);
        if (ResultIsError(addtxResult)) {
            CCerror = ResultGetError(addtxResult);
            return NullResults;
        }
    }
    UniValue sigData = TokenFinalizeTransferTx(mtx, cpTokens, remotepk, 10000, opret);
    if (ResultHasTx(sigData))   {
        results.push_back(sigData);
    }
    else
    {
        CCerror = ResultGetError(sigData);
    }

    cc_free(probeCond);
    return results;
}

UniValue KogsAddSlamParams(const CPubKey &remotepk, KogsSlamParams &newSlamParams)
{
    std::shared_ptr<KogsBaseObject> spbaseobj( LoadGameObject(newSlamParams.gameid) );
    if (spbaseobj == nullptr || spbaseobj->objectType != KOGSID_GAME)
    {
        CCerror = "can't load game";
        return NullUniValue;
    }

    // find the baton on mypk:
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > addressUnspents;
   
    struct CCcontract_info *cp, C;
    cp = CCinit(&C, EVAL_KOGS);

    //char my1of2ccaddr[KOMODO_ADDRESS_BUFSIZE];
    char myccaddr[KOMODO_ADDRESS_BUFSIZE];
    bool isRemote = IS_REMOTE(remotepk);
    CPubKey mypk = isRemote ? remotepk : pubkey2pk(Mypubkey());
    //GetCCaddress1of2(cp, my1of2ccaddr, GetUnspendable(cp, NULL), mypk); // use 1of2 now
    GetCCaddress(cp, myccaddr, mypk);

    LOGSTREAMFN("kogs", CCLOG_DEBUG1, stream << "finding 'my turn' baton on mypk" << std::endl);

    // find all games with unspent batons:
    uint256 batontxid = zeroid;
    KogsBaton* prevBaton = nullptr;
    SetCCunspentsWithMempool(addressUnspents, myccaddr, true);    // look for baton on my cc addr 
    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it = addressUnspents.begin(); it != addressUnspents.end(); it++)
    {
        uint256 dummytxid;
        int32_t dummyvout;
        // its very important to check if the baton not spent in mempool, otherwise we could pick up a previous already spent baton
        if (it->second.satoshis == KOGS_BATON_AMOUNT /*&& !myIsutxo_spentinmempool(dummytxid, dummyvout, it->first.txhash, it->first.index)*/) // picking batons with marker=20000
        {
            std::shared_ptr<KogsBaseObject> spbaton(LoadGameObject(it->first.txhash));
            if (spbaton != nullptr && spbaton->objectType == KOGSID_BATON)
            {
                KogsBaton* pbaton = (KogsBaton*)spbaton.get();
                if (pbaton->gameid == newSlamParams.gameid)
                {
                    if (pbaton->nextplayerid == newSlamParams.playerid)
                    {
                        std::shared_ptr<KogsBaseObject> spplayer(LoadGameObject(newSlamParams.playerid));
                        if (spplayer.get() != nullptr && spplayer->objectType == KOGSID_PLAYER)
                        {
                            KogsPlayer* pplayer = (KogsPlayer*)spplayer.get();
                            if (pplayer->encOrigPk == mypk)
                            {
                                batontxid = it->first.txhash;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    /*if (prevBaton != nullptr)  
    {
        std::shared_ptr<KogsBaseObject> spGameConfig(LoadGameObject(prevBaton->gameconfigid));
        if (spGameConfig->objectType != KOGSID_GAMECONFIG)
        {
            CCerror = "bad gameconfigid";
            return NullUniValue;
        }

        KogsGameConfig *pGameConfig = (KogsGameConfig*)spGameConfig.get();
        std::vector<std::shared_ptr<KogsContainer>> spcontainers;
        if(!KogsManageStack(pGameConfig, prevBaton, &newSlamParams, spcontainers))
        {
            CCerror = "could not manage stack: " + CCerror; // try use the CCerror from ManageStack
            return NullUniValue;
        }
        return CreateSlamParamTx(remotepk, batontxid, 0, newSlamParams);
    }
    else
    {
        CCerror = "could not find baton for your pubkey (not your turn)";
        return NullUniValue;
    }*/
    if (!batontxid.IsNull())
        return CreateSlamParamTx(remotepk, batontxid, 0, newSlamParams);
    else
    {
        CCerror = "could not find baton for your pubkey (not your turn)";
        return NullUniValue;
    }

}

UniValue KogsAdvertisePlayer(const CPubKey &remotepk, const KogsAdvertising &newad)
{
    std::shared_ptr<KogsBaseObject> spplayer(LoadGameObject(newad.playerId));
    if (spplayer == nullptr || spplayer->objectType != KOGSID_PLAYER)
    {
        CCerror = "can't load player object";
        return NullUniValue;
    }

    bool isRemote = IS_REMOTE(remotepk);
    CPubKey mypk = isRemote ? remotepk : pubkey2pk(Mypubkey());
    
    KogsPlayer *pplayer = (KogsPlayer*)spplayer.get();
    if (pplayer->encOrigPk != mypk) {
        CCerror = "not this pubkey player";
        return NullUniValue;
    }

    uint256 adtxid;
    int32_t advout;
    std::vector<KogsAdvertising> dummy;

    if (FindAdvertisings(newad.playerId, adtxid, advout, dummy)) {
        CCerror = "this player already made advertising";
        return NullUniValue;
    }
    return CreateAdvertisingTx(remotepk, newad);
}

void KogsAdvertisedList(std::vector<KogsAdvertising> &adlist)
{
    uint256 adtxid;
    int32_t nvout;
    
    FindAdvertisings(zeroid, adtxid, nvout, adlist);
}


static bool IsGameObjectDeleted(uint256 tokenid)
{
    uint256 spenttxid;
    int32_t vini, height;

    return CCgetspenttxid(spenttxid, vini, height, tokenid, KOGS_NFT_MARKER_VOUT) == 0;
}

// create txns to unseal pack and send NFTs to pack owner address
// this is the really actual case when we need to create many transaction in one rpc:
// when a pack has been unpacked then all the NFTs in it should be sent to the purchaser in several token transfer txns
std::vector<UniValue> KogsUnsealPackToOwner(const CPubKey &remotepk, uint256 packid, vuint8_t encryptkey, vuint8_t iv)
{
    CTransaction burntx, prevtx;

    if (IsNFTBurned(packid, burntx) && !IsGameObjectDeleted(packid))
    {
        CTransaction prevtx;
        int32_t nvout;
        std::vector<CPubKey> pks;

        if (GetNFTPrevVout(burntx, prevtx, nvout, pks))     // find who burned the pack and send to him the tokens from the pack
        {
            // load pack:
            std::shared_ptr<KogsBaseObject> sppackbaseobj( LoadGameObject(packid) );
            if (sppackbaseobj == nullptr || sppackbaseobj->objectType != KOGSID_PACK)
            {
                CCerror = "can't load pack NFT or not a pack";
                return NullResults;
            }

            KogsPack *pack = (KogsPack *)sppackbaseobj.get();  
            /*if (!pack->DecryptContent(encryptkey, iv))
            {
                CCerror = "can't decrypt pack content";
                return NullResults;
            }*/

            std::vector<UniValue> results;

            LockUtxoInMemory lockutxos;

            // create txns sending the pack's kog NFTs to pack's vout address:
            /*for (auto tokenid : pack->tokenids)
            {
                char tokensrcaddr[KOMODO_ADDRESS_BUFSIZE];
                bool isRemote = IS_REMOTE(remotepk);
                CPubKey mypk = isRemote ? remotepk : pubkey2pk(Mypubkey());
                struct CCcontract_info *cp, C;

                cp = CCinit(&C, EVAL_TOKENS);
                GetTokensCCaddress(cp, tokensrcaddr, mypk);

                UniValue sigData = TokenTransferExt(remotepk, 0, tokenid, tokensrcaddr, std::vector<std::pair<CC*, uint8_t*>>{}, pks, 1, true);
                if (!ResultHasTx(sigData)) {
                    results.push_back(MakeResultError("can't create transfer tx (nft could be already sent!): " + CCerror));
                    CCerror.clear(); // clear read CCerror
                }
                else
                    results.push_back(sigData);
            }*/

            if (results.size() > 0)
            {
                // create tx removing pack by spending the kogs marker
                UniValue sigData = KogsRemoveObject(remotepk, packid, KOGS_NFT_MARKER_VOUT);
                if (!ResultHasTx(sigData)) {
                    results.push_back(MakeResultError("can't create pack removal tx: " + CCerror));
                    CCerror.clear(); // clear used CCerror
                }
                else
                    results.push_back(sigData);
            }

            return results;
        }
    }
    else
    {
        CCerror = "can't unseal, pack NFT not burned yet or already removed";
    }
    return NullResults;
}

// temp burn error object by spending its eval_kog marker in vout=2
UniValue KogsBurnNFT(const CPubKey &remotepk, uint256 tokenid)
{
    // create burn tx
    const CAmount  txfee = 10000;

    bool isRemote = IS_REMOTE(remotepk);
    CPubKey mypk = isRemote ? remotepk : pubkey2pk(Mypubkey());

    if (!mypk.IsFullyValid())
    {
        CCerror = "mypk is not set";
        return  NullUniValue;
    }

    CPubKey burnpk = pubkey2pk(ParseHex(CC_BURNPUBKEY));
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
    struct CCcontract_info *cp, C;

    cp = CCinit(&C, EVAL_TOKENS);

    if (AddNormalinputs(mtx, mypk, txfee, 0x10000, isRemote) > 0)
    {
        if (AddTokenCCInputs(cp, mtx, mypk, tokenid, 1, 1, true) > 0)
        {
            std::vector<CPubKey> voutPks;
            char unspendableTokenAddr[KOMODO_ADDRESS_BUFSIZE]; uint8_t tokenpriv[32];
            struct CCcontract_info *cpTokens, tokensC;
            cpTokens = CCinit(&tokensC, EVAL_TOKENS);

            mtx.vin.push_back(CTxIn(tokenid, 0)); // spend token cc address marker
            CPubKey tokenGlobalPk = GetUnspendable(cpTokens, tokenpriv);
            GetCCaddress(cpTokens, unspendableTokenAddr, tokenGlobalPk);
            CCaddr2set(cp, EVAL_TOKENS, tokenGlobalPk, tokenpriv, unspendableTokenAddr);  // add token privkey to spend token cc address marker

            mtx.vout.push_back(MakeTokensCC1vout(EVAL_KOGS, 1, burnpk));    // burn tokens
            voutPks.push_back(burnpk);
            UniValue sigData = FinalizeCCTxExt(isRemote, 0, cp, mtx, mypk, txfee, EncodeTokenOpRetV1(tokenid, voutPks, {}));
            if (ResultHasTx(sigData))
                return sigData;
            else
                CCerror = "can't finalize or sign burn tx";
        }
        else
            CCerror = "can't find token inputs";
    }
    else
        CCerror = "can't find normals for txfee";
    return NullUniValue;
}

// special feature to hide object by spending its cc eval kog marker (for nfts it is in vout=2)
UniValue KogsRemoveObject(const CPubKey &remotepk, uint256 txid, int32_t nvout)
{
    // create burn tx
    const CAmount  txfee = 10000;
    bool isRemote = IS_REMOTE(remotepk);
    CPubKey mypk = isRemote ? remotepk : pubkey2pk(Mypubkey());
    if (!mypk.IsFullyValid())
    {
        CCerror = "mypk is not set";
        return  NullUniValue;
    }

    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
    struct CCcontract_info *cp, C;

    cp = CCinit(&C, EVAL_KOGS);

    if (AddNormalinputs(mtx, mypk, txfee, 0x10000, isRemote) > 0)
    {
        mtx.vin.push_back(CTxIn(txid, nvout));
        mtx.vout.push_back(CTxOut(txfee, CScript() << ParseHex(HexStr(mypk)) << OP_CHECKSIG));
        UniValue sigData = FinalizeCCTxExt(isRemote, 0, cp, mtx, mypk, txfee, CScript());
        if (ResultHasTx(sigData))
            return sigData;
        else
            CCerror = "can't finalize or sign removal tx";
    }
    else
        CCerror = "can't find normals for txfee";
    return NullUniValue;
}

// stop advertising player by spending the marker from kogs global address
UniValue KogsStopAdvertisePlayer(const CPubKey &remotepk, uint256 playerId)
{
    std::shared_ptr<KogsBaseObject> spplayer(LoadGameObject(playerId));
    if (spplayer == nullptr || spplayer->objectType != KOGSID_PLAYER)
    {
        CCerror = "can't load player object";
        return NullUniValue;
    }

    const CAmount  txfee = 10000;
    bool isRemote = IS_REMOTE(remotepk);
    CPubKey mypk = isRemote ? remotepk : pubkey2pk(Mypubkey());
    if (!mypk.IsFullyValid())
    {
        CCerror = "mypk is not set";
        return  NullUniValue;
    }

    KogsPlayer *pplayer = (KogsPlayer*)spplayer.get();
    if (pplayer->encOrigPk != mypk) {
        CCerror = "not this pubkey player";
        return NullUniValue;
    }


    uint256 adtxid;
    int32_t advout;
    std::vector<KogsAdvertising> adlist;

    if (!FindAdvertisings(playerId, adtxid, advout, adlist)) {
        CCerror = "can't find advertising tx";
        return NullUniValue;
    }

    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
    struct CCcontract_info *cp, C;

    cp = CCinit(&C, EVAL_KOGS);

    if (AddNormalinputs(mtx, mypk, txfee, 0x10000, isRemote) > 0)
    {
        mtx.vin.push_back(CTxIn(adtxid, advout));   // spend advertising marker:
        mtx.vout.push_back(CTxOut(KOGS_ADVERISING_AMOUNT, CScript() << ParseHex(HexStr(mypk)) << OP_CHECKSIG));
        UniValue sigData = FinalizeCCTxExt(isRemote, 0, cp, mtx, mypk, txfee, CScript());
        if (ResultHasTx(sigData))
            return sigData;
        else
            CCerror = "can't finalize or sign removal tx";
    }
    else
        CCerror = "can't find normals for txfee";
    return NullUniValue;
}

// retrieve game info: open/finished, won kogs
UniValue KogsGameStatus(const KogsGame &gameobj)
{
    UniValue info(UniValue::VOBJ);
    // go for the opret data from the last/unspent tx 't'
    uint256 txid = gameobj.creationtxid;
    int32_t nvout = 2;  // baton vout, ==2 for game object
    int32_t prevTurn = -1;  //-1 indicates we are before any turns
    int32_t nextTurn = -1;  
    uint256 prevPlayerid = zeroid;
    uint256 nextPlayerid = zeroid;
    std::vector<uint256> kogsInStack;
    std::vector<std::pair<uint256, uint256>> prevFlipped;
    std::vector<uint256> batons;
    uint256 batontxid;
    uint256 hashBlock;
    int32_t vini, height;
    bool isFinished = false;
    

    // browse the sequence of slamparam and baton txns: 
    while (CCgetspenttxid(batontxid, vini, height, txid, nvout) == 0)
    {
        std::shared_ptr<KogsBaseObject> spobj(LoadGameObject(batontxid));
        if (spobj == nullptr || (spobj->objectType != KOGSID_BATON && spobj->objectType != KOGSID_SLAMPARAMS && spobj->objectType != KOGSID_GAMEFINISHED))
        {
            LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "could not load baton txid=" << batontxid.GetHex() << std::endl);
            info.push_back(std::make_pair("error", "can't load baton"));
            return info;
        }

        LOGSTREAMFN("kogs", CCLOG_DEBUG1, stream << "found baton or SlamParam or GameFinished objectType=" << (char)spobj->objectType << " txid=" << batontxid.GetHex() << std::endl);
        batons.push_back(batontxid);

        if (spobj->objectType == KOGSID_BATON)
        {
            KogsBaton *pbaton = (KogsBaton *)spobj.get();
            prevTurn = nextTurn;
            nextTurn = pbaton->nextturn;

            LOGSTREAMFN("kogs", CCLOG_DEBUG1, stream << "pbaton->kogsInStack=" << pbaton->kogsInStack.size() << " pbaton->kogFlipped=" << pbaton->kogsFlipped.size() << std::endl);

            // for the first turn prevturn is (-1)
            // and no won kogs yet:
            if (prevTurn >= 0)  // there was a turn already
            {
                //if (wonkogs.find(pbaton->playerids[prevTurn]) == wonkogs.end())
                //    wonkogs[pbaton->playerids[prevTurn]] = 0;  // init map value
                //wonkogs[pbaton->playerids[prevTurn]] += pbaton->kogsFlipped.size
                prevPlayerid = pbaton->playerids[prevTurn];
            }
            prevFlipped = pbaton->kogsFlipped;
            kogsInStack = pbaton->kogsInStack;
            nextPlayerid = pbaton->playerids[nextTurn];
            nvout = 0;  // baton tx's next baton vout
        }
        else if (spobj->objectType == KOGSID_SLAMPARAMS)
        {
            nvout = 0;  // slamparams tx's next baton vout
        }
        else // KOGSID_GAMEFINISHED
        { 
            KogsGameFinished *pGameFinished = (KogsGameFinished *)spobj.get();
            isFinished = true;
            prevFlipped = pGameFinished->kogsFlipped;
            kogsInStack = pGameFinished->kogsInStack;
            nextPlayerid = zeroid;
            nextTurn = -1;

            LOGSTREAMFN("kogs", CCLOG_DEBUG1, stream << "pGameFinished->kogsInStack=" << kogsInStack.size() << " pGameFinished->kogsFlipped=" << prevFlipped.size() << std::endl);
            break;
        }

        txid = batontxid;        
    }

    UniValue arrWon(UniValue::VARR);
    UniValue arrWonTotals(UniValue::VARR);
    std::map<uint256, int> wonkogs;

    // get array of pairs (playerid, won kogid)
    for (const auto &f : prevFlipped)
    {
        UniValue elem(UniValue::VOBJ);
        elem.push_back(std::make_pair(f.first.GetHex(), f.second.GetHex()));
        arrWon.push_back(elem);
    }
    // calc total of won kogs by playerid
    for (const auto &f : prevFlipped)
    {
        if (wonkogs.find(f.first) == wonkogs.end())
            wonkogs[f.first] = 0;  // init map value
        wonkogs[f.first] ++;
    }
    // convert to UniValue
    for (const auto &w : wonkogs)
    {
        UniValue elem(UniValue::VOBJ);
        elem.push_back(std::make_pair(w.first.GetHex(), std::to_string(w.second)));
        arrWonTotals.push_back(elem);
    }

    info.push_back(std::make_pair("gameconfigid", gameobj.gameconfigid.GetHex()));
    info.push_back(std::make_pair("finished", (isFinished ? std::string("true") : std::string("false"))));
    info.push_back(std::make_pair("KogsWonByPlayerId", arrWon));
    info.push_back(std::make_pair("KogsWonByPlayerIdTotals", arrWonTotals));
    info.push_back(std::make_pair("PreviousTurn", (prevTurn < 0 ? std::string("none") : std::to_string(prevTurn))));
    info.push_back(std::make_pair("PreviousPlayerId", (prevTurn < 0 ? std::string("none") : prevPlayerid.GetHex())));
    info.push_back(std::make_pair("NextTurn", (nextTurn < 0 ? std::string("none") : std::to_string(nextTurn))));
    info.push_back(std::make_pair("NextPlayerId", (nextTurn < 0 ? std::string("none") : nextPlayerid.GetHex())));

    UniValue arrStack(UniValue::VARR);
    for (const auto &s : kogsInStack)
        arrStack.push_back(s.GetHex());
    info.push_back(std::make_pair("KogsInStack", arrStack));

    UniValue arrBatons(UniValue::VARR);
    for (const auto &b : batons)
        arrBatons.push_back(b.GetHex());
    info.push_back(std::make_pair("batons", arrBatons));

    //UniValue arrFlipped(UniValue::VARR);
    //for (auto f : prevFlipped)
    //    arrFlipped.push_back(f.GetHex());
    //info.push_back(std::make_pair("PreviousFlipped", arrFlipped));

    return info;
}

static UniValue DecodeObjectInfo(KogsBaseObject *pobj)
{
    UniValue info(UniValue::VOBJ), err(UniValue::VOBJ), infotokenids(UniValue::VARR);
    UniValue gameinfo(UniValue::VOBJ);
    UniValue heightranges(UniValue::VARR);
    UniValue strengthranges(UniValue::VARR);
    UniValue flipped(UniValue::VARR);

    if (pobj == nullptr) {
        err.push_back(std::make_pair("result", "error"));
        err.push_back(std::make_pair("error", "can't load object"));
        return err;
    }

    if (pobj->evalcode != EVAL_KOGS) {
        err.push_back(std::make_pair("result", "error"));
        err.push_back(std::make_pair("error", "not a kogs object"));
        return err;
    }

    info.push_back(std::make_pair("result", "success"));
    info.push_back(std::make_pair("evalcode", HexStr(std::string(1, pobj->evalcode))));
    info.push_back(std::make_pair("objectType", std::string(1, (char)pobj->objectType)));
    info.push_back(std::make_pair("objectDesc", objectids[pobj->objectType]));
    info.push_back(std::make_pair("version", std::to_string(pobj->version)));
    info.push_back(std::make_pair("nameId", pobj->nameId));
    info.push_back(std::make_pair("descriptionId", pobj->descriptionId));
    info.push_back(std::make_pair("originatorPubKey", HexStr(pobj->encOrigPk)));
    info.push_back(std::make_pair("creationtxid", pobj->creationtxid.GetHex()));

    switch (pobj->objectType)
    {
        KogsMatchObject *matchobj;
        KogsPack *packobj;
        KogsContainer *containerobj;
        KogsGame *gameobj;
        KogsGameConfig *gameconfigobj;
        KogsPlayer *playerobj;
        KogsGameFinished *gamefinishedobj;
        KogsBaton *batonobj;
        KogsSlamParams *slamparamsobj;
        KogsAdvertising *adobj;

    case KOGSID_KOG:
    case KOGSID_SLAMMER:
        matchobj = (KogsMatchObject*)pobj;
        info.push_back(std::make_pair("imageId", matchobj->imageId));
        info.push_back(std::make_pair("setId", matchobj->setId));
        info.push_back(std::make_pair("subsetId", matchobj->subsetId));
        info.push_back(std::make_pair("printId", std::to_string(matchobj->printId)));
        info.push_back(std::make_pair("appearanceId", std::to_string(matchobj->appearanceId)));
        if (pobj->objectType == KOGSID_SLAMMER)
            info.push_back(std::make_pair("borderId", std::to_string(matchobj->borderId)));
		if (!matchobj->packId.IsNull())
		    info.push_back(std::make_pair("packId", matchobj->packId.GetHex()));
        break;

    case KOGSID_PACK:
        packobj = (KogsPack*)pobj;
        break;

    case KOGSID_CONTAINER:
        containerobj = (KogsContainer*)pobj;
        info.push_back(std::make_pair("playerId", containerobj->playerid.GetHex()));
        ListContainerTokenids(*containerobj);
        for (const auto &t : containerobj->tokenids)
        {
            infotokenids.push_back(t.GetHex());
        }
        info.push_back(std::make_pair("tokenids", infotokenids));
        break;

    case KOGSID_GAME:
        gameobj = (KogsGame*)pobj;
        gameinfo = KogsGameStatus(*gameobj);
        info.push_back(std::make_pair("gameinfo", gameinfo));
        break;

    case KOGSID_GAMECONFIG:
        gameconfigobj = (KogsGameConfig*)pobj;
        info.push_back(std::make_pair("KogsInStack", gameconfigobj->numKogsInStack));
        info.push_back(std::make_pair("KogsInContainer", gameconfigobj->numKogsInContainer));
        info.push_back(std::make_pair("KogsToAdd", gameconfigobj->numKogsToAdd));
        info.push_back(std::make_pair("MaxTurns", gameconfigobj->maxTurns));
        for (const auto &v : gameconfigobj->heightRanges)
        {
            UniValue range(UniValue::VOBJ);

            range.push_back(Pair("Left", v.left));
            range.push_back(Pair("Right", v.right));
            range.push_back(Pair("UpperValue", v.upperValue));
            heightranges.push_back(range);
        }
        info.push_back(std::make_pair("HeightRanges", heightranges));
        for (const auto & v : gameconfigobj->strengthRanges)
        {
            UniValue range(UniValue::VOBJ);

            range.push_back(Pair("Left", v.left));
            range.push_back(Pair("Right", v.right));
            range.push_back(Pair("UpperValue", v.upperValue));
            strengthranges.push_back(range);
        }
        info.push_back(std::make_pair("StrengthRanges", strengthranges));
        break;

    case KOGSID_PLAYER:
        playerobj = (KogsPlayer*)pobj;
        break;

    case KOGSID_GAMEFINISHED:
        gamefinishedobj = (KogsGameFinished*)pobj;
        info.push_back(std::make_pair("gameid", gamefinishedobj->gameid.GetHex()));
        info.push_back(std::make_pair("winner", gamefinishedobj->winnerid.GetHex()));
        info.push_back(std::make_pair("isError", (bool)gamefinishedobj->isError));
        for (const auto &t : gamefinishedobj->kogsInStack)
        {
            infotokenids.push_back(t.GetHex());
        }
        info.push_back(std::make_pair("kogsInStack", infotokenids));
        for (const auto &f : gamefinishedobj->kogsFlipped)
        {
            UniValue elem(UniValue::VOBJ);
            elem.push_back(std::make_pair(f.first.GetHex(), f.second.GetHex()));
            flipped.push_back(elem);
        }
        info.push_back(std::make_pair("kogsFlipped", flipped));
        break;

    case KOGSID_BATON:
        batonobj = (KogsBaton*)pobj;
        info.push_back(std::make_pair("gameid", batonobj->gameid.GetHex()));
        info.push_back(std::make_pair("gameconfigid", batonobj->gameconfigid.GetHex()));
        info.push_back(std::make_pair("nextplayerid", batonobj->nextplayerid.GetHex()));
        info.push_back(std::make_pair("nextturn", batonobj->nextturn));
        for (const auto &t : batonobj->kogsInStack)
        {
            infotokenids.push_back(t.GetHex());
        }
        info.push_back(std::make_pair("kogsInStack", infotokenids));
        for (const auto &f : batonobj->kogsFlipped)
        {
            UniValue elem(UniValue::VOBJ);
            elem.push_back(std::make_pair(f.first.GetHex(), f.second.GetHex()));
            flipped.push_back(elem);
        }
        info.push_back(std::make_pair("kogsFlipped", flipped));
        break;  

    case KOGSID_SLAMPARAMS:
        slamparamsobj = (KogsSlamParams*)pobj;
        info.push_back(std::make_pair("gameid", slamparamsobj->gameid.GetHex()));
        info.push_back(std::make_pair("height", slamparamsobj->armHeight));
        info.push_back(std::make_pair("strength", slamparamsobj->armStrength));
        break;

    case KOGSID_ADVERTISING:
        adobj = (KogsAdvertising*)pobj;
        info.push_back(std::make_pair("gameOpts", std::to_string(adobj->gameOpts)));
        info.push_back(std::make_pair("playerid", adobj->playerId.GetHex()));
        break;

    default:
        err.push_back(std::make_pair("result", "error"));
        err.push_back(std::make_pair("error", "unsupported objectType: " + std::string(1, (char)pobj->objectType)));
        return err;
    }
    return info;
}

// output info about any game object
UniValue KogsObjectInfo(uint256 gameobjectid)
{
    std::shared_ptr<KogsBaseObject> spobj( LoadGameObject(gameobjectid) );
    
    if (spobj == nullptr)
    {
        UniValue err(UniValue::VOBJ);
        err.push_back(std::make_pair("result", "error"));
        err.push_back(std::make_pair("error", "could not load object or bad opreturn"));
        return err;
    }

    return DecodeObjectInfo(spobj.get());
}


// send containers back to the player:
static bool AddTransferContainerVouts(CMutableTransaction &mtx, struct CCcontract_info *cpTokens, uint256 gameid, const std::vector<std::shared_ptr<KogsContainer>> &containers, std::vector<CTransaction> &transferContainerTxns)
{
    char txidaddr[KOMODO_ADDRESS_BUFSIZE];
    CPubKey gametxidPk = CCtxidaddr_tweak(txidaddr, gameid);

    struct CCcontract_info *cpKogs, CKogs;
    cpKogs = CCinit(&CKogs, EVAL_KOGS);
    uint8_t kogsPriv[32];
    CPubKey kogsPk = GetUnspendable(cpKogs, kogsPriv);
    char tokensrcaddr[KOMODO_ADDRESS_BUFSIZE];
    GetTokensCCaddress1of2(cpKogs, tokensrcaddr, kogsPk, gametxidPk);  // get 1of2 address for game


    // TODO: if 'play for keeps' mode then try to create tokens back tx

    // try to create send back containers vouts
    for (const auto &c : containers)
    {
        CMutableTransaction mtxDummy;
        if (AddTokenCCInputs(cpTokens, mtxDummy, tokensrcaddr, c->creationtxid, 1, 5, true) > 0)  // check if container not transferred yet
        {
            //add probe condition to sign vintx 1of2 utxo:
            CC* probeCond = MakeTokensCCcond1of2(EVAL_KOGS, kogsPk, gametxidPk);
            uint8_t kogspriv[32];
            GetUnspendable(cpKogs, kogspriv);

            UniValue addResult = TokenAddTransferVout(mtx, cpTokens, CPubKey()/*to indicate use localpk*/, c->creationtxid, tokensrcaddr,  std::vector<CPubKey>{ c->encOrigPk }, {probeCond, kogspriv}, 1, true); // amount = 1 always for NFTs
            cc_free(probeCond);

            if (ResultIsError(addResult))   {
                LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "could not add transfer container back for containerid=" << c->creationtxid.GetHex() << " error=" << ResultGetError(addResult) << std::endl);
                return false;
            }

            /*vuint8_t vtx = ParseHex(ResultGetTx(sigData)); // unmarshal tx to get it txid;
            CTransaction transfertx;
            if (ResultHasTx(sigData) && E_UNMARSHAL(vtx, ss >> transfertx) && IsTxSigned(transfertx)) {
                transferContainerTxns.push_back(transfertx);
                LOGSTREAMFN("kogs", CCLOG_DEBUG1, stream << "created transfer container back tx=" << ResultGetTx(sigData) << " txid=" << transfertx.GetHash().GetHex() << std::endl);
                testcount++;
            }
            else
            {
                LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "could not create transfer container back tx=" << HexStr(E_MARSHAL(ss << transfertx)) << " for containerid=" << c->creationtxid.GetHex() << " CCerror=" << CCerror << std::endl);
                isError = true;
                break;  // restored break, do not create game finish on this node if it has errors
            }*/
        }
    }
    return true;
}

// get winner playerid
static uint256 GetWinner(const KogsBaton *pbaton)
{
    // calc total flipped for each playerid
    std::map<uint256, int32_t> flippedCounts;
    for(auto const &flipped : pbaton->kogsFlipped)
    {
        flippedCounts[flipped.second] ++;
    }

    // find max
    int32_t cur_max = 0;
    uint256 winner;
    for(auto const &counted : flippedCounts)
    {
        if (cur_max < counted.second)
        {
            winner = counted.first;
            cur_max = counted.second;
        }
    }
    return winner;
}

bool KogsCreateNewBaton(KogsBaseObject *pPrevObj, uint256 &gameid, std::shared_ptr<KogsGameConfig> &spGameConfig, std::shared_ptr<KogsPlayer> &spPlayer, std::shared_ptr<KogsBaton> &spPrevBaton, KogsBaton &newbaton, const KogsBaton *ptestbaton, KogsGameFinished &gamefinished, bool &bGameFinished)
{
	int32_t nextturn;
	int32_t turncount = 0;
	bGameFinished = false;

	if (pPrevObj->objectType != KOGSID_GAME && pPrevObj->objectType != KOGSID_SLAMPARAMS)	{
        LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "invalid previous object creationtxid=" << pPrevObj->creationtxid.GetHex() << std::endl);
		return false; 
	}

	// from prev baton or emty if no prev baton
	std::vector<uint256> playerids;
	std::vector<uint256> kogsInStack;
	std::vector<std::pair<uint256, uint256>> kogsFlipped;
	gameid = zeroid;
	uint256 gameconfigid = zeroid;

	if (pPrevObj->objectType == KOGSID_GAME)  // first turn
	{
		KogsGame *pgame = (KogsGame *)pPrevObj;
		if (pgame->playerids.size() < 2)
		{
			LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "playerids.size incorrect=" << pgame->playerids.size() << " pPrevObj creationtxid=" << pPrevObj->creationtxid.GetHex() << std::endl);
			return false;
		}
		// randomly select whose turn is the first:
        if (ptestbaton == nullptr)
		    nextturn = rand() % pgame->playerids.size();
        else
            nextturn == ptestbaton->nextturn; // validate
		playerids = pgame->playerids;
		gameid = pPrevObj->creationtxid;
		gameconfigid = pgame->gameconfigid;
	}
	else // slamdata
	{
		CTransaction slamParamsTx;
		uint256 hashBlock;
		
		if (myGetTransaction(pPrevObj->creationtxid, slamParamsTx, hashBlock))
		{
			KogsSlamParams *pslamparams = (KogsSlamParams *)pPrevObj;
			gameid = pslamparams->gameid;

			// load the baton
			// slam param txvin[0] is the baton txid
			KogsBaseObject *pPrevBaton = LoadGameObject(slamParamsTx.vin[0].prevout.hash);
			// LOGSTREAMFN("kogs", CCLOG_DEBUG3, stream << "p==null:" << (p==nullptr) << " p->objectType=" << (char)(p?p->objectType:' ') << std::endl);
			// spBaton.reset(p);
			if (pPrevBaton && pPrevBaton->objectType == KOGSID_BATON)
			{
				spPrevBaton.reset( (KogsBaton *)pPrevBaton );
				playerids = spPrevBaton->playerids;
				kogsInStack = spPrevBaton->kogsInStack;
				kogsFlipped = spPrevBaton->kogsFlipped;
				nextturn = spPrevBaton->nextturn;
				nextturn++;
				if (nextturn == playerids.size())
					nextturn = 0;
				turncount = spPrevBaton->prevturncount + 1; // previously passed turns' count
				gameconfigid = spPrevBaton->gameconfigid;
			}
			else
			{
				LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "slam params vin0 is not the baton, slamparams txid=" << pPrevObj->creationtxid.GetHex() << std::endl);
				return false;
			}
		}
		else
		{
			LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "could not load slamparams tx for txid=" << pPrevObj->creationtxid.GetHex() << std::endl);
			return false;
		}
	}

	KogsBaseObject *pGameConfig = LoadGameObject(gameconfigid);
	if (pGameConfig->objectType != KOGSID_GAMECONFIG)
	{
		LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "skipped prev baton for gameid=" << gameid.GetHex() << " can't load gameconfig with id=" << gameconfigid.GetHex() << std::endl);
		return false;
	}
	spGameConfig.reset((KogsGameConfig*)pGameConfig);

	KogsBaseObject *pPlayer( LoadGameObject(playerids[nextturn]) );
	if (pPlayer == nullptr || pPlayer->objectType != KOGSID_PLAYER)
	{
		LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "skipped prev baton for gameid=" << gameid.GetHex() << " can't load player with id=" << playerids[nextturn].GetHex() << std::endl);
		return false;
	}
	spPlayer.reset((KogsPlayer*)pPlayer);
	std::vector<std::shared_ptr<KogsContainer>> spcontainers;
	newbaton.nameId = "baton";
	newbaton.descriptionId = "turn";
	newbaton.nextturn = nextturn;
	newbaton.nextplayerid = playerids[nextturn];
	newbaton.playerids = playerids;
	newbaton.kogsInStack = kogsInStack;
	newbaton.kogsFlipped = kogsFlipped;
	newbaton.prevturncount = turncount;  
	newbaton.gameid = gameid;
	newbaton.gameconfigid = gameconfigid;
	bool bBatonCreated = KogsManageStack(*spGameConfig.get(), (KogsSlamParams *)pPrevObj, spPrevBaton.get(), newbaton, ptestbaton, spcontainers);
	if (!bBatonCreated) {
		LOGSTREAMFN("kogs", CCLOG_INFO, stream << "baton not created for gameid=" << gameid.GetHex() << std::endl);
		return false;
	}

	if (IsGameFinished(*spGameConfig.get(), &newbaton))
    {  
		bGameFinished = true;
	 	gamefinished.kogsInStack = spPrevBaton->kogsInStack;
		gamefinished.kogsFlipped = spPrevBaton->kogsFlipped;
		gamefinished.gameid = gameid;
		gamefinished.winnerid = GetWinner(spPrevBaton.get());
		gamefinished.isError = false;
	}
	return true;
}

// create baton or gamefnished object
void KogsCreateMinerTransactions(int32_t nHeight, std::vector<CTransaction> &minersTransactions)
{
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > addressUnspents;
    CPubKey mypk = pubkey2pk(Mypubkey());
    int txbatons = 0;
    int txtransfers = 0;

    if (mypk.size() != CPubKey::COMPRESSED_PUBLIC_KEY_SIZE) {
        static bool warnedMypk = false;
        if (!warnedMypk) {
            warnedMypk = true;
            LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "no -pubkey on this node, can't not create baton transactions" << std::endl);
        }
        return;
    }

    if (!CheckSysPubKey())   // only syspk can create miner txns
        return;

    struct CCcontract_info *cp, C;
    cp = CCinit(&C, EVAL_KOGS);

    LOGSTREAMFN("kogs", CCLOG_DEBUG3, stream << "listing all games with batons" << std::endl);

    //srand(time(NULL));  // TODO check srand already called in init()

    LockUtxoInMemory lockutxos;  // lock in memory tx inputs to prevent from subsequent adding

    // find all games with unspent batons:
    SetCCunspentsWithMempool(addressUnspents, cp->unspendableCCaddr, true);    // look all tx on the global cc addr
    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it = addressUnspents.begin(); it != addressUnspents.end(); it++)
    {
        uint256 dummytxid;
        int32_t dummyvout;

        // its very important to check if the baton not spent in mempool, otherwise we could pick up a previous already spent baton
        if (it->second.satoshis == KOGS_BATON_AMOUNT /*&& !myIsutxo_spentinmempool(dummytxid, dummyvout, it->first.txhash, it->first.index)*/) // picking game or slamparam utxos with markers=20000
        {
            std::shared_ptr<KogsBaseObject> spPrevObj(LoadGameObject(it->first.txhash)); // load and unmarshal game or slamparam
            LOGSTREAMFN("kogs", CCLOG_DEBUG3, stream << "found utxo" << " txid=" << it->first.txhash.GetHex() << " vout=" << it->first.index << " spPrevObj=" << spPrevObj.get() << std::endl);

            if (spPrevObj.get() != nullptr)
            {
				std::vector<CTransaction> myTransactions;
				std::shared_ptr<KogsGameConfig> spGameConfig;
				std::shared_ptr<KogsPlayer> spPlayer;
				std::shared_ptr<KogsBaton> spPrevBaton;
				KogsBaton newbaton;
				KogsGameFinished gamefinished;
				uint256 gameid;
				bool bGameFinished;

				if (!KogsCreateNewBaton(spPrevObj.get(), gameid, spGameConfig, spPlayer, spPrevBaton, newbaton, nullptr, gamefinished, bGameFinished))
					continue;

                // first requirement: finish the game if turncount == player.size * maxTurns and send kogs to the winners
                // my addition: finish if stack is empty
                if (bGameFinished)
                {                            
                    LOGSTREAMFN("kogs", CCLOG_INFO, stream << "either stack empty=" << spPrevBaton->kogsInStack.empty() << " or all reached max turns, total turns=" << spPrevBaton->prevturncount << ", starting to finish game=" << gameid.GetHex() << std::endl);

                    CMutableTransaction mtx;
                    struct CCcontract_info *cpTokens, CTokens;
                    cpTokens = CCinit(&CTokens, EVAL_TOKENS);

                    TokenBeginTransferTx(mtx, cpTokens, mypk, 10000);
                    std::vector<CTransaction> transferContainerTxns;
                    std::vector<std::shared_ptr<KogsContainer>> spcontainers;
                    KogsDepositedContainerListImpl(gameid, spcontainers);
                        
                    char txidaddr[KOMODO_ADDRESS_BUFSIZE];
                    CPubKey gametxidPk = CCtxidaddr_tweak(txidaddr, gameid);
                    CScript opret;
                    AddBatonVouts(mtx, cpTokens, it->first.txhash, it->first.index, &gamefinished, gametxidPk, opret);  // send game finished baton to unspendable addr

                    if (AddTransferContainerVouts(mtx, cpTokens, gameid, spcontainers, transferContainerTxns))
                    {
                        UniValue finalizeResult = TokenFinalizeTransferTx(mtx, cpTokens, CPubKey(), 10000, opret);
                        if (!ResultIsError(finalizeResult)) {
                            myTransactions.push_back(mtx);
                            txtransfers++;
                        }
                        else
                            LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "error finalizing tx for gameid=" << gameid.GetHex() << " error=" << ResultGetError(finalizeResult) << std::endl);

                    }
                    else
                        LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "error adding transfer container vouts for gameid=" << gameid.GetHex() << std::endl);

                }
                else
                {
                    // game not finished - send baton:
                    CTransaction batontx = CreateBatonTx(it->first.txhash, it->first.index, &newbaton, spPlayer->encOrigPk);  // send baton to player pubkey;
                    if (!batontx.IsNull() && IsTxSigned(batontx))
                    {
                        LOGSTREAMFN("kogs", CCLOG_INFO, stream << "created baton txid=" << batontx.GetHash().GetHex() << " to next playerid=" << newbaton.nextplayerid.GetHex() << std::endl);
                        txbatons++;
                        myTransactions.push_back(batontx);
                    }
                    else
                    {
                        LOGSTREAMFN("kogs", CCLOG_ERROR, stream << "could not create baton tx=" << HexStr(E_MARSHAL(ss << batontx)) << " to next playerid=" << newbaton.nextplayerid.GetHex() << std::endl);
                    }
                }
                
                
                for (const auto &tx : myTransactions)
                {
                    std::string hextx = HexStr(E_MARSHAL(ss << tx));
                    UniValue rpcparams(UniValue::VARR), txparam(UniValue::VOBJ);
                    txparam.setStr(hextx);
                    rpcparams.push_back(txparam);
                    try {
                        sendrawtransaction(rpcparams, false, mypk);  // NOTE: throws error!
                    }
                    catch (std::runtime_error error)
                    {
                        LOGSTREAMFN("kogs", CCLOG_ERROR, stream << std::string("cant send transaction: bad parameters: ") + error.what() << std::endl);
                    }
                    catch (UniValue error)
                    {
                        LOGSTREAMFN("kogs", CCLOG_ERROR, stream << std::string("error: can't send tx: ") + hextx + " error: " + ResultGetError(error) << " (" << error["code"].get_int()<< " " << error["message"].getValStr() << ")" << std::endl);
                    }
                }
            }
            else
                LOGSTREAMFN("kogs", CCLOG_DEBUG1, stream << "can't load object: " << (spPrevObj.get() ? std::string("incorrect objectType=") + std::string(1, (char)spPrevObj->objectType) : std::string("nullptr")) << std::endl);
        }
    }

    LOGSTREAMFN("kogs", CCLOG_DEBUG3, stream << "created batons=" << txbatons << " created container transfers=" << txtransfers << std::endl);
}

// decode kogs tx utils
static void decode_kogs_opret_to_univalue(const CTransaction &tx, int32_t nvout, UniValue &univout)
{

    std::shared_ptr<KogsBaseObject> spobj(DecodeGameObjectOpreturn(tx, nvout));

    UniValue uniret = DecodeObjectInfo(spobj.get());
    univout.pushKVs(uniret);
    if (spobj != nullptr)
    {
        if (spobj->istoken) {
            univout.push_back(std::make_pair("category", "token"));
            univout.push_back(std::make_pair("opreturn-from", "creation-tx"));
        }
        else    {
            univout.push_back(std::make_pair("category", "enclosure"));
            univout.push_back(std::make_pair("opreturn-from", "latest-tx"));
        }
    }
}

void decode_kogs_vout(const CTransaction &tx, int32_t nvout, UniValue &univout)
{
    vuint8_t vopret;

    if (!GetOpReturnData(tx.vout[nvout].scriptPubKey, vopret))
    {
        char addr[KOMODO_ADDRESS_BUFSIZE];

        univout.push_back(Pair("nValue", tx.vout[nvout].nValue));
        if (tx.vout[nvout].scriptPubKey.IsPayToCryptoCondition())
        {
            CScript ccopret;

            univout.push_back(Pair("vout-type", "cryptocondition"));
            if (MyGetCCopretV2(tx.vout[nvout].scriptPubKey, ccopret)) // reserved for cc opret
            {
                decode_kogs_opret_to_univalue(tx, nvout, univout);
            }
            else
            {
                univout.push_back(Pair("ccdata", "no"));
            }
        }
        else
        {
            univout.push_back(Pair("vout-type", "normal"));
        }
        Getscriptaddress(addr, tx.vout[nvout].scriptPubKey);
        univout.push_back(Pair("address", addr));
        univout.push_back(Pair("scriptPubKey", tx.vout[nvout].scriptPubKey.ToString()));
    }
    else
    {
        univout.push_back(Pair("vout-type", "opreturn"));
        decode_kogs_opret_to_univalue(tx, nvout, univout);
    }
}

UniValue KogsDecodeTxdata(const vuint8_t &txdata, bool printvins)
{
    UniValue result(UniValue::VOBJ);
    CTransaction tx;

    if (E_UNMARSHAL(txdata, ss >> tx))
    {
        result.push_back(Pair("object", "transaction"));

        UniValue univins(UniValue::VARR);

        if (tx.IsCoinBase())
        {
            UniValue univin(UniValue::VOBJ);
            univin.push_back(Pair("coinbase", ""));
            univins.push_back(univin);
        }
        else if (tx.IsCoinImport())
        {
            UniValue univin(UniValue::VOBJ);
            univin.push_back(Pair("coinimport", ""));
            univins.push_back(univin);
        }
        else
        {
            for (int i = 0; i < tx.vin.size(); i++)
            {
                CTransaction vintx;
                uint256 hashBlock;
                UniValue univin(UniValue::VOBJ);

                univin.push_back(Pair("n", std::to_string(i)));
                univin.push_back(Pair("prev-txid", tx.vin[i].prevout.hash.GetHex()));
                univin.push_back(Pair("prev-n", (int64_t)tx.vin[i].prevout.n));
                univin.push_back(Pair("scriptSig", tx.vin[i].scriptSig.ToString()));
                if (printvins)
                {
                    if (myGetTransaction(tx.vin[i].prevout.hash, vintx, hashBlock))
                    {
                        UniValue univintx(UniValue::VOBJ);
                        decode_kogs_vout(vintx, tx.vin[i].prevout.n, univintx);
                        univin.push_back(Pair("vout", univintx));
                    }
                    else
                    {
                        univin.push_back(Pair("error", "could not load vin tx"));
                    }
                }
                univins.push_back(univin);
            }
        }
        result.push_back(Pair("vins", univins));


        UniValue univouts(UniValue::VARR);

        for (int i = 0; i < tx.vout.size(); i++)
        {
            UniValue univout(UniValue::VOBJ);

            univout.push_back(Pair("n", std::to_string(i)));
            decode_kogs_vout(tx, i, univout);
            univouts.push_back(univout);
        }
        result.push_back(Pair("vouts", univouts));
    }
    else
    {
        CScript opret(txdata.begin(), txdata.end());
        CScript ccopret;
        vuint8_t vopret;
        UniValue univout(UniValue::VOBJ);

        if (GetOpReturnData(opret, vopret))
        {
            result.push_back(Pair("object", "opreturn"));
            //decode_kogs_opret_to_univalue(tx, tx.vout.size()-1, univout);
            
        }
        else if (MyGetCCopretV2(opret, ccopret))  // reserved until cc opret use
        {
            result.push_back(Pair("object", "vout-ccdata"));
            //decode_kogs_opret_to_univalue(ccopret, univout);
            GetOpReturnData(ccopret, vopret);
        }
        else {
            result.push_back(Pair("object", "cannot decode"));
        }
        if (vopret.size() > 2)
        {
            univout.push_back(Pair("eval-code", HexStr(std::string(1, vopret[0]))));
            univout.push_back(Pair("object-type", std::string(1, isprint(vopret[1]) ? vopret[1] : ' ')));
            univout.push_back(Pair("version", std::to_string(vopret[2])));
            result.push_back(Pair("decoded", univout));
        }
    }

    return result;
}

// consensus code
static bool log_and_return_error(Eval* eval, std::string errorStr, const CTransaction &tx) {
	LOGSTREAM("kogs", CCLOG_ERROR, stream << "KogsValidate() ValidationError: " << errorStr << " tx=" << HexStr(E_MARSHAL(ss << tx)) << std::endl);
	return eval->Error(errorStr);
}

static bool check_match_object(struct CCcontract_info *cp, const KogsMatchObject *pObj, const CTransaction &tx, std::string &errorStr)
{
	CPubKey kogsGlobalPk = GetUnspendable(cp, NULL);

	/*for(auto const &vin : tx.vin)
		if (check_signing_pubkey(vin.scriptSig) == kogsGlobalPk)	{ //spent with global pk
			return errorStr = "invalid kogs transfer from global address", false; // this is allowed only with the baton
		}*/
	return true;
}

static bool check_baton(struct CCcontract_info *cp, const KogsBaton *pbaton, const CTransaction &tx, std::string &errorStr)
{
	// get prev baton or game config
    std::shared_ptr<KogsGameConfig> spGameConfig;
    std::shared_ptr<KogsPlayer> spPlayer;
    std::shared_ptr<KogsBaton> spPrevBaton;
    KogsBaton testbaton;
    KogsGameFinished gamefinished;
    uint256 gameid;
    bool bGameFinished;

    if (tx.vin.size() == 0)
        return errorStr = "no vins", false;

    std::shared_ptr<KogsBaseObject> spPrevObj(LoadGameObject(tx.vin[0].prevout.hash)); 
    if (spPrevObj == nullptr)
        return errorStr = "could not load prev object", false;

    if (!KogsCreateNewBaton(spPrevObj.get(), gameid, spGameConfig, spPlayer, spPrevBaton, testbaton, pbaton, gamefinished, bGameFinished))
        return errorStr = "could not create test baton", false;

    // compare test and validated baton
    if (testbaton != *pbaton)   
        return errorStr = "could not validate test baton", false;

    if (bGameFinished)
    {                            
        // check cointainers if game is finished
        std::vector<std::shared_ptr<KogsContainer>> spcontainers;
        KogsDepositedContainerListImpl(gameid, spcontainers);
        std::set<uint256> gameConts;
        std::set<CPubKey> pks; 
        for(const auto &c : spcontainers)   {
            gameConts.insert(c->creationtxid);
            pks.insert(c->encOrigPk);
        }

        // get sent back containers
        std::set<uint256> sentConts;
        for(const auto &v : tx.vout) {
            uint256 tokenid;
            for(auto const &pk : pks)
                if (IsNFTSpkMine(tx.GetHash(), v.scriptPubKey, pk, tokenid))
                    sentConts.insert(tokenid);
        }
        if (sentConts.size() != spcontainers.size())
            return errorStr = "not all containers are sent back", false;
    }
	return true;
}

// check if adding removing kogs to the container is allowed
static bool check_container_ops(struct CCcontract_info *cp, const KogsContainerOps *pContOps, const CTransaction &tx, std::string &errorStr)
{

	// if kogs are added or removed to/from a container, check:
	// this is my container
	// container is not deposited to a game
	std::shared_ptr<KogsBaseObject> spObj( LoadGameObject(pContOps->containerid) );
	if (spObj == nullptr || spObj->objectType != KOGSID_CONTAINER)
		return errorStr = "could not load containerid", false;

	KogsContainer *pCont = (KogsContainer *)spObj.get();

	std::vector<CPubKey> destpks { GetUnspendable(cp, NULL), CCtxidaddr_tweak(NULL, pContOps->containerid) };
	std::set<CPubKey> mypk;
	std::vector<uint256> tokenids;
    //struct CCcontract_info *cp, C;
    //cp = CCinit(&C, EVAL_TOKENS);

	for (auto const &vout : tx.vout)	{
		uint256 tokenid;
		std::vector<CPubKey> pks;
		std::vector<vuint8_t> drops;
		if (DecodeTokenOpRetV1(vout.scriptPubKey, tokenid, pks, drops) != 0)	{
			if (pContOps->objectType == KOGSID_ADDTOCONTAINER)	{
				if (pks != destpks)		// check tokens are sent to the container 1of2 address
					return errorStr = "dest pubkeys do not match container id", false;
			}
			else {
				if (pks.size() == 1)
					mypk.insert(pks[0]); // collect token dest pubkeys
			}
			tokenids.push_back(tokenid);
		}
	}

	if (pContOps->objectType == KOGSID_ADDTOCONTAINER)	{
		// check that the sent tokens currently are on the container owner pk
		for(auto const &t : tokenids)	
			if (!IsNFTmine(t, pCont->encOrigPk))
				return errorStr = "not your container to add tokens", false;
	}
	else {
		// check all removed tokens go to the same pk
		if (mypk.size() != 1)	
			return errorStr = "should be only one dest pubkey for removing tokens", false;

		if (!IsEnclosureMine(pContOps->containerid, *mypk.begin()))
			return errorStr = "not your container to remove tokens", false;
	}

	LOGSTREAMFN("kogs", CCLOG_DEBUG1, stream << "validated ok" << std::endl);
	return true;
}

bool KogsValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn)
{
	std::string errorStr;

    //return true;

	if (tx.vout.size() == 0)
		return log_and_return_error(eval, "no vouts", tx);

	const KogsBaseObject *pBaseObj = DecodeGameObjectOpreturn(tx, tx.vout.size() - 1);
	if (pBaseObj == nullptr)	
		return log_and_return_error(eval, "can't decode game object", tx);

	// check creator pubkey:
	CPubKey sysPk = GetSystemPubKey();
	if (!sysPk.IsValid())
		return log_and_return_error(eval, "invalid sys pubkey", tx);

	// check pk for objects that only allowed to create by sys pk
	if (KogsIsSysCreateObject(pBaseObj->objectType) && sysPk != pBaseObj->encOrigPk)
		return log_and_return_error(eval, "invalid object creator pubkey", tx);

	if (!pBaseObj->istoken)
	{
        if (pBaseObj->funcid == 't' || pBaseObj->funcid == 'c')
        {
            switch(pBaseObj->objectType)	{
                case KOGSID_KOG:
                case KOGSID_SLAMMER:
                    if (!check_match_object(cp, (KogsMatchObject*)pBaseObj, tx, errorStr))
                        return log_and_return_error(eval, "invalid kog or slammer op: " + errorStr, tx);
                    else
                        return true;
                case KOGSID_PACK:
                    break;
                case KOGSID_CONTAINER:
                    break;
                case KOGSID_PLAYER:
                    return true;
                case KOGSID_GAMECONFIG:
                    // could only be in funcid 'c':
                    return log_and_return_error(eval, "invalid gameconfig object", tx);
                case KOGSID_GAME:
                    // could only be in funcid 'c':
                    return log_and_return_error(eval, "invalid game object", tx);
                case KOGSID_SLAMPARAMS:
                    return true;
                case KOGSID_GAMEFINISHED:
                case KOGSID_BATON:
                    if (!check_baton(cp, (KogsBaton*)pBaseObj, tx, errorStr))
                        return log_and_return_error(eval, "invalid baton: " + errorStr, tx);
                    else
                        return true;
                    break;
                case KOGSID_ADVERTISING:
                    break;
                case KOGSID_ADDTOCONTAINER:
                case KOGSID_REMOVEFROMCONTAINER:
                    if (!check_container_ops(cp, (KogsContainerOps*)pBaseObj, tx, errorStr))
                        return log_and_return_error(eval, "invalid op with kogs in container: " + errorStr, tx);
                    else
                        return true;			
                    break;
                default:
                    return log_and_return_error(eval, "invalid object type", tx);
            }
        }
        return log_and_return_error(eval, "invalid funcid", tx);
	}
    return true;  // allow simple token transfers
}