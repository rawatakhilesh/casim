#ifndef HAWKEYE_REPL_H_
#define HAWKEYE_REPL_H_

#include <vector>

#include "repl_policies.h"


// Hawkeye by Jain et al. 2016
class HawkeyeReplPolicy : public ReplPolicy {
	protected:
		uint64_t* array;
        uint32_t numLines;

	public:
        explicit HawkeyeReplPolicy(uint32_t _numLines) :numLines(_numLines) {
            array = gm_calloc<uint64_t>(numLines);
        }
		
		~HawkeyeReplPolicy() {
            gm_free(array);
        }

        void update(uint32_t id, const MemReq* req) {
			/* called on cache hit and on cache miss after rank, replaced are called*/   
        }

        void replaced(uint32_t id) {
			/* called when there is a miss*/
            
        }

        template <typename C> inline uint32_t rank(const MemReq* req, C cands) {
			
        }    
		
		DECL_RANK_BINDINGS;
	
	private:
		//template <typename C> uint64_t {}
        
       
};
#endif // HAWKEYE_REPL_H_


