#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#define FAIL 0
#define SUCCESS 1
# define WRITE 1
/**
 * The function fill all the RAM from the given frame with zeros.
 * * @param frameIndex the first frame to fill.
 */
void clearTable(uint64_t frameIndex)
{
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}
/**
 *  The function check if the given frame is a leaf - that is, it stores a page.
 * @param frameToCheck number frame to check.
 * @param frameNum needed to the recursion. expecting to get 0.
 * @param depth needed to the recursion. expecting to get 0.
 * @return true is the frame is leaf. false otherwise.
 */
bool isLeafFrame(uint64_t frameToCheck, uint64_t frameNum, int depth)
{
    if(depth == TABLES_DEPTH)
    {
        return frameNum == frameToCheck;
    }

    for (int i = 0; i < PAGE_SIZE; i++) {
        word_t temp;
        PMread(frameNum * PAGE_SIZE + i, &temp);
        if (temp != 0 && isLeafFrame(frameToCheck, temp, depth + 1)) {
            return true;
        }
    }
    return false;
}
/**
 * Check if the frame is empty frame- where all rows are 0.
 * @param frameIndex number of frame to check.
 * @return true is the frame is empty. false otherwise.
 */
bool isEmptyFrame(uint64_t frameIndex)
{
    for (uint64_t i = 0; i < PAGE_SIZE; ++i)
    {
        word_t temp;
        PMread(frameIndex * PAGE_SIZE + i, &temp);
        if(temp != 0)
        {
            return false;
        }
    }
    return true;
}

/**
 * Implements the first step in the algorithm: remove the reference to the empty table from its parent.
 * @param frameNum needed to the recursion. expecting to get 0.
 * @param depth needed to the recursion. expecting to get 0.
 * @param lastFrameUsed number of the last frame we used.
 */
void deleteEmptyFrameRef(uint64_t frameNum, int depth, uint64_t lastFrameUsed)
{
    if(depth == TABLES_DEPTH - 1)
    {
        return;
    }

    for (int i = 0; i < PAGE_SIZE; ++i)
    {
        word_t value;
        PMread(frameNum * PAGE_SIZE + i, &value);
        if (value != 0)
        {
            deleteEmptyFrameRef(value, depth + 1, lastFrameUsed);

            if (isEmptyFrame(value) && (uint64_t)value != lastFrameUsed)
            {
                PMwrite(frameNum * PAGE_SIZE + i, 0);
            }
        }
    }
}
/**
 * Implements the second step in the algorithm: fine an unused frame.
 * @param maxFrameNum number of the max frame that write in the RAM.
 * @param lastFrameUsed number of the last frame we used.
 * @return num of empty unused frame.
 */
uint64_t findEmptyFrame(uint64_t maxFrameNum, uint64_t lastFrameUsed)
{
    for (uint64_t i = maxFrameNum; i > 0; --i)
    {
        if(i != lastFrameUsed && isEmptyFrame(i) && !isLeafFrame(i, 0, 0))
        {
            return i;
        }
    }
    return -1;
}
/**
 * Calculate the number of the max frame that write in the RAM.
 * @return the number of the max frame that write in the RAM
 */
void calculateMaxFrame(uint64_t frameNum, uint64_t* maxFrameNum, int depth)
{
    if(depth == TABLES_DEPTH){
        return;
    }
    word_t temp;
    uint64_t maxFrame = 0;
    //for (uint64_t i = 0; i < NUM_FRAMES; ++i) {
        for (uint64_t j = 0; j < PAGE_SIZE; ++j) {
            PMread(frameNum * PAGE_SIZE + j, &temp);
            if(temp != 0){
                if ((uint64_t)temp > *maxFrameNum){
                    *maxFrameNum = temp;
                }
                calculateMaxFrame(temp, maxFrameNum, depth+1);
            }
        }
   // }
}
/**
 * When we get to the leaf and before we go back in recursion, we need to update the sum accordingly:
 * subtract the weights of the corresponding frame number and page number, according to the given algorithm.
 * @param frameNum number of the frame that store the given page.
 * @param sum sum of the weights of the path we have found so far.
 * @param pageNum number of the page that stored in the current leaf.
 */
void lowerFrameNumAndPageNum(const uint64_t *frameNum, uint64_t *sum, const uint64_t *pageNum) {
    if (*frameNum % 2 == 0)
    {
        *sum = *sum - WEIGHT_EVEN;
    }
    else
    {
        *sum = *sum - WEIGHT_ODD;
    }
    if (*pageNum % 2 == 0)
    {
        *sum = *sum - WEIGHT_EVEN;
    }
    else
    {
        *sum = *sum - WEIGHT_ODD;
    }
}
/**
 * Calculate the maximum weight of a path, according to the given algorithm.
 * @param frameNum number of the current frame.
 * @param sum weight of the current path.
 * @param pageNum number of the current page.
 * @param maxSum maximum weight.
 * @param pageToEvict number of the page with the maximum weight.
 * @param frameNumOfPageEvicted number of the frame that store the page with the maximum weight.
 */
void calculatePageToEvict(const uint64_t *frameNum, const uint64_t *sum, const uint64_t *pageNum, uint64_t *maxSum,
                          uint64_t *pageToEvict, uint64_t *frameNumOfPageEvicted) {
    if (*sum > *maxSum)
    {
        *maxSum = *sum;
        *pageToEvict = *pageNum;
        *frameNumOfPageEvicted = *frameNum;
    }
    if (*sum == *maxSum)
    {
        if (*pageNum < *pageToEvict)
        {
            *pageToEvict = *pageNum;
            *frameNumOfPageEvicted = *frameNum;
        }
    }
}
/**
 * During the recursion, we need to update the sum accordingly:
 * Adding the weights of the corresponding page number, according to the given algorithm.
 * @param sum weight of the path until now.
 * @param pageNum page number.
 */
void addPageNumToSum(uint64_t *sum, const uint64_t *pageNum) {
    if (*pageNum % 2 == 0)
    {
        *sum = *sum + WEIGHT_EVEN;
    }
    else
    {
        *sum = *sum + WEIGHT_ODD;
    }
}
/**
 * uring the recursion, we need to update the sum accordingly:
 * Adding the weights of the corresponding frame number, according to the given algorithm.
 * @param frameNum frame number.
 * @param sum weight of the path until now.
 */
void addFrameNumToSum(const uint64_t *frameNum, uint64_t *sum) {
    if (*frameNum % 2 == 0)
    {
        *sum = *sum + WEIGHT_EVEN;
    }
    else
    {
        *sum = *sum + WEIGHT_ODD;
    }
}
/**
 * Go over the tree with DFS. Used to realize the third step in the algorithm.
 * @param frameNum frame number in the RAM.
 * @param visited array of all the frames. Indicates which frame is already visited.
 * @param depth needed to the recursion. expecting to get 0.
 * @param sum needed to the recursion. expecting to get 0.
 * @param pageNum needed to the recursion. expecting to get 0.
 * @param maxSum needed to the recursion. expecting to get 0.
 * @param pageToEvict page number to evict.
 * @param frameNumOfPageEvicted frame number that store the chosen page.
 */
void DFS_util(uint64_t* frameNum, bool visited[], int* depth, uint64_t* sum, uint64_t* pageNum, uint64_t* maxSum, uint64_t* pageToEvict, uint64_t* frameNumOfPageEvicted)
{
    visited[*frameNum] = true;
    addFrameNumToSum(frameNum, sum);
    *depth = *depth + 1;

    if (*depth == TABLES_DEPTH + 1)
    {
        addPageNumToSum(sum, pageNum);
        calculatePageToEvict(frameNum, sum, pageNum, maxSum, pageToEvict, frameNumOfPageEvicted);
        lowerFrameNumAndPageNum(frameNum, sum, pageNum);
        return;
    }

    // recursively process all the adjacent vertices of the node
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        word_t childFrameNum;
        PMread((*frameNum * PAGE_SIZE) + i, &childFrameNum);
        if (childFrameNum != 0 && !visited[childFrameNum])
        {
            *pageNum = *pageNum << OFFSET_WIDTH;
            *pageNum = *pageNum + i;
            uint64_t temp = childFrameNum;
            DFS_util(&temp, visited, depth, sum, pageNum, maxSum, pageToEvict, frameNumOfPageEvicted);
            *depth = *depth - 1;
            *pageNum = *pageNum - i;
            *pageNum = *pageNum >> OFFSET_WIDTH;
        }

    }
}
/**
 * Implements the third step in the algorithm: found a page to be swapped out from some frame in order to replace it
 * with the relevant page. The evicted page chosen according to the given algorithm.
 * @param pageToEvict page number to evict.
 * @param frameNumOfPageEvicted frame number that store the chosen page.
 */
void DFS(uint64_t* pageToEvict, uint64_t* frameNumOfPageEvicted)
{
    uint64_t maxSum = 0, sum = 0, pageNum = 0;
    int j = 0;
    // initially none of the vertices are visited
    bool visited[NUM_FRAMES];
    for (bool & i : visited)
        i = false;

    // explore the vertices one by one by recursively calling DFS_util
    for (uint64_t i = 0; i < NUM_FRAMES; i++)
        if (!visited[i])
            DFS_util(&i, visited, &j, &sum, &pageNum, &maxSum, pageToEvict, frameNumOfPageEvicted);
}
/**
 * Finding the exact frame to put our page in and evicting another page if necessary.
 * @param lastFrameUsed number of the last used frame.
 * @return number of frame to put our page. -1  if we evict some page.
 */
uint64_t findFrame(uint64_t lastFrameUsed)
{
    // Case 1
    deleteEmptyFrameRef(0, 0, lastFrameUsed);
    // Case 2
    uint64_t maxFrameNum, maxEmptyFrame;
    maxFrameNum = 0;
    calculateMaxFrame(0, &maxFrameNum, 0);
    maxEmptyFrame = findEmptyFrame(maxFrameNum, lastFrameUsed);
    if (maxEmptyFrame > 0 && maxEmptyFrame <= maxFrameNum)
    {
        return maxEmptyFrame;
    }
    if (maxFrameNum + 1 < NUM_FRAMES)
    {
        return maxFrameNum + 1;
    }
    // Case 3
    uint64_t pageToEvict, frameNumOfPageEvicted;
    DFS(&pageToEvict, &frameNumOfPageEvicted);
    if (frameNumOfPageEvicted != 0){
        PMevict(frameNumOfPageEvicted, pageToEvict);
        for (uint64_t j = 0; j < NUM_FRAMES * PAGE_SIZE; ++j) {
            word_t temp;
            PMread(j, &temp);
            if ((uint64_t) temp == frameNumOfPageEvicted) {
                PMwrite(j, 0);
            }
        }
    }return frameNumOfPageEvicted;
}
/**
 * This function will be called before any other function is called. Fill all the RAM with zeros.
 */
void VMinitialize()
{
    clearTable(0);
}
/**
 * Get the offset from the virtual address.
 * @param virtualAddress full virtual address.
 * @return offset.
 */
uint64_t getAddrOffset(uint64_t &virtualAddress) {
    return virtualAddress & ((1 << OFFSET_WIDTH) - 1);
}
/**
 * Implementation a virtual memory interface using hierarchical page tables of arbitrary depth
 * using simulated physical memory.
 * @param virtualAddress virtual address to write into or read from it, depending the status.
 * @param status 1 for write, 0 to read.
 * @param value value to write to the RAM or store the information we read, depending on the status.
 */
void readData(uint64_t virtualAddress, int status, word_t* value)
{
    uint64_t addrWithoutOffset = virtualAddress >> OFFSET_WIDTH;
    uint64_t offset = getAddrOffset(virtualAddress);
    bool needToRestore = false;
    word_t addr1 = 0, addr2 = 0;
    uint64_t lastFrameUsed = 0;

    for (int i = TABLES_DEPTH - 1; i>=0 ; --i)
    {
        PMread(addr1 * PAGE_SIZE + ((addrWithoutOffset >> (OFFSET_WIDTH*i)) & (int)(pow(2, OFFSET_WIDTH) - 1)), &addr2);

        // advance to next frame
        if(addr2 != 0)
        {
            addr1 = addr2;
            needToRestore = false;
            lastFrameUsed = addr1;
        }
            // need to evict frame
        else {
            uint64_t frameToEvictInd = findFrame(lastFrameUsed);
            PMwrite(addr1 * PAGE_SIZE + ((addrWithoutOffset >> (OFFSET_WIDTH*i)) & (int)(pow(2, OFFSET_WIDTH) - 1)), frameToEvictInd);
            lastFrameUsed = frameToEvictInd;
            addr1 = frameToEvictInd;
            needToRestore = true;
            if (i != 0) {
                clearTable(frameToEvictInd);
            }
        }
    }

    if (needToRestore)
    {
        PMrestore(addr1, addrWithoutOffset);
    }

    if (status == WRITE)
    {
        PMwrite(addr1 * PAGE_SIZE + offset, *value);
    }
    else
    {
        PMread(addr1 * PAGE_SIZE + offset, value);
    }
}
/**
 * Check if the given address is valid.
 * @param address address to check.
 * @return true if the address is valid. false otherwise.
 */
bool validAddr(uint64_t address) {
    if (address >= VIRTUAL_MEMORY_SIZE) {
        return false;
    }
    return true;
}
/**
 * Reads the word from the virtual address virtualAddress into *value.
 * @param virtualAddress address to read from it.
 * @param value value to store the information.
 * @return 1 on success and 0 on failure.
 */
int VMread(uint64_t virtualAddress, word_t* value)
{
    if (!validAddr(virtualAddress)) {
        return FAIL;
    }
    readData(virtualAddress, 0 , value);
    return SUCCESS;
}
/**
 * Writes the word value into the virtual address virtualAddress.
 * @param virtualAddress address to write into.
 * @param value value to write.
 * @return 1 on success and 0 on failure.
 */
int VMwrite(uint64_t virtualAddress, word_t value)
{
    if (!validAddr(virtualAddress)) {
        return FAIL;
    }
    readData(virtualAddress, 1, &value);
    return SUCCESS;
}

