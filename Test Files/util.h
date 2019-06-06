//uints
#include <stdint.h>
// Random
#include <cstdlib>
// Seed
#include <ctime>
#include <string>
#define BUFF_SIZE 2048

typedef uint32_t uint32;

// Returns rand in the interval (inclusive). Default 0-1
// https://stackoverflow.com/a/686373/2939859
float getRand(int lo = 0, int high = 1)
{
    return lo + static_cast<float>(rand())
        / static_cast <float> (RAND_MAX / (high - lo));
}

/* Add this to test file to init everything:

int main()
{
    // Seed Rand
    srand(static_cast<unsigned>(time(0)));


}
}
*/

std::string loctext =
"0045310E\n"
"00453130 140\n"
"00453FC0 140 14C 19BC 19C8 19EC 20CC 2174 23a0 70 8F4 E0 E4 EC F0\n"
;
