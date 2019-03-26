//
// Created by Brett Chaldecott on 2019/03/22.
//

#ifndef KETO_WALLYSCOPE_HPP
#define KETO_WALLYSCOPE_HPP

namespace keto {
namespace cli {


class WallyScope {
public:
    WallyScope();
    WallyScope(const WallyScope& orig) = delete;
    virtual ~WallyScope();

};


}
}


#endif //KETO_WALLYSCOPE_HPP
