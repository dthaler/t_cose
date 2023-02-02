/*
 * t_cose_recipient_dec_aes_kw.c
 *
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 * Copyright (c) 2023, Laurence Lundblade. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * See BSD-3-Clause license in README.md
 *
 */

#include "t_cose/t_cose_recipient_dec_aes_kw.h" /* Interface implemented */
#include "t_cose/t_cose_common.h"
#include "t_cose/q_useful_buf.h"
#include "qcbor/qcbor_spiffy_decode.h"
#include "t_cose/t_cose_parameters.h"
#include "t_cose_crypto.h"
#include "t_cose_util.h"


/* This is an implementation of t_cose_recipient_dec_cb */
enum t_cose_err_t
t_cose_recipient_dec_keywrap_cb_private(struct t_cose_recipient_dec *me_x,
                                        const struct t_cose_header_location loc,
                                        QCBORDecodeContext *cbor_decoder,
                                        struct q_useful_buf cek_buffer,
                                        struct t_cose_parameter_storage *p_storage,
                                        struct t_cose_parameter **params,
                                        struct q_useful_buf_c *cek)
{
    struct q_useful_buf_c  ciphertext;
    struct q_useful_buf_c  protected_params;
    enum t_cose_err_t      err;
    int32_t                cose_algorithm_id;
    QCBORError             cbor_error;

    struct t_cose_recipient_dec_keywrap *me = (struct t_cose_recipient_dec_keywrap *)me_x;

    /* ---- The array of three that is a COSE_Recipient ---- */
    QCBORDecode_EnterArray(cbor_decoder, NULL);

    // TODO: support the header decode callbacks
    /* ----  First and second items -- protected & unprotected headers  ---- */
    *params = NULL;
    err = t_cose_headers_decode(cbor_decoder, /* in: decoder to read from */
                                loc,          /* in: location in COSE message */
                                NULL,         /* in: callback for specials */
                                NULL,         /* in: context for callback */
                                p_storage,    /* in: parameter storage */
                                params,       /* out: list of decoded params */
                               &protected_params /* out: encoded prot params */
                                );
    if(err != T_COSE_SUCCESS) {
        goto Done;
    }

    if(!q_useful_buf_c_is_empty(protected_params)) {
        /* There's can't be any protected headers here because
         * keywrap can't protected them (need an AEAD) */
        // TODO: the right error here
        return T_COSE_ERR_FAIL;
    }

    /* ---- Third item -- ciphertext ---- */
    QCBORDecode_GetByteString(cbor_decoder, &ciphertext);

    /* --- Close out the CBOR decoding ---- */
    QCBORDecode_ExitArray(cbor_decoder);
    cbor_error = QCBORDecode_GetError(cbor_decoder);
    if(cbor_error != QCBOR_SUCCESS) {
        return qcbor_decode_error_to_t_cose_error(cbor_error,
                                                  T_COSE_ERR_RECIPIENT_FORMAT);
    }

    // TODO: say how to find this in the not-protected bucket
    cose_algorithm_id = t_cose_find_parameter_alg_id(*params);

    // TODO: should probably check the kid here

    err = t_cose_crypto_kw_unwrap(cose_algorithm_id, /* in: alg id */
                                  me->kek,           /* in: kek    */
                                  ciphertext,        /* in: encrypted cek */
                                  cek_buffer,        /* in: buffer for cek */
                                  cek);              /* out: decrypted cek */

Done:
    return err;
}
