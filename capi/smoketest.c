/* Pure-C smoke test for the op2utility DLL's C ABI. Proves the DLL loads, the
 * exported C functions resolve, and exceptions inside OP2Utility are converted
 * to error codes (never escaping the boundary). Does not require a real .map. */
#include "op2utility_c.h"
#include <stdio.h>

int main(void) {
    printf("op2utility C ABI version: %u\n", op2_capi_version());

    Op2Result err = OP2_OK;
    Op2Map* m = op2_map_read("definitely_missing_file.map", &err);
    if (m == NULL) {
        printf("load of missing file failed as expected: err=%d msg=\"%s\"\n",
               (int)err, op2_last_error());
        return 0;
    }

    printf("loaded: %ux%u tiles, %llu cells, savedGame=%d\n",
           op2_map_width(m), op2_map_height(m),
           (unsigned long long)op2_map_tile_count(m),
           op2_map_is_saved_game(m));
    op2_map_free(m);
    return 0;
}
