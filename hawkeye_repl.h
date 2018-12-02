#ifndef HAWKEYE_REPL_H_
#define HAWKEYE_REPL_H_
//using namespace std;

#include <vector>
#include <array>
#include <map>
// include only if it asks for it.
//#include <pin.H>

#include "repl_policies.h"

const uint32_t pcHashbits = 13; //13 bits hashed PC
const uint16_t MAX_PREDICT_ENTRIES = pow(2, pcHashbits);
//const uint32_t MAX_WAYS = 16;



// Hawkeye by Jain et al. 2016
// Fall'18 CSCE 614 Term Project
class HawkeyeReplPolicy : public ReplPolicy {
	protected:

		/* The hawkeye predictor hold 8k entries with 3 bit counters
		for training. */
		array < uint32_t, MAX_PREDICT_ENTRIES > hawkPredictor;

		map < ADDRINT, Address > pcAccessSequence;
		/* a pc access sequence map which records which pc accessed which x
		pc -> lineAddr */
		map < ADDRINT, Address >::iterator pi;

		uint64_t* rripArray;
		uint32_t numLines; // number of cache lines
		bool predVal; // predval used by the hawkeye predictor
		bool currentAccess;
		uint8_t cacheAssoc;

		/*
		uint32_t cacheAssoc = MAX_WAYS; // cache associativity
		in case gcc complains about cacheAssoc not being available during compile
		we will use use MAX_WAYS defined here and not from configs file.
		*/

		/* TODO: Use a struct for these  */
		/* 8*cache associativity here is 128 */
		vector < Address > accessSequence;
		vector < uint32_t> occupancyVector;
		uint32_t size = 8*cacheAssoc;

		/* to be used by the OPTGen to keep track of access sequence */

		/* for set-associative caches, OPTgen maintains one occupancyVector for
		each cache set such that the maximum capacity of any occupancyVector entry
		never exceeds the cache associativity.*/

		hash < Address > hashAddr;

		uint8_t repCheck = 0; // replaced check
		/* there is definitely a better way to do it. I am being lazy here.*/
	
		const unsigned int numOffsetBits;
		const unsigned int numIndexBits;
		const unsigned int totalLineBits;
		/* line address is not a real address */

	public:
		explicit HawkeyeReplPolicy(uint32_t _numLines, uint32_t lineSize, uint32_t _cacheAssoc) :numLines(_numLines), 
			cacheAssoc(_cacheAssoc),
			numOffsetBits(ceil(log2(lineSize/8))),
			numIndexBits(ceil(log2(numLines))), 
			totalLineBits(numOffsetBits+numIndexBits) {

			rripArray = gm_calloc<uint64_t>(numLines);

			/* initialize hawkPredictor array to 0b000 */
			hawkPredictor.fill(0b000);
		}

		~HawkeyeReplPolicy() {
			gm_free(rripArray);
		}

		void update(uint32_t id, const MemReq* req) {
			/* called on cache hit and on cache miss after rank() and replaced()
			are called */

			/* OPTgen should be called on each cache access */
			bool OPTGenHit = OPTGen(req);

			updateOVector(req);

			/* update pc access sequence with current pc and the line address
			accessed */
			pcAccessSequence[req->insAddr] = req->lineAddr >> totalLineBits;
			/* for info on why we are doing this refer to https://github.com/s5z/zsim/issues/130 */
			predVal = hawkeyePredictor(req, OPTGenHit);
			/* Hawkeye predictor generates a binary prediction to indicate whether
			the line is cache-friendly or cache-averse. */

			if(repCheck == 0) {
				// this is a cache hit
				currentAccess = 1;
			} else {
				// there was a cache miss and replacement called
				currentAccess = 0; // cache miss
				repCheck = 0;
			}
			associateRRIP(id, predVal, currentAccess);
			/* It uses prediction generated by Hawkeye predictor. currentAcess is
			wether the current access to the cache line was a hit or a miss. For a
			hit, currentAcess is 1 and vice versa.*/

		}

		void replaced(uint32_t id) {
			/* called when there is a miss and a cache block replaced */
			repCheck = 1;
		}

		template <typename C> uint32_t rank(const MemReq* req, C cands) {
			/* function called by cache for best candidate to be replaced */

			uint32_t bestCand = -1;
			bestCand = findCand(cands);
			/* best candidate for replacement found */
			return bestCand;
		}

		DECL_RANK_BINDINGS;

	private:
		template <typename C> uint64_t findCand(C cands) {
			/* THE REPLACEMENT POLICY */
			for (auto ci = cands.begin(); ci != cands.end(); ci.inc()) {
				if (rripArray[*ci] == 7) {
					/* found the best candidate */
					return *ci;
				}
			}
			/* if could not find the best candidate, find one with highest rrip
			value i.e oldest cache-friendly line */
			uint64_t max = rripArray[*cands.begin()];
			for (auto ci = cands.begin(); ci != cands.end(); ci.inc()) {
				max = (rripArray[*ci] > max)? rripArray[*ci]:max;
			}

			/* detrain its load instruction if the evicted line is present in the
			sampler*/
			// TODO: implement the above line

			return max;
		}

		void updateOVector(const MemReq* req) {
			/* update occupancyVector */
			if (occupancyVector.size() == size) {
				/* if limit reached */
				occupancyVector.erase(occupancyVector.begin());
				accessSequence.erase(accessSequence.begin());
			}
			/* update occupancyVector */
			occupancyVector.push_back(0);
			/* update access sequence */
			accessSequence.push_back(req->lineAddr >> totalLineBits);
		}

		bool OPTGen(const MemReq* req) {
			bool roofHit = true;
			uint32_t found = -1;
			uint32_t last_access;
			/* what OPTGen thinks of the recent cache access */
			bool OPTGenHit = false;

			/* set the most recent entry of the occupancyVector to zero */
			/* upadted with accessSequence*/

			/* lookup last access to this mem */
			for (uint32_t index = 0; index < size; index ++) {
				if(accessSequence[index] == req->lineAddr >> totalLineBits) {
					last_access = index;
					found = 1;
					break;
				}
			}

			/* if found, check if every element corresponding to the usage interval is less
			than the cache capacity */
			if (found > 0) {
				for (uint32_t i = last_access; i < size; i++) {
					if (occupancyVector[i] == cacheAssoc) {
						roofHit = true;
					} else {
						roofHit = false;
					}
				}
			}

			/* if all the elements corresponding to the usage interval are less
			than the cache capacity then OPT would have placed 'X' in the cache,
			so the elements in usage interval in occupancyVector are incremented */

			if (!roofHit) {
				for (uint32_t i = last_access; i < size; i++) {
					occupancyVector[i]++;
				}
				OPTGenHit = true;
			}

		return OPTGenHit;

		}

		ADDRINT lastPCAccessed(const MemReq* req) {
			/* should only be called when there is a cache hit else hell may break loose.
			Looks up the last pc that accessed this line addr and return it */
			for (auto pi = pcAccessSequence.begin(); pi!=pcAccessSequence.end(); pi++){
				if (pi->second == req->lineAddr >> totalLineBits) {// line addr is the second element
					return pi->first;
					// if match, return the pc i.e. the first element
				}
			}
			// should not reach here
			return false;
		}

		bool hawkeyePredictor(const MemReq* req, bool _OPTGenHit) {
			/* THE HAWKEYE PREDICTOR */
			// req.lineaddr might be useful
			// TODO: implementation ; /*value of opt gen at that PC*

			/* find the pc from pcAccesSequence array which accessed X last
			and hash it */

			Address hashedPc = (Address) ((unsigned long) hashAddr(lastPCAccessed(req))
											% (unsigned long) pow(2, pcHashbits));
			if (_OPTGenHit) {
				/* if OPTgen say hit, pc that LAST accessed X is trained positively*/
				hawkPredictor[hashedPc]++;
			} else {
				/* train negatively */
				hawkPredictor[hashedPc]--;
			}

			/*
			Cache-friendly = 1
			Cache-averse = 0
			*/
			if (hawkPredictor[req->insAddr] >> 2 == 1) {
				// indexed by the current load pc
				// Higher order bit for the 3 bit counter
				return true; //Cache-friendly
			} else {
				return false; //Cache-averse
			}
			// return predVal;
		}

		void associateRRIP(uint32_t _id, bool _predVal, bool _currentAccess) {
			/* associate all cache resident lines with 3 bit RRIP counters*/
			if (_currentAccess && _predVal) {
				/* cache hit and pred to be cache friendly */
				// set RRIP to  0
				rripArray[_id] = 0;
			} else if (_currentAccess && !_predVal) {
				/* cache hit and pred to be cache averse */
				// set RRIP value to 7
				rripArray[_id] = 7;
			} else if (!_currentAccess && _predVal) {
				/* cache miss and pred to be cache friendly */
				// set RRIP val to 0
				rripArray[_id] = 0;
				/* increment RRIP values of OTHER cache-friendly lines to track their
				relative age */
				ageAllCacheLines(_id);
			} else if (!_currentAccess && !_predVal) {
				/* cache miss and pred to be cache averse */
				// set RRIP to 7
				rripArray[_id] = 7;
			}
		}

		inline void ageAllCacheLines(uint32_t _id) {
			/* age all OTHER cache lines: if (RRIP < 6) RRIP ++ */
			for (uint32_t i = 0; i < numLines; i++) {
				if (_id!=i) {
					if (rripArray[i] < 6) {rripArray[i]++;}
				}
			}
		}

		//template <typename C> uint64_t {}
};

#endif // HAWKEYE_REPL_H_
