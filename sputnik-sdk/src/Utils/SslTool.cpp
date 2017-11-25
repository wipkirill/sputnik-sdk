#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/pem.h>
#include <openssl/engine.h> 

#include <UrbanLabs/Sdk/Utils/SslTool.h>
#include <UrbanLabs/Sdk/Utils/Logger.h>

/**
* Usage on end-user side:
* SslTool sTool;
* sTool.publicKey(SslTool::PUBLIC_KEY);
* sTool.verify(...)
* 
* Usage for encryption:
* SslTool sTool;
* sTool.privateKey(...)
* sTool.sign(...)
*/

const std::string SslTool::PUBLIC_KEY = "-----BEGIN PUBLIC KEY-----\n" 
"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDCCDuRc2xr8coa5t/cgzWAUsy3\n" 
"iRYQF3SE11yyLekC4wudkM/A+212Vd0NnX37vjBJEaHiKVbroxGpo/2TjVgYWlGT\n" 
"idpsyblRROAPG+oPeYHhT0+wNFZnphXss7ESleQSuw0f0rkbSNHkhfd6buxPUD1y\n" 
"EzKbkco4FQzQkXRjOwIDAQAB\n" 
"-----END PUBLIC KEY-----\n";

SslTool::SslTool() {
	OpenSSL_add_all_algorithms();
}

SslTool::~SslTool() {
    if(sKey_)
        EVP_PKEY_free(sKey_);
    
    if(vKey_)
        EVP_PKEY_free(vKey_);
}
/**
* Sets public key
*/
bool SslTool::publicKey(const std::string& pKey) {
    if(vKey_ != NULL) {
        EVP_PKEY_free(vKey_);
        vKey_ = NULL;
    }

    BIO* bio = BIO_new_mem_buf((void *)pKey.c_str(), pKey.size());
	// reads a public key
	vKey_ = PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL); 

    return (vKey_ != NULL);
}
/**
* Sets private key
*/
bool SslTool::privateKey(const std::string& privateKey) {
    if(sKey_ != NULL) {
        EVP_PKEY_free(sKey_);
        sKey_ = NULL;
    }
    BIO* bio = BIO_new_mem_buf((char *)privateKey.c_str(), privateKey.size());
    // reads a private key
	sKey_ = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
   
    return (sKey_ != NULL);
}
/**
* Sign msg with private key
*/
bool SslTool::sign(const std::string& msg, std::string& signedMsg) {
	const char hn[] = "SHA256";
    /* Returned to caller */
    bool result = false;
    
    if(msg.size() == 0 || !sKey_) {
        LOGG(Logger::DEBUG) << "Messsage or private key is empty" << Logger::FLUSH;
        return false;
    }

    unsigned char* sig = NULL;
    size_t slen = 0;
    
    EVP_MD_CTX* ctx = NULL;
    
    do
    {
        ctx = EVP_MD_CTX_create();
        if(ctx == NULL) {
            LOGG(Logger::DEBUG) << "EVP_MD_CTX_create failed, error 0x" << ERR_get_error() << Logger::FLUSH;
            break;
        }
        
        const EVP_MD* md = EVP_get_digestbyname(hn);
        if(md == NULL) {
            LOGG(Logger::DEBUG) << "EVP_get_digestbyname failed, error 0x"<< ERR_get_error() << Logger::FLUSH;
            break;
        }
        
        int rc = EVP_DigestInit_ex(ctx, md, NULL);
        if(rc != 1) {
            LOGG(Logger::DEBUG) << "EVP_DigestInit_ex failed, error 0x"<< ERR_get_error() << Logger::FLUSH;
            break; /* failed */
        }
        
        rc = EVP_DigestSignInit(ctx, NULL, md, NULL, sKey_);
        if(rc != 1) {
            LOGG(Logger::DEBUG) << "EVP_DigestSignInit failed, error 0x"<< ERR_get_error() << Logger::FLUSH;
            break;
        }
        
        rc = EVP_DigestSignUpdate(ctx, msg.c_str(), msg.size());
        if(rc != 1) {
            LOGG(Logger::DEBUG) << "EVP_DigestSignUpdate failed, error 0x"<< ERR_get_error() << Logger::FLUSH;
            break;
        }
        
        size_t req = 0;
        rc = EVP_DigestSignFinal(ctx, NULL, &req);
        if(rc != 1) {
            LOGG(Logger::DEBUG) << "EVP_DigestSignFinal failed (1), error 0x"<< ERR_get_error() << Logger::FLUSH;
            break; /* failed */
        }
        
        if(req == 0) {
            LOGG(Logger::DEBUG) << "EVP_DigestSignFinal failed (2), error 0x"<< ERR_get_error() << Logger::FLUSH;
            break;
        }
        
        sig = (unsigned char *)OPENSSL_malloc(req);
        if(sig == NULL) {
            LOGG(Logger::DEBUG) << "OPENSSL_malloc failed, error 0x"<< ERR_get_error() << Logger::FLUSH;
            break;
        }
        
        slen = req;
        rc = EVP_DigestSignFinal(ctx, sig, &slen);
        if(rc != 1) {
            LOGG(Logger::DEBUG) << "EVP_DigestSignFinal failed (3), return code" << rc << "error 0x"<< ERR_get_error() << Logger::FLUSH;
            break;
        }
        
        if(req != slen) {
            LOGG(Logger::DEBUG) << "EVP_DigestSignFinal failed, mismatched signature sizes" << Logger::FLUSH;
            break;
        }
        
        result = true;
        
    } while(0);

    signedMsg = std::string((const char *)sig, slen);
    
    if(ctx) {
        EVP_MD_CTX_destroy(ctx);
        ctx = NULL;
    }

    if(sig)
        OPENSSL_free(sig);
    
    return result;
}

bool SslTool::verify(const std::string& msg, const std::string& signedMsg) {
	const char hn[] = "SHA256";
    /* Returned to caller */
    bool result = false;
    
    if(msg.size() == 0 || signedMsg.size() == 0 || !vKey_) {
        LOGG(Logger::DEBUG) << "Message or signature is empty" << Logger::FLUSH;
        return false;
    }
    
    EVP_MD_CTX* ctx = NULL;
    
    do
    {
        ctx = EVP_MD_CTX_create();
        if(ctx == NULL) {
            LOGG(Logger::DEBUG) << "EVP_MD_CTX_create failed, error 0x"<< ERR_get_error() << Logger::FLUSH;
            break;
        }
        
        const EVP_MD* md = EVP_get_digestbyname(hn);
        if(md == NULL) {
            LOGG(Logger::DEBUG) << "EVP_get_digestbyname failed, error 0x"<< ERR_get_error() << Logger::FLUSH;
            break;
        }
        
        int rc = EVP_DigestInit_ex(ctx, md, NULL);
        if(rc != 1) {
            LOGG(Logger::DEBUG) << "EVP_DigestInit_ex failed, error 0x"<< ERR_get_error() << Logger::FLUSH;
            break;
        }
        
        rc = EVP_DigestVerifyInit(ctx, NULL, md, NULL, vKey_);
        if(rc != 1) {
            LOGG(Logger::DEBUG) << "EVP_DigestVerifyInit failed, error 0x"<< ERR_get_error() << Logger::FLUSH;
            break;
        }
        
        rc = EVP_DigestVerifyUpdate(ctx, msg.c_str(), msg.size());
        if(rc != 1) {
            LOGG(Logger::DEBUG) << "EVP_DigestVerifyUpdate failed, error 0x"<< ERR_get_error() << Logger::FLUSH;
            break;
        }
        
        /* Clear any errors for the call below */
        ERR_clear_error();
        
        rc = EVP_DigestVerifyFinal(ctx, (const unsigned char *)signedMsg.c_str(), signedMsg.size());
        if(rc != 1) {
            LOGG(Logger::DEBUG) << "EVP_DigestVerifyFinal failed, error 0x"<< ERR_get_error() << Logger::FLUSH;
            break;
        }
        
        result = true;
    } while(0);
    
    if(ctx) {
        EVP_MD_CTX_destroy(ctx);
        ctx = NULL;
    }
    
    return result;
}