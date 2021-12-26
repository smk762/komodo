
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

// todo:
// spentinfo via CC

// headers "sync" make sure it connects to prior blocks to notarization. use getinfo hdrht to get missing hdrs

// make sure to sanity check all vector lengths on receipt
// make sure no files are updated (this is to allow nSPV=1 and later nSPV=0 without affecting database)
// bug: under load, fullnode was returning all 0 nServices

#include <stdint.h>
#include <stdlib.h>
#include "main.h"
#include "notarisationdb.h"
#include "komodo_defs.h"
#include "komodo_utils.h"
#include "cc/CCinclude.h"
#include "nspv_defs.h"

// useful utility functions

uint256 NSPV_doublesha256(uint8_t *data,int32_t datalen)
{
    bits256 _hash; uint256 hash; int32_t i;
    _hash = bits256_doublesha256(0,data,datalen);
    for (i=0; i<32; i++)
        ((uint8_t *)&hash)[i] = _hash.bytes[31 - i];
    return(hash);
}

uint256 NSPV_hdrhash(const struct NSPV_equihdr &hdr)
{
    CBlockHeader block;
    block.nVersion = hdr.nVersion;
    block.hashPrevBlock = hdr.hashPrevBlock;
    block.hashMerkleRoot = hdr.hashMerkleRoot;
    block.hashFinalSaplingRoot = hdr.hashFinalSaplingRoot;
    block.nTime = hdr.nTime;
    block.nBits = hdr.nBits;
    block.nNonce = hdr.nNonce;
    block.nSolution = hdr.nSolution;
    return(block.GetHash());
}

int32_t NSPV_txextract(CTransaction &tx, const vuint8_t &data)
{
    if ( data.size() < MAX_TX_SIZE_AFTER_SAPLING )
    {
        if ( DecodeHexTx(tx, HexStr(data)) != 0 )
            return(0);
    }
    return(-1);
}


int32_t NSPV_fastnotariescount(CTransaction tx,uint8_t elected[64][33],uint32_t nTime)
{
    CPubKey pubkeys[64]; uint8_t sig[512]; CScript scriptPubKeys[64]; CMutableTransaction mtx(tx); int32_t vini,j,siglen,retval; uint64_t mask = 0; char *str; std::vector<std::vector<unsigned char>> vData;
    for (j=0; j<64; j++)
    {
        pubkeys[j] = buf2pk(elected[j]);
        scriptPubKeys[j] = (CScript() << ParseHex(HexStr(pubkeys[j])) << OP_CHECKSIG);
        //fprintf(stderr,"%d %s\n",j,HexStr(pubkeys[j]).c_str());
    }
    fprintf(stderr,"txid %s\n",tx.GetHash().GetHex().c_str());
    //for (vini=0; vini<tx.vin.size(); vini++)
    //    mtx.vin[vini].scriptSig.resize(0);
    for (vini=0; vini<tx.vin.size(); vini++)
    {
        CScript::const_iterator pc = tx.vin[vini].scriptSig.begin();
        if ( tx.vin[vini].scriptSig.GetPushedData(pc,vData) != 0 )
        {
            vData[0].pop_back();
            for (j=0; j<64; j++)
            {
                if ( ((1LL << j) & mask) != 0 )
                    continue;
                char coinaddr[64]; Getscriptaddress(coinaddr,scriptPubKeys[j]);
                NSPV_SignTx(mtx,vini,10000,scriptPubKeys[j],nTime); // sets SIG_TXHASH
                //fprintf(stderr,"%s ",SIG_TXHASH.GetHex().c_str());
                if ( (retval= pubkeys[j].Verify(SIG_TXHASH,vData[0])) != 0 )
                {
                    //fprintf(stderr,"(vini.%d %s.%d) ",vini,coinaddr,retval);
                    mask |= (1LL << j);
                    break;
                }
            }
            //fprintf(stderr," vini.%d verified %llx\n",vini,(long long)mask);
        }
    }
    return(bitweight(mask));
}

/*
 NSPV_notariescount is the slowest process during full validation as it requires looking up 13 transactions.
 one way that would be 10000x faster would be to bruteforce validate the signatures in each vin, against all 64 pubkeys! for a valid tx, that is on average 13*32 secp256k1/sapling verify operations, which is much faster than even a single network request.
 Unfortunately, due to the complexity of calculating the hash to sign for a tx, this bruteforcing would require determining what type of signature method and having sapling vs legacy methods of calculating the txhash.
 It could be that the fullnode side could calculate this and send it back to the superlite side as any hash that would validate 13 different ways has to be the valid txhash.
 However, since the vouts being spent by the notaries are highly constrained p2pk vouts, the txhash can be deduced if a specific notary pubkey is indeed the signer
 */
int32_t NSPV_notariescount(CTransaction tx,uint8_t elected[64][33])
{
    uint8_t *script; CTransaction vintx; int64_t rewardsum = 0; int32_t i,j,utxovout,scriptlen,numsigs = 0,txheight,currentheight; uint256 hashBlock;
    for (i=0; i<tx.vin.size(); i++)
    {
        utxovout = tx.vin[i].prevout.n;
        if ( NSPV_gettransaction(1,utxovout,tx.vin[i].prevout.hash,0,vintx,hashBlock,txheight,currentheight,-1,0,rewardsum) != 0 )
        {
            fprintf(stderr,"error getting %s/v%d\n",tx.vin[i].prevout.hash.GetHex().c_str(),utxovout);
            return(numsigs);
        }
        if ( utxovout < vintx.vout.size() )
        {
            script = (uint8_t *)&vintx.vout[utxovout].scriptPubKey[0];
            if ( (scriptlen= vintx.vout[utxovout].scriptPubKey.size()) == 35 )
            {
                for (j=0; j<64; j++)
                    if ( memcmp(&script[1],elected[j],33) == 0 )
                    {
                        numsigs++;
                        break;
                    }
            } else fprintf(stderr,"invalid scriptlen.%d\n",scriptlen);
        } else fprintf(stderr,"invalid utxovout.%d vs %d\n",utxovout,(int32_t)vintx.vout.size());
    }
    return(numsigs);
}

int32_t NSPV_notarizationextract(int32_t verifyntz, int32_t* ntzheightp, uint256* blockhashp, uint256* desttxidp, int16_t *momdepthp, CTransaction tx)
{
    vuint8_t vopret;
    AssertLockHeld(cs_main);

    if ( tx.vout.size() >= 2 )
    {
        //const char *symbol = (ASSETCHAINS_SYMBOL[0] == 0) ? "KMD" : ASSETCHAINS_SYMBOL;
        NotarisationData nota;
        GetOpReturnData(tx.vout[1].scriptPubKey, vopret);
        //if ( vopret.size() >= 32*2+4 )
        if (!vopret.empty() && E_UNMARSHAL(vopret, ss >> nota))
        {
            //sleep(1); // needed to avoid no pnodes error
            //*desttxidp = NSPV_opretextract(ntzheightp, blockhashp, symbol, vopret);
            *ntzheightp = nota.height;
            *blockhashp = nota.blockHash;
            *desttxidp = nota.txHash;  
            *momdepthp = nota.MoMDepth;         
            if (verifyntz != 0)   
            {
                uint32_t nTime = NSPV_blocktime(*ntzheightp);
                int32_t numsigs=0; 
                uint8_t elected[64][33]; 

                komodo_notaries(elected,*ntzheightp,nTime);
                if ((numsigs= NSPV_fastnotariescount(tx,elected,nTime)) < 12 )
                {
                    LogPrintf("%s error notaries numsigs.%d less than required min 12\n", __func__, numsigs);
                    return(-3);
                } 
            }
            return(0);
        }
        else
        {
            LogPrintf("%s opretsize.%d error\n", __func__, (int32_t)vopret.size());
            return(-2);
        }
    } else {
        LogPrintf("%s tx.vout.size().%d error\n", __func__, (int32_t)tx.vout.size());
        return(-1);
    }
}

