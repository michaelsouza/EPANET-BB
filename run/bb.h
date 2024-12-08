#include "types.h"

typedef struct {
  char id[MAXID];
  int index;
  int pattern_index;
  double *pattern_values;
} BBPump;

typedef struct {
  char id[MAXID];
  int index;
  double *level;
  double min_level;
  double max_level;
} BBTank;

typedef struct {
  char id[MAXID];
  int index;
} BBNode;

typedef struct {
  int num_hours;
  int num_pumps;
  int num_tanks;
  int prices_index;
  BBPump *pumps;
  BBTank *tanks;
  BBNode *nodes;
  int hour; // 0-23
} BBData;
