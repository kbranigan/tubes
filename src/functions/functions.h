
#ifndef BLOCK_ALL_FUNCTIONS_H
#define BLOCK_ALL_FUNCTIONS_H

#ifdef __cplusplus
extern "C" {
#endif

struct Block * unique(struct Block * block, struct Block * opt);
struct Block * offset(struct Block * block, struct Block * opt);
struct Block * multiply(struct Block * block, struct Block * opt);
struct Block * normalize(struct Block * block, struct Block * opt);

#ifdef __cplusplus
}
#endif

#endif
