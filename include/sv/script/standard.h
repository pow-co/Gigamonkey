// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2019 Bitcoin Association
// Distributed under the Open BSV software license, see the accompanying file LICENSE.

#ifndef BITCOIN_SCRIPT_STANDARD_H
#define BITCOIN_SCRIPT_STANDARD_H

#include <sv/script/interpreter.h>

#include <cstdint>

static const bool DEFAULT_ACCEPT_DATACARRIER = true;

class CKeyID;
class CScript;

//!< bytes (+1 for OP_RETURN, +2 for the pushdata opcodes)
static const uint64_t DEFAULT_DATA_CARRIER_SIZE = UINT32_MAX;
extern bool fAcceptDatacarrier;

/**
 * Mandatory script verification flags that all new blocks must comply with for
 * them to be valid. (but old blocks may not comply with) Currently just P2SH,
 * but in the future other flags may be added, such as a soft-fork to enforce
 * strict DER encoding.
 *
 * Failing one of these tests may trigger a DoS ban - see CheckInputs() for
 * details.
 */
static const uint32_t MANDATORY_SCRIPT_VERIFY_FLAGS =
    SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_STRICTENC |
    SCRIPT_ENABLE_SIGHASH_FORKID | SCRIPT_VERIFY_LOW_S | SCRIPT_VERIFY_NULLFAIL;

enum txnouttype {
    TX_NONSTANDARD,
    // 'standard' transaction types:
    TX_PUBKEY,
    TX_PUBKEYHASH,
    TX_SCRIPTHASH,
    TX_MULTISIG,
    TX_NULL_DATA,
};

const char *GetTxnOutputType (txnouttype t);

/**
 * Return public keys or hashes from scriptPubKey, for 'standard' transaction
 * types.
 */
bool Solver (const CScript &scriptPubKey, bool genesisEnabled, txnouttype &typeRet,
    std::vector<std::vector<uint8_t>> &vSolutionsRet);
    
CScript GetScriptForRawPubKey (const CPubKey &pubkey);
CScript GetScriptForMultisig (int nRequired, const std::vector<CPubKey> &keys);

#endif // BITCOIN_SCRIPT_STANDARD_H
