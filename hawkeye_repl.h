#ifndef HAWKEYE_REPL_H_
#define HAWKEYE_REPL_H_

#include "repl_policies.h"

// Hawkeye by Jain et al. 2016
class HawkeyeReplPolicy : public ReplPolicy {
protected:


public:

    ~HawkeyeReplPolicy() {
            gm_free(array);
        }


private:
        
        DECL_RANK_BINDINGS;
};
#endif // HAWKEYE_REPL_H_


