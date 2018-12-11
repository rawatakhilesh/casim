#ifndef RRIP_REPL_H_
#define RRIP_REPL_H_

#include "repl_policies.h"

// Static RRIP
class SRRIPReplPolicy : public ReplPolicy {
protected:
    uint64_t* array;
    uint32_t numLines;
    uint32_t repCheck = 0; //replaced check
    // add class member variables here

public:
    explicit SRRIPReplPolicy(uint32_t _numLines) : numLines(_numLines) {
            array = gm_calloc<uint64_t>(numLines);
            for (uint32_t i = 0; i < numLines; i++) {// set to "3"
              array[i] = 3;
            }
        }

    ~SRRIPReplPolicy() {
            gm_free(array);
        }

        void update(uint32_t id, const MemReq* req) {
            if(repCheck == 1) {  // if replaced a funcition ago
              // update after replacement on a miss
              array[id] = 2;
              repCheck = 0;
            } else {
              // update on hit
              array[id] = 0;
            }
        }

        void replaced(uint32_t id) {
            // cache block replaced
            repCheck = 1;
            // no use
            array[id] = 0;
        }

        template <typename C> uint32_t rank(const MemReq* req, C cands) {
            uint32_t bestCand = -1;
            bestCand = findCand(cands);
            // best candidate for replacement found
            return bestCand;
        }

private:
        template <typename C> uint64_t findCand(C cands) {
          // recursive function to find the first RRPV = 3
          // if not found, increments all by 1 and seach again
          for (auto ci = cands.begin(); ci != cands.end(); ci.inc()) {
            if (array[*ci] == 3) {
              // found the first "3"
              return *ci;
            }
          }
          for (auto ci = cands.begin(); ci != cands.end(); ci.inc()) {
            // no three found, increment all
            array[*ci]++;
          }
          // attempt again to find a "3",  when found return
          return findCand(cands);
        }
        // add member methods here, refer to repl_policies.h

        DECL_RANK_BINDINGS;
};
#endif // RRIP_REPL_H_


