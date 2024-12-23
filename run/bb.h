// run/bb.h

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
  double min_level;
  double max_level;
} BBTank;

typedef struct {
  char id[MAXID];
  int index;
} BBNode;

typedef struct {
  char id[MAXID];
  int index;
  int type; // Type of link: EN_PIPE, EN_PUMP, EN_VALVE
} BBLink;

typedef struct {
  int num_hours;
  int num_pumps;
  int num_tanks;
  int num_nodes;
  int num_links;
  int prices_index;
  BBPump *pumps;
  BBTank *tanks;
  BBNode *nodes;
  BBLink *links;
  int hour; // Current hour in the simulation (0-23)
} BBData;

typedef struct {
  int num_pumps;
  int num_tanks;
  int num_nodes;
  double *pump_states;
  double *tank_levels;
  double *node_pressures;
  double *link_flows;
} BBState;
