#include <gtest/gtest.h>
#include "utilstrencodings.h"
#include "base58.h"
#include <univalue.h>
#include "komodo_utils.h"
#include "komodo_hardfork.h"
#include "txdb.h"

#include <iostream>
#include <vector>
#include <utility>
#include <string>
#include <cstring>
#include <memory>

bool FindBlockPos(int32_t tmpflag,CValidationState &state, CDiskBlockPos &pos, unsigned int nAddSize, unsigned int nHeight, uint64_t nTime, bool fKnown); // main.cpp
uint64_t RewardsCalc(int64_t amount, uint256 txid, int64_t APR, int64_t minseconds, int64_t maxseconds, uint32_t timestamp); // rewards cpp
bool BnFitsCAmount(arith_uint256 const &au_num); // payments.cpp
std::string BnToString(const arith_uint256 &au_num);

namespace GMPArithTests
{

    /*
        We are currently utilizing mini-gmp.c in our project specifically for bitcoin_base58decode / bitcoin_base58encode
        in a few areas, such as bitcoin_address (found in komodo_utils.cpp) or NSPV-related functions in
        komodo_nSPV_superlite.h or komodo_nSPV_wallet.h. However, our codebase already has other methods to compute
        base58 related tasks, such as EncodeBase58 / DecodeBase58 in base58.h. This means we could easily eliminate
        the base58 implementation provided by mini-gmp.h. However, we need to complete a few tests first.

    */

    std::vector<std::pair<std::string, std::string>> vBase58EncodeDecode = {
        {"", ""},
        {"61", "2g"},
        {"626262", "a3gV"},
        {"636363", "aPEr"},
        {"73696d706c792061206c6f6e6720737472696e67", "2cFupjhnEsSn59qHXstmK2ffpLv2"},
        {"00eb15231dfceb60925886b67d065299925915aeb172c06647", "1NS17iag9jJgTHD1VXjvLCEnZuQ3rJDE9L"},
        {"516b6fcd0f", "ABnLTmg"},
        {"bf4f89001e670274dd", "3SEo3LWLoPntC"},
        {"572e4794", "3EFU7m"},
        {"ecac89cad93923c02321", "EJDM8drfXA6uyA"},
        {"10c8511e", "Rt5zm"},
        {"00000000000000000000", "1111111111"},
        {"3cf1dce4182fce875748c4986b240ff7d7bc3fffb02fd6b4d5", "RXL3YXG2ceaB6C5hfJcN4fvmLH2C34knhA"},
        {"3c19494a36d54f01d680aa3c9b390f522a2a3dff7e2836626b", "RBatmanSuperManPaddingtonBearpcCTt"},
        {"3c531cca970048c1564bfdf8320729381d7755d481522e4402", "RGreetzToCA333FromDecker33332cYmMb"},
        {"22d73e8f", "test2"},
        {"bc907ece717a8f94e07de7bf6f8b3e9f91abb8858ebf831072cdbb9016ef53bc5d01bf0891e2", "UtrRXqvRFUAtCrCTRAHPH6yroQKUrrTJRmxt2h5U4QTUN1jCxTAh"},
        {"bc907ece717a8f94e07de7bf6f8b3e9f91abb8858ebf831072cdbb9016ef53bc5de225fbd6", "7KYb75jv5BgrDCbmW36yhofiBy2vSLpCCWDfJ9dMdZxPWnKicJh"},
    };

    TEST(GMPArithTests, base58operations)
    {

        for (const auto &element : vBase58EncodeDecode)
        {

            assert(element.first.size() % 2 == 0);

            // encode test
            size_t encoded_length = element.second.size() + 1;
            std::vector<char> encodedStr(encoded_length, 0);

            std::vector<unsigned char> sourceData = ParseHex(element.first);
            const char *p_encoded = bitcoin_base58encode(encodedStr.data(), sourceData.data(), sourceData.size());

            ASSERT_STREQ(p_encoded, element.second.c_str());

            p_encoded = nullptr;

            // decode test
            size_t decoded_length = element.second.size();
            /*  Here we assume that the decoded data is not larger than the encoded data.
                Typically, the decoded data is around 73% of the encoded data, calculated as
                <size_t>(element.second.size() * 0.732 ) + 1. However, if there are many
                leading '1's, each of them will decode into a single zero byte. In this case,
                the decoded data will be 100% of the encoded data.

                https://stackoverflow.com/questions/48333136/size-of-buffer-to-hold-base58-encoded-data
            */
            std::vector<uint8_t> decodedStr(decoded_length + 1, 0); // +1 to handle the case when decoded_length == 0, i.e. decodedStr.data() == nullptr (A)
            int32_t data_size = bitcoin_base58decode(decodedStr.data(), element.second.c_str()); // *(char (*)[10])decodedStr.get(),h
            // std::cerr << "'" << element.second << "': " << data_size << std::endl;

            ASSERT_GE(decoded_length, sourceData.size());
            ASSERT_EQ(data_size, sourceData.size());
            ASSERT_TRUE(std::memcmp(decodedStr.data(), sourceData.data(), sourceData.size()) == 0);
        }

        // special cases test (decode)
        std::vector<std::pair<std::string, int32_t>> decodeTestCases = {
            {"", 0},          // empty data to decode
            {"ERROR", -1},    // capital "O" is not a part of base58 alphabet
            {"test2", 4},
            {"RGreetzToCA333FromDecker33332cYmMb", 25},
            {"  test2", 4},   // spaces behind
            {"test2  ", 4},   // spaces after
            {"  test2  ", 4}, // spaces behind and after
            {"te st2", -1},   // spaces in-between
        };
        for (auto const &test : decodeTestCases)
        {
            std::vector<uint8_t> decodedStr(test.first.length() + 1, 0); // +1 for the same as in (A)
            int32_t data_size = bitcoin_base58decode(decodedStr.data(), test.first.c_str());
            // std::cerr << "'" << test.first << "': " << data_size << " - " << HexStr(decodedStr) << std::endl;
            ASSERT_EQ(data_size, test.second);
        }

        // special cases test (encode)
        char buf[] = { '\xff' };
        uint8_t data[] = {0xDE, 0xAD, 0xCA, 0xFE};
        const char *p_encoded = bitcoin_base58encode(&buf[0], data, 0);
        ASSERT_EQ(buf[0], '\0');
    }

    /* In addition to libsnark (./src/snark/libsnark/algebra/fields/bigint.hpp),
       it appears that we are also using functions from gmp.h in rewards.cpp and
       payments.cpp. The purpose of these tests is to perform the same computations
       without using functions from the GMP library. Once the new code without
       GMP function usage is written, the old code will be removed.
    */

    struct TestRewardsParams {
        int64_t amount;
        int64_t duration; // duration is actually limited by uint32_t as nTime is 32 bit (!)
        int64_t APR;
        uint64_t rewards;
    };

    CTransaction CreateCoinBaseTransaction(int nBlockHeight) {
        // Create coinbase transaction
        auto consensusParams = Params().GetConsensus();
        CMutableTransaction mtx = CreateNewContextualCMutableTransaction(consensusParams, nBlockHeight);
        CScript scriptSig = (CScript() << nBlockHeight << CScriptNum(1)) + COINBASE_FLAGS;
        mtx.vin.push_back(CTxIn(COutPoint(), scriptSig, 0));
        mtx.vout.push_back(CTxOut(300000000, CScript() << ParseHex("031111111111111111111111111111111111111111111111111111111111111111") << OP_CHECKSIG));
        CTransaction tx(mtx);
        return tx;
    }

    // Helper function to delete and set to nullptr if not nullptr
    template<typename T>
    void deleteIfUsedBefore(T*& ptr) {
        if (ptr != nullptr) {
            delete ptr;
            ptr = nullptr;
        }
    }

    TEST(GMPArithTests, RewardsTest)
    {

        bool fPrintToConsoleOld = fPrintToConsole;
        assetchain assetchainOld = chainName; // save the chainName before test
        chainName = assetchain("OTHER");      // considering this is not KMD
        const CChainParams& chainparams = Params();
        fPrintToConsole = true;
        uint256 txid;

        // reward = (((amount * duration) / (365 * 24 * 3600LL)) * (APR / 1000000)) / 10000;
        std::vector<TestRewardsParams> testCasesBeforeHF = {
            {-31536, -10000000, 1000000, 1},
            {-31536, -10000000, 2000000, 2},
            {std::numeric_limits<int64_t>::min(), -2, 1000000, 0},            // numeric overflow
            {std::numeric_limits<int64_t>::min(), -315360000000, 1000000, 0}, // overflow on multiply and on entire result ... 9 223 372 036 854 775 808 > INT64_MAX
        };

        // reward = (amount * APR * duration) / COIN*100*365*24*3600;

        // NB! can't use negative amounts in these tests, bcz -1 will be converted to 18446744073709551615 (2^64 -1) after mpz_set_lli
        std::vector<TestRewardsParams> testCasesAfterHF = {
            {1, 1, 315360000000000000, 1},
            {2, 2, 315360000000000000, 2},
            {777 * COIN, 86400, 315360000000000000, 777 * COIN},
            {std::numeric_limits<int64_t>::max(), 1, 1, 29},
            {std::numeric_limits<int64_t>::max(), 2, 1, 58},
            {std::numeric_limits<int64_t>::max(), 777, 777, 17657335},
            {std::numeric_limits<int64_t>::max(), 1, std::numeric_limits<uint32_t>::max(), 125615427599},
            // test below will cause crash on in mpz_get_ui2, as reward will be 269 757 076 770 150 354 665, which
            // is more than can fit in int64_t (!), so mpz_get_ui2 should be modified
            // {std::numeric_limits<int64_t>::max(), 1, std::numeric_limits<int64_t>::max(), 0},
        };

        for (const TestRewardsParams &testCase : testCasesBeforeHF)
        {
            // myGetTransaction inside CCduration will return 0, so duration will be equal maxseconds
            uint64_t rewards = RewardsCalc(testCase.amount, txid, testCase.APR, 0 /* minseconds */, testCase.duration /* maxseconds */, nStakedDecemberHardforkTimestamp);
            ASSERT_EQ(rewards, testCase.rewards);
        }

        // here we should force myGetTransaction inside CCduration to find given
        // txid via fTxIndex, i.e. create tx index, blockfile with given txid in /tmp first.
        UnloadBlockIndex();
        ClearDatadirCache();
        boost::filesystem::path temp = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();

        bool fRewardTestFailure = false;
        std::string RewardTestErrorMessage = "";

        if (boost::filesystem::create_directories(temp)) {
            std::string datadirOld;
            if (mapArgs.find("-datadir") != mapArgs.end()) {
                datadirOld = mapArgs["-datadir"];
            }
            mapArgs["-datadir"] = temp.string();
            // std::cerr << "Data directory: " << temp.string() << std::endl;

            // previous tests may accidentally leave behind pblocktree and pcoinsTip,
            // so we need to ensure that the memory is free and no remnants are left.
            deleteIfUsedBefore(pblocktree);
            deleteIfUsedBefore(pcoinsTip);

            pblocktree = new CBlockTreeDB(1 << 20, true); // see testutils.cpp
            CCoinsViewDB *pcoinsdbview = new CCoinsViewDB(1 << 23, true);
            pcoinsTip = new CCoinsViewCache(pcoinsdbview);

            if (InitBlockIndex() && fTxIndex)
            {

                CBlockIndex *pindex = chainActive.Tip(); // genesis block
                if (pindex)
                {
                    // Compose a new block
                    CBlock b;
                    int nHeight = pindex->nHeight + 1;
                    b.vtx.push_back(CreateCoinBaseTransaction(nHeight));
                    CBlockIndex indexDummy(b);
                    indexDummy.nHeight = nHeight;
                    indexDummy.pprev = pindex;
                    txid = b.vtx[0].GetHash();

                    for (const TestRewardsParams &testCase : testCasesAfterHF)
                    {
                        b.nTime = pindex->nTime + testCase.duration;
                        indexDummy.nTime = b.nTime;
                        mapBlockIndex.insert(std::make_pair(b.GetHash(), &indexDummy));

                        CDiskBlockPos blockPos;
                        CValidationState state;

                        unsigned int nBlockSize = ::GetSerializeSize(b, SER_DISK, CLIENT_VERSION);
                        // here every test iteration new block will written on new place in a file, but for tests purposes it's ok
                        if (!FindBlockPos(0, state, blockPos, nBlockSize + 8, indexDummy.nHeight, b.GetBlockTime(), false))
                        {
                            fRewardTestFailure = true;
                            RewardTestErrorMessage = "FindBlockPos failed!";
                        }
                        if (!WriteBlockToDisk(b, blockPos, chainparams.MessageStart()))
                        {
                            fRewardTestFailure = true;
                            RewardTestErrorMessage = "WriteBlockToDisk failed!";
                        }
                        if (fRewardTestFailure)
                        {
                            mapBlockIndex.erase(b.GetHash());
                            break;
                        }

                        indexDummy.nFile = blockPos.nFile;
                        indexDummy.nDataPos = blockPos.nPos;
                        indexDummy.nStatus |= BLOCK_HAVE_DATA;

                        std::vector<std::pair<uint256, CDiskTxPos>> vPos;
                        CDiskBlockPos dbp = indexDummy.GetBlockPos();
                        CDiskTxPos pos(dbp, /* nTxOffsetIn */ GetSizeOfCompactSize(b.vtx.size()));
                        vPos.push_back(std::make_pair(txid, pos));

                        if (pblocktree->WriteTxIndex(vPos))
                        {
                            chainActive.SetTip(&indexDummy);

                            CBlock new_block;
                            CBlockIndex indexDummy2(new_block);
                            indexDummy2.nHeight = indexDummy.nHeight + 1;
                            indexDummy2.nTime = indexDummy.nTime + testCase.duration;
                            chainActive.SetTip(&indexDummy2);

                            uint64_t rewards = RewardsCalc(testCase.amount, txid, testCase.APR, 0 /* minseconds */, testCase.duration /* maxseconds */, nStakedDecemberHardforkTimestamp + 1);
                            ASSERT_EQ(rewards, testCase.rewards);
                        }
                        else
                        {
                            fRewardTestFailure = true;
                            RewardTestErrorMessage = "WriteTxIndex failed!";
                        }

                        mapBlockIndex.erase(b.GetHash());
                    }
                }
                else
                {
                    fRewardTestFailure = true;
                    RewardTestErrorMessage = "pindex on Genesis block failed!";
                }
            }
            else
            {
                fRewardTestFailure = true;
                RewardTestErrorMessage = "Can't init block index or tx index is not enabled!";
            }

            deleteIfUsedBefore(pcoinsTip);
            deleteIfUsedBefore(pcoinsdbview);
            deleteIfUsedBefore(pblocktree);

            if (!datadirOld.empty()) {
                mapArgs["-datadir"] = datadirOld;
            }

        } else {
            fRewardTestFailure = true; RewardTestErrorMessage = "Can't create data directories";
        }
        

        boost::filesystem::remove_all(temp);

        fPrintToConsole = fPrintToConsoleOld;
        chainName = assetchainOld; // restore saved values

        if (fRewardTestFailure) {
            FAIL() << RewardTestErrorMessage;
        }
    }

/*     bool BnFitsCAmount(const mpz_t &bn_mpz)
    {
        const int64_t max = std::numeric_limits<int64_t>::max();
        mpz_t bn_max;
        mpz_init(bn_max);
        mpz_import(bn_max, 1, 1, sizeof(max), 0, 0, &max);
        if (mpz_cmp(bn_mpz, bn_max) < 1)
        {
            mpz_clear(bn_max);
            return true;
        }
        mpz_clear(bn_max);
        return false;
    }

    std::string BnToString(const mpz_t &bn_mpz)
    {
        size_t num_limbs = mpz_sizeinbase(bn_mpz, 10);
        std::unique_ptr<char[]> buffer(new char[num_limbs + 2]); // possible sign and '\0'
        mpz_get_str(buffer.get(), 10, bn_mpz);
        std::string result(buffer.get());
        return result;
    }
 */
    TEST(GMPArithTests, PaymentsTest)
    {
        /* BnFitsCAmount */
        const int64_t int64_t_max = std::numeric_limits<int64_t>::max();
        arith_uint256 au(int64_t_max);
        ASSERT_TRUE(BnFitsCAmount(au));
        au += 1;
        ASSERT_FALSE(BnFitsCAmount(au));
    }
}