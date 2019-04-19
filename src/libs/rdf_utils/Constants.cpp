//
// Created by Brett Chaldecott on 2019/04/19.
//

#include "keto/rdf_utils/Constants.hpp"


namespace keto {
namespace rdf_utils {

const std::vector<const char*> Constants::EXCLUDES{
    EXCLUDE::TRANSACTION,
    EXCLUDE::BLOCK
};

}
}