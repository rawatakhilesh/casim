#ifndef HAWKEYE_REPL_H_
#define HAWKEYE_REPL_H_

#include <vector>

#include "repl_policies.h"


// Hawkeye by Jain et al. 2016
class HawkeyeReplPolicy : public ReplPolicy {

	private:
		uint64_t* rripArray;
		uint32_t numLines; // number of cache lines
		bool predVal; // predval used by the hawkeye predictor
		bool currentAcess;

	public:
		explicit HawkeyeReplPolicy(uint32_t _numLines) :numLines(_numLines) {
			rripArray = gm_calloc<uint64_t>(numLines);
		}

		~HawkeyeReplPolicy() {
			gm_free(rripArray);
		}

		void update(uint32_t id, const MemReq* req) {
			/* called on cache hit and on cache miss after rank() and replaced()
			are called */
			predVal = hawkeyePredictor();
			/* Hawkeye predictor generates a binary prediction to indicate whether
			the line is cache-friendly or cache-averse. */

			associateRRIP(predVal, currentAcess);
			/* It uses prediction generated by Hawkeye predictor. currentAcess is
			wether the current access to the cache line was a hit or a miss. For a
			hit, currentAcess is 1 and vice versa.*/

		}

		void replaced(uint32_t id) {
			/* called when there is a miss and a cache block replaced */

		}

		template <typename C> inline uint32_t rank(const MemReq* req, C cands) {
			/* function called by cache for best candidate to be replaced */

			uint32_t bestCand = -1;
			bestCand = findCand(C cands);
			/* best candidate for replacement found */
			return bestCand;
		}

		inline uint64_t findCand(C cands) {
			/* the replacement policy */
			for (auto ci = cands.begin(); ci != cands.end(); ci.inc()) {
				if (rripArray[*ci] == 7) {
					/* found the best candidate */
					return *ci;
				}
			}
			/* if could not find the best candidate, find one with highest rrip
			value i.e oldest cache-friendly line */
			uint64_t max = rripArray[*cands.begin()];
			for (auto ci = cands.begin(); ci != cands.end(); ci.end()) {
				max = (rripArray[*ci] > max)? rripArray[*ci]:max;
			}

			/* detrain its load instruction if the evicted line is present in the
			sampler*/
			// TODO: implement the above line

			return max;
		}

		bool hawkeyePredictor() {
			/*
				Cache-friendly = 1
				Cache-averse = 0
			*/
			return predVal;
		}

		void associateRRIP(bool _predVal, bool _currentAccess) {
			/* associate all cache resident lines with 3 bit RRIP counters*/
			if (_currentAccess && _predVal) {
				/* cache hit and pred to be cache friendly */
				// set RRIP to  0
			} else if (_currentAccess && !_predVal) {
				/* cache hit and pred to be cache averse */
				// set RRIP value to 7
			} else if (!_currentAccess && _predVal) {
				/* cache miss and pred to be cache friendly */
				// set RRIP val to 0
				// age all lines: if (RRIP < 6) RRIP ++
			} else if (!_currentAccess && !_predVal) {
				/* cache miss and pred to be cache averse */
				// set RRIP to 7
			}
		}

		//template <typename C> uint64_t {}
		DECL_RANK_BINDINGS;
};

#endif // HAWKEYE_REPL_H_
