
#include <cryptoconditions.h>
#include <gtest/gtest.h>

#include "cc/CCinclude.h"

#include "cc/CCtokens.h"
#include "cc/CCtokens_impl.h"

#include "cc/CCassets.h"
#include "cc/CCassetstx_impl.h"

#include "cc/CCNFTData.h"


#include "cc/eval.h"
#include "importcoin.h"
#include "base58.h"
#include "core_io.h"
#include "key.h"
#include "main.h"
#include "primitives/transaction.h"
#include "script/cc.h"
#include "script/interpreter.h"
#include "script/serverchecker.h"
#include "txmempool.h"

extern Eval* EVAL_TEST;

namespace CCAssetsTests {

static uint8_t testNum = 0;

uint256 getRandomHash()
{
    CSHA256 sha;
    uint256 res; 
    uint32_t rnd = rand();
    sha.Write((uint8_t*)&rnd, sizeof(rnd));
    sha.Finalize((uint8_t*)&res);
    return res;
}


class EvalMock : public Eval
{
public:
    int currentHeight;
    std::map<uint256, CTransaction> txs;
    std::map<uint256, CBlockIndex> blocks;
    //sstd::map<uint256, std::vector<CTransaction>> spends;

    bool GetTxUnconfirmed(const uint256 &hash, CTransaction &txOut, uint256 &hashBlock) const
    {
        auto r = txs.find(hash);
        if (r != txs.end()) {
            txOut = r->second;
            if (blocks.count(hash) > 0)
                hashBlock = hash;
            return true;
        }
        return false;
    }
};

static EvalMock eval;

static CTransaction txnormal1, txnormal2, txnormal3, txnormal4, txnormalg, txask, txbid, txtokencreate1, txtokencreate2, txtokencreate3, txtokencreateUnused;
static uint256 tokenid1, tokenid2, tokenid3, tokenidUnused;

//  RJXkCF7mn2DRpUZ77XBNTKCe55M2rJbTcu
static CPubKey pk1 = CPubKey(ParseHex("035d3b0f2e98cf0fba19f80880ec7c08d770c6cf04aa5639bc57130d5ac54874db"));
static uint8_t privkey1[] = { 0x0d, 0xf0, 0x44, 0xc4, 0xbe, 0xd3, 0x3b, 0x74, 0xaf, 0x69, 0x6b, 0x05, 0x1d, 0xbf, 0x70, 0x14, 0x2f, 0xc3, 0xa7, 0x8d, 0xa3, 0x47, 0x38, 0xc0, 0x33, 0x6f, 0x50, 0x15, 0xe3, 0xd2, 0x85, 0xee };
//  RR2nTYFBPTJafxQ6en2dhUgaJcMDk4RWef
static CPubKey pk2 = CPubKey(ParseHex("034777b18effce6f7a849b72de8e6810bf7a7e050274b3782e1b5a13d0263a44dc"));
static uint8_t privkey2[] = { 0x9e, 0x69, 0x33, 0x27, 0x1c, 0x1c, 0x60, 0x5e, 0x57, 0xaf, 0xb6, 0xb7, 0xfd, 0xeb, 0xac, 0xa3, 0x11, 0x41, 0xd1, 0x3a, 0xe2, 0x36, 0x47, 0xfc, 0xe4, 0xe0, 0x79, 0x44, 0xae, 0xee, 0x43, 0xde };
//  RTbiYv9u1mrp7TmJspxduJCe3oarCqv9K4
static CPubKey pk3 = CPubKey(ParseHex("025f97b6c42409e8e69eb2fdab281219aafe15169deec801ee621c63cc1ba0bb8c"));
static uint8_t privkey3[] = { 0x0d, 0xf8, 0xcd, 0xd4, 0x42, 0xbd, 0x77, 0xd2, 0xdd, 0x44, 0x89, 0x4b, 0x21, 0x78, 0xbf, 0x8d, 0x8a, 0xc3, 0x30, 0x0c, 0x5a, 0x70, 0x3d, 0xbe, 0xc9, 0x21, 0x75, 0x33, 0x23, 0x77, 0xd3, 0xde };

std::map<CPubKey, uint8_t*> testKeys = {
    { pk1, privkey1 },
    { pk2, privkey2 },
    { pk3, privkey3 },
};

//char Tokensv2CChexstr[] = { "032fd27f72591b02f13a7f9701246eb0296b2be7cfdad32c520e594844ec3d4801" };
//uint8_t Tokensv2CCpriv[] = { 0xb5, 0xba, 0x92, 0x7f, 0x53, 0x45, 0x4f, 0xf8, 0xa4, 0xad, 0x0d, 0x38, 0x30, 0x4f, 0xd0, 0x97, 0xd1, 0xb7, 0x94, 0x1b, 0x1f, 0x52, 0xbd, 0xae, 0xa2, 0xe7, 0x49, 0x06, 0x2e, 0xd2, 0x2d, 0xa5 };

static CPubKey pkunused = CPubKey(ParseHex("034b082c5819b5bf8798a387630ad236a8e800dbce4c4e24a46f36dfddab3cbff5"));

CAmount TestAddNormalInputs(CMutableTransaction &mtx, CPubKey mypk, CAmount amount)
{
    CAmount totalInputs = 0LL;
    char mypkaddr[KOMODO_ADDRESS_BUFSIZE];
    Getscriptaddress(mypkaddr,  CScript() << ParseHex(HexStr(mypk)) << OP_CHECKSIG);
    for (auto const &t : eval.txs) {
        for (int32_t v = 0; v < t.second.vout.size(); v ++) {
            char utxoaddr[KOMODO_ADDRESS_BUFSIZE];
            Getscriptaddress(utxoaddr, t.second.vout[v].scriptPubKey);
            //std::cerr << __func__ << " utxoaddr=" << utxoaddr << " mypkaddr=" << mypkaddr << " tx=" << t.second.GetHash().GetHex() << " v=" << v << std::endl;
            if (strcmp(utxoaddr, mypkaddr) == 0)    {
                mtx.vin.push_back(CTxIn(t.second.GetHash(), v));
                totalInputs += t.second.vout[v].nValue;
                if (totalInputs >= amount)
                    return totalInputs;
            }
        }
    }
    return 0LL;
}

bool TestSignTx(const CKeyStore& keystore, CMutableTransaction& mtx, int32_t vini, int64_t utxovalue, const CScript scriptPubKey)
{
    CTransaction txNewConst(mtx);
    SignatureData sigdata;
    auto consensusBranchId = CurrentEpochBranchId(chainActive.Height() + 1, Params().GetConsensus());
    if (ProduceSignature(TransactionSignatureCreator(&keystore, &txNewConst, vini, utxovalue, SIGHASH_ALL), scriptPubKey, sigdata, consensusBranchId) != 0) {
        UpdateTransaction(mtx, vini, sigdata);
        return true;
    } else {
        std::cerr << __func__ << " signing error for vini=" << vini << " amount=" << utxovalue << std::endl;
        return false;
    }
}

bool TestFinalizeTx(CMutableTransaction& mtx, struct CCcontract_info *cp, uint8_t *myprivkey, CAmount txfee, CScript opret)
{
    auto consensusBranchId = CurrentEpochBranchId(chainActive.Height() + 1, Params().GetConsensus());
    CAmount totaloutputs = 0LL;
    for (int i = 0; i < mtx.vout.size(); i ++) 
        totaloutputs += mtx.vout[i].nValue;

    if (!cp || !cp->evalcode)  {
        std::cerr << __func__ << " invalid cp param" << std::endl;
        return false;
    }

    CAmount totalinputs = 0LL;
    for (int i = 0; i < mtx.vin.size(); i ++) 
    {
        CTransaction vintx;
        uint256 hashBlock;
        if (i == 0 && mtx.vin[i].prevout.n == 10e8)
            continue; // skip pegs vin
        
        if (GetTxUnconfirmedOpt(&eval, mtx.vin[i].prevout.hash, vintx, hashBlock)) {
            totalinputs += vintx.vout[mtx.vin[i].prevout.n].nValue;
        } else {
            fprintf(stderr, "%s couldnt find vintx %s\n", __func__, mtx.vin[i].prevout.hash.ToString().c_str());
            return false;
        }
    }

    if (!myprivkey)  {
        std::cerr << __func__ << "myprivkey not set" << std::endl;
        return false;
    }

    CKey mykey;
    mykey.SetKey32(myprivkey);
    CPubKey mypk = mykey.GetPubKey();
    if (totalinputs >= totaloutputs + txfee) {
        CAmount change = totalinputs - (totaloutputs + txfee);
        if (change > 0)
            mtx.vout.push_back(CTxOut(change, CScript() << ParseHex(HexStr(mypk)) << OP_CHECKSIG));
    }

    if (opret.size() > 0)
        mtx.vout.push_back(CTxOut(0, opret));

    CPubKey globalpk;
    char myccaddr[KOMODO_ADDRESS_BUFSIZE], 
         globaladdr[KOMODO_ADDRESS_BUFSIZE],
         mytokenaddr[KOMODO_ADDRESS_BUFSIZE] = { '\0' };
    globalpk = GetUnspendable(cp, NULL);
    _GetCCaddress(myccaddr, cp->evalcode, mypk, true);
    _GetCCaddress(globaladdr, cp->evalcode, globalpk, true);
    GetTokensCCaddress(cp, mytokenaddr, mypk, true); // get token or nft probe

    PrecomputedTransactionData txdata(mtx);
    for (int i = 0; i < mtx.vin.size(); i++) 
    {
        CTransaction vintx;
        uint256 hashBlock;
        bool mgret;

        if (i == 0 && mtx.vin[i].prevout.n == 10e8) // skip PEGS vin
            continue;
        if ((mgret = GetTxUnconfirmedOpt(&eval, mtx.vin[i].prevout.hash, vintx, hashBlock)) != false) {
            CCwrapper cond;
            uint8_t *privkey = NULL;

            if (!vintx.vout[mtx.vin[i].prevout.n].scriptPubKey.IsPayToCryptoCondition()) 
            {
                /*char coinaddr[KOMODO_ADDRESS_BUFSIZE];
                Getscriptaddress(coinaddr, CScript() << ParseHex(HexStr(mypk)) << OP_CHECKSIG);
                if (strcmp(destaddr, coinaddr) != 0)    {
                    fprintf(stderr, "%s signing error for normal vini.%d myprivkey does not match\n", __func__, i);
                    return false;
                }*/

                CBasicKeyStore tempKeystore;
                CKey key;
                key.Set(myprivkey, myprivkey+32, true);
                tempKeystore.AddKey(key);
                if (!TestSignTx(tempKeystore, mtx, i, vintx.vout[mtx.vin[i].prevout.n].nValue, vintx.vout[mtx.vin[i].prevout.n].scriptPubKey))  {
                    fprintf(stderr, "%s signing error for normal vini.%d\n", __func__, i);
                    return false;
                }

            } else {
                char destaddr[KOMODO_ADDRESS_BUFSIZE];
                if (!Getscriptaddress(destaddr, vintx.vout[mtx.vin[i].prevout.n].scriptPubKey))  {
                    std::cerr << __func__ << " could not Getscriptaddress for scriptPubKey=" << vintx.vout[mtx.vin[i].prevout.n].scriptPubKey.ToString() << std::endl;
                    return false;
                }

                if (strcmp(destaddr, globaladdr) == 0) {
                    std::cerr << __func__ << " found globaladdress=" << globaladdr << " destaddr=" << destaddr << " strlen=" << strlen(globaladdr) << " evalcode=" << (int)cp->evalcode << std::endl;

                    privkey = cp->CCpriv;
                    cond.reset(MakeCCcond1(cp->evalcode, globalpk));
                } else if (strcmp(destaddr, myccaddr) == 0) {
                    privkey = myprivkey;
                    cond.reset(MakeCCcond1(cp->evalcode, mypk));
                    std::cerr << __func__ << " found myccaddr=" << myccaddr << std::endl;

                } else if (strcmp(destaddr, mytokenaddr) == 0) {
                    privkey = myprivkey;
                    cond.reset(MakeTokensv2CCcond1(cp->evalcode, cp->evalcodeNFT, mypk));
                    std::cerr << __func__ << " found mytokenaddr=" << mytokenaddr << " evalcode=" << (int)cp->evalcode << " evalcodeNFT=" << (int)cp->evalcodeNFT << std::endl;
                } else {
                    const uint8_t nullpriv[32] = {'\0'};
                    // use vector of dest addresses and conds to probe vintxconds
                    for (auto& t : cp->CCvintxprobes) {
                        char coinaddr[KOMODO_ADDRESS_BUFSIZE];
                        if (t.CCwrapped.get() != NULL) {
                            CCwrapper anonCond = t.CCwrapped;
                            CCtoAnon(anonCond.get());
                            Getscriptaddress(coinaddr, CCPubKey(anonCond.get(), true));
                            if (strcmp(destaddr, coinaddr) == 0) {
                                std::cerr << __func__ << " found vintxprobe=" << coinaddr  << " privkey=" << (memcmp(t.CCpriv, nullpriv, sizeof(t.CCpriv) / sizeof(t.CCpriv[0])) != 0) << std::endl;
                                if (memcmp(t.CCpriv, nullpriv, sizeof(t.CCpriv) / sizeof(t.CCpriv[0])) != 0)
                                    privkey = t.CCpriv;
                                else
                                    privkey = myprivkey;
                                cond = t.CCwrapped;
                                break;
                            }
                        }
                    }
                }
                if (cond.get() == NULL) {
                    fprintf(stderr, "%s vini.%d has CC signing error: could not find matching cond, address.(%s) %s\n", __func__, i, destaddr, EncodeHexTx(mtx).c_str());
                    return false;
                }

                uint256 sighash = SignatureHash(CCPubKey(cond.get()), mtx, i, SIGHASH_ALL, vintx.vout[mtx.vin[i].prevout.n].nValue, consensusBranchId, &txdata);
                if (cc_signTreeSecp256k1Msg32(cond.get(), privkey, sighash.begin()) == 0) {
                    fprintf(stderr, "%s vini.%d has CC signing error: cc_signTreeSecp256k1Msg32 returned error, address.(%s) %s\n", __func__, i, destaddr, EncodeHexTx(mtx).c_str());
                    return false;
                }
                mtx.vin[i].scriptSig = CCSig(cond.get());
            }
        } else {
            fprintf(stderr, "%s could not find tx %s myGetTransaction returned %d\n", __func__, mtx.vin[i].prevout.hash.ToString().c_str(), mgret);
            return false;
        }
    }
    return true;
}



class TestAssetsCC : public ::testing::Test {
public:
    //uint32_t GetAssetchainsCC() const { return testCcid; }
    //std::string GetAssetchainsSymbol() const { return testSymbol; }

protected:
    static void SetUpTestCase() {  
        // setup eval for tests
        ASSETCHAINS_CC = 2;

        CreateTransactions();
    }
    virtual void SetUp() {
        // enable print
        fDebug = true;
        fPrintToConsole = true;
        mapMultiArgs["-debug"] = { "cctokens", "ccassets" };
    }

    static void CreateTransactions()
    {
        std::cerr << __func__ << " enterred" << std::endl;
        txnormal1 = MakeNormalTx(pk1, 20000);
        txnormal2 = MakeNormalTx(pk1, 20000);
        txnormal3 = MakeNormalTx(pk2, 20000);
        txnormal4 = MakeNormalTx(pk2, 20000);

        struct CCcontract_info *cpTokens, tokensC;  
        cpTokens = CCinit(&tokensC, TokensV2::EvalCode());
        txnormalg = MakeNormalTx(GetUnspendable(cpTokens, NULL), 20000);

        txtokencreate1 = MakeTokenV2CreateTx(pk1);
        tokenid1 = txtokencreate1.GetHash();

        txtokencreate2 = MakeTokenV2CreateTx(pk2);
        tokenid2 = txtokencreate2.GetHash();

        txtokencreate3 = MakeTokenV2CreateTx(pk2);
        tokenid3 = txtokencreate3.GetHash();

        txtokencreateUnused = MakeTokenV2CreateTx(pk1);
        tokenidUnused = txtokencreateUnused.GetHash();

        txask = MakeTokenV2AskTx(pk1);
        txbid = MakeTokenV2BidTx(pk2);
    }

    static CTransaction MakeNormalTx(CPubKey pk, CAmount val)
    {
        CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
        mtx.vin.push_back(CTxIn(getRandomHash(), 0));
        mtx.vout.push_back(CTxOut(val, GetScriptForDestination(pk)));
        eval.txs[mtx.GetHash()] = mtx;
        return CTransaction(mtx);
    }

    /*static CTransaction MakeNormalTxG()
    {
        struct CCcontract_info *cpTokens, tokensC;  
        cpTokens = CCinit(&tokensC, TokensV2::EvalCode());
        uint8_t privkeyg[32];
        CPubKey pkg = GetUnspendable(cpTokens, privkeyg);
        CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
        mtx.vin.push_back(CTxIn(getRandomHash(), 0));
        mtx.vout.push_back(CTxOut(100000, GetScriptForDestination(pkg)));
        return CTransaction(mtx);
    }*/

    static CTransaction MakeTokenV2CreateTx(CPubKey pk)
    {
        struct CCcontract_info *cp, C;
	    cp = CCinit(&C, TokensV2::EvalCode());
        std::string name = "Test";
        std::string description = "desc";

        CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
        mtx.vin.push_back(CTxIn(getRandomHash(), 0));
        mtx.vout.push_back(TokensV2::MakeCC1vout(TokensV2::EvalCode(), TOKENS_MARKER_VALUE, GetUnspendable(cp, NULL)));           
		mtx.vout.push_back(TokensV2::MakeTokensCC1vout(0, 10, pk));
        mtx.vout.push_back(CTxOut(0, TokensV2::EncodeTokenCreateOpRet(vscript_t(pk.begin(), pk.end()), name, description, {  })));
        eval.txs[mtx.GetHash()] = mtx;
        return CTransaction(mtx);
    }


    static CTransaction MakeTokenV2AskTx(CPubKey pk)
    {
        CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
        struct CCcontract_info *cpAssets, C; 

        CAmount askamount = 1000;
        CAmount numtokens = 2;
        CAmount txfee = 10000;

        cpAssets = CCinit(&C, AssetsV2::EvalCode());   // NOTE: assets here!

        CAmount unit_price = askamount / numtokens;

        mtx.vin.push_back(CTxIn(txnormal1.GetHash(), 0));

        CPubKey unspendableAssetsPubkey = GetUnspendable(cpAssets, 0);
        mtx.vout.push_back(TokensV2::MakeTokensCC1vout(AssetsV2::EvalCode(), askamount, unspendableAssetsPubkey));  // tokens to global
        mtx.vout.push_back(TokensV2::MakeCC1vout(AssetsV2::EvalCode(), ASSETS_MARKER_AMOUNT, pk));  // marker for my orders

        mtx.vout.push_back(CTxOut(0, 
            TokensV2::EncodeTokenOpRet(tokenid1, {},     
                { AssetsV2::EncodeAssetOpRet('s', zeroid, unit_price, vuint8_t(pk.begin(), pk.end())) })));
        eval.txs[mtx.GetHash()] = mtx;
        return CTransaction(mtx);
    }

    static CTransaction MakeTokenV2BidTx(CPubKey pk)
    {
        CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
        struct CCcontract_info *cpAssets, C; 

        CAmount bidamount = 1000;
        CAmount numtokens = 2;
        CAmount txfee = 10000;

        cpAssets = CCinit(&C, AssetsV2::EvalCode());   // NOTE: assets here!

        CAmount unit_price = bidamount / numtokens;

        mtx.vin.push_back(CTxIn(txnormal3.GetHash(), 0));

        CPubKey unspendableAssetsPubkey = GetUnspendable(cpAssets, 0);

        mtx.vout.push_back(TokensV2::MakeCC1vout(AssetsV2::EvalCode(), bidamount, unspendableAssetsPubkey));
        mtx.vout.push_back(TokensV2::MakeCC1vout(AssetsV2::EvalCode(), ASSETS_MARKER_AMOUNT, pk));  // marker for my orders

        mtx.vout.push_back(CTxOut(0, 
            TokensV2::EncodeTokenOpRet(tokenid2, {},     
                { AssetsV2::EncodeAssetOpRet('b', zeroid, unit_price, vuint8_t(pk.begin(), pk.end())) })));
        eval.txs[mtx.GetHash()] = mtx;
        return CTransaction(mtx);
    }

    bool TestRunCCEval(const CMutableTransaction &mtx)
    {
        CTransaction tx(mtx);
        PrecomputedTransactionData txdata(tx);
        ServerTransactionSignatureChecker checker(&tx, 0, 0, false, NULL, txdata);
        CValidationState verifystate;
        VerifyEval verifyEval = [] (CC *cond, void *checker) {
            //fprintf(stderr,"checker.%p\n",(TransactionSignatureChecker*)checker);
            return ((TransactionSignatureChecker*)checker)->CheckEvalCondition(cond);
        };

        // set some vars used in validation:
        KOMODO_CONNECTING = 1;
        KOMODO_CCACTIVATE = 1;
        eval.state = CValidationState(); // clear validation state
        EVAL_TEST = &eval;

        for(const auto &vin : tx.vin)  {
            if (IsCCInput(vin.scriptSig))  {
                CC *cond = GetCryptoCondition(vin.scriptSig);
                if (cond == NULL) {
                    std::cerr << __func__ << " GetCryptoCondition could not decode vin.scriptSig" << std::endl;
                    return false;
                }

                int r = cc_verifyEval(cond, verifyEval, &checker);
                if (r == 0) {
                    std::cerr << __func__ << " cc_verify error D" << std::endl;
                    return false;
                }
                std::cerr << __func__ << " cc_verify okay for vin.hash=" << vin.prevout.hash.GetHex() << std::endl;
                break;
            }
        }
        for(const auto &vout : tx.vout)  {
            if (vout.scriptPubKey.IsPayToCCV2())  {

                ScriptError error;

                bool bCheck = checker.CheckCryptoCondition(vout.scriptPubKey.GetCCV2SPK(), &error);
                if (!bCheck) {
                    std::cerr << __func__ << " CheckCryptoCondition error=" << ScriptErrorString(error) << std::endl;
                    return false;
                }
                std::cerr << __func__ << " cc_verify okay for vout.nValue=" << vout.nValue << std::endl;
            }
        }
        return true;
    }
};

TEST_F(TestAssetsCC, tokenv2ask)
{
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cpAssets, C; 
	struct CCcontract_info *cpTokens, tokensC;

    CAmount askamount = 1000;
    CAmount numtokens = 2;
    CAmount txfee = 10000;

    cpAssets = CCinit(&C, AssetsV2::EvalCode());   
    cpTokens = CCinit(&tokensC, TokensV2::EvalCode());  

    uint8_t evalcodeNFT = cpTokens->evalcodeNFT ? cpTokens->evalcodeNFT : 0;
    CAmount unit_price = askamount / numtokens;

    ASSERT_TRUE( TestAddNormalInputs(mtx, pk1, txfee) > 0LL);
    mtx.vin.push_back(CTxIn(txtokencreate1.GetHash(), 1));

    CAmount inputs = txtokencreate1.vout[1].nValue;

    CPubKey unspendableAssetsPubkey = GetUnspendable(cpAssets, NULL);
    mtx.vout.push_back(TokensV2::MakeTokensCC1vout(AssetsV2::EvalCode(), evalcodeNFT, numtokens, unspendableAssetsPubkey));
    mtx.vout.push_back(TokensV2::MakeCC1vout(AssetsV2::EvalCode(), ASSETS_MARKER_AMOUNT, pk1));  
    CAmount CCchange = inputs - numtokens;
    if (CCchange != 0LL) {
        // change to single-eval or non-fungible token vout (although for non-fungible token change currently is not possible)
        mtx.vout.push_back(TokensV2::MakeTokensCC1vout(evalcodeNFT ? evalcodeNFT : TokensV2::EvalCode(), CCchange, pk1));	
    }

    // cond to spend NFT from mypk 
    CCwrapper wrCond(TokensV2::MakeTokensCCcond1(evalcodeNFT, pk1));
    CCAddVintxCond(cpTokens, wrCond, NULL); //NULL indicates to use myprivkey

    // sign vins:
    ASSERT_TRUE(TestFinalizeTx(mtx, cpTokens, testKeys[pk1], txfee,
        TokensV2::EncodeTokenOpRet(tokenid1, { unspendableAssetsPubkey },     
            { AssetsV2::EncodeAssetOpRet('s', zeroid, unit_price, vuint8_t(pk1.begin(), pk1.end())) })));  

    EXPECT_TRUE(TestRunCCEval(mtx));
}

TEST_F(TestAssetsCC, tokenv2bid)
{
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cpAssets, C; 

    CAmount bidamount = 1000;
    CAmount numtokens = 2;
    CAmount txfee = 10000;

    cpAssets = CCinit(&C, AssetsV2::EvalCode());   // NOTE: assets here!

    CAmount unit_price = bidamount / numtokens;

    ASSERT_TRUE( TestAddNormalInputs(mtx, pk1, txfee) > 0LL);

    CPubKey unspendableAssetsPubkey = GetUnspendable(cpAssets, 0);
    mtx.vout.push_back(TokensV2::MakeCC1vout(AssetsV2::EvalCode(), bidamount, unspendableAssetsPubkey));
    mtx.vout.push_back(TokensV2::MakeCC1vout(AssetsV2::EvalCode(), ASSETS_MARKER_AMOUNT, pk1));  // marker for my orders

    // sign vins:
    ASSERT_TRUE(TestFinalizeTx(mtx, cpAssets, testKeys[pk1], txfee,
        TokensV2::EncodeTokenOpRet(tokenid1, {},     
            { AssetsV2::EncodeAssetOpRet('b', zeroid, unit_price, vuint8_t(pk1.begin(), pk1.end())) })));   

    EXPECT_TRUE(TestRunCCEval(mtx));
}


TEST_F(TestAssetsCC, tokenv2fillask)
{

    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cpAssets, C; 
	struct CCcontract_info *cpTokens, tokensC;

    CAmount unit_price = 0;;
    CAmount fillunits = 2;
    CAmount txfee = 10000;
	const int32_t askvout = ASSETS_GLOBALADDR_VOUT; 
    uint256 assetidOpret;
    CAmount paid_unit_price = -1;  // not set
    CAmount orig_assetoshis = txask.vout[askvout].nValue;

    cpAssets = CCinit(&C, AssetsV2::EvalCode());   
    cpTokens = CCinit(&tokensC, TokensV2::EvalCode());  

    TokenDataTuple tokenData;
    vuint8_t vopretNonfungible;
    uint8_t evalcodeNFT = 0;
    uint64_t royaltyFract = 0;  // royaltyFract is N in N/1000 fraction
    ASSERT_TRUE( GetTokenData<TokensV2>(&eval, tokenid1, tokenData, vopretNonfungible) );
    if (vopretNonfungible.size() > 0)  {
        evalcodeNFT = vopretNonfungible.begin()[0];
        GetNftDataAsUint64(vopretNonfungible, NFTPROP_ROYALTY, royaltyFract);
        if (royaltyFract > NFTROYALTY_DIVISOR-1)
            royaltyFract = NFTROYALTY_DIVISOR-1; // royalty upper limit
    }
    vuint8_t ownerpubkey = std::get<0>(tokenData);

    vuint8_t origpubkey;
    uint8_t funcid = GetOrderParams<AssetsV2>(origpubkey, unit_price, assetidOpret, txask); // get orig pk, orig value

    if (paid_unit_price <= 0LL)
        paid_unit_price = unit_price;

    CAmount paid_nValue = paid_unit_price * fillunits;
    CAmount royaltyValue = royaltyFract > 0 ? paid_nValue / NFTROYALTY_DIVISOR * royaltyFract : 0;

    ASSERT_TRUE( TestAddNormalInputs(mtx, pk2, txfee) > 0LL);
    mtx.vin.push_back(CTxIn(txask.GetHash(), askvout, CScript()));  // spend order tx

    // vout.0 tokens remainder to unspendable cc addr:
    mtx.vout.push_back(TokensV2::MakeTokensCC1vout(AssetsV2::EvalCode(), evalcodeNFT, orig_assetoshis - fillunits, GetUnspendable(cpAssets, NULL)));  // token remainder on cc global addr

    //vout.1 purchased tokens to self token single-eval or dual-eval token+nonfungible cc addr:
    mtx.vout.push_back(TokensV2::MakeTokensCC1vout(evalcodeNFT ? evalcodeNFT : TokensV2::EvalCode(), fillunits, pk2));					
    mtx.vout.push_back(CTxOut(paid_nValue - royaltyValue, CScript() << origpubkey << OP_CHECKSIG));		//vout.2 coins to ask originator's normal addr
    if (royaltyFract > 0)       // note it makes the vout even if roaltyValue is 0
        mtx.vout.push_back(CTxOut(royaltyValue, CScript() << ownerpubkey << OP_CHECKSIG));	// vout.3 royalty to token owner

    if (orig_assetoshis - fillunits > 0) // we dont need the marker if order is filled
        mtx.vout.push_back(TokensV2::MakeCC1vout(AssetsV2::EvalCode(), ASSETS_MARKER_AMOUNT, origpubkey));    //vout.3(4 if royalty) marker to origpubkey (for my tokenorders?)

    uint8_t unspendableAssetsPrivkey[32];
    CPubKey unspendableAssetsPk = GetUnspendable(cpAssets, unspendableAssetsPrivkey);

    CCwrapper wrCond(TokensV2::MakeTokensCCcond1(AssetsV2::EvalCode(), evalcodeNFT, unspendableAssetsPk));
    CCAddVintxCond(cpAssets, wrCond, unspendableAssetsPrivkey);

    ASSERT_TRUE(TestFinalizeTx(mtx, cpAssets, testKeys[pk2], txfee,
        TokensV2::EncodeTokenOpRet(tokenid1, { pk2 }, 
            { AssetsV2::EncodeAssetOpRet('S', zeroid, unit_price, origpubkey) } )));   

    EXPECT_TRUE(TestRunCCEval(mtx));
}


TEST_F(TestAssetsCC, tokenv2fillbid)
{
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	struct CCcontract_info *cpAssets, C; 
	struct CCcontract_info *cpTokens, tokensC;

	const int32_t bidvout = ASSETS_GLOBALADDR_VOUT; 
    CAmount paid_amount;
    CAmount bid_amount;
    CAmount orig_units;
    CAmount unit_price = 0;
    CAmount fill_units = 2;
    CAmount txfee = 10000;
    uint256 assetidOpret;
    CAmount paid_unit_price = 0;  // not set

    cpAssets = CCinit(&C, AssetsV2::EvalCode());   
    cpTokens = CCinit(&tokensC, TokensV2::EvalCode());  

    TokenDataTuple tokenData;
    vuint8_t vopretNonfungible;
    uint8_t evalcodeNFT = 0;
    uint64_t royaltyFract = 0;  // royaltyFract is N in N/1000 fraction
    ASSERT_TRUE( GetTokenData<TokensV2>(&eval, tokenid1, tokenData, vopretNonfungible) );
    if (vopretNonfungible.size() > 0)  {
        evalcodeNFT = vopretNonfungible.begin()[0];
        GetNftDataAsUint64(vopretNonfungible, NFTPROP_ROYALTY, royaltyFract);
        if (royaltyFract > NFTROYALTY_DIVISOR-1)
            royaltyFract = NFTROYALTY_DIVISOR-1; // royalty upper limit
    }
    vuint8_t ownerpubkey = std::get<0>(tokenData);

    vuint8_t origpubkey;
    bid_amount = txbid.vout[bidvout].nValue;
    ASSERT_TRUE(GetOrderParams<AssetsV2>(origpubkey, unit_price, assetidOpret, txbid) != 0); // get orig pk, orig value
    ASSERT_TRUE(unit_price != 0);

    orig_units = bid_amount / unit_price;

    if (paid_unit_price <= 0LL)
        paid_unit_price = unit_price;

    ASSERT_TRUE( TestAddNormalInputs(mtx, pk2, txfee) > 0LL);
    mtx.vin.push_back(CTxIn(txtokencreate2.GetHash(), 1, CScript()));  // spend token tx
    mtx.vin.push_back(CTxIn(txbid.GetHash(), bidvout, CScript()));  // spend order tx

    //CAmount inputs = txnormal2.vout[0].nValue;
    CAmount tokenInputs = txtokencreate2.vout[1].nValue;

    ASSERT_TRUE(SetBidFillamounts(unit_price, paid_amount, bid_amount, fill_units, orig_units, paid_unit_price));

    CAmount royaltyValue = royaltyFract > 0 ? paid_amount / NFTROYALTY_DIVISOR * royaltyFract : 0;
    CAmount tokensChange = tokenInputs - fill_units;

    uint8_t unspendableAssetsPrivkey[32];
    CPubKey unspendableAssetsPk = GetUnspendable(cpAssets, unspendableAssetsPrivkey);

    if (orig_units - fill_units > 0 || bid_amount - paid_amount <= ASSETS_NORMAL_DUST) { // bidder has coins for more tokens or only dust is sent back to global address
        mtx.vout.push_back(TokensV2::MakeCC1vout(AssetsV2::EvalCode(), bid_amount - paid_amount, unspendableAssetsPk));     // vout0 coins remainder or the dust is sent back to cc global addr
        if (bid_amount - paid_amount <= ASSETS_NORMAL_DUST)
            std::cerr << __func__ << " dust detected (bid_amount - paid_amount)=" << (bid_amount - paid_amount) << std::endl;
    }
    else
        mtx.vout.push_back(CTxOut(bid_amount - paid_amount, CScript() << ParseHex(HexStr(origpubkey)) << OP_CHECKSIG));     // vout0 if no more tokens to buy, send the remainder to originator
    mtx.vout.push_back(CTxOut(paid_amount - royaltyValue, CScript() << ParseHex(HexStr(pk2)) << OP_CHECKSIG));	// vout1 coins to mypk normal 
    if (royaltyFract > 0)   // note it makes vout even if roaltyValue is 0
        mtx.vout.push_back(CTxOut(royaltyValue, CScript() << ParseHex(HexStr(ownerpubkey)) << OP_CHECKSIG));  // vout2 trade royalty to token owner
    mtx.vout.push_back(TokensV2::MakeTokensCC1vout(evalcodeNFT ? evalcodeNFT : TokensV2::EvalCode(), fill_units, pubkey2pk(origpubkey)));	  // vout2(3) single-eval tokens sent to the originator
    if (orig_units - fill_units > 0)  // order is not finished yet
        mtx.vout.push_back(TokensV2::MakeCC1vout(AssetsV2::EvalCode(), ASSETS_MARKER_AMOUNT, origpubkey));                    // vout3(4 if royalty) marker to origpubkey

    if (tokensChange != 0LL)
        mtx.vout.push_back(TokensV2::MakeTokensCC1vout(evalcodeNFT ? evalcodeNFT : TokensV2::EvalCode(), tokensChange, pk2));  // change in single-eval tokens

    CMutableTransaction mtx2(mtx);  // copy

    CCwrapper wrCond1(MakeCCcond1(AssetsV2::EvalCode(), unspendableAssetsPk));  // spend coins
    CCAddVintxCond(cpTokens, wrCond1, unspendableAssetsPrivkey);
    
    CCwrapper wrCond2(TokensV2::MakeTokensCCcond1(evalcodeNFT, pk2));  // spend my tokens to fill buy
    CCAddVintxCond(cpTokens, wrCond2, NULL); //NULL indicates to use myprivkey

    ASSERT_TRUE(TestFinalizeTx(mtx, cpTokens, testKeys[pk2], txfee,
        TokensV2::EncodeTokenOpRet(tokenid2, { pubkey2pk(origpubkey) },
            { AssetsV2::EncodeAssetOpRet('B', zeroid, unit_price, origpubkey) })));    
    EXPECT_TRUE(TestRunCCEval(mtx));

    // spend bad tokenid 
    mtx2.vin[1] = CTxIn(txtokencreate3.GetHash(), 1, CScript());  // spend bad tokenid3
    ASSERT_TRUE(TestFinalizeTx(mtx2, cpTokens, testKeys[pk2], txfee,
        TokensV2::EncodeTokenOpRet(tokenid3, { pubkey2pk(origpubkey) },
            { AssetsV2::EncodeAssetOpRet('B', zeroid, unit_price, origpubkey) }))); 

    EXPECT_FALSE(TestRunCCEval(mtx2));


}

TEST_F(TestAssetsCC, tokenv2cancelask)
{
	struct CCcontract_info *cpAssets, C; 
	struct CCcontract_info *cpTokens, tokensC;

    CAmount txfee = 10000;

    cpAssets = CCinit(&C, AssetsV2::EvalCode());   
    cpTokens = CCinit(&tokensC, TokensV2::EvalCode());  

    for (CTransaction vintx : std::vector<CTransaction>{ txask })  // TODO add more txasks
    {
        CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());

        uint256 asktxid = vintx.GetHash();
        CAmount askamount = vintx.vout[ASSETS_GLOBALADDR_VOUT].nValue;

        TokenDataTuple tokenData;
        vuint8_t vopretNonfungible;
        ASSERT_TRUE( GetTokenData<TokensV2>(&eval, tokenid1, tokenData, vopretNonfungible) );
        uint8_t evalcodeNFT = 0;
        if (vopretNonfungible.size() > 0)
            evalcodeNFT = vopretNonfungible[0];
        vuint8_t ownerpubkey = std::get<0>(tokenData);

        ASSERT_TRUE( TestAddNormalInputs(mtx, ownerpubkey, txfee) > 0LL);
        mtx.vin.push_back(CTxIn(asktxid, ASSETS_GLOBALADDR_VOUT, CScript()));
        
        uint8_t dummyEvalCode; 
        uint256 dummyAssetid, dummyAssetid2; 
        int64_t dummyPrice; 
        std::vector<uint8_t> dummyOrigpubkey;
        uint8_t funcid = AssetsV2::DecodeAssetTokenOpRet(vintx.vout.back().scriptPubKey, dummyEvalCode, dummyAssetid, dummyAssetid2, dummyPrice, dummyOrigpubkey);
        if (funcid == 's' && vintx.vout.size() > 1)
            mtx.vin.push_back(CTxIn(asktxid, 1, CScript()));		// spend marker if funcid='s'
        else if (funcid == 'S' && vintx.vout.size() > 3)
            mtx.vin.push_back(CTxIn(asktxid, 3, CScript()));		// spend marker if funcid='S'
        else {
            std::cerr << __func__ << "invalid ask tx" << std::endl;
            ASSERT_FALSE(true);
        }

        mtx.vout.push_back(TokensV2::MakeTokensCC1vout(cpAssets->evalcodeNFT ? cpAssets->evalcodeNFT : TokensV2::EvalCode(), askamount, ownerpubkey));	// one-eval token vout

        // init assets 'unspendable' privkey and pubkey
        uint8_t unspendableAssetsPrivkey[32];
        CPubKey unspendableAssetsPk = GetUnspendable(cpAssets, unspendableAssetsPrivkey);

        CCwrapper wrCond(TokensV2::MakeTokensCCcond1(AssetsV2::EvalCode(), cpAssets->evalcodeNFT, unspendableAssetsPk));
        CCAddVintxCond(cpAssets, wrCond, unspendableAssetsPrivkey);

        ASSERT_TRUE(TestFinalizeTx(mtx, cpAssets, testKeys[ownerpubkey], txfee,
            TokensV2::EncodeTokenOpRet(tokenid1, { ownerpubkey },
                { AssetsV2::EncodeAssetOpRet('x', zeroid, 0, vuint8_t(ownerpubkey.begin(), ownerpubkey.end())) } )));

        EXPECT_TRUE(TestRunCCEval(mtx));

        // invalid pk in assets opreturn
        CMutableTransaction mtx2(mtx);
        mtx2.vout.pop_back();
        mtx2.vout.push_back(CTxOut(0, TokensV2::EncodeTokenOpRet(tokenid1, { ownerpubkey },
                                { AssetsV2::EncodeAssetOpRet('x', zeroid, 0, vuint8_t(pk2.begin(), pk2.end())) } )));
        // EXPECT_FALSE(TestRunCCEval(mtx2)); // is not checked

        // invalid pk in token opret
        CMutableTransaction mtx3(mtx);
        mtx3.vout.pop_back();
        mtx3.vout.push_back(CTxOut(0, TokensV2::EncodeTokenOpRet(tokenid1, { pkunused },
                                { AssetsV2::EncodeAssetOpRet('x', zeroid, 0, vuint8_t(ownerpubkey.begin(), ownerpubkey.end())) } )));
        // EXPECT_FALSE(TestRunCCEval(mtx3)); // is not checked

        // invalid pk where to funds sent
        CMutableTransaction mtx4(mtx);
        mtx4.vout[0] = TokensV2::MakeTokensCC1vout(cpAssets->evalcodeNFT ? cpAssets->evalcodeNFT : TokensV2::EvalCode(), askamount, pk2);
        EXPECT_FALSE(TestRunCCEval(mtx4)); // is not checked

        // invalid tokenid in token opret
        CMutableTransaction mtx5(mtx);
        mtx5.vout.pop_back();
        mtx5.vout.push_back(CTxOut(0, TokensV2::EncodeTokenOpRet(tokenidUnused, { ownerpubkey },
                                { AssetsV2::EncodeAssetOpRet('x', zeroid, 0, vuint8_t(ownerpubkey.begin(), ownerpubkey.end())) } )));
        EXPECT_FALSE(TestRunCCEval(mtx5)); // is not checked

        // opposite funcid in token opret
        CMutableTransaction mtx6(mtx);
        mtx6.vout.pop_back();
        mtx6.vout.push_back(CTxOut(0, TokensV2::EncodeTokenOpRet(tokenid1, { ownerpubkey },
                                { AssetsV2::EncodeAssetOpRet('o', zeroid, 0, vuint8_t(ownerpubkey.begin(), ownerpubkey.end())) } )));
        EXPECT_FALSE(TestRunCCEval(mtx6)); // is not checked
    }
}

TEST_F(TestAssetsCC, tokenv2cancelbid)
{
	struct CCcontract_info *cpAssets, C; 
	struct CCcontract_info *cpTokens, tokensC;
    CAmount txfee = 10000;

    cpAssets = CCinit(&C, AssetsV2::EvalCode());   
    cpTokens = CCinit(&tokensC, TokensV2::EvalCode());  

    for (CTransaction vintx : std::vector<CTransaction>{ txbid })  // TODO add more txasks
    {
        CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
        uint256 bidtxid = vintx.GetHash();
        CAmount bidamount = vintx.vout[ASSETS_GLOBALADDR_VOUT].nValue;

        TokenDataTuple tokenData;
        vuint8_t vopretNonfungible;
        ASSERT_TRUE( GetTokenData<TokensV2>(&eval, tokenid2, tokenData, vopretNonfungible) );
        uint8_t evalcodeNFT = 0;
        if (vopretNonfungible.size() > 0)
            evalcodeNFT = vopretNonfungible[0];
        vuint8_t ownerpubkey = std::get<0>(tokenData);

        ASSERT_TRUE( TestAddNormalInputs(mtx, ownerpubkey, txfee) > 0LL);
        mtx.vin.push_back(CTxIn(bidtxid, ASSETS_GLOBALADDR_VOUT, CScript()));
        
        uint8_t dummyEvalCode; 
        uint256 dummyAssetid, dummyAssetid2; 
        int64_t dummyPrice; 
        std::vector<uint8_t> dummyOrigpubkey;
        uint8_t funcid = AssetsV2::DecodeAssetTokenOpRet(vintx.vout.back().scriptPubKey, dummyEvalCode, dummyAssetid, dummyAssetid2, dummyPrice, dummyOrigpubkey);
        if (funcid == 'b' && vintx.vout.size() > 1)
            mtx.vin.push_back(CTxIn(bidtxid, 1, CScript()));		// spend marker if funcid='b'
        else if (funcid == 'B' && vintx.vout.size() > 3)
            mtx.vin.push_back(CTxIn(bidtxid, 3, CScript()));		// spend marker if funcid='B'
        else {
            std::cerr << __func__ << "invalid bid tx" << std::endl;
            ASSERT_FALSE(true);
        }

        if (bidamount > ASSETS_NORMAL_DUST)  
            mtx.vout.push_back(CTxOut(bidamount, CScript() << ParseHex(HexStr(ownerpubkey)) << OP_CHECKSIG));
        else {
            // send dust back to global addr
            mtx.vout.push_back(TokensV2::MakeCC1vout(AssetsV2::EvalCode(), bidamount, GetUnspendable(cpAssets, NULL)));
            std::cerr << __func__ << " dust detected bidamount=" << bidamount << std::endl;
        }

        // init assets 'unspendable' privkey and pubkey
        uint8_t unspendableAssetsPrivkey[32];
        CPubKey unspendableAssetsPk = GetUnspendable(cpAssets, unspendableAssetsPrivkey);

        // sign, add change, add opreturn
        ASSERT_TRUE(TestFinalizeTx(mtx, cpAssets, testKeys[ownerpubkey], txfee,
                            TokensV2::EncodeTokenOpRet(tokenid2, { ownerpubkey },
                                { AssetsV2::EncodeAssetOpRet('o', zeroid, 0, vuint8_t(ownerpubkey.begin(), ownerpubkey.end())) } )));

        EXPECT_TRUE(TestRunCCEval(mtx));

        // invalid pk in assets opreturn
        CMutableTransaction mtx2(mtx);
        mtx2.vout.pop_back();
        mtx2.vout.push_back(CTxOut(0, TokensV2::EncodeTokenOpRet(tokenid2, { ownerpubkey },
                                { AssetsV2::EncodeAssetOpRet('o', zeroid, 0, vuint8_t(pk1.begin(), pk1.end())) } )));
        // EXPECT_FALSE(TestRunCCEval(mtx2)); // is not checked

        // invalid pk in token opret
        CMutableTransaction mtx3(mtx);
        mtx3.vout.pop_back();
        mtx3.vout.push_back(CTxOut(0, TokensV2::EncodeTokenOpRet(tokenid2, { pkunused },
                                { AssetsV2::EncodeAssetOpRet('o', zeroid, 0, vuint8_t(ownerpubkey.begin(), ownerpubkey.end())) } )));
        // EXPECT_FALSE(TestRunCCEval(mtx3)); // is not checked

        // invalid pk where to funds sent
        CMutableTransaction mtx4(mtx);
        mtx4.vout[0] = CTxOut(bidamount, CScript() << ParseHex(HexStr(pkunused)) << OP_CHECKSIG);
        EXPECT_FALSE(TestRunCCEval(mtx4)); // is not checked

        // invalid tokenid in token opret
        CMutableTransaction mtx5(mtx);
        mtx5.vout.pop_back();
        mtx5.vout.push_back(CTxOut(0, TokensV2::EncodeTokenOpRet(tokenidUnused, { ownerpubkey },
                                { AssetsV2::EncodeAssetOpRet('o', zeroid, 0, vuint8_t(ownerpubkey.begin(), ownerpubkey.end())) } )));
        EXPECT_FALSE(TestRunCCEval(mtx5)); 

        // opposite funcid in token opret
        CMutableTransaction mtx6(mtx);
        mtx6.vout.pop_back();
        mtx6.vout.push_back(CTxOut(0, TokensV2::EncodeTokenOpRet(tokenid2, { pk2 },
                                { AssetsV2::EncodeAssetOpRet('x', zeroid, 0, vuint8_t(ownerpubkey.begin(), ownerpubkey.end())) } )));
        EXPECT_FALSE(TestRunCCEval(mtx6)); 
    }
}


/* ---------------------------------------------------------------------------------------------------------------------------------- */
// tokens tests:

TEST_F(TestAssetsCC, tokenv2create)
{
    struct CCcontract_info *cp, C;
    cp = CCinit(&C, TokensV2::EvalCode());
    std::string name = "T1";
    std::string description = "desc";
    CAmount txfee = 10000;

    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
    ASSERT_TRUE( TestAddNormalInputs(mtx, pk1, txfee) > 0LL);
    mtx.vout.push_back(TokensV2::MakeCC1vout(TokensV2::EvalCode(), TOKENS_MARKER_VALUE, GetUnspendable(cp, NULL)));           
    mtx.vout.push_back(TokensV2::MakeTokensCC1vout(0, 10, pk1));

    // copies:
    CMutableTransaction mtx1(mtx);
    CMutableTransaction mtx2(mtx);
    //CMutableTransaction mtx3(mtx);

    // sign vins:
    // normal vin:
    /*CBasicKeyStore tempKeystore;
    CKey key;
    key.Set(privkey1, privkey1+sizeof(privkey1), true);
    tempKeystore.AddKey(key);
    int32_t ivin = 0;
    ASSERT_TRUE(TestSignTx(tempKeystore, mtx, ivin, txnormal2.vout[0].nValue, txnormal2.vout[0].scriptPubKey));*/

    // sign, add change, add opreturn
    ASSERT_TRUE(TestFinalizeTx(mtx, cp, testKeys[pk1], txfee,
                        TokensV2::EncodeTokenCreateOpRet(vscript_t(pk1.begin(), pk1.end()), name, description, {  })));

    EXPECT_TRUE(TestRunCCEval(mtx));

    // token sent to another pk
    mtx1.vout[1] = TokensV2::MakeTokensCC1vout(0, 10, pk2); 
    ASSERT_TRUE(TestFinalizeTx(mtx1, cp, testKeys[pk1], txfee,
                        TokensV2::EncodeTokenCreateOpRet(vscript_t(pk1.begin(), pk1.end()), name, description, {  })));
    EXPECT_TRUE(TestRunCCEval(mtx1));  // allow sent created tokens to any pk!!

    // invalid pk in opreturn:
    ASSERT_TRUE(TestFinalizeTx(mtx2, cp, testKeys[pk1], txfee,
                        TokensV2::EncodeTokenCreateOpRet(vscript_t(pk2.begin(), pk2.end()), name, description, {  })));
    EXPECT_FALSE(TestRunCCEval(mtx2)); // must fail

    // create token with global pk:
    CMutableTransaction mtx3 = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	
    struct CCcontract_info *cpTokens, tokensC;  
    cpTokens = CCinit(&tokensC, TokensV2::EvalCode());
    uint8_t privkeyg[32];
    CPubKey pkg = GetUnspendable(cpTokens, privkeyg);

    mtx3.vin.push_back(CTxIn(txnormalg.GetHash(), 0));
    mtx3.vout.push_back(TokensV2::MakeCC1vout(TokensV2::EvalCode(), TOKENS_MARKER_VALUE, GetUnspendable(cp, NULL)));           
    mtx3.vout.push_back(TokensV2::MakeTokensCC1vout(0, 10, pk1));

    ASSERT_TRUE(TestFinalizeTx(mtx3, cp, privkeyg, txfee,
                        TokensV2::EncodeTokenCreateOpRet(vscript_t(pkg.begin(), pkg.end()), name, description, {  })));
    EXPECT_FALSE(TestRunCCEval(mtx3));  // must fail

}

} /* namespace CCAssetsTests */
