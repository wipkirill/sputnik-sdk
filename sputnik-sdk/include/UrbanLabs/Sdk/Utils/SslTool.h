#pragma once
#include <string>

#include <openssl/evp.h>

class SslTool {
private:
	std::string privateKey_;
	std::string publicKey_;
private:
	// secure private key
	EVP_PKEY *sKey_ = NULL;
	// verification public key
	EVP_PKEY *vKey_ = NULL;
public:
	static const std::string PUBLIC_KEY;
public:
	SslTool();
	~SslTool();
	bool publicKey(const std::string& pKey);
	bool privateKey(const std::string& pKey);
	bool sign(const std::string& msg, std::string& signedMsg);
	bool verify(const std::string& msg, const std::string& signedMsg);
};