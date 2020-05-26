// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2014 Martin Šošić
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
**/

#include "SPCommon.h"
#include "SPSearchDistance.h"

NS_SP_EXT_BEGIN(search)

// from https://github.com/Martinsos/edlib

using bytes = memory::bytes;
using Word = uint64_t;
static const int WORD_SIZE = sizeof(Word) * 8; // Size of Word in bits
static const Word WORD_1 = (Word)1;
static const Word HIGH_BIT_MASK = WORD_1 << (WORD_SIZE - 1);  // 100..00

// Data needed to find alignment.
struct AlignmentData : memory::AllocPool {
	Word * Ps = nullptr;
	Word * Ms = nullptr;
	int * scores = nullptr;
	int * firstBlocks = nullptr;
	int * lastBlocks = nullptr;

	bytes data;

	AlignmentData() { }

	void resize(int maxNumBlocks, int targetLength) {
		data.resize(2 * sizeof(Word) * maxNumBlocks * targetLength
				+ sizeof(int) * maxNumBlocks * targetLength
				+ 2 * sizeof(int) * targetLength);
		// We build a complete table and mark first and last block for each column
		// (because algorithm is banded so only part of each columns is used).
		// TODO: do not build a whole table, but just enough blocks for each column.
		Ps = (Word *)(data.data());
		Ms = (Word *)(data.data() + sizeof(Word) * maxNumBlocks * targetLength);
		scores = (int *)(data.data() + 2 * sizeof(Word) * maxNumBlocks * targetLength);
		firstBlocks = (int *)(data.data() + 2 * sizeof(Word) * maxNumBlocks * targetLength
				+ sizeof(int) * maxNumBlocks * targetLength);
		lastBlocks = (int *)(data.data() + 2 * sizeof(Word) * maxNumBlocks * targetLength
				+ sizeof(int) * maxNumBlocks * targetLength + sizeof(int) * targetLength);
	}
};

struct Block {
    Word P = 0;  // Pvin
    Word M = 0;  // Mvin
    int score = 0; // score of last cell in block;

    Block() {}
    Block(Word P, Word M, int score) :P(P), M(M), score(score) {}
};

static int myersCalcEditDistanceNW(const memory::vector<Word> & Peq, int W, int maxNumBlocks,
                                   const bytes &query, const bytes &target,
                                   int alphabetLength, int k, int* bestScore_,
                                   int* position_, bool findAlignment,
                                   AlignmentData & alignData, int targetStopPosition);

static int obtainAlignment(const bytes & query, const bytes & rQuery, const bytes & target, const bytes & rTarget,
		const int alphabetLength, const int bestScore, Distance::Storage & alignment);

static int obtainAlignmentHirschberg(const bytes & query, const bytes & rQuery, const bytes & target, const bytes & rTarget,
        int alphabetLength, int bestScore, Distance::Storage & alignment);

static int obtainAlignmentTraceback(int queryLength, int targetLength, int bestScore, const AlignmentData & alignData, Distance::Storage & alignment);

static memory::string transformSequences(const StringView &query, const StringView &target,
		memory::bytes & queryTransformed, memory::bytes & targetTransformed);

static inline int ceilDiv(int x, int y);

static inline bytes createReverseCopy(const memory::bytes &);

static inline memory::vector<Word> buildPeq(const int alphabetLength, const memory::bytes &query);

#define EDLIB_STATUS_OK 0
#define EDLIB_STATUS_ERROR 1

#define EDLIB_EDOP_MATCH 0    //!< Match.
#define EDLIB_EDOP_INSERT 1   //!< Insertion to target = deletion from query.
#define EDLIB_EDOP_DELETE 2   //!< Deletion from target = insertion to query.
#define EDLIB_EDOP_MISMATCH 3 //!< Mismatch.

/**
 * Build Peq table for given query and alphabet.
 * Peq is table of dimensions alphabetLength+1 x maxNumBlocks.
 * Bit i of Peq[s * maxNumBlocks + b] is 1 if i-th symbol from block b of query equals symbol s, otherwise it is 0.
 * NOTICE: free returned array with delete[]!
 */
static inline memory::vector<Word> buildPeq(const int alphabetLength, const memory::bytes &query) {
    int maxNumBlocks = ceilDiv(query.size(), WORD_SIZE);
    // table of dimensions alphabetLength+1 x maxNumBlocks. Last symbol is wildcard.
    memory::vector<Word> Peq; Peq.resize((alphabetLength + 1) * maxNumBlocks);

    // Build Peq (1 is match, 0 is mismatch). NOTE: last column is wildcard(symbol that matches anything) with just 1s
    for (int symbol = 0; symbol <= alphabetLength; symbol++) {
        for (int b = 0; b < maxNumBlocks; b++) {
            if (symbol < alphabetLength) {
                Peq[symbol * maxNumBlocks + b] = 0;
                for (int r = (b+1) * WORD_SIZE - 1; r >= b * WORD_SIZE; r--) {
                    Peq[symbol * maxNumBlocks + b] <<= 1;
                    // NOTE: We pretend like query is padded at the end with W wildcard symbols
                    if (size_t(r) >= query.size() || (query[r] == symbol)) {
                        Peq[symbol * maxNumBlocks + b] += 1;
                    }
                }
            } else { // Last symbol is wildcard, so it is all 1s
                Peq[symbol * maxNumBlocks + b] = (Word)-1;
            }
        }
    }

    return Peq;
}

/**
 * Returns new sequence that is reverse of given sequence.
 * Free returned array with delete[].
 */
static inline memory::bytes createReverseCopy(const memory::bytes &seq) {
	const auto length = seq.size();
	memory::bytes rSeq; rSeq.resize(length);
    for (size_t i = 0; i < length; i++) {
        rSeq[i] = seq[length - i - 1];
    }
    return rSeq;
}

/**
 * Corresponds to Advance_Block function from Myers.
 * Calculates one word(block), which is part of a column.
 * Highest bit of word (one most to the left) is most bottom cell of block from column.
 * Pv[i] and Mv[i] define vin of cell[i]: vin = cell[i] - cell[i-1].
 * @param [in] Pv  Bitset, Pv[i] == 1 if vin is +1, otherwise Pv[i] == 0.
 * @param [in] Mv  Bitset, Mv[i] == 1 if vin is -1, otherwise Mv[i] == 0.
 * @param [in] Eq  Bitset, Eq[i] == 1 if match, 0 if mismatch.
 * @param [in] hin  Will be +1, 0 or -1.
 * @param [out] PvOut  Bitset, PvOut[i] == 1 if vout is +1, otherwise PvOut[i] == 0.
 * @param [out] MvOut  Bitset, MvOut[i] == 1 if vout is -1, otherwise MvOut[i] == 0.
 * @param [out] hout  Will be +1, 0 or -1.
 */
static inline int calculateBlock(Word Pv, Word Mv, Word Eq, const int hin,
                                 Word &PvOut, Word &MvOut) {
    // hin can be 1, -1 or 0.
    // 1  -> 00...01
    // 0  -> 00...00
    // -1 -> 11...11 (2-complement)

    Word hinIsNeg = (Word)(hin >> 2) & WORD_1; // 00...001 if hin is -1, 00...000 if 0 or 1

    Word Xv = Eq | Mv;
    // This is instruction below written using 'if': if (hin < 0) Eq |= (Word)1;
    Eq |= hinIsNeg;
    Word Xh = (((Eq & Pv) + Pv) ^ Pv) | Eq;

    Word Ph = Mv | ~(Xh | Pv);
    Word Mh = Pv & Xh;

    int hout = 0;
    // This is instruction below written using 'if': if (Ph & HIGH_BIT_MASK) hout = 1;
    hout = (Ph & HIGH_BIT_MASK) >> (WORD_SIZE - 1);
    // This is instruction below written using 'if': if (Mh & HIGH_BIT_MASK) hout = -1;
    hout -= (Mh & HIGH_BIT_MASK) >> (WORD_SIZE - 1);

    Ph <<= 1;
    Mh <<= 1;

    // This is instruction below written using 'if': if (hin < 0) Mh |= (Word)1;
    Mh |= hinIsNeg;
    // This is instruction below written using 'if': if (hin > 0) Ph |= (Word)1;
    Ph |= (Word)((hin + 1) >> 1);

    PvOut = Mh | ~(Xv | Ph);
    MvOut = Ph & Xv;

    return hout;
}

/**
 * Does ceiling division x / y.
 * Note: x and y must be non-negative and x + y must not overflow.
 */
static inline int ceilDiv(const int x, const int y) {
    return x % y ? x / y + 1 : x / y;
}

static inline int min(const int x, const int y) {
    return x < y ? x : y;
}

static inline int max(const int x, const int y) {
    return x > y ? x : y;
}


/**
 * @param [in] block
 * @return Values of cells in block, starting with bottom cell in block.
 */
static inline memory::vector<int> getBlockCellValues(const Block block) {
	memory::vector<int> scores(WORD_SIZE);
    int score = block.score;
    Word mask = HIGH_BIT_MASK;
    for (int i = 0; i < WORD_SIZE - 1; i++) {
        scores[i] = score;
        if (block.P & mask) score--;
        if (block.M & mask) score++;
        mask >>= 1;
    }
    scores[WORD_SIZE - 1] = score;
    return scores;
}

/**
 * Writes values of cells in block into given array, starting with first/top cell.
 * @param [in] block
 * @param [out] dest  Array into which cell values are written. Must have size of at least WORD_SIZE.
 */
static inline void readBlock(const Block block, int* const dest) {
    int score = block.score;
    Word mask = HIGH_BIT_MASK;
    for (int i = 0; i < WORD_SIZE - 1; i++) {
        dest[WORD_SIZE - 1 - i] = score;
        if (block.P & mask) score--;
        if (block.M & mask) score++;
        mask >>= 1;
    }
    dest[0] = score;
}

/**
 * Writes values of cells in block into given array, starting with last/bottom cell.
 * @param [in] block
 * @param [out] dest  Array into which cell values are written. Must have size of at least WORD_SIZE.
 */
static inline void readBlockReverse(const Block block, int* const dest) {
    int score = block.score;
    Word mask = HIGH_BIT_MASK;
    for (int i = 0; i < WORD_SIZE - 1; i++) {
        dest[i] = score;
        if (block.P & mask) score--;
        if (block.M & mask) score++;
        mask >>= 1;
    }
    dest[WORD_SIZE - 1] = score;
}

/**
 * Uses Myers' bit-vector algorithm to find edit distance for global(NW) alignment method.
 * @param [in] Peq  Query profile.
 * @param [in] W  Size of padding in last block.
 *                TODO: Calculate this directly from query, instead of passing it.
 * @param [in] maxNumBlocks  Number of blocks needed to cover the whole query.
 *                           TODO: Calculate this directly from query, instead of passing it.
 * @param [in] query
 * @param [in] queryLength
 * @param [in] target
 * @param [in] targetLength
 * @param [in] alphabetLength
 * @param [in] k
 * @param [out] bestScore_  Edit distance.
 * @param [out] position_  0-indexed position in target at which best score was found.
 * @param [in] findAlignment  If true, whole matrix is remembered and alignment data is returned.
 *                            Quadratic amount of memory is consumed.
 * @param [out] alignData  Data needed for alignment traceback (for reconstruction of alignment).
 *                         Set only if findAlignment is set to true, otherwise it is NULL.
 *                         Make sure to free this array using delete[].
 * @param [out] targetStopPosition  If set to -1, whole calculation is performed normally, as expected.
 *         If set to p, calculation is performed up to position p in target (inclusive)
 *         and column p is returned as the only column in alignData.
 * @return Status.
 */
static int myersCalcEditDistanceNW(const memory::vector<Word> & Peq, const int W, const int maxNumBlocks,
		const bytes &query, const bytes &target,
		const int alphabetLength, int k, int* const bestScore_,
		int* const position_, const bool findAlignment,
		AlignmentData &alignData, const int targetStopPosition) {
	if (targetStopPosition > -1 && findAlignment) {
		// They can not be both set at the same time!
		return EDLIB_STATUS_ERROR;
	}

	// Each STRONG_REDUCE_NUM column is reduced in more expensive way.
	const int STRONG_REDUCE_NUM = 2048;// TODO: Choose this number dinamically (based on query and target lengths?), so it does not affect speed of computation

	if (k < abs(int(target.size()) - int(query.size()))) {
		*bestScore_ = *position_ = -1;
		return EDLIB_STATUS_OK;
	}

	k = min(k, max(query.size(), target.size()));  // Upper bound for k

	// firstBlock is 0-based index of first block in Ukkonen band.
	// lastBlock is 0-based index of last block in Ukkonen band.
	int firstBlock = 0;
	// This is optimal now, by my formula.
	int lastBlock = min(maxNumBlocks, ceilDiv(min(k, (k + query.size() - target.size()) / 2) + 1, WORD_SIZE)) - 1;
	Block* bl;// Current block

	memory::vector<Block> blocks; blocks.resize(maxNumBlocks);

	// Initialize P, M and score
	bl = blocks.data();
	for (int b = 0; b <= lastBlock; b++) {
		bl->score = (b + 1) * WORD_SIZE;
		bl->P = (Word)-1; // All 1s
		bl->M = (Word)0;
		bl++;
	}

	// If we want to find alignment, we have to store needed data.
	if (findAlignment) {
		alignData.resize(maxNumBlocks, target.size());
	} else if (targetStopPosition > -1) {
		alignData.resize(maxNumBlocks, 1);
	}

	const unsigned char* targetChar = target.data();
	for (size_t c = 0; c < target.size(); c++) { // for each column
		const Word* Peq_c = Peq.data() + *targetChar * maxNumBlocks;

		//----------------------- Calculate column -------------------------//
		int hout = 1;
		bl = blocks.data() + firstBlock;
		for (int b = firstBlock; b <= lastBlock; b++) {
			hout = calculateBlock(bl->P, bl->M, Peq_c[b], hout, bl->P, bl->M);
			bl->score += hout;
			bl++;
		}
		bl--;
		//------------------------------------------------------------------//
		// bl now points to last block

		// Update k. I do it only on end of column because it would slow calculation too much otherwise.
		// NOTICE: I add W when in last block because it is actually result from W cells to the left and W cells up.
		k = min(k, bl->score
				+ max(target.size() - c - 1, query.size() - ((1 + lastBlock) * WORD_SIZE - 1) - 1)
				+ (lastBlock == maxNumBlocks - 1 ? W : 0));

		//---------- Adjust number of blocks according to Ukkonen ----------//
		//--- Adjust last block ---//
		// If block is not beneath band, calculate next block. Only next because others are certainly beneath band.
		if (lastBlock + 1 < maxNumBlocks
				&& !(//score[lastBlock] >= k + WORD_SIZE ||  // NOTICE: this condition could be satisfied if above block also!
						int(((lastBlock + 1) * WORD_SIZE - 1)
								> int(k - bl->score + 2 * WORD_SIZE - 2 - target.size() + c + query.size())))) {
			lastBlock++; bl++;
			bl->P = (Word)-1; // All 1s
			bl->M = (Word)0;
			int newHout = calculateBlock(bl->P, bl->M, Peq_c[lastBlock], hout, bl->P, bl->M);
			bl->score = (bl - 1)->score - hout + WORD_SIZE + newHout;
			hout = newHout;
		}

		// While block is out of band, move one block up.
		// NOTE: Condition used here is more loose than the one from the article, since I simplified the max() part of it.
		// I could consider adding that max part, for optimal performance.
		while (lastBlock >= firstBlock
				&& (bl->score >= k + WORD_SIZE
						|| int(((lastBlock + 1) * WORD_SIZE - 1) >
		// TODO: Does not work if do not put +1! Why???
								int(k - bl->score + 2 * WORD_SIZE - 2 - target.size() + c + query.size() + 1)))) {
			lastBlock--; bl--;
		}
		//-------------------------//

		//--- Adjust first block ---//
		// While outside of band, advance block
		while (firstBlock <= lastBlock
				&& (blocks[firstBlock].score >= k + WORD_SIZE
						|| int(((firstBlock + 1) * WORD_SIZE - 1) <
								int(blocks[firstBlock].score - k - target.size() + query.size() + c)))) {
			firstBlock++;
		}
		//--------------------------/

		// TODO: consider if this part is useful, it does not seem to help much
		if (c % STRONG_REDUCE_NUM == 0) { // Every some columns do more expensive but more efficient reduction
			while (lastBlock >= firstBlock) {
				// If all cells outside of band, remove block
				memory::vector<int> scores = getBlockCellValues(*bl);
				int numCells = lastBlock == maxNumBlocks - 1 ? WORD_SIZE - W : WORD_SIZE;
				int r = lastBlock * WORD_SIZE + numCells - 1;
				bool reduce = true;
				for (int i = WORD_SIZE - numCells; i < WORD_SIZE; i++) {
					// TODO: Does not work if do not put +1! Why???
					if (int(scores[i]) <= k && r <= int(k - scores[i] - target.size() + c + query.size() + 1)) {
						reduce = false;
						break;
					}
					r--;
				}
				if (!reduce) break;
				lastBlock--; bl--;
			}

			while (firstBlock <= lastBlock) {
				// If all cells outside of band, remove block
				auto scores = getBlockCellValues(blocks[firstBlock]);
				int numCells = firstBlock == maxNumBlocks - 1 ? WORD_SIZE - W : WORD_SIZE;
				int r = firstBlock * WORD_SIZE + numCells - 1;
				bool reduce = true;
				for (int i = WORD_SIZE - numCells; i < WORD_SIZE; i++) {
					if (int(scores[i]) <= k && r >= int(scores[i] - k - target.size() + c + query.size())) {
						reduce = false;
						break;
					}
					r--;
				}
				if (!reduce) break;
				firstBlock++;
			}
		}

		// If band stops to exist finish
		if (lastBlock < firstBlock) {
			*bestScore_ = *position_ = -1;
			return EDLIB_STATUS_OK;
		}
		//------------------------------------------------------------------//

		//---- Save column so it can be used for reconstruction ----//
		if (findAlignment && c < target.size()) {
			bl = blocks.data() + firstBlock;
			for (int b = firstBlock; b <= lastBlock; b++) {
				alignData.Ps[maxNumBlocks * c + b] = bl->P;
				alignData.Ms[maxNumBlocks * c + b] = bl->M;
				alignData.scores[maxNumBlocks * c + b] = bl->score;
				alignData.firstBlocks[c] = firstBlock;
				alignData.lastBlocks[c] = lastBlock;
				bl++;
			}
		}
		//----------------------------------------------------------//
		//---- If this is stop column, save it and finish ----//
		if (int(c) == targetStopPosition) {
			for (int b = firstBlock; b <= lastBlock; b++) {
				alignData.Ps[b] = (blocks.data() + b)->P;
				alignData.Ms[b] = (blocks.data() + b)->M;
				alignData.scores[b] = (blocks.data() + b)->score;
				alignData.firstBlocks[0] = firstBlock;
				alignData.lastBlocks[0] = lastBlock;
			}
			*bestScore_ = -1;
			*position_ = targetStopPosition;
			return EDLIB_STATUS_OK;
		}
		//----------------------------------------------------//

		targetChar++;
	}

	if (lastBlock == maxNumBlocks - 1) { // If last block of last column was calculated
		// Obtain best score from block -> it is complicated because query is padded with W cells
		int bestScore = getBlockCellValues(blocks[lastBlock])[W];
		if (bestScore <= k) {
			*bestScore_ = bestScore;
			*position_ = target.size() - 1;
			return EDLIB_STATUS_OK;
		}
	}

	*bestScore_ = *position_ = -1;
	return EDLIB_STATUS_OK;
}


/**
 * Finds one possible alignment that gives optimal score by moving back through the dynamic programming matrix,
 * that is stored in alignData. Consumes large amount of memory: O(queryLength * targetLength).
 * @param [in] queryLength  Normal length, without W.
 * @param [in] targetLength  Normal length, without W.
 * @param [in] bestScore  Best score.
 * @param [in] alignData  Data obtained during finding best score that is useful for finding alignment.
 * @param [out] alignment  Alignment.
 * @param [out] alignmentLength  Length of alignment.
 * @return Status code.
 */
static int obtainAlignmentTraceback(const int queryLength, const int targetLength, const int bestScore,
		const AlignmentData & alignData, Distance::Storage & alignment) {
    const int maxNumBlocks = ceilDiv(queryLength, WORD_SIZE);
    const int W = maxNumBlocks * WORD_SIZE - queryLength;

    alignment.reserve(queryLength + targetLength - 1);
    int c = targetLength - 1; // index of column
    int b = maxNumBlocks - 1; // index of block in column
    int currScore = bestScore; // Score of current cell
    int lScore  = -1; // Score of left cell
    int uScore  = -1; // Score of upper cell
    int ulScore = -1; // Score of upper left cell
    Word currP = alignData.Ps[c * maxNumBlocks + b]; // P of current block
    Word currM = alignData.Ms[c * maxNumBlocks + b]; // M of current block
    // True if block to left exists and is in band
    bool thereIsLeftBlock = c > 0 && b >= alignData.firstBlocks[c-1] && b <= alignData.lastBlocks[c-1];
    // We set initial values of lP and lM to 0 only to avoid compiler warnings, they should not affect the
    // calculation as both lP and lM should be initialized at some moment later (but compiler can not
    // detect it since this initialization is guaranteed by "business" logic).
    Word lP = 0, lM = 0;
    if (thereIsLeftBlock) {
        lP = alignData.Ps[(c - 1) * maxNumBlocks + b]; // P of block to the left
        lM = alignData.Ms[(c - 1) * maxNumBlocks + b]; // M of block to the left
    }
    currP <<= W;
    currM <<= W;
    int blockPos = WORD_SIZE - W - 1; // 0 based index of current cell in blockPos

    // TODO(martin): refactor this whole piece of code. There are too many if-else statements,
    // it is too easy for a bug to hide and to hard to effectively cover all the edge-cases.
    // We need better separation of logic and responsibilities.
    while (true) {
        if (c == 0) {
            thereIsLeftBlock = true;
            lScore = b * WORD_SIZE + blockPos + 1;
            ulScore = lScore - 1;
        }

        // TODO: improvement: calculate only those cells that are needed,
        //       for example if I calculate upper cell and can move up,
        //       there is no need to calculate left and upper left cell
        //---------- Calculate scores ---------//
        if (lScore == -1 && thereIsLeftBlock) {
            lScore = alignData.scores[(c - 1) * maxNumBlocks + b]; // score of block to the left
            for (int i = 0; i < WORD_SIZE - blockPos - 1; i++) {
                if (lP & HIGH_BIT_MASK) lScore--;
                if (lM & HIGH_BIT_MASK) lScore++;
                lP <<= 1;
                lM <<= 1;
            }
        }
        if (ulScore == -1) {
            if (lScore != -1) {
                ulScore = lScore;
                if (lP & HIGH_BIT_MASK) ulScore--;
                if (lM & HIGH_BIT_MASK) ulScore++;
            }
            else if (c > 0 && b-1 >= alignData.firstBlocks[c-1] && b-1 <= alignData.lastBlocks[c-1]) {
                // This is the case when upper left cell is last cell in block,
                // and block to left is not in band so lScore is -1.
                ulScore = alignData.scores[(c - 1) * maxNumBlocks + b - 1];
            }
        }
        if (uScore == -1) {
            uScore = currScore;
            if (currP & HIGH_BIT_MASK) uScore--;
            if (currM & HIGH_BIT_MASK) uScore++;
            currP <<= 1;
            currM <<= 1;
        }
        //-------------------------------------//

        // TODO: should I check if there is upper block?

        //-------------- Move --------------//
        // Move up - insertion to target - deletion from query
        if (uScore != -1 && uScore + 1 == currScore) {
            currScore = uScore;
            lScore = ulScore;
            uScore = ulScore = -1;
            if (blockPos == 0) { // If entering new (upper) block
                if (b == 0) { // If there are no cells above (only boundary cells)
                	alignment.emplace_back(Distance::Value::Insert);
                    for (int i = 0; i < c + 1; i++) // Move left until end
                    	alignment.emplace_back(Distance::Value::Delete);
                    break;
                } else {
                    blockPos = WORD_SIZE - 1;
                    b--;
                    currP = alignData.Ps[c * maxNumBlocks + b];
                    currM = alignData.Ms[c * maxNumBlocks + b];
                    if (c > 0 && b >= alignData.firstBlocks[c-1] && b <= alignData.lastBlocks[c-1]) {
                        thereIsLeftBlock = true;
                        lP = alignData.Ps[(c - 1) * maxNumBlocks + b]; // TODO: improve this, too many operations
                        lM = alignData.Ms[(c - 1) * maxNumBlocks + b];
                    } else {
                        thereIsLeftBlock = false;
                        // TODO(martin): There may not be left block, but there can be left boundary - do we
                        // handle this correctly then? Are l and ul score set correctly? I should check that / refactor this.
                    }
                }
            } else {
                blockPos--;
                lP <<= 1;
                lM <<= 1;
            }
            // Mark move
            alignment.emplace_back(Distance::Value::Insert);
        }
        // Move left - deletion from target - insertion to query
        else if (lScore != -1 && lScore + 1 == currScore) {
            currScore = lScore;
            uScore = ulScore;
            lScore = ulScore = -1;
            c--;
            if (c == -1) { // If there are no cells to the left (only boundary cells)
            	alignment.emplace_back(Distance::Value::Delete); // Move left
                int numUp = b * WORD_SIZE + blockPos + 1;
                for (int i = 0; i < numUp; i++) // Move up until end
                	alignment.emplace_back(Distance::Value::Insert);
                break;
            }
            currP = lP;
            currM = lM;
            if (c > 0 && b >= alignData.firstBlocks[c-1] && b <= alignData.lastBlocks[c-1]) {
                thereIsLeftBlock = true;
                lP = alignData.Ps[(c - 1) * maxNumBlocks + b];
                lM = alignData.Ms[(c - 1) * maxNumBlocks + b];
            } else {
                if (c == 0) { // If there are no cells to the left (only boundary cells)
                    thereIsLeftBlock = true;
                    lScore = b * WORD_SIZE + blockPos + 1;
                    ulScore = lScore - 1;
                } else {
                    thereIsLeftBlock = false;
                }
            }
            // Mark move
            alignment.emplace_back(Distance::Value::Delete);
        }
        // Move up left - (mis)match
        else if (ulScore != -1) {
            auto moveCode = ulScore == currScore ? Distance::Value::Match : Distance::Value::Replace;
            currScore = ulScore;
            uScore = lScore = ulScore = -1;
            c--;
            if (c == -1) { // If there are no cells to the left (only boundary cells)
            	alignment.emplace_back(moveCode); // Move left
                int numUp = b * WORD_SIZE + blockPos;
                for (int i = 0; i < numUp; i++) // Move up until end
                	alignment.emplace_back(Distance::Value::Insert);
                break;
            }
            if (blockPos == 0) { // If entering upper left block
                if (b == 0) { // If there are no more cells above (only boundary cells)
                	alignment.emplace_back(moveCode); // Move up left
                    for (int i = 0; i < c + 1; i++) // Move left until end
                    	alignment.emplace_back(Distance::Value::Delete);
                    break;
                }
                blockPos = WORD_SIZE - 1;
                b--;
                currP = alignData.Ps[c * maxNumBlocks + b];
                currM = alignData.Ms[c * maxNumBlocks + b];
            } else { // If entering left block
                blockPos--;
                currP = lP;
                currM = lM;
                currP <<= 1;
                currM <<= 1;
            }
            // Set new left block
            if (c > 0 && b >= alignData.firstBlocks[c-1] && b <= alignData.lastBlocks[c-1]) {
                thereIsLeftBlock = true;
                lP = alignData.Ps[(c - 1) * maxNumBlocks + b];
                lM = alignData.Ms[(c - 1) * maxNumBlocks + b];
            } else {
                if (c == 0) { // If there are no cells to the left (only boundary cells)
                    thereIsLeftBlock = true;
                    lScore = b * WORD_SIZE + blockPos + 1;
                    ulScore = lScore - 1;
                } else {
                    thereIsLeftBlock = false;
                }
            }
            // Mark move
            alignment.emplace_back(moveCode);
        } else {
            // Reached end - finished!
            break;
        }
        //----------------------------------//
    }

    alignment.reverse();
    return EDLIB_STATUS_OK;
}


/**
 * Finds one possible alignment that gives optimal score (bestScore).
 * It will split problem into smaller problems using Hirschberg's algorithm and when they are small enough,
 * it will solve them using traceback algorithm.
 * @param [in] query
 * @param [in] rQuery  Reversed query.
 * @param [in] queryLength
 * @param [in] target
 * @param [in] rTarget  Reversed target.
 * @param [in] targetLength
 * @param [in] equalityDefinition
 * @param [in] alphabetLength
 * @param [in] bestScore  Best(optimal) score.
 * @param [out] alignment  Sequence of edit operations that make target equal to query.
 * @param [out] alignmentLength  Length of alignment.
 * @return Status code.
 */
static int obtainAlignment(const bytes & query, const bytes & rQuery, const bytes & target, const bytes & rTarget,
		const int alphabetLength, const int bestScore, Distance::Storage & alignment) {
	const auto queryLength = min(query.size(), rQuery.size());
	const auto targetLength = min(target.size(), rTarget.size());

	// Handle special case when one of sequences has length of 0.
	if (queryLength == 0 || targetLength == 0) {
		alignment.reserve(targetLength + queryLength);
		for (int i = 0; i < targetLength + queryLength; ++ i) {
			alignment.emplace_back(queryLength == 0 ? Distance::Value::Delete : Distance::Value::Insert);
		}
		return EDLIB_STATUS_OK;
	}

	const int maxNumBlocks = ceilDiv(queryLength, WORD_SIZE);
	const int W = maxNumBlocks * WORD_SIZE - queryLength;
	int statusCode;

	// TODO: think about reducing number of memory allocations in alignment functions, probably
	// by sharing some memory that is allocated only once. That refers to: Peq, columns in Hirschberg,
	// and it could also be done for alignments - we could have one big array for alignment that would be
	// sparsely populated by each of steps in recursion, and at the end we would just consolidate those results.

	// If estimated memory consumption for traceback algorithm is smaller than 1MB use it,
	// otherwise use Hirschberg's algorithm. By running few tests I choose boundary of 1MB as optimal.
	long long alignmentDataSize = (long long) (2 * sizeof(Word) + sizeof(int)) * maxNumBlocks * targetLength
			+ (long long) 2 * sizeof(int) * targetLength;
	if (alignmentDataSize < 1024 * 1024) {
		int score_, endLocation_;  // Used only to call function.
		AlignmentData alignData;
		auto Peq = buildPeq(alphabetLength, query);
		myersCalcEditDistanceNW(Peq, W, maxNumBlocks, query, target,
				alphabetLength, bestScore, &score_, &endLocation_, true, alignData, -1);
		assert(score_ == bestScore);
		assert(endLocation_ == targetLength - 1);

		statusCode = obtainAlignmentTraceback(queryLength, targetLength, bestScore, alignData, alignment);
	} else {
		statusCode = obtainAlignmentHirschberg(query, rQuery, target, rTarget, alphabetLength, bestScore, alignment);
	}
	return statusCode;
}


/**
 * Finds one possible alignment that gives optimal score (bestScore).
 * Uses Hirschberg's algorithm to split problem into two sub-problems, solve them and combine them together.
 * @param [in] query
 * @param [in] rQuery  Reversed query.
 * @param [in] queryLength
 * @param [in] target
 * @param [in] rTarget  Reversed target.
 * @param [in] targetLength
 * @param [in] alphabetLength
 * @param [in] bestScore  Best(optimal) score.
 * @param [out] alignment  Sequence of edit operations that make target equal to query.
 * @param [out] alignmentLength  Length of alignment.
 * @return Status code.
 */
static int obtainAlignmentHirschberg(const bytes & query, const bytes & rQuery, const bytes & target, const bytes & rTarget,
        const int alphabetLength, const int bestScore, Distance::Storage & alignment) {

	const auto queryLength = query.size();
	const auto targetLength = target.size();

	const int maxNumBlocks = ceilDiv(queryLength, WORD_SIZE);
	const int W = maxNumBlocks * WORD_SIZE - queryLength;

	auto Peq = buildPeq(alphabetLength, query);
	auto rPeq = buildPeq(alphabetLength, rQuery);

	// Used only to call functions.
	int score_, endLocation_;

	// Divide dynamic matrix into two halfs, left and right.
	const int leftHalfWidth = targetLength / 2;
	const int rightHalfWidth = targetLength - leftHalfWidth;

	// Calculate left half.
	AlignmentData alignDataLeftHalf;
	int leftHalfCalcStatus = myersCalcEditDistanceNW(
			Peq, W, maxNumBlocks,
			query, target,
			alphabetLength, bestScore,
			&score_, &endLocation_, false, alignDataLeftHalf, leftHalfWidth - 1);

	// Calculate right half.
	AlignmentData alignDataRightHalf;
	int rightHalfCalcStatus = myersCalcEditDistanceNW(
			rPeq, W, maxNumBlocks,
			rQuery, rTarget,
			alphabetLength, bestScore,
			&score_, &endLocation_, false, alignDataRightHalf, rightHalfWidth - 1);

	if (leftHalfCalcStatus == EDLIB_STATUS_ERROR || rightHalfCalcStatus == EDLIB_STATUS_ERROR) {
		return EDLIB_STATUS_ERROR;
	}

	// Unwrap the left half.
	int firstBlockIdxLeft = alignDataLeftHalf.firstBlocks[0];
	int lastBlockIdxLeft = alignDataLeftHalf.lastBlocks[0];
	// TODO: avoid this allocation by using some shared array?
	// scoresLeft contains scores from left column, starting with scoresLeftStartIdx row (query index)
	// and ending with scoresLeftEndIdx row (0-indexed).
	int scoresLeftLength = (lastBlockIdxLeft - firstBlockIdxLeft + 1) * WORD_SIZE;
	memory::vector<int> scoresLeft; scoresLeft.resize(scoresLeftLength);
	for (int blockIdx = firstBlockIdxLeft; blockIdx <= lastBlockIdxLeft; blockIdx++) {
		Block block(alignDataLeftHalf.Ps[blockIdx], alignDataLeftHalf.Ms[blockIdx],
				alignDataLeftHalf.scores[blockIdx]);
		readBlock(block, scoresLeft.data() + (blockIdx - firstBlockIdxLeft) * WORD_SIZE);
	}
	int scoresLeftStartIdx = firstBlockIdxLeft * WORD_SIZE;
	// If last block contains padding, shorten the length of scores for the length of padding.
	if (lastBlockIdxLeft == maxNumBlocks - 1) {
		scoresLeftLength -= W;
	}

	// Unwrap the right half (I also reverse it while unwraping).
	int firstBlockIdxRight = alignDataRightHalf.firstBlocks[0];
	int lastBlockIdxRight = alignDataRightHalf.lastBlocks[0];
	int scoresRightLength = (lastBlockIdxRight - firstBlockIdxRight + 1) * WORD_SIZE;
	memory::vector<int> scoresRightVec; scoresRightVec.resize(scoresRightLength);
	int* scoresRight = scoresRightVec.data();
	for (int blockIdx = firstBlockIdxRight; blockIdx <= lastBlockIdxRight; blockIdx++) {
		Block block(alignDataRightHalf.Ps[blockIdx], alignDataRightHalf.Ms[blockIdx],
				alignDataRightHalf.scores[blockIdx]);
		readBlockReverse(block, scoresRight + (lastBlockIdxRight - blockIdx) * WORD_SIZE);
	}
	int scoresRightStartIdx = queryLength - (lastBlockIdxRight + 1) * WORD_SIZE;
	// If there is padding at the beginning of scoresRight (that can happen because of reversing that we do),
	// move pointer forward to remove the padding (that is why we remember originalStart).
	if (scoresRightStartIdx < 0) {
		assert(scoresRightStartIdx == -1 * W);
		scoresRight += W;
		scoresRightStartIdx += W;
		scoresRightLength -= W;
	}

	//--------------------- Find the best move ----------------//
	// Find the query/row index of cell in left column which together with its lower right neighbour
	// from right column gives the best score (when summed). We also have to consider boundary cells
	// (those cells at -1 indexes).
	//  x|
	//  -+-
	//   |x
	int queryIdxLeftStart = max(scoresLeftStartIdx, scoresRightStartIdx - 1);
	int queryIdxLeftEnd = min(scoresLeftStartIdx + scoresLeftLength - 1,
			scoresRightStartIdx + scoresRightLength - 2);
	int leftScore, rightScore;
	int queryIdxLeftAlignment;// Query/row index of cell in left column where alignment is passing through.
	bool queryIdxLeftAlignmentFound = false;
	for (int queryIdx = queryIdxLeftStart; queryIdx <= queryIdxLeftEnd; queryIdx++) {
		leftScore = scoresLeft[queryIdx - scoresLeftStartIdx];
		rightScore = scoresRight[queryIdx + 1 - scoresRightStartIdx];
		if (leftScore + rightScore == bestScore) {
			queryIdxLeftAlignment = queryIdx;
			queryIdxLeftAlignmentFound = true;
			break;
		}
	}
	// Check boundary cells.
	if (!queryIdxLeftAlignmentFound && scoresLeftStartIdx == 0 && scoresRightStartIdx == 0) {
		leftScore = leftHalfWidth;
		rightScore = scoresRight[0];
		if (leftScore + rightScore == bestScore) {
			queryIdxLeftAlignment = -1;
			queryIdxLeftAlignmentFound = true;
		}
	}
	if (!queryIdxLeftAlignmentFound && scoresLeftStartIdx + scoresLeftLength == int(queryLength)
			&& scoresRightStartIdx + scoresRightLength == int(queryLength)) {
		leftScore = scoresLeft[scoresLeftLength - 1];
		rightScore = rightHalfWidth;
		if (leftScore + rightScore == bestScore) {
			queryIdxLeftAlignment = queryLength - 1;
			queryIdxLeftAlignmentFound = true;
		}
	}

	if (queryIdxLeftAlignmentFound == false) {
		// If there was no move that is part of optimal alignment, then there is no such alignment
		// or given bestScore is not correct!
		return EDLIB_STATUS_ERROR;
	}
    //----------------------------------------------------------//

	// Calculate alignments for upper half of left half (upper left - ul)
	// and lower half of right half (lower right - lr).
	const int ulHeight = queryIdxLeftAlignment + 1;
	const int lrHeight = queryLength - ulHeight;
	const int ulWidth = leftHalfWidth;
	const int lrWidth = rightHalfWidth;

	Distance::Storage ulAlignment;
	bytes ulrQuery(rQuery.begin() + lrHeight, rQuery.begin() + lrHeight + ulHeight);
	bytes ulrTarget(rTarget.begin() + lrWidth, rTarget.begin() + lrWidth + ulWidth);
	int ulStatusCode = obtainAlignment(query, ulrQuery, target, ulrTarget,
			alphabetLength, leftScore, ulAlignment);

	Distance::Storage lrAlignment;
	bytes lrQuery(query.begin() + ulHeight, query.begin() + ulHeight + lrHeight);
	bytes lrTarget(target.begin() + ulWidth, target.begin() + ulWidth + lrWidth);
	int lrStatusCode = obtainAlignment(lrQuery, rQuery, lrTarget, rTarget,
			alphabetLength, rightScore, lrAlignment);

	if (ulStatusCode == EDLIB_STATUS_ERROR || lrStatusCode == EDLIB_STATUS_ERROR) {
		return EDLIB_STATUS_ERROR;
	}

    // Build alignment by concatenating upper left alignment with lower right alignment.
	alignment = Distance::Storage::merge(ulAlignment, lrAlignment);
    return EDLIB_STATUS_OK;
}


/**
 * Takes char query and char target, recognizes alphabet and transforms them into unsigned char sequences
 * where elements in sequences are not any more letters of alphabet, but their index in alphabet.
 * Most of internal edlib functions expect such transformed sequences.
 * This function will allocate queryTransformed and targetTransformed, so make sure to free them when done.
 * Example:
 *   Original sequences: "ACT" and "CGT".
 *   Alphabet would be recognized as "ACTG". Alphabet length = 4.
 *   Transformed sequences: [0, 1, 2] and [1, 3, 2].
 * @param [in] queryOriginal
 * @param [in] queryLength
 * @param [in] targetOriginal
 * @param [in] targetLength
 * @param [out] queryTransformed  It will contain values in range [0, alphabet length - 1].
 * @param [out] targetTransformed  It will contain values in range [0, alphabet length - 1].
 * @return  Alphabet as a string of unique characters, where index of each character is its value in transformed
 *          sequences.
 */
static memory::string transformSequences(const StringView &query, const StringView &target,
		bytes &queryTransformed, bytes &targetTransformed) {
    // Alphabet is constructed from letters that are present in sequences.
    // Each letter is assigned an ordinal number, starting from 0 up to alphabetLength - 1,
    // and new query and target are created in which letters are replaced with their ordinal numbers.
    // This query and target are used in all the calculations later.
	queryTransformed.reserve(query.size());
	targetTransformed.reserve(target.size());

    memory::string alphabet;

    // Alphabet information, it is constructed on fly while transforming sequences.
    // letterIdx[c] is index of letter c in alphabet.
    unsigned char letterIdx[std::numeric_limits<unsigned char>::max() + 1];
    bool inAlphabet[std::numeric_limits<unsigned char>::max() + 1]; // inAlphabet[c] is true if c is in alphabet
    for (int i = 0; i < std::numeric_limits<unsigned char>::max() + 1; i++) inAlphabet[i] = false;

    for (size_t i = 0; i < query.size(); i++) {
        unsigned char c = static_cast<unsigned char>(query[i]);
        if (!inAlphabet[c]) {
            inAlphabet[c] = true;
            letterIdx[c] = alphabet.size();
            alphabet.emplace_back(query[i]);
        }
        queryTransformed.emplace_back(letterIdx[c]);
    }
    for (size_t i = 0; i < target.size(); i++) {
        unsigned char c = static_cast<unsigned char>(target[i]);
        if (!inAlphabet[c]) {
            inAlphabet[c] = true;
            letterIdx[c] = alphabet.size();
            alphabet.emplace_back(target[i]);
        }
        targetTransformed.emplace_back(letterIdx[c]);
    }

    return alphabet;
}

class MemPoolHolder {
public:
	MemPoolHolder() {
		memory::pool::initialize();
	}

	~MemPoolHolder() {
		if (pool) {
			memory::pool::destroy(pool);
		}
		memory::pool::terminate();
	}

	memory::pool_t * getPool() {
		if (!pool) {
			pool = memory::pool::create((memory::pool_t *)nullptr);
		}
		return pool;
	}

private:
	memory::pool_t *pool = nullptr;
};

thread_local MemPoolHolder tl_pool;

Distance::Distance(const StringView &origin, const StringView &canonical, size_t maxDistance) {
	auto pool = tl_pool.getPool();
	//auto pool = memory::pool::create(memory::pool::acquire());
	//auto pool = memory::pool::acquire();
	memory::pool::push(pool);
	do {
	Storage alignment;

	int editDistance = -1;

	/*------------ TRANSFORM SEQUENCES AND RECOGNIZE ALPHABET -----------*/
	memory::bytes query, target;
	memory::string alphabet = transformSequences(canonical, origin, query, target);
	/*-------------------------------------------------------*/

	/*--------------------- INITIALIZATION ------------------*/
	int maxNumBlocks = ceilDiv(query.size(), WORD_SIZE); // bmax in Myers
	int W = maxNumBlocks * WORD_SIZE - query.size();// number of redundant cells in last level blocks
	auto Peq = buildPeq(alphabet.size(), query);
	/*-------------------------------------------------------*/

	/*------------------ MAIN CALCULATION -------------------*/
	// TODO: Store alignment data only after k is determined? That could make things faster.
	int positionNW;// Used only when mode is NW.
	AlignmentData alignData;
	bool dynamicK = false;
	int k = maxDistance != maxOf<size_t>() ? maxDistance : -1;
	if (k < 0) { // If valid k is not given, auto-adjust k until solution is found.
		dynamicK = true;
		k = WORD_SIZE;// Gives better results than smaller k.
	}

	do {
		myersCalcEditDistanceNW(Peq, W, maxNumBlocks, query, target,
				alphabet.size(), k, &editDistance, &positionNW,
				false, alignData, -1);
		k *= 2;
	} while(dynamicK && editDistance == -1);

	if (editDistance >= 0) {  // If there is solution.
        // Find alignment -> all comes down to finding alignment for NW.
        // Currently we return alignment only for first pair of locations.

        bytes rAlnTarget(createReverseCopy(target));
        bytes rQuery(createReverseCopy(query));
            obtainAlignment(query, rQuery, target, rAlnTarget, alphabet.size(),
            		editDistance, alignment);
    }
    /*-------------------------------------------------------*/

	_storage = alignment;
	} while (0);

	memory::pool::clear(pool);
	memory::pool::pop();
	//memory::pool::destroy(pool);
}

NS_SP_EXT_END(search)
