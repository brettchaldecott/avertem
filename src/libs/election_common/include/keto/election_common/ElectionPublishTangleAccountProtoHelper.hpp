//
// Created by Brett Chaldecott on 2019-08-26.
//

#ifndef KETO_ELECTIONPUBLISHTANGLEACCOUNTPROTOHELPER_HPP
#define KETO_ELECTIONPUBLISHTANGLEACCOUNTPROTOHELPER_HPP

#include <string>
#include <map>
#include <memory>
#include <vector>

#include "Election.pb.h"

#include "keto/obfuscate/MetaString.hpp"
#include "keto/asn1/HashHelper.hpp"


namespace keto {
namespace election_common {

class ElectionPublishTangleAccountProtoHelper;
typedef std::shared_ptr <ElectionPublishTangleAccountProtoHelper> ElectionPublishTangleAccountProtoHelperPtr;


class ElectionPublishTangleAccountProtoHelper {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    ElectionPublishTangleAccountProtoHelper();
    ElectionPublishTangleAccountProtoHelper(const keto::proto::ElectionPublishTangleAccount& electionPeerMessage);
    ElectionPublishTangleAccountProtoHelper(const std::string& msg);
    ElectionPublishTangleAccountProtoHelper(const ElectionPublishTangleAccountProtoHelper& orig) = default;
    virtual ~ElectionPublishTangleAccountProtoHelper();

    keto::asn1::HashHelper getAccount() const;
    ElectionPublishTangleAccountProtoHelper& setAccount(const keto::asn1::HashHelper& account);

    std::vector<keto::asn1::HashHelper> getTangles() const;
    ElectionPublishTangleAccountProtoHelper& addTangle(const keto::asn1::HashHelper& tangleHash);
    int size();

    bool isGrowing() const;
    ElectionPublishTangleAccountProtoHelper& setGrowing(bool growing);

    operator keto::proto::ElectionPublishTangleAccount() const;
    operator std::string() const;

private:
    keto::proto::ElectionPublishTangleAccount electionPublishTangleAccount;

};


}
}


#endif //KETO_ELECTIONPUBLISHTANGLEACCOUNTPROTOHELPER_HPP
