// Copyright 2015-2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "sdkconfig.h"

#include "utils/common.h"
#include "crypto/aes_wrap.h"
#include "crypto/sha256.h"
#include "crypto/crypto.h"
#include "crypto/md5.h"
#include "crypto/sha1.h"
#include "crypto/aes.h"
#include "crypto/dh_group5.h"
#include "wps/wps.h"
#include "wps/wps_i.h"

#include "eap_peer/eap.h"
#include "tls/tls.h"
#include "eap_peer/eap_methods.h"
#include "eap_peer/eap_i.h"
#include "eap_peer/eap_common.h"
#include "esp_wifi_crypto_types.h"

int fast_sha256_vector(size_t num_elem, const uint8_t *addr[], const size_t *len,
		       uint8_t *mac);
int __must_check fast_aes_wrap(const uint8_t *kek, int n, const uint8_t *plain, uint8_t *cipher);
int __must_check fast_aes_unwrap(const uint8_t *kek, int n, const uint8_t *cipher, uint8_t *plain);
int __must_check fast_aes_128_cbc_encrypt(const uint8_t *key, const uint8_t *iv, uint8_t *data,
				          size_t data_len);
int __must_check fast_aes_128_cbc_decrypt(const uint8_t *key, const uint8_t *iv, uint8_t *data,
				          size_t data_len);
void fast_hmac_sha256_vector(const uint8_t *key, size_t key_len, size_t num_elem,
		             const uint8_t *addr[], const size_t *len, uint8_t *mac);
void fast_hmac_sha256(const uint8_t *key, size_t key_len, const uint8_t *data,
		      size_t data_len, uint8_t *mac);
void fast_sha256_prf(const uint8_t *key, size_t key_len, const char *label,
	             const uint8_t *data, size_t data_len, uint8_t *buf, size_t buf_len);
int __must_check fast_aes_wrap(const uint8_t *kek, int n, const uint8_t *plain, uint8_t *cipher);
int __must_check fast_aes_unwrap(const uint8_t *kek, int n, const uint8_t *cipher, uint8_t *plain);
int __must_check fast_aes_128_cbc_encrypt(const uint8_t *key, const uint8_t *iv, uint8_t *data,
				          size_t data_len);
int __must_check fast_aes_128_cbc_decrypt(const uint8_t *key, const uint8_t *iv, uint8_t *data,
				          size_t data_len);
int __must_check crypto_mod_exp(const u8 *base, size_t base_len,
				const u8 *power, size_t power_len,
				const u8 *modulus, size_t modulus_len,
				u8 *result, size_t *result_len);
void fast_crypto_hash_update(struct crypto_hash *ctx, const uint8_t *data, size_t len);
int fast_crypto_hash_finish(struct crypto_hash *ctx, uint8_t *hash, size_t *len);
struct crypto_cipher * fast_crypto_cipher_init(enum crypto_cipher_alg alg,
					            const uint8_t *iv, const uint8_t *key,
					            size_t key_len);
int __must_check fast_crypto_cipher_decrypt(struct crypto_cipher *ctx,
				            const uint8_t *crypt, uint8_t *plain, size_t len);
void fast_crypto_cipher_deinit(struct crypto_cipher *ctx);
int __must_check fast_crypto_mod_exp(const uint8_t *base, size_t base_len,
				     const uint8_t *power, size_t power_len,
				     const uint8_t *modulus, size_t modulus_len,
				     uint8_t *result, size_t *result_len);
struct crypto_hash * fast_crypto_hash_init(enum crypto_hash_alg alg, const uint8_t *key,
				                size_t key_len);
int __must_check fast_crypto_cipher_encrypt(struct crypto_cipher *ctx,
				            const uint8_t *plain, uint8_t *crypt, size_t len);

#if CONFIG_SSL_USING_MBEDTLS
/* 
 * The parameters is used to set the cyrpto callback function for station connect when in security mode,
 * every callback function can register as fast_xxx or normal one, i.e, fast_aes_wrap or aes_wrap, the 
 * difference between them is the normal API is calculate by software, the fast one use the hardware 
 * crypto in it, can be faster than the normal one, so the callback function register in default is which
 * we recommend, so as the API in WPS default and WPA2 default.
 */
const wpa_crypto_funcs_t g_wifi_default_wpa_crypto_funcs = {
    .size = sizeof(wpa_crypto_funcs_t), 
    .version = ESP_WIFI_CRYPTO_VERSION,
    .aes_wrap = (esp_aes_wrap_t)fast_aes_wrap,
    .aes_unwrap = (esp_aes_unwrap_t)fast_aes_unwrap,
    .hmac_sha256_vector = (esp_hmac_sha256_vector_t)fast_hmac_sha256_vector,
    .sha256_prf = (esp_sha256_prf_t)fast_sha256_prf,
    .hmac_md5 = (esp_hmac_md5_t)hmac_md5,
    .hamc_md5_vector = (esp_hmac_md5_vector_t)hmac_md5_vector,
    .hmac_sha1 = (esp_hmac_sha1_t)hmac_sha1,
    .hmac_sha1_vector = (esp_hmac_sha1_vector_t)hmac_sha1_vector,
    .sha1_prf = (esp_sha1_prf_t)sha1_prf,
    .sha1_vector = (esp_sha1_vector_t)sha1_vector,
    .pbkdf2_sha1 = (esp_pbkdf2_sha1_t)pbkdf2_sha1,
    .rc4_skip = (esp_rc4_skip_t)rc4_skip,
    .md5_vector = (esp_md5_vector_t)md5_vector,
    .aes_encrypt = (esp_aes_encrypt_t)aes_encrypt,
    .aes_encrypt_init = (esp_aes_encrypt_init_t)aes_encrypt_init,
    .aes_encrypt_deinit = (esp_aes_encrypt_deinit_t)aes_encrypt_deinit,
    .aes_decrypt = (esp_aes_decrypt_t)aes_decrypt,
    .aes_decrypt_init = (esp_aes_decrypt_init_t)aes_decrypt_init,
    .aes_decrypt_deinit = (esp_aes_decrypt_deinit_t)aes_decrypt_deinit
};

const wps_crypto_funcs_t g_wifi_default_wps_crypto_funcs = {
    .size = sizeof(wps_crypto_funcs_t), 
    .version = ESP_WIFI_CRYPTO_VERSION,
    .aes_128_encrypt = (esp_aes_128_encrypt_t)fast_aes_128_cbc_encrypt,
    .aes_128_decrypt = (esp_aes_128_decrypt_t)fast_aes_128_cbc_decrypt,
    .crypto_mod_exp = (esp_crypto_mod_exp_t)fast_crypto_mod_exp,
    .hmac_sha256 = (esp_hmac_sha256_t)fast_hmac_sha256,
    .hmac_sha256_vector = (esp_hmac_sha256_vector_t)fast_hmac_sha256_vector,
    .sha256_vector = (esp_sha256_vector_t)fast_sha256_vector,
    .uuid_gen_mac_addr = (esp_uuid_gen_mac_addr_t)uuid_gen_mac_addr,
    .dh5_free = (esp_dh5_free_t)dh5_free,
    .wps_build_assoc_req_ie = (esp_wps_build_assoc_req_ie_t)wps_build_assoc_req_ie,
    .wps_build_assoc_resp_ie = (esp_wps_build_assoc_resp_ie_t)wps_build_assoc_resp_ie,
    .wps_build_probe_req_ie = (esp_wps_build_probe_req_ie_t)wps_build_probe_req_ie,
    .wps_build_public_key = (esp_wps_build_public_key_t)wps_build_public_key,
    .wps_enrollee_get_msg = (esp_wps_enrollee_get_msg_t)wps_enrollee_get_msg,
    .wps_enrollee_process_msg = (esp_wps_enrollee_process_msg_t)wps_enrollee_process_msg,
    .wps_generate_pin = (esp_wps_generate_pin_t)wps_generate_pin,
    .wps_is_selected_pin_registrar = (esp_wps_is_selected_pin_registrar_t)wps_is_selected_pin_registrar,
    .wps_is_selected_pbc_registrar = (esp_wps_is_selected_pbc_registrar_t)wps_is_selected_pbc_registrar,
    .eap_msg_alloc = (esp_eap_msg_alloc_t)eap_msg_alloc
};

/*
 * What should notice is that the cyrpto hash type function and crypto cipher type function can not register
 * as different, i.e, if you use fast_crypto_hash_init, you should use fast_crypto_hash_update and 
 * fast_crypto_hash_finish for finish hash calculate, rather than call crypto_hash_update and 
 * crypto_hash_finish, so do crypto_cipher.
 */
const wpa2_crypto_funcs_t g_wifi_default_wpa2_crypto_funcs = {
    .size = sizeof(wpa2_crypto_funcs_t),
    .version = ESP_WIFI_CRYPTO_VERSION,
    .crypto_hash_init = (esp_crypto_hash_init_t)fast_crypto_hash_init,
    .crypto_hash_update = (esp_crypto_hash_update_t)fast_crypto_hash_update,
    .crypto_hash_finish = (esp_crypto_hash_finish_t)fast_crypto_hash_finish,
    .crypto_cipher_init = (esp_crypto_cipher_init_t)fast_crypto_cipher_init,
    .crypto_cipher_encrypt = (esp_crypto_cipher_encrypt_t)fast_crypto_cipher_encrypt,
    .crypto_cipher_decrypt = (esp_crypto_cipher_decrypt_t)fast_crypto_cipher_decrypt,
    .crypto_cipher_deinit = (esp_crypto_cipher_deinit_t)fast_crypto_cipher_deinit,
    .crypto_mod_exp = (esp_crypto_mod_exp_t)crypto_mod_exp,
    .sha256_vector = (esp_sha256_vector_t)fast_sha256_vector,
    .tls_init = (esp_tls_init_t)tls_init,
    .tls_deinit = (esp_tls_deinit_t)tls_deinit,
    .eap_peer_blob_init = (esp_eap_peer_blob_init_t)eap_peer_blob_init,
    .eap_peer_blob_deinit = (esp_eap_peer_blob_deinit_t)eap_peer_blob_deinit,
    .eap_peer_config_init = (esp_eap_peer_config_init_t)eap_peer_config_init,
    .eap_peer_config_deinit = (esp_eap_peer_config_deinit_t)eap_peer_config_deinit,
    .eap_peer_register_methods = (esp_eap_peer_register_methods_t)eap_peer_register_methods,
    .eap_peer_unregister_methods = (esp_eap_peer_unregister_methods_t)eap_peer_unregister_methods,
    .eap_deinit_prev_method = (esp_eap_deinit_prev_method_t)eap_deinit_prev_method,
    .eap_peer_get_eap_method = (esp_eap_peer_get_eap_method_t)eap_peer_get_eap_method,
    .eap_sm_abort = (esp_eap_sm_abort_t)eap_sm_abort,
    .eap_sm_build_nak = (esp_eap_sm_build_nak_t)eap_sm_build_nak,
    .eap_sm_build_identity_resp = (esp_eap_sm_build_identity_resp_t)eap_sm_build_identity_resp,
    .eap_msg_alloc = (esp_eap_msg_alloc_t)eap_msg_alloc
};
#else

/* 
 * The parameters is used to set the cyrpto callback function for station connect when in security mode,
 * every callback function can register as fast_xxx or normal one, i.e, fast_aes_wrap or aes_wrap, the 
 * difference between them is the normal API is calculate by software, the fast one use the hardware 
 * crypto in it, can be faster than the normal one, so the callback function register in default is which
 * we recommend, so as the API in WPS default and WPA2 default.
 */
const wpa_crypto_funcs_t g_wifi_default_wpa_crypto_funcs = {
    .size = sizeof(wpa_crypto_funcs_t), 
    .version = ESP_WIFI_CRYPTO_VERSION,
    .aes_wrap = (esp_aes_wrap_t)aes_wrap,
    .aes_unwrap = (esp_aes_unwrap_t)aes_unwrap,
    .hmac_sha256_vector = (esp_hmac_sha256_vector_t)hmac_sha256_vector,
    .sha256_prf = (esp_sha256_prf_t)fast_sha256_prf,
    .hmac_md5 = (esp_hmac_md5_t)hmac_md5,
    .hamc_md5_vector = (esp_hmac_md5_vector_t)hmac_md5_vector,
    .hmac_sha1 = (esp_hmac_sha1_t)hmac_sha1,
    .hmac_sha1_vector = (esp_hmac_sha1_vector_t)hmac_sha1_vector,
    .sha1_prf = (esp_sha1_prf_t)sha1_prf,
    .sha1_vector = (esp_sha1_vector_t)sha1_vector,
    .pbkdf2_sha1 = (esp_pbkdf2_sha1_t)pbkdf2_sha1,
    .rc4_skip = (esp_rc4_skip_t)rc4_skip,
    .md5_vector = (esp_md5_vector_t)md5_vector,
    .aes_encrypt = (esp_aes_encrypt_t)aes_encrypt,
    .aes_encrypt_init = (esp_aes_encrypt_init_t)aes_encrypt_init,
    .aes_encrypt_deinit = (esp_aes_encrypt_deinit_t)aes_encrypt_deinit,
    .aes_decrypt = (esp_aes_decrypt_t)aes_decrypt,
    .aes_decrypt_init = (esp_aes_decrypt_init_t)aes_decrypt_init,
    .aes_decrypt_deinit = (esp_aes_decrypt_deinit_t)aes_decrypt_deinit
};

const wps_crypto_funcs_t g_wifi_default_wps_crypto_funcs = {
    .size = sizeof(wps_crypto_funcs_t), 
    .version = ESP_WIFI_CRYPTO_VERSION,
    .aes_128_encrypt = (esp_aes_128_encrypt_t)aes_128_cbc_encrypt,
    .aes_128_decrypt = (esp_aes_128_decrypt_t)aes_128_cbc_decrypt,
    .crypto_mod_exp = (esp_crypto_mod_exp_t)crypto_mod_exp,
    .hmac_sha256 = (esp_hmac_sha256_t)hmac_sha256,
    .hmac_sha256_vector = (esp_hmac_sha256_vector_t)hmac_sha256_vector,
    .sha256_vector = (esp_sha256_vector_t)sha256_vector,
    .uuid_gen_mac_addr = (esp_uuid_gen_mac_addr_t)uuid_gen_mac_addr,
    .dh5_free = (esp_dh5_free_t)dh5_free,
    .wps_build_assoc_req_ie = (esp_wps_build_assoc_req_ie_t)wps_build_assoc_req_ie,
    .wps_build_assoc_resp_ie = (esp_wps_build_assoc_resp_ie_t)wps_build_assoc_resp_ie,
    .wps_build_probe_req_ie = (esp_wps_build_probe_req_ie_t)wps_build_probe_req_ie,
    .wps_build_public_key = (esp_wps_build_public_key_t)wps_build_public_key,
    .wps_enrollee_get_msg = (esp_wps_enrollee_get_msg_t)wps_enrollee_get_msg,
    .wps_enrollee_process_msg = (esp_wps_enrollee_process_msg_t)wps_enrollee_process_msg,
    .wps_generate_pin = (esp_wps_generate_pin_t)wps_generate_pin,
    .wps_is_selected_pin_registrar = (esp_wps_is_selected_pin_registrar_t)wps_is_selected_pin_registrar,
    .wps_is_selected_pbc_registrar = (esp_wps_is_selected_pbc_registrar_t)wps_is_selected_pbc_registrar,
    .eap_msg_alloc = (esp_eap_msg_alloc_t)eap_msg_alloc
};

/*
 * What should notice is that the cyrpto hash type function and crypto cipher type function can not register
 * as different, i.e, if you use fast_crypto_hash_init, you should use fast_crypto_hash_update and 
 * fast_crypto_hash_finish for finish hash calculate, rather than call crypto_hash_update and 
 * crypto_hash_finish, so do crypto_cipher.
 */
const wpa2_crypto_funcs_t g_wifi_default_wpa2_crypto_funcs = {
    .size = sizeof(wpa2_crypto_funcs_t),
    .version = ESP_WIFI_CRYPTO_VERSION,
    .crypto_hash_init = (esp_crypto_hash_init_t)crypto_hash_init,
    .crypto_hash_update = (esp_crypto_hash_update_t)crypto_hash_update,
    .crypto_hash_finish = (esp_crypto_hash_finish_t)crypto_hash_finish,
    .crypto_cipher_init = (esp_crypto_cipher_init_t)crypto_cipher_init,
    .crypto_cipher_encrypt = (esp_crypto_cipher_encrypt_t)crypto_cipher_encrypt,
    .crypto_cipher_decrypt = (esp_crypto_cipher_decrypt_t)crypto_cipher_decrypt,
    .crypto_cipher_deinit = (esp_crypto_cipher_deinit_t)crypto_cipher_deinit,
    .crypto_mod_exp = (esp_crypto_mod_exp_t)crypto_mod_exp,
    .sha256_vector = (esp_sha256_vector_t)sha256_vector,
    .tls_init = (esp_tls_init_t)tls_init,
    .tls_deinit = (esp_tls_deinit_t)tls_deinit,
    .eap_peer_blob_init = (esp_eap_peer_blob_init_t)eap_peer_blob_init,
    .eap_peer_blob_deinit = (esp_eap_peer_blob_deinit_t)eap_peer_blob_deinit,
    .eap_peer_config_init = (esp_eap_peer_config_init_t)eap_peer_config_init,
    .eap_peer_config_deinit = (esp_eap_peer_config_deinit_t)eap_peer_config_deinit,
    .eap_peer_register_methods = (esp_eap_peer_register_methods_t)eap_peer_register_methods,
    .eap_peer_unregister_methods = (esp_eap_peer_unregister_methods_t)eap_peer_unregister_methods,
    .eap_deinit_prev_method = (esp_eap_deinit_prev_method_t)eap_deinit_prev_method,
    .eap_peer_get_eap_method = (esp_eap_peer_get_eap_method_t)eap_peer_get_eap_method,
    .eap_sm_abort = (esp_eap_sm_abort_t)eap_sm_abort,
    .eap_sm_build_nak = (esp_eap_sm_build_nak_t)eap_sm_build_nak,
    .eap_sm_build_identity_resp = (esp_eap_sm_build_identity_resp_t)eap_sm_build_identity_resp,
    .eap_msg_alloc = (esp_eap_msg_alloc_t)eap_msg_alloc
};
#endif